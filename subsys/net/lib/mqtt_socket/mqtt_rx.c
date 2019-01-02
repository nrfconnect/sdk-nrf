/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(net_mqtt_rx, CONFIG_MQTT_LOG_LEVEL);

#include "mqtt_internal.h"
#include "mqtt_os.h"

/** @file mqtt_rx.c
 *
 * @brief MQTT Received data handling.
 */

static int mqtt_handle_packet(struct mqtt_client *client, u8_t *data,
			      u32_t datalen, u32_t offset)
{
	int err_code = 0;
	bool notify_event = true;
	struct mqtt_evt evt;

	/* Success by default, overwritten in special cases. */
	evt.result = 0;

	switch (data[0] & 0xF0) {
	case MQTT_PKT_TYPE_CONNACK:
		MQTT_TRC("[CID %p]: Received MQTT_PKT_TYPE_CONNACK!", client);

		evt.type = MQTT_EVT_CONNACK;
		err_code = connect_ack_decode(client, data, datalen, offset,
					      &evt.param.connack);

		if (err_code == 0) {
			MQTT_TRC("[CID %p]: return_code: %d", client,
				 evt.param.connack.return_code);

			if (evt.param.connack.return_code ==
						MQTT_CONNECTION_ACCEPTED) {
				/* Set state. */
				MQTT_SET_STATE(client, MQTT_STATE_CONNECTED);
			}

			evt.result = evt.param.connack.return_code;
		} else {
			evt.result = err_code;
		}

		break;

	case MQTT_PKT_TYPE_PUBLISH:
		MQTT_TRC("[CID %p]: Received MQTT_PKT_TYPE_PUBLISH", client);

		evt.type = MQTT_EVT_PUBLISH;
		err_code = publish_decode(data, datalen, offset,
					  &evt.param.publish);
		evt.result = err_code;

		MQTT_TRC("PUB QoS:%02x, message len %08x, topic len %08x",
			 evt.param.publish.message.topic.qos,
			 evt.param.publish.message.payload.len,
			 evt.param.publish.message.topic.topic.size);

		break;

	case MQTT_PKT_TYPE_PUBACK:
		MQTT_TRC("[CID %p]: Received MQTT_PKT_TYPE_PUBACK!", client);

		evt.type = MQTT_EVT_PUBACK;
		err_code = publish_ack_decode(data, datalen, offset,
					      &evt.param.puback);
		evt.result = err_code;
		break;

	case MQTT_PKT_TYPE_PUBREC:
		MQTT_TRC("[CID %p]: Received MQTT_PKT_TYPE_PUBREC!", client);

		evt.type = MQTT_EVT_PUBREC;
		err_code = publish_receive_decode(data, datalen, offset,
						  &evt.param.pubrec);
		evt.result = err_code;
		break;

	case MQTT_PKT_TYPE_PUBREL:
		MQTT_TRC("[CID %p]: Received MQTT_PKT_TYPE_PUBREL!", client);

		evt.type = MQTT_EVT_PUBREL;
		err_code = publish_release_decode(data, datalen, offset,
						  &evt.param.pubrel);
		evt.result = err_code;
		break;

	case MQTT_PKT_TYPE_PUBCOMP:
		MQTT_TRC("[CID %p]: Received MQTT_PKT_TYPE_PUBCOMP!", client);

		evt.type = MQTT_EVT_PUBCOMP;
		err_code = publish_complete_decode(data, datalen, offset,
						   &evt.param.pubcomp);
		evt.result = err_code;
		break;

	case MQTT_PKT_TYPE_SUBACK:
		MQTT_TRC("[CID %p]: Received MQTT_PKT_TYPE_SUBACK!", client);

		evt.type = MQTT_EVT_SUBACK;
		err_code = subscribe_ack_decode(data, datalen, offset,
						&evt.param.suback);
		evt.result = err_code;
		break;

	case MQTT_PKT_TYPE_UNSUBACK:
		MQTT_TRC("[CID %p]: Received MQTT_PKT_TYPE_UNSUBACK!", client);

		evt.type = MQTT_EVT_UNSUBACK;
		err_code = unsubscribe_ack_decode(data, datalen, offset,
						  &evt.param.unsuback);
		evt.result = err_code;
		break;

	case MQTT_PKT_TYPE_PINGRSP:
		MQTT_TRC("[CID %p]: Received MQTT_PKT_TYPE_PINGRSP!", client);

		/* No notification of Ping response to application. */
		notify_event = false;
		break;

	default:
		/* Nothing to notify. */
		notify_event = false;
		break;
	}

	if (notify_event == true) {
		event_notify(client, &evt, MQTT_EVT_FLAG_NONE);
	}

	return err_code;
}

u32_t mqtt_handle_rx_data(struct mqtt_client *client, u8_t *data, u32_t datalen)
{
	int err_code = 0;
	u32_t offset = 0;

	while (offset < datalen) {
		u32_t start = offset;
		u32_t remaining_length = 0;

		offset = 1; /* Skip first byte to offset MQTT packet length. */
		err_code = packet_length_decode(data + start, datalen - start,
						&remaining_length, &offset);
		if (err_code != 0) {
			return datalen;
		}

		u32_t packet_length = offset + remaining_length;

		if (packet_length > MQTT_MAX_PACKET_LENGTH) {
			/* We receiving data we cannot handle. */
			return packet_length;
		}

		if (start + packet_length > datalen) {
			/* We may want to save this packet for later use.
			 * This means we received a truncated packet.
			 */
			return start;
		}

		err_code = mqtt_handle_packet(client, data + start,
					      packet_length, offset);
		if (err_code != 0) {
			return packet_length;
		}

		offset = start + packet_length;
	}

	return datalen;
}
