/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/** @file mqtt_transport_socket_tls.h
 *
 * @brief Internal functions to handle transport over TLS socket.
 */

#define LOG_MODULE_NAME net_mqtt_sock_tls
#define NET_LOG_LEVEL CONFIG_MQTT_LOG_LEVEL

#include <errno.h>
#include <net/socket.h>
#include <net/mqtt_socket.h>

#include "mqtt_os.h"

/**@brief Handles connect request for TLS socket transport.
 *
 * @param[in] client Identifies the client on which the procedure is requested.
 *
 * @retval 0 or an error code indicating reason for failure.
 */
int mqtt_client_tls_connect(struct mqtt_client *client)
{
	const struct sockaddr *broker = client->broker;
	struct mqtt_sec_config *tls_config = &client->transport.tls.config;
	int ret;

	client->transport.tls.sock = socket(broker->sa_family,
					    SOCK_STREAM, IPPROTO_TLS_1_2);
	if (client->transport.tls.sock < 0) {
		return -errno;
	}

	MQTT_TRC("Created socket %d", client->transport.tls.sock);

	/* Set secure socket options. */
	ret = setsockopt(client->transport.tls.sock, SOL_TLS, TLS_PEER_VERIFY,
			 &tls_config->peer_verify,
			 sizeof(tls_config->peer_verify));
	if (ret < 0) {
		goto error;
	}

	if (tls_config->cipher_list != NULL && tls_config->cipher_count > 0) {
		ret = setsockopt(client->transport.tls.sock, SOL_TLS,
				 TLS_CIPHERSUITE_LIST, tls_config->cipher_list,
				 sizeof(int) * tls_config->cipher_count);
		if (ret < 0) {
			goto error;
		}
	}

	if (tls_config->seg_tag_list != NULL && tls_config->sec_tag_count > 0) {
		ret = setsockopt(client->transport.tls.sock, SOL_TLS,
				 TLS_SEC_TAG_LIST, tls_config->seg_tag_list,
				 sizeof(sec_tag_t) * tls_config->sec_tag_count);
		if (ret < 0) {
			goto error;
		}
	}

	if (tls_config->hostname) {
		ret = setsockopt(client->transport.tls.sock, SOL_TLS,
				 TLS_HOSTNAME, tls_config->hostname,
				 strlen(tls_config->hostname));
		if (ret < 0) {
			goto error;
		}
	}

	size_t peer_addr_size = sizeof(struct sockaddr_in6);

	if (broker->sa_family == AF_INET) {
		peer_addr_size = sizeof(struct sockaddr_in);
	}

	ret = connect(client->transport.tls.sock, client->broker,
		      peer_addr_size);
	if (ret < 0) {
		goto error;
	}

	MQTT_TRC("Connect completed");
	return 0;

error:
	(void)close(client->transport.tls.sock);
	return -errno;
}

/**@brief Handles write requests on TLS socket transport.
 *
 * @param[in] client Identifies the client on which the procedure is requested.
 * @param[in] data Data to be written on the transport.
 * @param[in] datalen Length of data to be written on the transport.
 *
 * @retval 0 or an error code indicating reason for failure.
 */
int mqtt_client_tls_write(struct mqtt_client *client, const u8_t *data,
			  u32_t datalen)
{
	u32_t offset = 0;
	int ret;

	while (offset < datalen) {
		ret = send(client->transport.tls.sock, data + offset,
			   datalen - offset, 0);
		if (ret < 0) {
			return -errno;
		}

		offset += ret;
	}

	return 0;
}

/**@brief Handles read requests on TLS socket transport.
 *
 * @param[in] client Identifies the client on which the procedure is requested.
 * @param[in] data Pointer where read data is to be fetched.
 * @param[in] datalen Size of memory provided for the operation.
 *
 * @retval 0 or an error code indicating reason for failure.
 */
int mqtt_client_tls_read(struct mqtt_client *client, u8_t *data, u32_t *datalen)
{
	int ret;

	ret = recv(client->transport.tls.sock, data, *datalen, MSG_DONTWAIT);
	if (ret < 0) {
		return -errno;
	}

	*datalen = ret;
	return 0;
}

/**@brief Handles transport disconnection requests on TLS socket transport.
 *
 * @param[in] client Identifies the client on which the procedure is requested.
 *
 * @retval 0 or an error code indicating reason for failure.
 */
int mqtt_client_tls_disconnect(struct mqtt_client *client)
{
	int ret;

	MQTT_TRC("Closing socket %d", client->transport.tls.sock);
	ret = close(client->transport.tls.sock);
	if (ret < 0) {
		return -errno;
	}

	return 0;
}
