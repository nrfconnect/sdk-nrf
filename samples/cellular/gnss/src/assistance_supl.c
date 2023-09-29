/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/socket.h>
#include <supl_os_client.h>
#include <supl_session.h>

#include "assistance.h"

LOG_MODULE_DECLARE(gnss_sample, CONFIG_GNSS_SAMPLE_LOG_LEVEL);

#define SUPL_SERVER        CONFIG_GNSS_SAMPLE_SUPL_HOSTNAME
#define SUPL_SERVER_PORT   CONFIG_GNSS_SAMPLE_SUPL_PORT

BUILD_ASSERT(sizeof(CONFIG_GNSS_SAMPLE_SUPL_HOSTNAME) > 1, "Server hostname must be configured");

static int supl_fd = -1;
static volatile bool assistance_active;

static ssize_t supl_read(void *p_buff, size_t nbytes, void *user_data)
{
	ARG_UNUSED(user_data);

	ssize_t rc = recv(supl_fd, p_buff, nbytes, 0);

	if (rc < 0 && (errno == EAGAIN)) {
		/* Return 0 to indicate a timeout. */
		rc = 0;
	} else if (rc == 0) {
		/* Peer closed the socket, return an error. */
		rc = -1;
	}

	return rc;
}

static ssize_t supl_write(const void *p_buff, size_t nbytes, void *user_data)
{
	ARG_UNUSED(user_data);

	return send(supl_fd, p_buff, nbytes, 0);
}

static int inject_agnss_type(void *agnss, size_t agnss_size, uint16_t type, void *user_data)
{
	ARG_UNUSED(user_data);

	int retval = nrf_modem_gnss_agnss_write(agnss, agnss_size, type);

	if (retval != 0) {
		LOG_ERR("Failed to write A-GNSS data, type: %d (errno: %d)", type, errno);
		return -1;
	}

	LOG_INF("Injected A-GNSS data, type: %d, size: %d", type, agnss_size);

	return 0;
}

static int supl_logger(int level, const char *fmt, ...)
{
	char buffer[256] = { 0 };
	va_list args;

	va_start(args, fmt);
	int ret = vsnprintk(buffer, sizeof(buffer), fmt, args);

	va_end(args);

	if (ret < 0) {
		LOG_ERR("%s: encoding error", __func__);
		return ret;
	} else if ((size_t)ret >= sizeof(buffer)) {
		LOG_ERR("%s: too long message,"
		       "it will be cut short", __func__);
	}

	LOG_INF("%s", buffer);

	return ret;
}

static int open_supl_socket(void)
{
	int err;
	char port[6];
	struct addrinfo *info;

	struct addrinfo hints = {
		.ai_flags = AI_NUMERICSERV,
		.ai_family = AF_UNSPEC, /* Both IPv4 and IPv6 addresses accepted. */
		.ai_socktype = SOCK_STREAM
	};

	snprintf(port, sizeof(port), "%d", SUPL_SERVER_PORT);

	err = getaddrinfo(SUPL_SERVER, port, &hints, &info);
	if (err) {
		LOG_ERR("Failed to resolve hostname %s, error: %d", SUPL_SERVER, err);

		return -1;
	}

	/* Not connected. */
	err = -1;

	for (struct addrinfo *addr = info; addr != NULL; addr = addr->ai_next) {
		char ip[INET6_ADDRSTRLEN] = { 0 };
		struct sockaddr *const sa = addr->ai_addr;

		supl_fd = socket(sa->sa_family, SOCK_STREAM, IPPROTO_TCP);
		if (supl_fd < 0) {
			LOG_ERR("Failed to create socket, errno %d", errno);
			goto cleanup;
		}

		/* The SUPL library expects a 1 second timeout for the read function. */
		struct timeval timeout = {
			.tv_sec = 1,
			.tv_usec = 0,
		};

		err = setsockopt(supl_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
		if (err) {
			LOG_ERR("Failed to set socket timeout, errno %d", errno);
			goto cleanup;
		}

		inet_ntop(sa->sa_family,
			  (void *)&((struct sockaddr_in *)sa)->sin_addr,
			  ip,
			  INET6_ADDRSTRLEN);
		LOG_INF("Connecting to %s port %d", ip, SUPL_SERVER_PORT);

		err = connect(supl_fd, sa, addr->ai_addrlen);
		if (err) {
			close(supl_fd);
			supl_fd = -1;

			/* Try the next address. */
			LOG_WRN("Connecting to server failed, errno %d", errno);
		} else {
			/* Connected. */
			break;
		}
	}

cleanup:
	freeaddrinfo(info);

	if (err) {
		/* Unable to connect, close socket. */
		LOG_ERR("Could not connect to SUPL server");
		if (supl_fd > -1) {
			close(supl_fd);
			supl_fd = -1;
		}
		return -1;
	}

	return 0;
}

static void close_supl_socket(void)
{
	if (close(supl_fd) < 0) {
		LOG_ERR("Failed to close SUPL socket");
	}
}

int assistance_init(struct k_work_q *assistance_work_q)
{
	ARG_UNUSED(assistance_work_q);

	static struct supl_api supl_api = {
		.read       = supl_read,
		.write      = supl_write,
		.handler    = inject_agnss_type,
		.logger     = supl_logger,
		.counter_ms = k_uptime_get
	};

	if (supl_init(&supl_api) != 0) {
		LOG_ERR("Failed to initialize SUPL library");
		return -1;
	}

	return 0;
}

int assistance_request(struct nrf_modem_gnss_agnss_data_frame *agnss_request)
{
	int err;

	assistance_active = true;

	err = open_supl_socket();
	if (err) {
		goto exit;
	}

	LOG_INF("Starting SUPL session");
	err = supl_session(agnss_request);
	LOG_INF("Done");
	close_supl_socket();

exit:
	assistance_active = false;

	return err;
}

bool assistance_is_active(void)
{
	return assistance_active;
}
