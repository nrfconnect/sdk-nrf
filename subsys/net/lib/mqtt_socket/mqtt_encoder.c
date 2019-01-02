/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/** @file mqtt_encoder.c
 *
 * @brief Encoding functions needed to create packet to be sent to the broker.
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(net_mqtt_enc, CONFIG_MQTT_LOG_LEVEL);

#include "mqtt_internal.h"
#include "mqtt_os.h"

#define MQTT_3_1_0_PROTO_DESC_LEN 6
#define MQTT_3_1_1_PROTO_DESC_LEN 4

static const u8_t mqtt_3_1_0_proto_desc_str[MQTT_3_1_0_PROTO_DESC_LEN] = {
	'M', 'Q', 'I', 's', 'd', 'p'
};
static const u8_t mqtt_3_1_1_proto_desc_str[MQTT_3_1_1_PROTO_DESC_LEN] = {
	'M', 'Q', 'T', 'T'
};

static const struct mqtt_utf8 mqtt_3_1_0_proto_desc = {
	.utf8 = (u8_t *)mqtt_3_1_0_proto_desc_str,
	.size = MQTT_3_1_0_PROTO_DESC_LEN
};

static const struct mqtt_utf8 mqtt_3_1_1_proto_desc = {
	.utf8 = (u8_t *)mqtt_3_1_1_proto_desc_str,
	.size = MQTT_3_1_1_PROTO_DESC_LEN
};

/** Never changing ping request, needed for Keep Alive. */
static const u8_t ping_packet[MQTT_PKT_HEADER_SIZE] = {
	MQTT_PKT_TYPE_PINGREQ,
	0x00
};

/** Never changing disconnect request. */
static const u8_t disc_packet[MQTT_PKT_HEADER_SIZE] = {
	MQTT_PKT_TYPE_DISCONNECT,
	0x00
};

/**
 * @brief Packs unsigned 8 bit value to the buffer at the offset requested.
 *
 * @param[in] val Value to be packed.
 * @param[in] buffer_len Total size of the buffer on which value is to be
 *                       packed. This shall not be zero.
 * @param[out] buffer Buffer where the value is to be packed.
 * @param[inout] offset Offset on the buffer where the value is to be packed.
 *                      If the procedure is successful, the offset is
 *                      incremented to point to the next write/pack location
 *                      on the buffer.
 *
 * @retval 0 if procedure is successful.
 * @retval -EINVAL if the offset is greater than or equal to the buffer length.
 */
static int pack_uint8(u8_t val, u32_t buffer_len, u8_t *buffer,
		      u32_t *offset)
{
	int err_code = -EINVAL;

	if (buffer_len > *offset) {
		MQTT_TRC(">> V:%02x BL:%08x, B:%p, O:%08x", val, buffer_len,
			 buffer, *offset);

		/* Pack value. */
		buffer[*offset] = val;

		/* Increment offset. */
		*offset += sizeof(u8_t);

		/* Indicate success. */
		err_code = 0;
	}

	return err_code;
}

/**
 * @brief Packs unsigned 16 bit value to the buffer at the offset requested.
 *
 * @param[in] val Value to be packed.
 * @param[in] buffer_len Total size of the buffer on which value is to be
 *                       packed. This shall not be zero.
 * @param[out] buffer Buffer where the value is to be packed.
 * @param[inout] offset Offset on the buffer where the value is to be packed.
 *                      If the procedure is successful, the offset is
 *                      incremented to point to the next write/pack location on
 *                      the buffer.
 *
 * @retval 0 if the procedure is successful.
 * @retval -EINVAL if the offset is greater than or equal to the buffer length
 *                 minus the size of unsigned 16 bit integer.
 */
static int pack_uint16(u16_t val, u32_t buffer_len, u8_t *buffer,
		       u32_t *offset)
{
	int err_code = -EINVAL;

	if (buffer_len > *offset) {
		const u32_t available_len = buffer_len - *offset;

		MQTT_TRC(">> V:%04x BL:%08x, B:%p, O:%08x A:%08x", val,
			 buffer_len, buffer, *offset, available_len);

		if (available_len >= sizeof(u16_t)) {
			/* Pack value. */
			buffer[*offset] = (val >> 8) & 0xFF;
			buffer[*offset + 1] = val & 0xFF;

			/* Increment offset. */
			*offset += sizeof(u16_t);

			/* Indicate success. */
			err_code = 0;
		}
	}

	return err_code;
}

/**
 * @brief Packs utf8 string to the buffer at the offset requested.
 *
 * @param[in] str UTF-8 string and its length to be packed.
 * @param[in] buffer_len Total size of the buffer on which string is to be
 *                       packed. This shall not be zero.
 * @param[out] buffer Buffer where the string is to be packed.
 * @param[inout] offset Offset on the buffer where the string is to be packed.
 *                      If the procedure is successful, the offset is
 *                      incremented to point to the next write/pack location on
 *                      the buffer.
 *
 * @retval 0 if the procedure is successful.
 * @retval -EINVAL if the offset is greater than or equal to the buffer length
 *                 minus the size of unsigned 16 bit integer.
 * @retval -ENOMEM if there is no room on the buffer to pack the string.
 */
static int pack_utf8_str(const struct mqtt_utf8 *str, u32_t buffer_len,
			 u8_t *buffer, u32_t *offset)
{
	int err_code = -EINVAL;

	if (buffer_len > *offset) {
		const u32_t available_len = buffer_len - *offset;

		MQTT_TRC(">> USL:%08x BL:%08x, B:%p, O:%08x A:%08x",
			 GET_UT8STR_BUFFER_SIZE(str), buffer_len,
			 buffer, *offset, available_len);

		if (available_len >= GET_UT8STR_BUFFER_SIZE(str)) {
			/* Length followed by string. */
			err_code = pack_uint16(str->size, buffer_len,
					       buffer, offset);

			if (err_code == 0) {
				memcpy(&buffer[*offset], str->utf8,
				       str->size);

				*offset += str->size;
			}
		} else {
			err_code = -ENOMEM;
		}
	}

	return err_code;
}

/**
 * @brief Packs binary string to the buffer at the offset requested.
 *
 * @param[in] str Binary string and its length to be packed.
 * @param[in] buffer_len Total size of the buffer on which string is to be
 *                       packed.
 * @param[in] buffer Buffer where the string is to be packed.
 * @param[inout] offset Offset on the buffer where the string is to be packed.
 *                      If the procedure is successful, the offset is
 *                      incremented to point to the next write/pack location on
 *                      the buffer.
 *
 * @retval 0 if the procedure is successful.
 * @retval -EINVAL if the offset is greater than or equal to the buffer length.
 * @retval -ENOMEM if there is no room on the buffer to pack the string.
 */
static int pack_data(const struct mqtt_binstr *str, u32_t buffer_len,
		     u8_t *buffer, u32_t *offset)
{
	int err_code = -EINVAL;

	if (buffer_len > *offset) {
		const u32_t available_len = buffer_len - *offset;

		MQTT_TRC(">> BSL:%08x BL:%08x, B:%p, O:%08x A:%08x",
			 GET_BINSTR_BUFFER_SIZE(str), buffer_len,
			 buffer, *offset, available_len);

		if (available_len >= GET_BINSTR_BUFFER_SIZE(str)) {
			memcpy(&buffer[*offset], str->data,
			       str->len);

			*offset += str->len;
			err_code = 0;
		} else {
			err_code = -ENOMEM;
		}
	}

	return err_code;
}

/**
 * @brief Computes and encodes length for the MQTT fixed header.
 *
 * @note The remaining length is not packed as a fixed unsigned 32 bit integer.
 *       Instead it is packed on algorithm below:
 *
 * @code
 * do
 *            encodedByte = X MOD 128
 *            X = X DIV 128
 *            // if there are more data to encode, set the top bit of this byte
 *            if ( X > 0 )
 *                encodedByte = encodedByte OR 128
 *            endif
 *                'output' encodedByte
 *       while ( X > 0 )
 * @endcode
 *
 * @param[in] remaining_length Length of variable header and payload in the
 *                             MQTT message.
 * @param[out] buffer Buffer where the length is to be packed.
 * @param[inout] offset Offset on the buffer where the length is to be packed.
 */
static void packet_length_encode(u32_t remaining_length, u8_t *buff,
				 u32_t *size)
{
	u16_t index = 0;
	const u32_t offset = *size;

	MQTT_TRC(">> RL:0x%08x O:%08x P:%p", remaining_length, offset, buff);

	do {
		if (buff != NULL) {
			buff[offset + index] = remaining_length &
							MQTT_LENGTH_VALUE_MASK;
		}

		remaining_length >>= MQTT_LENGTH_SHIFT;
		if (remaining_length > 0) {
			if (buff != NULL) {
				buff[offset + index] |=
						MQTT_LENGTH_CONTINUATION_BIT;
			}
		}

		index++;

	} while (remaining_length > 0);

	MQTT_TRC("<< RLS:0x%08x", index);

	*size += index;
}

/**
 * @brief Encodes fixed header for the MQTT message and provides pointer to
 *        start of the header.
 *
 * @param[in] message_type Message type containing packet type and the flags.
 *                         Use @ref MQTT_MESSAGES_OPTIONS to construct the
 *                         message_type.
 * @param[in] length Length of the encoded variable header and payload.
 * @param[inout] packet Pointer to the MQTT message variable header and payload.
 *                      The 5 bytes before the start of the message are assumed
 *                      by the routine to be available to pack the fixed header.
 *                      However, since the fixed header length is variable
 *                      length, the pointer to the start of the MQTT message
 *                      along with encoded fixed header is supplied as output
 *                      parameter if the procedure was successful.
 *
 * @retval 0xFFFFFFFF if the procedure failed, else length of total MQTT message
 *                    along with the fixed header.
 */
static u32_t mqtt_encode_fixed_header(u8_t message_type, u32_t length,
				      u8_t **packet)
{
	u32_t packet_length = 0xFFFFFFFF;

	if (length <= MQTT_MAX_PAYLOAD_SIZE) {
		u32_t offset = 1;

		MQTT_TRC("<< MT:0x%02x L:0x%08x", message_type, length);
		packet_length_encode(length, NULL, &offset);

		MQTT_TRC("Remaining length size = %02x", offset);

		u8_t *mqtt_header = *packet - offset;

		/* Reset offset. */
		offset = 0;
		(void)pack_uint8(message_type, MQTT_MAX_PACKET_LENGTH,
				 mqtt_header, &offset);
		packet_length_encode(length, mqtt_header, &offset);

		*packet = mqtt_header;

		packet_length = (length + offset);
	}

	return packet_length;
}

/**
 * @brief Encodes a string of a zero length.
 *
 * @param[in] buffer_len Total size of the buffer on which string will be
 *                       encoded. This shall not be zero.
 * @param[out] buffer Buffer where the string is to be encoded.
 * @param[inout] offset Offset on the buffer where the string is to be encoded.
 *                      If the procedure is successful, the offset is
 *                      incremented to point to the next write/pack location on
 *                      the buffer.
 *
 * @retval 0 if the procedure is successful.
 * @retval -EINVAL if the offset is greater than or equal to the buffer length.
 */
static int zero_len_str_encode(u32_t buffer_len, u8_t *buffer,
			       u32_t *offset)
{
	return pack_uint16(0x0000, buffer_len, buffer, offset);
}

/**
 * @brief Encodes and sends messages that contain only message id in
 *        the variable header.
 *
 * @param[in] client Identifies the client for which the procedure is requested.
 * @param[in] message_type Message type and reserved bit fields.
 * @param[in] message_id Message id to be encoded in the variable header.
 * @param[out] packet A pointer to store a pointer to encoded message.
 * @param[out] packet_length A pointer to store message length.
 * @retval 0 or an error code indicating a reason for failure.
 */
static int mqtt_message_id_only_enc(const struct mqtt_client *client,
				    u8_t message_type, u16_t message_id,
				    const u8_t **packet, u32_t *packet_length)
{
	int err_code = -ENOTCONN;
	u32_t offset = 0;
	u32_t mqtt_packetlen = 0;
	u8_t *payload;

	/* Message id zero is not permitted by spec. */
	if (message_id == 0) {
		return -EINVAL;
	}

	payload = &client->tx_buf[MQTT_FIXED_HEADER_EXTENDED_SIZE];
	memset(payload, 0, MQTT_MAX_VARIABLE_HEADER_N_PAYLOAD);

	err_code = pack_uint16(message_id,
			       MQTT_MAX_VARIABLE_HEADER_N_PAYLOAD,
			       payload, &offset);

	if (err_code == 0) {
		mqtt_packetlen = mqtt_encode_fixed_header(message_type,
							  offset,
							  &payload);

		*packet_length = mqtt_packetlen;
		*packet = payload;
	} else {
		*packet_length = 0;
		*packet = NULL;
	}

	return err_code;
}

int connect_request_encode(const struct mqtt_client *client,
			   const u8_t **packet, u32_t *packet_length)
{
	u32_t offset = 0;
	u8_t *payload = &client->tx_buf[MQTT_FIXED_HEADER_EXTENDED_SIZE];
	/* Clean session always. */
	u8_t connect_flags = client->clean_session << 1;
	int err_code;
	const struct mqtt_utf8 *mqtt_proto_desc;

	if (client->protocol_version == MQTT_VERSION_3_1_1) {
		mqtt_proto_desc = &mqtt_3_1_1_proto_desc;
	} else {
		mqtt_proto_desc = &mqtt_3_1_0_proto_desc;
	}

	memset(payload, 0, MQTT_MAX_VARIABLE_HEADER_N_PAYLOAD);

	/* Pack protocol description. */
	MQTT_TRC("Encoding Protocol Description. Str:%s Size:%08x.",
		 mqtt_proto_desc->utf8, mqtt_proto_desc->size);

	err_code = pack_utf8_str(
		mqtt_proto_desc, MQTT_MAX_VARIABLE_HEADER_N_PAYLOAD,
		payload, &offset);
	if (err_code == 0) {
		MQTT_TRC("Encoding Protocol Version %02x.",
			 client->protocol_version);
		/* Pack protocol version. */
		err_code = pack_uint8(client->protocol_version,
				      MQTT_MAX_VARIABLE_HEADER_N_PAYLOAD,
				      payload, &offset);
	}

	/* Remember position of connect flag and leave one byte for it to
	 * be packed once we determine its value.
	 */
	const u32_t connect_flag_offset =
		MQTT_FIXED_HEADER_EXTENDED_SIZE + offset;
	offset++;

	if (err_code == 0) {
		MQTT_TRC("Encoding Keep Alive Time %04x.", MQTT_KEEPALIVE);
		/* Pack keep alive time. */
		err_code = pack_uint16(MQTT_KEEPALIVE,
				       MQTT_MAX_VARIABLE_HEADER_N_PAYLOAD,
				       payload, &offset);
	}

	if (err_code == 0) {
		MQTT_TRC("Encoding Client Id. Str:%s Size:%08x.",
			 client->client_id.utf8,
			 client->client_id.size);

		/* Pack client id */
		err_code = pack_utf8_str(&client->client_id,
					 MQTT_MAX_VARIABLE_HEADER_N_PAYLOAD,
					 payload, &offset);
	}

	if (err_code == 0) {
		/* Pack will topic and QoS */
		if (client->will_topic != NULL) {
			MQTT_TRC("Encoding Will Topic. Str:%s Size:%08x.",
				 client->will_topic->topic.utf8,
				 client->will_topic->topic.size);

			/* Set Will topic in connect flags. */
			connect_flags |= MQTT_CONNECT_FLAG_WILL_TOPIC;

			err_code = pack_utf8_str(
				&client->will_topic->topic,
				MQTT_MAX_VARIABLE_HEADER_N_PAYLOAD,
				payload, &offset);

			if (err_code == 0) {
				/* QoS is always 1 as of now. */
				connect_flags |=
					((client->will_topic->qos & 0x03) << 3);
				connect_flags |= client->will_retain << 5;

				if (client->will_message != NULL) {
					MQTT_TRC(
					    "Encoding Will Message. "
					    "Str:%s Size:%08x.",
					    client->will_message->utf8,
					    client->will_message->size);

					err_code = pack_utf8_str(
					    client->will_message,
					    MQTT_MAX_VARIABLE_HEADER_N_PAYLOAD,
					    payload, &offset);
				} else {
					MQTT_TRC("Encoding Zero Length Will "
						 "Message.");
					err_code = zero_len_str_encode(
					    MQTT_MAX_VARIABLE_HEADER_N_PAYLOAD,
					    payload, &offset);
				}
			}
		}
	}

	if (err_code == 0) {
		/* Pack Username if any. */
		if (client->user_name != NULL) {
			connect_flags |= MQTT_CONNECT_FLAG_USERNAME;

			MQTT_TRC("Encoding Username. Str:%s, Size:%08x.",
				 client->user_name->utf8,
				 client->user_name->size);

			err_code = pack_utf8_str(
				client->user_name,
				MQTT_MAX_VARIABLE_HEADER_N_PAYLOAD,
				payload, &offset);

			if (err_code == 0) {
				/* Pack Password if any. */
				if (client->password != NULL) {
					MQTT_TRC("Encoding Password. "
						 "Str:%s Size:%08x.",
						 client->password->utf8,
						 client->password->size);

					connect_flags |=
						MQTT_CONNECT_FLAG_PASSWORD;
					err_code = pack_utf8_str(
					    client->password,
					    MQTT_MAX_VARIABLE_HEADER_N_PAYLOAD,
					    payload, &offset);
				}
			}
		}
	}

	if (err_code == 0) {
		u8_t connect_pkt_qos = MQTT_QOS_0_AT_MOST_ONCE;

		if (client->protocol_version == MQTT_VERSION_3_1_0) {
			connect_pkt_qos = MQTT_QOS_1_AT_LEAST_ONCE;
		}

		/* Pack the connect flags. */
		client->tx_buf[connect_flag_offset] = connect_flags;

		const u8_t message_type = MQTT_MESSAGES_OPTIONS(
			MQTT_PKT_TYPE_CONNECT,
			0, connect_pkt_qos, 0);

		offset = mqtt_encode_fixed_header(message_type, offset,
						  &payload);

		*packet_length = offset;
		*packet = payload;
	} else {
		*packet_length = 0;
		*packet = NULL;
	}

	return err_code;
}

int publish_encode(const struct mqtt_client *client,
		   const struct mqtt_publish_param *param,
		   const u8_t **packet, u32_t *packet_length)
{
	int err_code = -ENOTCONN;
	u32_t offset = 0;
	u32_t mqtt_packetlen = 0;
	u8_t *payload;

	/* Message id zero is not permitted by spec. */
	if ((param->message.topic.qos) && (param->message_id == 0)) {
		return -EINVAL;
	}

	payload = &client->tx_buf[MQTT_FIXED_HEADER_EXTENDED_SIZE];
	memset(payload, 0, MQTT_MAX_VARIABLE_HEADER_N_PAYLOAD);

	/* Pack topic. */
	err_code = pack_utf8_str(&param->message.topic.topic,
				 MQTT_MAX_VARIABLE_HEADER_N_PAYLOAD,
				 payload, &offset);

	if (err_code == 0) {
		if (param->message.topic.qos) {
			err_code = pack_uint16(
				param->message_id,
				MQTT_MAX_VARIABLE_HEADER_N_PAYLOAD,
				payload, &offset);
		}
	}

	if (err_code == 0) {
		/* Pack message on the topic. */
		err_code = pack_data(&param->message.payload,
					MQTT_MAX_VARIABLE_HEADER_N_PAYLOAD,
					payload, &offset);
	}

	if (err_code == 0) {
		const u8_t message_type = MQTT_MESSAGES_OPTIONS(
			MQTT_PKT_TYPE_PUBLISH, param->dup_flag,
			param->message.topic.qos, param->retain_flag);

		mqtt_packetlen = mqtt_encode_fixed_header(message_type,
							  offset,
							  &payload);

		*packet_length = mqtt_packetlen;
		*packet = payload;
	} else {
		*packet_length = 0;
		*packet = NULL;
	}

	return err_code;
}

int publish_ack_encode(const struct mqtt_client *client,
		       const struct mqtt_puback_param *param,
		       const u8_t **packet, u32_t *packet_length)
{
	const u8_t message_type =
		MQTT_MESSAGES_OPTIONS(MQTT_PKT_TYPE_PUBACK, 0, 0, 0);

	return mqtt_message_id_only_enc(client, message_type, param->message_id,
					packet, packet_length);
}

int publish_receive_encode(const struct mqtt_client *client,
			   const struct mqtt_pubrec_param *param,
			   const u8_t **packet, u32_t *packet_length)
{
	const u8_t message_type =
		MQTT_MESSAGES_OPTIONS(MQTT_PKT_TYPE_PUBREC, 0, 0, 0);

	return mqtt_message_id_only_enc(client, message_type, param->message_id,
					packet, packet_length);
}

int publish_release_encode(const struct mqtt_client *client,
			   const struct mqtt_pubrel_param *param,
			   const u8_t **packet, u32_t *packet_length)
{
	const u8_t message_type =
		MQTT_MESSAGES_OPTIONS(MQTT_PKT_TYPE_PUBREL, 0, 1, 0);

	return mqtt_message_id_only_enc(client, message_type, param->message_id,
					packet, packet_length);
}

int publish_complete_encode(const struct mqtt_client *client,
			    const struct mqtt_pubcomp_param *param,
			    const u8_t **packet, u32_t *packet_length)
{
	const u8_t message_type =
		MQTT_MESSAGES_OPTIONS(MQTT_PKT_TYPE_PUBCOMP, 0, 0, 0);

	return mqtt_message_id_only_enc(client, message_type, param->message_id,
					packet, packet_length);
}

int disconnect_encode(const struct mqtt_client *client, const u8_t **packet,
		      u32_t *packet_length)
{
	(void)client;

	*packet = disc_packet;
	*packet_length = sizeof(disc_packet);

	return 0;
}

int subscribe_encode(const struct mqtt_client *client,
		     const struct mqtt_subscription_list *param,
		     const u8_t **packet, u32_t *packet_length)
{
	int err_code;
	u32_t offset = 0;
	u32_t count = 0;
	u32_t mqtt_packetlen = 0;
	u8_t *payload;

	/* Message id zero is not permitted by spec. */
	if (param->message_id == 0) {
		return -EINVAL;
	}

	payload = &client->tx_buf[MQTT_FIXED_HEADER_EXTENDED_SIZE];
	memset(payload, 0, MQTT_MAX_VARIABLE_HEADER_N_PAYLOAD);

	err_code = pack_uint16(param->message_id,
			       MQTT_MAX_VARIABLE_HEADER_N_PAYLOAD,
			       payload, &offset);
	if (err_code == 0) {
		do {
			err_code = pack_utf8_str(
				&param->list[count].topic,
				MQTT_MAX_VARIABLE_HEADER_N_PAYLOAD,
				payload, &offset);
			if (err_code == 0) {
				err_code = pack_uint8(
				    param->list[count].qos,
				    MQTT_MAX_VARIABLE_HEADER_N_PAYLOAD,
				    payload, &offset);
			}
			count++;
		} while ((err_code == 0) &&
			 (count < param->list_count));
	}

	if (err_code == 0) {
		const u8_t message_type = MQTT_MESSAGES_OPTIONS(
			MQTT_PKT_TYPE_SUBSCRIBE, 0, 1, 0);

		/* Rewind the packet to encode the packet correctly. */
		mqtt_packetlen = mqtt_encode_fixed_header(message_type,
							  offset,
							  &payload);
	}

	if (err_code == 0) {
		*packet_length = mqtt_packetlen;
		*packet = payload;
	} else {
		*packet_length = 0;
		*packet = NULL;
	}

	return err_code;
}

int unsubscribe_encode(const struct mqtt_client *client,
		       const struct mqtt_subscription_list *param,
		       const u8_t **packet, u32_t *packet_length)
{
	int err_code;
	u32_t count = 0;
	u32_t offset = 0;
	u32_t mqtt_packetlen = 0;
	u8_t *payload;

	payload = &client->tx_buf[MQTT_FIXED_HEADER_EXTENDED_SIZE];
	memset(payload, 0, MQTT_MAX_VARIABLE_HEADER_N_PAYLOAD);

	err_code = pack_uint16(param->message_id,
			       MQTT_MAX_VARIABLE_HEADER_N_PAYLOAD,
			       payload,
			       &offset);

	if (err_code == 0) {
		do {
			err_code = pack_utf8_str(
				&param->list[count].topic,
				MQTT_MAX_VARIABLE_HEADER_N_PAYLOAD,
				payload, &offset);
			count++;
		} while ((err_code == 0) &&
			 (count < param->list_count));
	}

	if (err_code == 0) {
		const u8_t message_type =
			MQTT_MESSAGES_OPTIONS(MQTT_PKT_TYPE_UNSUBSCRIBE,
					      0,
					      MQTT_QOS_1_AT_LEAST_ONCE,
					      0);

		/* Rewind the packet to encode the packet correctly. */
		mqtt_packetlen = mqtt_encode_fixed_header(
			/* Message type, Duplicate Flag, QoS and
			 * retain flag setting.
			 */
			message_type,
			/* Payload size without the fixed header */
			offset,
			/* Address where the payload is contained.
			 * Header will encoded by rewinding the location
			 */
			&payload);
	}

	if (err_code == 0) {
		*packet_length = mqtt_packetlen;
		*packet = payload;
	} else {
		*packet_length = 0;
		*packet = NULL;
	}

	return err_code;
}

int ping_request_encode(const struct mqtt_client *client, const u8_t **packet,
			u32_t *packet_length)
{
	(void)client;

	*packet = ping_packet;
	*packet_length = sizeof(ping_packet);

	return 0;
}
