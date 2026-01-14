/*
 * Copyright (c) 2021 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <sys/socket.h>
#include <unistd.h>

#include <zephyr/logging/log.h>
#include <zephyr/net/tls_credentials.h>

#include <certificate.h>
#include <psa_tls_functions.h>
#include <psa_tls_credentials.h>

LOG_MODULE_REGISTER(psa_tls_server);

static char recv_buffer[RECV_BUFFER_SIZE]; /* Buffer for storing payload */

/** @brief Function for setting up the server socket to use for TLS handshake.
 *
 * @returns The socket identifier.
 */
static int setup_tls_server_socket(void)
{
	int err;
	int sock;
	struct sockaddr_in my_addr;

	/* List of security tags to register. */
	sec_tag_t sec_tag_list[] = {
		SERVER_CERTIFICATE_TAG,
		PSK_TAG,
	};

	memset(&my_addr, 0, sizeof(my_addr));
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(SERVER_PORT);
#if defined(CONFIG_MBEDTLS_TLS_VERSION_1_3)
	sock = socket(my_addr.sin_family, SOCK_STREAM, IPPROTO_TLS_1_3);
#else
	sock = socket(my_addr.sin_family, SOCK_STREAM, IPPROTO_TLS_1_2);
#endif

	err = setsockopt(sock, SOL_TLS, TLS_SEC_TAG_LIST, sec_tag_list,
			 sizeof(sec_tag_list));
	if (err < 0) {
		LOG_ERR("Failed to set TLS security TAG list. Err: %d", errno);
		(void)close(sock);
		return -errno;
	}

	int cache = TLS_SESSION_CACHE_ENABLED;

	err = setsockopt(sock, SOL_TLS, TLS_SESSION_CACHE, &cache, sizeof(cache));
	if (err < 0) {
		LOG_ERR("Failed to set TLS Session cache. Err: %d", errno);
		(void)close(sock);
		return -errno;
	}

	err = bind(sock, (struct sockaddr *)&my_addr, sizeof(my_addr));
	if (err < 0) {
		LOG_ERR("Failed to bind TLS socket. Err: %d", errno);
		(void)close(sock);
		return -errno;
	}

	err = listen(sock, MAX_CLIENT_QUEUE);
	if (err < 0) {
		LOG_ERR("Failed to listen on TLS socket. Err: %d", errno);
		(void)close(sock);
		return -errno;
	}

	return sock;
}

/** @brief Thread function that handles all the steps of the TLS handshake for
 *  a server. Loops forever and accepts connections on the given port number.
 *
 */
void process_psa_tls(void)
{
	int err;
	int sock;
	int client;
	int received;
	struct sockaddr_in client_addr;
	socklen_t client_addr_len = sizeof(client_addr);

retry:
	sock = setup_tls_server_socket();
	if (sock < 0) {
		LOG_INF("Retrying to create a socket");
		k_sleep(K_MSEC(1000));
		goto retry;
	}

	while (true) {
		LOG_INF("Waiting for TLS connection on port %d ...",
			SERVER_PORT);
		client = accept(sock, (struct sockaddr *)&client_addr,
				&client_addr_len);

		if (client < 0) {
			LOG_ERR("TLS accept error (%d)", -errno);
			(void)close(sock);
			k_sleep(K_MSEC(1000));
			goto retry;
		}

		LOG_INF("Accepted TLS connection");

		while (true) {
			/* Try to receive data while connected */
			received = recv(client, recv_buffer,
					sizeof(recv_buffer), 0);

			if (received == 0) {
				/* Connection closed */
				LOG_INF("TLS Connection closed");
				break;

			} else if (received < 0) {
				/* Socket error */
				LOG_ERR("TLS Connection error: %d", errno);
				break;
			}

			LOG_INF("TLS received %d bytes", received);

			err = psa_tls_send_buffer(client, recv_buffer, received);
			if (err != 0) {
				LOG_ERR("TLS Failed to send. Err: %d", errno);
				break;
			}

			LOG_INF("TLS replied with %d bytes", received);
		}

		LOG_INF("Closing TLS connection");
		(void)close(client);
	}
}
