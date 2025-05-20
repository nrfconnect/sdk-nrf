/*
 * Copyright (c) 2023 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(psa_dtls_server);

#include <nrf.h>
#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/tls_credentials.h>
#include <zephyr/linker/sections.h>

#include "certificate.h"
#include "psa_tls_functions.h"
#include "psa_tls_credentials.h"

static char recv_buffer[RECV_BUFFER_SIZE]; /* Buffer for storing payload */

/** @brief Function for setting up the server socket to use for DTLS handshake.
 *
 * @returns The socket identifier.
 */
static int setup_dtls_server_socket(void)
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

	sock = socket(my_addr.sin_family, SOCK_DGRAM, IPPROTO_DTLS_1_2);
	if (sock < 0) {
		LOG_ERR("Failed to create a DTLS socket. Err: %d", errno);
		return -errno;
	}

	err = setsockopt(sock, SOL_TLS, TLS_SEC_TAG_LIST, sec_tag_list, sizeof(sec_tag_list));
	if (err < 0) {
		LOG_ERR("Failed to set DTLS security TAG list. Err: %d", errno);
		(void)close(sock);
		return -errno;
	}

	int role = TLS_DTLS_ROLE_SERVER;

	err = setsockopt(sock, SOL_TLS, TLS_DTLS_ROLE, &role, sizeof(role));
	if (err < 0) {
		LOG_ERR("Failed to set DTLS role secure option: %d", errno);
		(void)close(sock);
		return -errno;
	}

	err = bind(sock, (struct sockaddr *)&my_addr, sizeof(my_addr));
	if (err < 0) {
		LOG_ERR("Failed to bind DTLS socket. Err: %d", errno);
		(void)close(sock);
		return -errno;
	}

	return sock;
}

/** @brief Function that handles all the steps of the DTLS handshake for
 *  a server. Loops forever and accepts connections on the given port number.
 *
 */
void process_psa_tls(void)
{
	int ret;
	int client;
	int received = 0;
	struct sockaddr client_addr;
	socklen_t client_addr_len = sizeof(client_addr);

	while (true) {
		client = setup_dtls_server_socket();
		if (client < 0) {
			LOG_INF("Retrying to create a socket");
			k_sleep(K_MSEC(1000));
			continue;
		}

		LOG_INF("Waiting for DTLS connection on port %d ...", SERVER_PORT);

		while (true) {
			/* Try to receive data while connected */
			received = recvfrom(client, recv_buffer, sizeof(recv_buffer), 0,
					    &client_addr, &client_addr_len);

			if (received == 0) {
				/* Connection closed */
				LOG_INF("DTLS Connection closed");
				break;

			} else if (received < 0) {
				/* Socket error */
				LOG_ERR("DTLS Connection error: %d", errno);
				break;
			}

			LOG_INF("DTLS received %d bytes", received);

			ret = psa_tls_send_buffer(client, recv_buffer, received);
			if (ret != 0) {
				LOG_ERR("DTLS Failed to send. Err: %d", errno);
				break;
			}

			LOG_INF("DTLS replied with %d bytes", received);
		}

		LOG_INF("Closing DTLS connection");
		(void)close(client);
	}
}
