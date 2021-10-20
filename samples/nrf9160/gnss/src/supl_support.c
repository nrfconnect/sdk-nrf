/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <supl_session.h>
#include <stdio.h>
#include <net/socket.h>

#define SUPL_SERVER        "supl.google.com"
#define SUPL_SERVER_PORT   "7276"

static int supl_fd = -1;

int open_supl_socket(void)
{
	int err;
	struct addrinfo *info;

	struct addrinfo hints = {
		.ai_flags = AI_NUMERICSERV,
		.ai_family = AF_UNSPEC, /* Both IPv4 and IPv6 addresses accepted. */
		.ai_socktype = SOCK_STREAM
	};

	err = getaddrinfo(SUPL_SERVER, SUPL_SERVER_PORT, &hints, &info);
	if (err) {
		printk("Failed to resolve hostname %s, error: %d\n", SUPL_SERVER, err);

		return -1;
	}

	/* Not connected. */
	err = -1;

	for (struct addrinfo *addr = info; addr != NULL; addr = addr->ai_next) {
		char ip[INET6_ADDRSTRLEN] = { 0 };
		struct sockaddr *const sa = addr->ai_addr;

		supl_fd = socket(sa->sa_family, SOCK_STREAM, IPPROTO_TCP);
		if (supl_fd < 0) {
			printk("Failed to create socket, errno %d\n", errno);
			goto cleanup;
		}

		/* The SUPL library expects a 1 second timeout for the read function. */
		struct timeval timeout = {
			.tv_sec = 1,
			.tv_usec = 0,
		};

		err = setsockopt(supl_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
		if (err) {
			printk("Failed to set socket timeout, errno %d\n", errno);
			goto cleanup;
		}

		inet_ntop(sa->sa_family,
			  (void *)&((struct sockaddr_in *)sa)->sin_addr,
			  ip,
			  INET6_ADDRSTRLEN);
		printk("Connecting to %s port %s\n", ip, SUPL_SERVER_PORT);

		err = connect(supl_fd, sa, addr->ai_addrlen);
		if (err) {
			close(supl_fd);
			supl_fd = -1;

			/* Try the next address. */
			printk("Connecting to server failed, errno %d\n", errno);
		} else {
			/* Connected. */
			break;
		}
	}

cleanup:
	freeaddrinfo(info);

	if (err) {
		/* Unable to connect, close socket. */
		printk("Could not connect to SUPL server\n");
		if (supl_fd > -1) {
			close(supl_fd);
			supl_fd = -1;
		}
		return -1;
	}

	return 0;
}

void close_supl_socket(void)
{
	if (close(supl_fd) < 0) {
		printk("Failed to close SUPL socket\n");
	}
}

ssize_t supl_write(const void *p_buff, size_t nbytes, void *user_data)
{
	ARG_UNUSED(user_data);
	return send(supl_fd, p_buff, nbytes, 0);
}

int supl_logger(int level, const char *fmt, ...)
{
	char buffer[256] = { 0 };
	va_list args;

	va_start(args, fmt);
	int ret = vsnprintk(buffer, sizeof(buffer), fmt, args);

	va_end(args);

	if (ret < 0) {
		printk("%s: encoding error\n", __func__);
		return ret;
	} else if ((size_t)ret >= sizeof(buffer)) {
		printk("%s: too long message,"
		       "it will be cut short\n", __func__);
	}

	printk("%s\n", buffer);

	return ret;
}

ssize_t supl_read(void *p_buff, size_t nbytes, void *user_data)
{
	ARG_UNUSED(user_data);
	ssize_t rc = recv(supl_fd, p_buff, nbytes, 0);

	if (rc < 0 && (errno == ETIMEDOUT)) {
		return 0;
	}

	return rc;
}
