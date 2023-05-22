/*
 * Copyright (c) 2021 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(psa_tls_client);

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


/** @brief Function for setting up the client socket to use for TLS handshake.
 *
 * @returns The socket identifier.
 */
static int setup_tls_client_socket(void)
{
	int err;
	int sock;
	int verify = TLS_PEER_VERIFY_REQUIRED;

	/* List of security tags to register. */
	sec_tag_t sec_tag_list[] = {
		CA_CERTIFICATE_TAG,
		PSK_TAG,
	};

	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TLS_1_2);
	if (sock < 0) {
		LOG_ERR("Failed to create a socket. Err: %d", errno);
		return -errno;
	}

	err = setsockopt(sock, SOL_TLS, TLS_PEER_VERIFY,
			 &verify, sizeof(verify));
	if (err < 0) {
		LOG_ERR("Failed to set TLS peer verification. Err %d", errno);
		(void)close(sock);
		return -errno;
	}

	err = setsockopt(sock, SOL_TLS, TLS_SEC_TAG_LIST, sec_tag_list,
			 sizeof(sec_tag_list));
	if (err < 0) {
		LOG_ERR("Failed to set TLS security TAG list. Err: %d", errno);
		(void)close(sock);
		return -errno;
	}

	err = setsockopt(sock, SOL_TLS, TLS_HOSTNAME,
			 TLS_PEER_HOSTNAME, sizeof(TLS_PEER_HOSTNAME));
	if (err < 0) {
		LOG_ERR("Failed to set TLS_HOSTNAME option. Err: %d", errno);
		(void)close(sock);
		return -errno;
	}

	return sock;
}

/** @brief Thread function that handles all the steps of the TLS handshake for
 *  a client. Loops forever and connects on given port number.
 *
 */
void process_psa_tls(void)
{
	int ret;
	int sock;
	int received;
	struct sockaddr_in server_addr;
	socklen_t server_addr_len = sizeof(server_addr);

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SERVER_PORT);
	inet_pton(AF_INET, CONFIG_NET_CONFIG_PEER_IPV4_ADDR,
		  &server_addr.sin_addr);

	while (true) {

		sock = setup_tls_client_socket();
		if (sock < 0) {
			LOG_INF("Retrying acquiring socket");
			k_sleep(K_MSEC(1000));
			continue;
		}

		LOG_INF("Initiates TLS connection on port %d ...",
			SERVER_PORT);
		ret = connect(sock, (struct sockaddr *)&server_addr,
			      server_addr_len);

		if (ret < 0) {
			LOG_WRN("Cannot make TLS connection. Err: %d", errno);
			LOG_INF("Will re-try connection in 1 second");
			(void)close(sock);
			k_sleep(K_MSEC(1000));
			continue;
		}
		LOG_INF("TLS handshake completed");

		while (true) {
			/* Try to receive data while connected */
			received = recv(sock, recv_buffer,
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

			LOG_INF("TLS received %d byte(s)", received);

			ret = psa_tls_send_buffer(sock, recv_buffer, received);
			if (ret != 0) {
				LOG_ERR("TLS Failed to send. Err: %d", errno);
				break;
			}

			LOG_INF("TLS replied with %d bytes", received);
		}

		LOG_INF("Closing TLS connection");
		(void)close(sock);
	}
}
