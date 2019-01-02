/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/** @file mqtt_decoder.c
 *
 * @brief Decoder functions needed for decoding packets received from the broker.
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(net_mqtt_dec, CONFIG_MQTT_LOG_LEVEL);

#include "mqtt_internal.h"
#include "mqtt_os.h"

/**
 * @brief Unpacks unsigned 8 bit value from the buffer from the offset
 *        requested.
 *
 * @param[out] val Memory where the value is to be unpacked.
 * @param[in] buffer_len Total size of the buffer. This shall not be zero.
 * @param[in] buffer Buffer from which the value is to be unpacked.
 * @param[inout] offset Offset on the buffer from where the value is to be
 *                      unpacked. If the procedure is successful, the offset is
 *                      incremented to point to the next read/unpack location
 *                      on the buffer.
 *
 * @retval 0 if the procedure is successful.
 * @retval -EINVAL if the offset is greater than or equal to the buffer length.
 */
static int unpack_uint8(u8_t *val, u32_t buffer_len, u8_t *buffer,
			u32_t *offset)
{
	int err_code = -EINVAL;

	if (buffer_len > *offset) {
		const u32_t available_len = buffer_len - *offset;

		MQTT_TRC(">> BL:%08x, B:%p, O:%08x A:%08x", buffer_len, buffer,
			 *offset, available_len);

		if (available_len >= sizeof(u8_t)) {
			/* Create unit8 value. */
			*val = buffer[*offset];

			/* Increment offset. */
			*offset += sizeof(u8_t);

			/* Indicate success. */
			err_code = 0;
		}
	}

	MQTT_TRC("<< result: 0x%08x val: 0x%02x", err_code, *val);

	return err_code;
}

/**
 * @brief Unpacks unsigned 16 bit value from the buffer from the offset
 *        requested.
 *
 * @param[out] val Memory where the value is to be unpacked.
 * @param[in] buffer_len Total size of the buffer. This shall not be zero.
 * @param[in] buffer Buffer from which the value is to be unpacked.
 * @param[inout] offset Offset on the buffer from where the value is to be
 *                      unpacked. If the procedure is successful, the offset
 *                      is incremented to point to the next read/unpack location
 *                      on the buffer.
 *
 * @retval 0 if the procedure is successful.
 * @retval -EINVAL if the offset is greater than or equal to the buffer length.
 */
static int unpack_uint16(u16_t *val, u32_t buffer_len, u8_t *buffer,
			 u32_t *offset)
{
	int err_code = -EINVAL;

	if (buffer_len > *offset) {
		const u32_t available_len = buffer_len - *offset;

		MQTT_TRC(">> BL:%08x, B:%p, O:%08x A:%08x", buffer_len, buffer,
			 *offset, available_len);

		if (available_len >= sizeof(u16_t)) {
			/* Create unit16 value. */
			*val = ((buffer[*offset] & 0x00FF) << 8); /* MSB */
			*val |= (buffer[*offset + 1] & 0x00FF); /* LSB */

			/* Increment offset. */
			*offset += sizeof(u16_t);

			/* Indicate success. */
			err_code = 0;
		}
	}

	MQTT_TRC("<< result:0x%08x val:0x%04x", err_code, *val);

	return err_code;
}

/**
 * @brief Unpacks utf8 string from the buffer from the offset requested.
 *
 * @param[out] str Pointer to a string that will hold the string location
 *                 in the buffer.
 * @param[in] buffer_len Total size of the buffer. This shall not be zero.
 * @param[in] buffer Buffer from which the string is to be unpacked.
 * @param[inout] offset Offset on the buffer from where the value is to be
 *                      unpacked. If the procedure is successful, the offset
 *                      is incremented to point to the next read/unpack location
 *                      on the buffer.
 *
 * @retval 0 if the procedure is successful.
 * @retval -EINVAL if the offset is greater than or equal to the buffer length.
 */
static int unpack_utf8_str(struct mqtt_utf8 *str, u32_t buffer_len,
			   u8_t *buffer, u32_t *offset)
{
	u16_t utf8_strlen;
	int err_code;

	err_code = unpack_uint16(&utf8_strlen, buffer_len, buffer, offset);

	str->utf8 = NULL;
	str->size = 0;

	if (err_code == 0) {
		const u32_t available_len = buffer_len - *offset;

		MQTT_TRC(">> BL:%08x, B:%p, O:%08x A:%08x", buffer_len, buffer,
			 *offset, available_len);

		if (utf8_strlen <= available_len) {
			/* Zero length UTF8 strings permitted. */
			if (utf8_strlen) {
				/* Point to right location in buffer. */
				str->utf8 = &buffer[*offset];
			}

			/* Populate length field. */
			str->size = utf8_strlen;

			/* Increment offset. */
			*offset += utf8_strlen;

			/* Indicate success */
			err_code = 0;
		} else {
			/* Reset to original value. */
			*offset -= sizeof(u16_t);

			err_code = -EINVAL;
		}
	}

	MQTT_TRC("<< result:0x%08x utf8 len:0x%08x", err_code, str->size);

	return err_code;
}

/**
 * @brief Unpacks binary string from the buffer from the offset requested.
 *
 * @param[out] str Pointer to a binary string that will hold the binary string
 *                 location in the buffer.
 * @param[in] buffer_len Total size of the buffer. This shall not be zero.
 * @param[in] buffer Buffer where the value is to be unpacked.
 * @param[inout] offset Offset on the buffer from where the value is to be
 *                      unpacked. If the procedure is successful, the offset
 *                      is incremented to point to the next read/unpack location
 *                      on the buffer.
 *
 * @retval 0 if the procedure is successful.
 * @retval -EINVAL if the offset is greater than or equal to the buffer length.
 */
static int unpack_data(struct mqtt_binstr *str, u32_t buffer_len,
		       u8_t *buffer, u32_t *offset)
{
	int err_code = -EINVAL;

	MQTT_TRC(">> BL:%08x, B:%p, O:%08x ", buffer_len, buffer, *offset);

	if (buffer_len >= *offset) {
		str->data = NULL;
		str->len = 0;

		/* Indicate success zero length binary strings are permitted. */
		err_code = 0;

		const u32_t available_len = buffer_len - *offset;

		if (available_len) {
			/* Point to start of binary string. */
			str->data = &buffer[*offset];
			str->len = available_len;

			/* Increment offset. */
			*offset += available_len;
		}
	}

	MQTT_TRC("<< bin len:0x%08x", str->len);

	return err_code;
}

int packet_length_decode(u8_t *buffer, u32_t buffer_len,
			 u32_t *remaining_length, u32_t *offset)
{
	u32_t index = *offset;
	u32_t length = 0;
	u8_t shift = 0;

	do {
		if (index >= buffer_len) {
			return -EINVAL;
		}

		length += ((u32_t)buffer[index] & MQTT_LENGTH_VALUE_MASK)
								<< shift;
		shift += MQTT_LENGTH_SHIFT;
	} while ((buffer[index++] & MQTT_LENGTH_CONTINUATION_BIT) != 0);

	*offset = index;
	*remaining_length = length;

	MQTT_TRC("RL:0x%08x RLS:0x%08x", length, index);

	return 0;
}

int connect_ack_decode(const struct mqtt_client *client, u8_t *data,
		       u32_t datalen, u32_t offset,
		       struct mqtt_connack_param *param)
{
	int err_code;
	u8_t flags, ret_code;

	err_code = unpack_uint8(&flags, datalen, data, &offset);
	if (err_code == 0) {
		if (client->protocol_version == MQTT_VERSION_3_1_1) {
			param->session_present_flag =
				flags & MQTT_CONNACK_FLAG_SESSION_PRESENT;

			MQTT_TRC("[CID %p]: session_present_flag: %d", client,
				 param->session_present_flag);
		}

		err_code = unpack_uint8(&ret_code, datalen, data, &offset);
	}

	if (err_code == 0) {
		param->return_code = (enum mqtt_conn_return_code)ret_code;
	}

	return err_code;
}

int publish_decode(u8_t *data, u32_t datalen, u32_t offset,
		   struct mqtt_publish_param *param)
{
	int err_code;

	param->dup_flag = data[0] & MQTT_HEADER_DUP_MASK;
	param->retain_flag =
		data[0] & MQTT_HEADER_RETAIN_MASK;
	param->message.topic.qos =
		((data[0] & MQTT_HEADER_QOS_MASK) >> 1);

	err_code = unpack_utf8_str(
		&param->message.topic.topic,
		datalen, data, &offset);

	if (err_code == 0) {
		if (param->message.topic.qos) {
			err_code = unpack_uint16(&param->message_id,
						 datalen, data, &offset);
		}
	}

	if (err_code == 0) {
		err_code = unpack_data(&param->message.payload,
					  datalen, data, &offset);

		/* Zero length publish messages are permitted. */
		if (err_code != 0) {
			param->message.payload.data = NULL;
			param->message.payload.len = 0;
		}
	}

	return err_code;
}

int publish_ack_decode(u8_t *data, u32_t datalen, u32_t offset,
		       struct mqtt_puback_param *param)
{
	return unpack_uint16(&param->message_id, datalen, data, &offset);
}

int publish_receive_decode(u8_t *data, u32_t datalen, u32_t offset,
			   struct mqtt_pubrec_param *param)
{
	return unpack_uint16(&param->message_id, datalen, data, &offset);
}

int publish_release_decode(u8_t *data, u32_t datalen, u32_t offset,
			   struct mqtt_pubrel_param *param)
{
	return unpack_uint16(&param->message_id, datalen, data, &offset);
}

int publish_complete_decode(u8_t *data, u32_t datalen, u32_t offset,
			    struct mqtt_pubcomp_param *param)
{
	return unpack_uint16(&param->message_id, datalen, data, &offset);
}

int subscribe_ack_decode(u8_t *data, u32_t datalen, u32_t offset,
			 struct mqtt_suback_param *param)
{
	int err_code;

	err_code = unpack_uint16(&param->message_id, datalen, data, &offset);

	if (err_code == 0) {
		err_code = unpack_data(&param->return_codes, datalen,
					  data, &offset);
	}

	return err_code;
}

int unsubscribe_ack_decode(u8_t *data, u32_t datalen, u32_t offset,
			   struct mqtt_unsuback_param *param)
{
	return unpack_uint16(&param->message_id, datalen, data, &offset);
}
