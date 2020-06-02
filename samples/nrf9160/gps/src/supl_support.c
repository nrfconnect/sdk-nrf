/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <supl_session.h>
#include <stdio.h>
#include <logging/log.h>
#include <net/socket.h>

#define SUPL_SERVER        "supl.google.com"
#define SUPL_SERVER_PORT   7276

/* Number of getaddrinfo attempts */
#define GAI_ATTEMPT_COUNT  3

static int supl_fd;

int open_supl_socket(void)
{
	int err = -1;
	int proto;
	int gai_cnt = 0;
	u16_t port;
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
				printk("Failed to resolve hostname %s on IPv4, errno: %d)\n",
					SUPL_SERVER, errno);

				return -1;
			}
		}
	} while (err);

	/* Create socket */
	supl_fd = socket(AF_INET, SOCK_STREAM, proto);
	if (supl_fd < 0) {
		printk("Failed to create socket, errno %d\n", errno);
		goto cleanup;
	}

	struct timeval timeout = {
		.tv_sec = 1,
		.tv_usec = 0,
	};

	err = setsockopt(supl_fd,
			 NRF_SOL_SOCKET,
			 NRF_SO_RCVTIMEO,
			 &timeout,
			 sizeof(timeout));
	if (err) {
		printk("Failed to setup socket timeout, errno %d\n", errno);
		return -1;
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
			char ip[255] = { 0 };

			inet_ntop(NRF_AF_INET,
				  (void *)&((struct sockaddr_in *)
				  sa)->sin_addr,
				  ip,
				  255);
			printk("ip %s (%x) port %d\n",
				ip,
				((struct sockaddr_in *)sa)->sin_addr.s_addr,
				ntohs(port));
			break;
		}

		err = connect(supl_fd, sa, addr->ai_addrlen);
		if (err) {
			/* Try next address */
			printk("Unable to connect, errno %d\n", errno);
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
