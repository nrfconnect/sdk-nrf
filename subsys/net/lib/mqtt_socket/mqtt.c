/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/** @file mqtt.c
 *
 * @brief MQTT Client API Implementation.
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(net_mqtt, CONFIG_MQTT_LOG_LEVEL);

#include <net/mqtt_socket.h>

#include "mqtt_transport.h"
#include "mqtt_internal.h"
#include "mqtt_os.h"

/** Number of buffers needed per client - one for RX and one for TX */
#define BUFFER_COUNT_PER_CLIENT 2

/** Total number of buffers needed by the module. */
#define TOTAL_BUFFER_COUNT (BUFFER_COUNT_PER_CLIENT * MQTT_MAX_CLIENTS)

/** MQTT Client table. */
static struct mqtt_client *mqtt_client[MQTT_MAX_CLIENTS];

/** Mutex used by MQTT implementation. */
struct k_mutex mqtt_mutex;

/** Memory slab submitted to the MQTT memory management.*/
struct k_mem_slab mqtt_slab;
static char __aligned(4) mqtt_slab_buffer[TOTAL_BUFFER_COUNT *
					  MQTT_MAX_PACKET_LENGTH];

static u32_t get_client_index(struct mqtt_client *client)
{
	for (u32_t index = 0; index < MQTT_MAX_CLIENTS; index++) {
		if (mqtt_client[index] == client) {
			return index;
		}
	}

	return MQTT_MAX_CLIENTS;
}

static void client_free(struct mqtt_client *client)
{
	MQTT_STATE_INIT(client);

	/* Free memory used for TX packets and reset the pointer. */
	if (client->tx_buf != NULL) {
		mqtt_free(client->tx_buf);
		client->tx_buf = NULL;
	}

	if (client->rx_buf != NULL) {
		mqtt_free(client->rx_buf);
		client->rx_buf = NULL;
	}
}

static void client_init(struct mqtt_client *client)
{
	memset(client, 0, sizeof(*client));

	MQTT_STATE_INIT(client);

	client->protocol_version = MQTT_VERSION_3_1_1;
	client->clean_session = 1;

	/* Allocate buffer packets in TX and RX path. */
	client->tx_buf = mqtt_malloc(MQTT_MAX_PACKET_LENGTH);
	client->rx_buf = mqtt_malloc(MQTT_MAX_PACKET_LENGTH);
}

/**@brief Notifies event to the application.
 *
 * @param[in] client Identifies the client for which the procedure is requested.
 * @param[in] evt Reason for disconnection.
 */
void event_notify(struct mqtt_client *client, const struct mqtt_evt *evt,
		  u32_t flags)
{
	const mqtt_evt_cb_t evt_cb = client->evt_cb;

	if (evt_cb != NULL) {
		mqtt_mutex_unlock();

		evt_cb(client, evt);

		mqtt_mutex_lock();
	}
}

/**@brief Notifies disconnection event to the application.
 *
 * @param[in] client Identifies the client for which the procedure is requested.
 * @param[in] result Reason for disconnection.
 */
static void disconnect_event_notify(struct mqtt_client *client, int result)
{
	const u32_t client_index = get_client_index(client);
	struct mqtt_evt evt;

	/* Remove the client from internal table. */
	if (client_index != MQTT_MAX_CLIENTS) {
		mqtt_client[client_index] = NULL;
	}

	/* Determine appropriate event to generate. */
	if (MQTT_VERIFY_STATE(client, MQTT_STATE_CONNECTED) ||
	    MQTT_VERIFY_STATE(client, MQTT_STATE_DISCONNECTING)) {
		evt.type = MQTT_EVT_DISCONNECT;
		evt.result = result;
	} else {
		evt.type = MQTT_EVT_CONNACK;
		evt.result = -ECONNREFUSED;
	}

	/* Free the instance. */
	client_free(client);

	/* Notify application. */
	event_notify(client, &evt, MQTT_EVT_FLAG_INSTANCE_RESET);
}

static void client_abort(struct mqtt_client *client)
{
	if (mqtt_transport_disconnect(client) < 0) {
		MQTT_ERR("Failed to disconnect transport!");
	} else {
		disconnect_event_notify(client, -ECONNABORTED);
	}
}

static int client_disconnect(struct mqtt_client *client, int result)
{
	int err_code = -EACCES;

	if (MQTT_VERIFY_STATE(client, MQTT_STATE_TCP_CONNECTED) ||
	    MQTT_VERIFY_STATE(client, MQTT_STATE_DISCONNECTING)) {
		err_code = mqtt_transport_disconnect(client);
		if (err_code < 0) {
			MQTT_ERR("Failed to disconnect transport!");
		} else {
			disconnect_event_notify(client, result);
		}
	}

	return err_code;
}

static int client_connect(struct mqtt_client *client)
{
	int err_code = mqtt_transport_connect(client);
	const u8_t *packet;
	u32_t packetlen;

	if (err_code == 0) {
		MQTT_SET_STATE(client, MQTT_STATE_TCP_CONNECTED);

		err_code = connect_request_encode(client, &packet, &packetlen);

		if (err_code == 0) {
			/* Send MQTT identification message to broker. */
			MQTT_SET_STATE(client, MQTT_STATE_PENDING_WRITE);

			err_code = mqtt_transport_write(client, packet,
							packetlen);

			MQTT_RESET_STATE(client, MQTT_STATE_PENDING_WRITE);
		}

		if (err_code == 0) {
			client->last_activity = mqtt_sys_tick_in_ms_get();
		} else {
			client_abort(client);
		}
	}

	MQTT_TRC("Connect completed");

	return err_code;
}

static int client_read(struct mqtt_client *client)
{
	u32_t data_len = MQTT_MAX_PACKET_LENGTH - client->rx_buf_datalen;
	int err_code = 0;

	err_code = mqtt_transport_read(client,
				       client->rx_buf + client->rx_buf_datalen,
				       &data_len);

	if (err_code < 0) {
		if (err_code == -EAGAIN) {
			/* No data received yet. Try again later. */
			err_code = 0;
		} else {
			MQTT_TRC("Error receiving data, error = %d, "
				 "closing connection", err_code);
			client_abort(client);
		}
	} else {
		if (data_len == 0) {
			/* Receiving 0 bytes indicates an orderly shutdown. */
			MQTT_TRC("Received end of stream, closing connection");
			err_code = client_disconnect(client, 0);
		} else {
			u32_t processed_length = 0;

			client->rx_buf_datalen += data_len;

			processed_length =
				mqtt_handle_rx_data(client,
						    client->rx_buf,
						    client->rx_buf_datalen);

			MQTT_TRC("Processed %d bytes", processed_length);

			if (processed_length > client->rx_buf_datalen) {
				client_disconnect(client, -EIO);
				err_code = -EIO;
			} else {
				/* Flush data consumed. */
				client->rx_buf_datalen -= processed_length;
				if (client->rx_buf_datalen > 0) {
					memmove(
					    client->rx_buf,
					    client->rx_buf + processed_length,
					    client->rx_buf_datalen);
				}
			}
		}
	}

	return err_code;
}

static int client_write(struct mqtt_client *client, const u8_t *data,
			u32_t datalen)
{
	int err_code;

	MQTT_TRC("[%p]: Transport writing %d bytes.", client, datalen);

	MQTT_SET_STATE(client, MQTT_STATE_PENDING_WRITE);

	err_code = mqtt_transport_write(client, data, datalen);

	MQTT_RESET_STATE(client, MQTT_STATE_PENDING_WRITE);

	if (err_code != 0) {
		MQTT_TRC("TCP write failed, errno = %d, "
			 "closing connection", errno);
		client_disconnect(client, err_code);
		return -EIO;
	}

	MQTT_TRC("[%p]: Transport write complete.", client);
	client->last_activity = mqtt_sys_tick_in_ms_get();

	return 0;
}

int mqtt_init(void)
{
	mqtt_mutex_init();

	mqtt_mutex_lock();

	memset(mqtt_client, 0, sizeof(mqtt_client));

	k_mem_slab_init(&mqtt_slab, mqtt_slab_buffer,
			MQTT_MAX_PACKET_LENGTH, TOTAL_BUFFER_COUNT);

	mqtt_mutex_unlock();

	return 0;
}

void mqtt_client_init(struct mqtt_client *client)
{
	NULL_PARAM_CHECK_VOID(client);

	mqtt_mutex_lock();

	client_init(client);

	mqtt_mutex_unlock();
}

int mqtt_connect(struct mqtt_client *client)
{
	/* Look for a free instance if available. */
	int err_code;
	u32_t client_index;

	NULL_PARAM_CHECK(client);
	NULL_PARAM_CHECK(client->client_id.utf8);

	mqtt_mutex_lock();

	for (client_index = 0; client_index < MQTT_MAX_CLIENTS;
	     client_index++) {
		if (mqtt_client[client_index] == NULL) {
			/* Found a free instance. */
			mqtt_client[client_index] = client;
			break;
		}
	}

	if ((client_index == MQTT_MAX_CLIENTS) || (client->tx_buf == NULL) ||
	    (client->rx_buf == NULL)) {
		client_free(client);
		err_code = -ENOMEM;
	} else {
		err_code = client_connect(client);
		if (err_code != 0) {
			/* Free the instance. */
			client_free(client);
			mqtt_client[client_index] = NULL;
			err_code = -ECONNREFUSED;
		}
	}

	mqtt_mutex_unlock();

	return err_code;
}

static int verify_tx_state(const struct mqtt_client *client)
{
	if (MQTT_VERIFY_STATE(client, MQTT_STATE_PENDING_WRITE)) {
		return -EBUSY;
	}

	if (!MQTT_VERIFY_STATE(client, MQTT_STATE_CONNECTED)) {
		return -ENOTCONN;
	}

	return 0;
}

int mqtt_publish(struct mqtt_client *client,
		 const struct mqtt_publish_param *param)
{
	int err_code;
	const u8_t *packet;
	u32_t packetlen;

	NULL_PARAM_CHECK(client);
	NULL_PARAM_CHECK(param);

	MQTT_TRC("[CID %p]:[State 0x%02x]: >> Topic size 0x%08x, "
		 "Data size 0x%08x", client, client->state,
		 param->message.topic.topic.size,
		 param->message.payload.len);

	mqtt_mutex_lock();

	err_code = verify_tx_state(client);
	if (err_code == 0) {
		err_code = publish_encode(client, param, &packet, &packetlen);

		if (err_code == 0) {
			err_code = client_write(client, packet, packetlen);
		}
	}

	mqtt_mutex_unlock();

	MQTT_TRC("[CID %p]:[State 0x%02x]: << result 0x%08x",
			 client, client->state, err_code);

	return err_code;
}

int mqtt_publish_qos1_ack(struct mqtt_client *client,
			  const struct mqtt_puback_param *param)
{
	int err_code;
	const u8_t *packet;
	u32_t packetlen;

	NULL_PARAM_CHECK(client);
	NULL_PARAM_CHECK(param);

	MQTT_TRC("[CID %p]:[State 0x%02x]: >> Message id 0x%04x",
		 client, client->state, param->message_id);

	mqtt_mutex_lock();

	err_code = verify_tx_state(client);
	if (err_code == 0) {
		err_code = publish_ack_encode(client, param, &packet,
					      &packetlen);

		if (err_code == 0) {
			err_code = client_write(client, packet, packetlen);
		}
	}

	mqtt_mutex_unlock();

	MQTT_TRC("[CID %p]:[State 0x%02x]: << result 0x%08x",
		 client, client->state, err_code);

	return err_code;
}

int mqtt_publish_qos2_receive(struct mqtt_client *client,
			      const struct mqtt_pubrec_param *param)
{
	int err_code;
	const u8_t *packet;
	u32_t packetlen;

	NULL_PARAM_CHECK(client);
	NULL_PARAM_CHECK(param);

	MQTT_TRC("[CID %p]:[State 0x%02x]: >> Message id 0x%04x",
		 client, client->state, param->message_id);

	mqtt_mutex_lock();

	err_code = verify_tx_state(client);
	if (err_code == 0) {
		err_code = publish_receive_encode(client, param, &packet,
						   &packetlen);

		if (err_code == 0) {
			err_code = client_write(client, packet, packetlen);
		}
	}

	mqtt_mutex_unlock();

	MQTT_TRC("[CID %p]:[State 0x%02x]: << result 0x%08x",
		 client, client->state, err_code);

	return err_code;
}

int mqtt_publish_qos2_release(struct mqtt_client *client,
			      const struct mqtt_pubrel_param *param)
{
	int err_code;
	const u8_t *packet;
	u32_t packetlen;

	NULL_PARAM_CHECK(client);
	NULL_PARAM_CHECK(param);

	MQTT_TRC("[CID %p]:[State 0x%02x]: >> Message id 0x%04x",
		 client, client->state, param->message_id);

	mqtt_mutex_lock();

	err_code = verify_tx_state(client);
	if (err_code == 0) {
		err_code = publish_release_encode(client, param, &packet,
						   &packetlen);

		if (err_code == 0) {
			err_code = client_write(client, packet, packetlen);
		}
	}

	mqtt_mutex_unlock();

	MQTT_TRC("[CID %p]:[State 0x%02x]: << result 0x%08x",
		 client, client->state, err_code);

	return err_code;
}

int mqtt_publish_qos2_complete(struct mqtt_client *client,
			       const struct mqtt_pubcomp_param *param)
{
	int err_code;
	const u8_t *packet;
	u32_t packetlen;

	NULL_PARAM_CHECK(client);
	NULL_PARAM_CHECK(param);

	MQTT_TRC("[CID %p]:[State 0x%02x]: >> Message id 0x%04x",
		 client, client->state, param->message_id);

	mqtt_mutex_lock();

	err_code = verify_tx_state(client);
	if (err_code == 0) {
		err_code = publish_complete_encode(client, param, &packet,
						   &packetlen);

		if (err_code == 0) {
			err_code = client_write(client, packet, packetlen);
		}
	}

	mqtt_mutex_unlock();

	MQTT_TRC("[CID %p]:[State 0x%02x]: << result 0x%08x",
		 client, client->state, err_code);

	return err_code;
}

int mqtt_disconnect(struct mqtt_client *client)
{
	int err_code;
	const u8_t *packet;
	u32_t packetlen;

	NULL_PARAM_CHECK(client);

	mqtt_mutex_lock();

	err_code = verify_tx_state(client);
	if (err_code == 0) {
		err_code = disconnect_encode(client, &packet, &packetlen);

		if (err_code == 0) {
			err_code = client_write(client, packet, packetlen);
		}

		if (err_code == 0) {
			MQTT_SET_STATE_EXCLUSIVE(client,
						 MQTT_STATE_DISCONNECTING);
		}
	}

	mqtt_mutex_unlock();

	return err_code;
}

int mqtt_subscribe(struct mqtt_client *client,
		   const struct mqtt_subscription_list *param)
{
	int err_code;
	const u8_t *packet;
	u32_t packetlen;

	NULL_PARAM_CHECK(client);
	NULL_PARAM_CHECK(param);

	MQTT_TRC("[CID %p]:[State 0x%02x]: >> message id 0x%04x "
		 "topic count 0x%04x", client, client->state,
		 param->message_id, param->list_count);

	mqtt_mutex_lock();

	err_code = verify_tx_state(client);
	if (err_code == 0) {
		err_code = subscribe_encode(client, param, &packet, &packetlen);

		if (err_code == 0) {
			err_code = client_write(client, packet, packetlen);
		}
	}

	MQTT_TRC("[CID %p]:[State 0x%02x]: << result 0x%08x",
		 client, client->state, err_code);

	mqtt_mutex_unlock();

	return err_code;
}

int mqtt_unsubscribe(struct mqtt_client *client,
		     const struct mqtt_subscription_list *param)
{
	int err_code;
	const u8_t *packet;
	u32_t packetlen;

	NULL_PARAM_CHECK(client);
	NULL_PARAM_CHECK(param);

	mqtt_mutex_lock();

	err_code = verify_tx_state(client);
	if (err_code == 0) {
		err_code = unsubscribe_encode(client, param, &packet,
					      &packetlen);

		if (err_code == 0) {
			err_code = client_write(client, packet, packetlen);
		}
	}

	mqtt_mutex_unlock();

	return err_code;
}

int mqtt_ping(struct mqtt_client *client)
{
	int err_code;
	const u8_t *packet;
	u32_t packetlen;

	NULL_PARAM_CHECK(client);

	mqtt_mutex_lock();

	err_code = verify_tx_state(client);
	if (err_code == 0) {
		err_code = ping_request_encode(client, &packet, &packetlen);

		if (err_code == 0) {
			err_code = client_write(client, packet, packetlen);
		}
	}

	mqtt_mutex_unlock();

	return err_code;
}

int mqtt_abort(struct mqtt_client *client)
{
	mqtt_mutex_lock();

	NULL_PARAM_CHECK(client);

	if (client->state != MQTT_STATE_IDLE) {
		client_abort(client);
	}

	mqtt_mutex_unlock();

	return 0;
}

int mqtt_live(void)
{
	u32_t elapsed_time;
	u32_t index;

	mqtt_mutex_lock();

	for (index = 0; index < MQTT_MAX_CLIENTS; index++) {
		struct mqtt_client *client = mqtt_client[index];

		if (client != NULL) {
			if (MQTT_VERIFY_STATE(client,
					      MQTT_STATE_DISCONNECTING)) {
				client_disconnect(client, 0);
			} else {
				elapsed_time = mqtt_elapsed_time_in_ms_get(
							client->last_activity);

				if ((MQTT_KEEPALIVE > 0) &&
				    (elapsed_time >= (MQTT_KEEPALIVE * 1000))) {
					(void)mqtt_ping(client);
				}
			}
		}
	}

	mqtt_mutex_unlock();

	return 0;
}

int mqtt_input(struct mqtt_client *client)
{
	int err_code;

	NULL_PARAM_CHECK(client);

	mqtt_mutex_lock();

	MQTT_TRC("state:0x%08x", client->state);

	if (MQTT_VERIFY_STATE(client, MQTT_STATE_DISCONNECTING)) {
		err_code = client_disconnect(client, 0);
	} else if (MQTT_VERIFY_STATE(client, MQTT_STATE_TCP_CONNECTED)) {
		err_code = client_read(client);
	} else {
		err_code = -EACCES;
	}

	mqtt_mutex_unlock();

	return err_code;
}
