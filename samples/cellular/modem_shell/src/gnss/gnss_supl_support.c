/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#if !defined(CONFIG_NET_SOCKETS_POSIX_NAMES)
#include <zephyr/posix/unistd.h>
#include <zephyr/posix/netdb.h>
#include <zephyr/posix/sys/time.h>
#include <zephyr/posix/sys/socket.h>
#include <zephyr/posix/arpa/inet.h>
#else
#include <zephyr/net/socket.h>
#endif
#include <supl_session.h>

#include "mosh_print.h"

/* This sample uses Google SUPL server by default. For commercial use, a license is required. */
#define SUPL_SERVER "supl.google.com"
#define SUPL_SERVER_PORT 7276

static int supl_fd = -1;

int open_supl_socket(void)
{
	int err = -1;
	struct addrinfo *info;

	struct addrinfo hints = {
		.ai_family = AF_UNSPEC, /* Both IPv4 and IPv6 addresses accepted. */
		.ai_socktype = SOCK_STREAM
	};

	err = getaddrinfo(SUPL_SERVER, NULL, &hints, &info);
	if (err) {
		mosh_error("GNSS: Failed to resolve hostname %s, errno: %d)", SUPL_SERVER, errno);
		return -1;
	}

	/* Not connected */
	err = -1;

	for (struct addrinfo *addr = info; addr != NULL; addr = addr->ai_next) {
		char ip[INET6_ADDRSTRLEN] = { 0 };
		struct sockaddr *const sa = addr->ai_addr;

		switch (sa->sa_family) {
		case AF_INET6:
			((struct sockaddr_in6 *)sa)->sin6_port = htons(SUPL_SERVER_PORT);
			break;
		case AF_INET:
			((struct sockaddr_in *)sa)->sin_port = htons(SUPL_SERVER_PORT);
			break;
		}

		supl_fd = socket(sa->sa_family, SOCK_STREAM, IPPROTO_TCP);
		if (supl_fd < 0) {
			mosh_error("GNSS: Failed to create socket, errno %d", errno);
			goto cleanup;
		}

		/* The SUPL library expects a 1 second timeout for the read function. */
		struct timeval timeout = {
			.tv_sec = 1,
			.tv_usec = 0,
		};

		err = setsockopt(supl_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
		if (err) {
			mosh_error("GNSS: Failed to set socket timeout, errno %d", errno);
			goto cleanup;
		}

		inet_ntop(sa->sa_family,
			  (void *)&((struct sockaddr_in *)sa)->sin_addr,
			  ip,
			  INET6_ADDRSTRLEN);
		mosh_print("GNSS: Connecting to %s port %d", ip, SUPL_SERVER_PORT);

		err = connect(supl_fd, sa, addr->ai_addrlen);
		if (err) {
			close(supl_fd);
			supl_fd = -1;

			/* Try the next address */
			mosh_error("GNSS: Connecting to server failed, errno %d", errno);
		} else {
			/* Connected */
			break;
		}
	}

cleanup:
	freeaddrinfo(info);

	if (err) {
		/* Unable to connect, close socket */
		mosh_error("GNSS: Could not connect to SUPL server");
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
		mosh_error("GNSS: Failed to close SUPL socket");
	}
}

ssize_t supl_write(const void *buf, size_t nbytes, void *user_data)
{
	ARG_UNUSED(user_data);

	return send(supl_fd, buf, nbytes, 0);
}

int supl_logger(int level, const char *fmt, ...)
{
	char buffer[256] = { 0 };
	va_list args;

	va_start(args, fmt);
	int ret = vsnprintk(buffer, sizeof(buffer), fmt, args);

	va_end(args);

	if (ret < 0) {
		mosh_error("GNSS: %s: encoding error", __func__);
		return ret;
	} else if ((size_t)ret >= sizeof(buffer)) {
		mosh_error("GNSS: %s: too long message, it will be cut short", __func__);
	}

	mosh_print("GNSS: %s", buffer);

	return ret;
}

ssize_t supl_read(void *buf, size_t nbytes, void *user_data)
{
	ARG_UNUSED(user_data);

	ssize_t rc = recv(supl_fd, buf, nbytes, 0);

	if (rc < 0 && (errno == EAGAIN)) {
		/* Return 0 to indicate a timeout. */
		rc = 0;
	} else if (rc == 0) {
		/* Peer closed the socket, return an error. */
		rc = -1;
	}

	return rc;
}
