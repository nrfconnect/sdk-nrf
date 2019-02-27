/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <net/socket.h>
#include <stddef.h>
#include <zephyr/types.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(http_client);

int httpc_connect(const char *const host, const char *const port, u32_t family,
		  u32_t proto)
{
	int fd;

	if (host == NULL) {
		return -1;
	}

	struct addrinfo *addrinf = NULL;
	struct addrinfo hints = {
	    .ai_family = family,
	    .ai_socktype = SOCK_STREAM,
	    .ai_protocol = proto,
	};

	LOG_ERR("Requesting getaddrinfo() for %s", host);

	/* DNS resolve the port. */
	int rc = getaddrinfo(host, port, &hints, &addrinf);
	if (rc < 0 || (addrinf == NULL)) {
		LOG_ERR("getaddrinfo() failed, err %d", errno);
		return -1;
	}

	struct addrinfo *addr = addrinf;
	struct sockaddr *remoteaddr;

	int addrlen = (family == AF_INET6)
			  ? sizeof(struct sockaddr_in6)
			  : sizeof(struct sockaddr_in);

	/* Open a socket based on the local address. */

	fd = socket(family, SOCK_STREAM, proto);
	if (fd >= 0) {
		/* Look for IPv4 address of the broker. */
		while (addr != NULL) {
			remoteaddr = addr->ai_addr;

			LOG_INF("Resolved address family %d\n", addr->ai_family);
			LOG_INF("Resolved address family %d\n", remoteaddr->sa_family);

			if (remoteaddr->sa_family == family) {
				((struct sockaddr_in*)remoteaddr)->sin_port = htons(80);

				LOG_HEXDUMP_INF((const uint8_t *)remoteaddr, addr->ai_addrlen, "Resolved addr");

				/* TODO: Need to set security setting for HTTPS. */
				rc = connect(fd, remoteaddr, addrlen);
				if (rc == 0) {
					break;
				}
			}
			addr++;
		}
	}

	freeaddrinfo(addrinf);

	if (rc < 0) {
		close(fd);
		fd = -1;
	}

	return fd;
}

void httpc_disconnect(int fd)
{
	(void)close(fd);
}

int httpc_request(int fd, char *req, u32_t len)
{
	return send(fd, req, len, 0);
}

int httpc_recv(int fd, char *buf, u32_t len, u32_t flags)
{
	return recv(fd, buf, len, flags);
}
