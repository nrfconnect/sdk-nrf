/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <zephyr.h>
#include <logging/log.h>
#include <shell/shell.h>
#if !defined(CONFIG_NET_SOCKETS_POSIX_NAMES)
#include <posix/unistd.h>
#include <posix/netdb.h>
#include <posix/sys/time.h>
#include <posix/sys/socket.h>
#include <posix/arpa/inet.h>
#else
#include <net/socket.h>
#endif
#include <supl_session.h>

#define SUPL_SERVER "supl.google.com"
#define SUPL_SERVER_PORT 7276

/* Number of getaddrinfo attempts */
#define GAI_ATTEMPT_COUNT 3

static int supl_fd;

extern const struct shell *shell_global;

int open_supl_socket(void)
{
	int err = -1;
	int proto;
	int gai_cnt = 0;
	uint16_t port;
	struct addrinfo *addr;
	struct addrinfo *info;

	proto = IPPROTO_TCP;
	port = htons(SUPL_SERVER_PORT);

	struct addrinfo hints = {
		.ai_family = AF_INET,
		.ai_socktype = SOCK_STREAM,
		.ai_protocol = proto,
		/* Either a valid,
		 * NULL-terminated access point name or NULL.
		 */
		.ai_canonname = NULL,
	};

	/* Try getaddrinfo many times, sleep a bit between retries */
	do {
		err = getaddrinfo(SUPL_SERVER, NULL, &hints, &info);
		gai_cnt++;

		if (err) {
			if (gai_cnt < GAI_ATTEMPT_COUNT) {
				/* Sleep between retries */
				k_sleep(K_MSEC(1000 * gai_cnt));
			} else {
				/* Return if no success after many retries */
				shell_error(
					shell_global,
					"GNSS: Failed to resolve hostname %s on IPv4, errno: %d)",
					SUPL_SERVER, errno);

				return -1;
			}
		}
	} while (err);

	/* Create socket */
	supl_fd = socket(AF_INET, SOCK_STREAM, proto);
	if (supl_fd < 0) {
		shell_error(shell_global,
			    "GNSS: Failed to create socket, errno %d", errno);
		goto cleanup;
	}

	struct timeval timeout = {
		.tv_sec = 1,
		.tv_usec = 0,
	};

	err = setsockopt(supl_fd, NRF_SOL_SOCKET, NRF_SO_RCVTIMEO, &timeout,
			 sizeof(timeout));
	if (err) {
		shell_error(shell_global,
			    "GNSS: Failed to set socket timeout, errno %d",
			    errno);
		goto cleanup;
	}

	/* Not connected */
	err = -1;

	for (addr = info; addr != NULL; addr = addr->ai_next) {
		struct sockaddr *const sa = addr->ai_addr;

		switch (sa->sa_family) {
		case AF_INET6:
			((struct sockaddr_in6 *)sa)->sin6_port = port;
			break;
		case AF_INET:
			((struct sockaddr_in *)sa)->sin_port = port;
			break;
		}

		err = connect(supl_fd, sa, addr->ai_addrlen);
		if (err) {
			/* Try next address */
			shell_error(shell_global,
				    "GNSS: Unable to connect, errno %d", errno);
		} else {
			/* Connected */
			break;
		}
	}

cleanup:
	freeaddrinfo(info);

	if (err) {
		/* Unable to connect, close socket */
		close(supl_fd);
		supl_fd = -1;
	}

	return 0;
}

void close_supl_socket(void)
{
	if (close(supl_fd) < 0) {
		shell_error(shell_global,
			    "GNSS: Failed to close SUPL socket");
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
		shell_error(shell_global, "GNSS: %s: encoding error",
			    __func__);
		return ret;
	} else if ((size_t)ret >= sizeof(buffer)) {
		shell_error(shell_global,
			    "GNSS: %s: too long message, it will be cut short",
			    __func__);
	}

	shell_print(shell_global, "GNSS: %s", buffer);

	return ret;
}

ssize_t supl_read(void *buf, size_t nbytes, void *user_data)
{
	ARG_UNUSED(user_data);

	ssize_t rc = recv(supl_fd, buf, nbytes, 0);

	if (rc < 0 && (errno == ETIMEDOUT)) {
		return 0;
	}

	return rc;
}
