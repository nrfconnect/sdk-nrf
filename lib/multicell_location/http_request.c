/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <stdio.h>
#if defined(CONFIG_POSIX_API)
#include <posix/arpa/inet.h>
#include <posix/unistd.h>
#include <posix/netdb.h>
#include <posix/sys/socket.h>
#else
#include <net/socket.h>
#endif
#include <modem/lte_lc.h>
#include <net/tls_credentials.h>
#include <modem/modem_key_mgmt.h>
#include <net/multicell_location.h>

#include <logging/log.h>

#include "location_service.h"

#define HTTPS_PORT	CONFIG_MULTICELL_LOCATION_HTTPS_PORT
#define HOSTNAME	CONFIG_MULTICELL_LOCATION_HOSTNAME

LOG_MODULE_DECLARE(multicell_location, CONFIG_MULTICELL_LOCATION_LOG_LEVEL);

static int tls_setup(int fd)
{
	int err;
	int verify = TLS_PEER_VERIFY_REQUIRED;
	/* Security tag that we have provisioned the certificate to */
	const sec_tag_t tls_sec_tag[] = {
		CONFIG_MULTICELL_LOCATION_TLS_SEC_TAG,
	};

	err = setsockopt(fd, SOL_TLS, TLS_PEER_VERIFY, &verify, sizeof(verify));
	if (err) {
		LOG_ERR("Failed to setup peer verification, err %d", errno);
		return -errno;
	}

	/* Associate the socket with the security tag
	 * we have provisioned the certificate with.
	 */
	err = setsockopt(fd, SOL_TLS, TLS_SEC_TAG_LIST, tls_sec_tag,
			 sizeof(tls_sec_tag));
	if (err) {
		LOG_ERR("Failed to setup TLS sec tag, error: %d", errno);
		return -errno;
	}

	err = setsockopt(fd, SOL_TLS, TLS_HOSTNAME, HOSTNAME, sizeof(HOSTNAME) - 1);
	if (err < 0) {
		LOG_ERR("Failed to set hostname option, errno: %d", errno);
		return -errno;
	}

	return 0;
}

int execute_http_request(const char *request, size_t request_len,
			 char *response, uint16_t response_len)
{
	int err, fd, bytes;
	size_t offset = 0;
	struct addrinfo *res;
	struct addrinfo hints = {
		.ai_family = AF_INET,
		.ai_socktype = SOCK_STREAM,
	};

	err = getaddrinfo(location_service_get_hostname(), NULL, &hints, &res);
	if (err) {
		LOG_ERR("getaddrinfo() failed, error: %d", err);
		return err;
	}

	if (IS_ENABLED(CONFIG_MULTICELL_LOCATION_LOG_LEVEL_DBG)) {
		char ip[INET6_ADDRSTRLEN];

		inet_ntop(AF_INET, &((struct sockaddr_in *)res->ai_addr)->sin_addr,
			  ip, sizeof(ip));
		LOG_DBG("IP address: %s", log_strdup(ip));
	}

	((struct sockaddr_in *)res->ai_addr)->sin_port = htons(HTTPS_PORT);

	fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TLS_1_2);
	if (fd == -1) {
		LOG_ERR("Failed to open socket, errno: %d", errno);
		err = -errno;
		goto clean_up;
	}

	/* Setup TLS socket options */
	err = tls_setup(fd);
	if (err) {
		goto clean_up;
	}

	if (CONFIG_MULTICELL_LOCATION_SEND_TIMEOUT > 0) {
		struct timeval timeout = {
			.tv_sec = CONFIG_MULTICELL_LOCATION_SEND_TIMEOUT,
		};

		err = setsockopt(fd,
				 SOL_SOCKET,
				 SO_SNDTIMEO,
				 &timeout,
				 sizeof(timeout));
		if (err) {
			LOG_ERR("Failed to setup socket send timeout, errno %d", errno);
			err = -errno;
			goto clean_up;
		}
	}

	if (CONFIG_MULTICELL_LOCATION_RECV_TIMEOUT > 0) {
		struct timeval timeout = {
			.tv_sec = CONFIG_MULTICELL_LOCATION_RECV_TIMEOUT,
		};

		err = setsockopt(fd,
				 SOL_SOCKET,
				 SO_RCVTIMEO,
				 &timeout,
				 sizeof(timeout));
		if (err) {
			LOG_ERR("Failed to setup socket receive timeout, errno %d", errno);
			err = -errno;
			goto clean_up;
		}
	}

	err = connect(fd, res->ai_addr, sizeof(struct sockaddr_in));
	if (err) {
		LOG_ERR("connect() failed, errno: %d", errno);
		err = -errno;
		goto clean_up;
	}

	do {
		bytes = send(fd, &request[offset], request_len - offset, 0);
		if (bytes < 0) {
			LOG_ERR("send() failed, errno: %d", errno);
			err = -errno;
			goto clean_up;
		}

		offset += bytes;
	} while (offset < request_len);

	LOG_DBG("Sent %d bytes", offset);

	offset = 0;

	do {
		/* TODO: This receive blocks */
		bytes = recv(fd, &response[offset], response_len - offset - 1, 0);
		if (bytes < 0) {
			if ((errno == EAGAIN) || (errno == EWOULDBLOCK) || (errno == ETIMEDOUT)) {
				LOG_WRN("Receive timeout, possibly incomplete data received");

				/* It's been observed that some services seemingly doesn't
				 * close the connection as expected, causing recv() to never
				 * return 0. In these cases we may have received all data,
				 * so we choose to break here to continue parsing while
				 * propagating the timeout information.
				 */
				err = -ETIMEDOUT;
				break;
			}

			LOG_ERR("recv() failed, errno: %d", errno);

			err = -errno;
			goto clean_up;
		} else {
			LOG_DBG("Received HTTP response chunk of %d bytes", bytes);
		}

		offset += bytes;
	} while (bytes != 0);

	response[offset] = '\0';

	LOG_DBG("Received %d bytes", offset);

	if (offset > 0) {
		LOG_DBG("HTTP response:\n%s\n", log_strdup(response));
	}

	LOG_DBG("Closing socket");

	/* Propagate timeout information */
	if (err != -ETIMEDOUT) {
		err = 0;
	}

clean_up:
	freeaddrinfo(res);
	(void)close(fd);

	return err;
}
