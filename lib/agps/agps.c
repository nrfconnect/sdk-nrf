/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <stdio.h>
#include <logging/log.h>
#include <net/socket.h>
#include <nrf_modem_gnss.h>

#if defined(CONFIG_AGPS_SRC_SUPL)
#include <supl_session.h>
#include <supl_os_client.h>
#elif defined(CONFIG_AGPS_SRC_NRF_CLOUD)
#include <net/nrf_cloud_agps.h>
#endif

LOG_MODULE_REGISTER(agps, CONFIG_AGPS_LOG_LEVEL);

#if defined(CONFIG_AGPS_SRC_SUPL)
static int supl_fd = -1;
#endif /* CONFIG_AGPS_SRC_SUPL */

#if defined(CONFIG_AGPS_SRC_SUPL)
static int inject_agps_type(void *agps,
			    size_t agps_size,
			    uint16_t type,
			    void *user_data)
{
	ARG_UNUSED(user_data);
	int err;

	err = nrf_modem_gnss_agps_write(agps, agps_size, type);
	if (err) {
		LOG_ERR("Failed to send A-GPS data, type: %d (err: %d)",
			type, err);
		return err;
	}

	LOG_DBG("Injected A-GPS data, type: %d, size: %d", type, agps_size);

	return 0;
}

static int open_supl_socket(void)
{
	int err;
	struct addrinfo *info;

	struct addrinfo hints = {
		.ai_family = AF_UNSPEC, /* Both IPv4 and IPv6 addresses accepted. */
		.ai_socktype = SOCK_STREAM
	};

	err = getaddrinfo(CONFIG_AGPS_SUPL_HOST_NAME, NULL, &hints, &info);
	if (err) {
		LOG_ERR("Failed to resolve hostname %s, error: %d",
			CONFIG_AGPS_SUPL_HOST_NAME, err);

		return -1;
	}

	/* Not connected */
	err = -1;

	for (struct addrinfo *addr = info; addr != NULL; addr = addr->ai_next) {
		char ip[INET6_ADDRSTRLEN] = { 0 };
		struct sockaddr *const sa = addr->ai_addr;

		switch (sa->sa_family) {
		case AF_INET6:
			((struct sockaddr_in6 *)sa)->sin6_port = htons(CONFIG_AGPS_SUPL_PORT);
			break;
		case AF_INET:
			((struct sockaddr_in *)sa)->sin_port = htons(CONFIG_AGPS_SUPL_PORT);
			break;
		}

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
		LOG_DBG("Connecting to %s port %d", log_strdup(ip), CONFIG_AGPS_SUPL_PORT);

		err = connect(supl_fd, sa, addr->ai_addrlen);
		if (err) {
			close(supl_fd);
			supl_fd = -1;

			/* Try the next address */
			LOG_WRN("Connecting to server failed, errno %d", errno);
		} else {
			/* Connected */
			break;
		}
	}

cleanup:
	freeaddrinfo(info);

	if (err) {
		/* Unable to connect, close socket */
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

static ssize_t supl_write(const void *buf, size_t nbytes, void *user_data)
{
	ARG_UNUSED(user_data);

	return send(supl_fd, buf, nbytes, 0);
}

static int supl_logger(int level, const char *fmt, ...)
{
	char buffer[256];
	va_list args;
	int ret;

	va_start(args, fmt);

	ret = vsnprintk(buffer, sizeof(buffer), fmt, args);

	va_end(args);

	if (ret < 0) {
		LOG_ERR("%s: encoding error", __func__);
		return ret;
	} else if ((size_t)ret >= sizeof(buffer)) {
		LOG_ERR("%s: too long message, it will be cut short",
			__func__);
	}

	LOG_DBG("%s", log_strdup(buffer));

	return ret;
}

static ssize_t supl_read(void *buf, size_t nbytes, void *user_data)
{
	ARG_UNUSED(user_data);
	ssize_t rc = recv(supl_fd, buf, nbytes, 0);

	if (rc < 0 && (errno == ETIMEDOUT)) {
		return 0;
	}

	return rc;
}

static int init_supl(void)
{
	int err;
	struct supl_api supl_api = {
		.read       = supl_read,
		.write      = supl_write,
		.handler    = inject_agps_type,
		.logger     = supl_logger,
		.counter_ms = k_uptime_get
	};

	err = supl_init(&supl_api);
	if (err) {
		LOG_ERR("Failed to initialize SUPL library, error: %d", err);
		return err;
	}

	LOG_INF("SUPL is initialized");

	return 0;
}

static int supl_start(const struct nrf_modem_gnss_agps_data_frame request)
{
	int err;

	err = open_supl_socket();
	if (err) {
		LOG_ERR("Failed to open SUPL socket");
		return err;
	}

	LOG_INF("Starting SUPL session");

	err = supl_session(&request);
	if (err) {
		LOG_ERR("SUPL session failed, error: %d", err);
		goto cleanup;
	}

	LOG_INF("SUPL session finished successfully");

cleanup:
	close_supl_socket();

	return err;
}

#endif /* CONFIG_AGPS_SRC_SUPL */

int agps_request_send(struct nrf_modem_gnss_agps_data_frame request)
{
	int err;

#if defined(CONFIG_AGPS_SRC_SUPL)
	static bool supl_is_init;

	if (!supl_is_init) {
		err = init_supl();
		if (err) {
			LOG_ERR("SUPL initialization failed, error: %d", err);
			return err;
		}

		supl_is_init = true;
	}

	err = supl_start(request);
	if (err) {
		LOG_ERR("SUPL request failed, error: %d", err);
		return err;
	}

#elif defined(CONFIG_AGPS_SRC_NRF_CLOUD) && defined(CONFIG_NRF_CLOUD_MQTT)
	err = nrf_cloud_agps_request(&request);
	if (err) {
		LOG_ERR("nRF Cloud A-GPS request failed, error: %d", err);
		return err;
	}
#elif defined(CONFIG_AGPS_SRC_NRF_CLOUD) && !defined(CONFIG_NRF_CLOUD_MQTT)
	LOG_ERR("CONFIG_NRF_CLOUD_MQTT must be enabled to make A-GPS requests");
	return -EOPNOTSUPP;
	(void)err;
#endif /* CONFIG_AGPS_SRC_SUPL */

	return 0;
}

int agps_cloud_data_process(const uint8_t *buf, size_t len)
{
	int err = 0;

#if defined(CONFIG_AGPS_SRC_NRF_CLOUD) && defined(CONFIG_NRF_CLOUD_AGPS)
	err = nrf_cloud_agps_process(buf, len);
	if (err) {
		LOG_ERR("A-GPS failed, error: %d", err);
		return err;
	}

	LOG_INF("A-GPS data successfully processed");

#else /* CONFIG_AGPS_SRC_NRF_CLOUD && CONFIG_NRF_CLOUD_AGPS */

	LOG_WRN("Processing of incoming A-GPS data is not supported");
	return -EOPNOTSUPP;
#endif

	return err;
}
