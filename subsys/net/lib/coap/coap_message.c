/*
 * Copyright (c) 2014 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <logging/log.h>
#define LOG_LEVEL CONFIG_NRF_COAP_LOG_LEVEL
LOG_MODULE_REGISTER(coap_message);

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <net/coap_api.h>
#include <net/coap_message.h>

#include "coap.h"

#define COAP_PAYLOAD_MARKER_SIZE  1

/**@brief Verify that there is a index available for a new option. */
#define OPTION_INDEX_AVAIL_CHECK(COUNT) do { \
		if ((COUNT) >= COAP_MAX_NUMBER_OF_OPTIONS) { \
			return ENOMEM; \
		} \
	} while (false)

u32_t coap_message_create(coap_message_t *message,
			  coap_message_conf_t *init_config)
{
	NULL_PARAM_CHECK(message);
	NULL_PARAM_CHECK(init_config);

	if (init_config->transport == NULL) {
		return EINVAL;
	}

	/* Setting default value for version. */
	message->header.version = COAP_VERSION;

	/* Copy values from the init config. */
	message->header.type = init_config->type;
	message->header.token_len = init_config->token_len;
	message->header.code = init_config->code;
	message->header.id = init_config->id;
	message->response_callback = init_config->response_callback;
	message->arg = NULL;
	message->transport = init_config->transport;

	memcpy(message->token, init_config->token, sizeof(init_config->token));

	return 0;
}

/**@brief Decode CoAP option
 *
 * @param[in]    raw_option Pointer to the memory buffer where the raw option
 *                          is located.
 * @param[inout] message    Pointer to the current message. Used to retrieve
 *                          information about where current option delta and
 *                          the size of free memory to add the values of the
 *                          option. Used as a container where to put the parsed
 *                          option.
 * @param[out]   byte_count Number of bytes parsed. Used to indicate where the
 *                          next option might be located (if any left) in the
 *                          raw message buffer.
 *
 * @retval 0 If the option parsing went successful.
 */
static u32_t decode_option(const u8_t *raw_option, coap_message_t *message,
			   u16_t *byte_count)
{
	u16_t byte_index = 0;
	u8_t option_num = message->options_count;

	/* Calculate the option number. */
	u16_t option_delta = (raw_option[byte_index] & 0xF0) >> 4;
	/* Calculate the option length. */
	u16_t option_length = (raw_option[byte_index] & 0x0F);

	byte_index++;

	u16_t acc_option_delta = message->options_delta;

	if (option_delta == 13) {
		/* read one additional byte to get the extended delta. */
		acc_option_delta += 13 + raw_option[byte_index++];

	} else if (option_delta == 14) {
		/* read one additional byte to get the extended delta. */
		acc_option_delta += 269;
		acc_option_delta += (raw_option[byte_index++] << 8);
		acc_option_delta += (raw_option[byte_index++]);
	} else {
		acc_option_delta += option_delta;
	}

	/* Set the accumulated delta as the option number. */
	message->options[option_num].number = acc_option_delta;

	if (option_length == 13) {
		option_length = 13 + raw_option[byte_index++];
	} else if (option_length == 14) {
		option_length = 269;
		option_length += (raw_option[byte_index++] << 8);
		option_length += raw_option[byte_index++];
	}

	/* Set the option length including extended bytes. */
	message->options[option_num].length = option_length;

	/* Point data to the memory where to find the option value. */
	message->options[option_num].data = (u8_t *)&raw_option[byte_index];

	/* Update the delta counter with latest option number. */
	message->options_delta = message->options[option_num].number;

	byte_index += message->options[option_num].length;
	*byte_count = byte_index;

	return 0;
}

/**@brief Encode CoAP option delta and length bytes.
 *
 * @param[inout] encoded_value     Value to encode. In return the value after
 *                                 encoding.
 * @param[out]   encoded_value_ext The value of the encoded extended bytes.
 *
 * @return The size of the extended byte field.
 */
static inline u8_t encode_extended_bytes(u16_t *value, u16_t *value_ext)
{
	u16_t raw_value = *value;

	u8_t ext_size = 0;

	if (raw_value >= 269) {
		*value = 14;
		*value_ext = raw_value - 269;
		ext_size = 2;
	} else if (raw_value >= 13) {
		*value = 13;
		*value_ext = raw_value - 13;
		ext_size = 1;
	} else {
		*value = raw_value;
		*value_ext = 0;
	}

	return ext_size;
}

static u32_t encode_option(u8_t *buffer, coap_option_t *option,
			   u16_t *byte_count)
{
	u16_t delta_ext = 0;
	u16_t delta = option->number;

	u8_t delta_ext_size = encode_extended_bytes(&delta, &delta_ext);

	u16_t length = option->length;
	u16_t length_ext = 0;

	u8_t length_ext_size = encode_extended_bytes(&length, &length_ext);

	if (buffer == NULL) {
		u16_t header_size = 1;
		*byte_count = header_size + delta_ext_size + length_ext_size +
			      option->length;
		return 0;
	}

	u16_t byte_index = 0;

	/* Add the option header. */
	buffer[byte_index++] = ((delta & 0x0F) << 4) | (length & 0x0F);

	/* Add option delta extended bytes to the buffer. */
	if (delta_ext_size == 1) {
		/* Add first byte of delta_ext to the option header. */
		buffer[byte_index++] = (u8_t)delta_ext;
	} else if (delta_ext_size == 2) {
		/* u16 in Network Byte Order. */
		buffer[byte_index++] = (u8_t)((delta_ext & 0xFF00) >> 8);
		buffer[byte_index++] = (u8_t)((delta_ext & 0x00FF));
	}

	if (length_ext_size == 1) {
		/* Add first byte of length_ext to the option header. */
		buffer[byte_index++] = (u8_t)length_ext;
	} else if (length_ext_size == 2) {
		/* u16 in Network Byte Order. */
		buffer[byte_index++] = (u8_t)((length_ext & 0xFF00) >> 8);
		buffer[byte_index++] = (u8_t)((length_ext & 0x00FF));
	}

	memcpy(&buffer[byte_index], option->data, option->length);
	*byte_count = byte_index + option->length;

	return 0;
}

u32_t coap_message_decode(coap_message_t *message, const u8_t *raw_message,
			  u16_t message_len)
{
	NULL_PARAM_CHECK(message);
	NULL_PARAM_CHECK(raw_message);

	/* Check that the raw message contains the mandatory header. */
	if (message_len < 4) {
		return EMSGSIZE;
	}

	/* Parse the content of the raw message buffer. */
	u16_t byte_index = 0;

	/* Parse the 4 byte CoAP header. */
	message->header.version = (raw_message[byte_index] >> 6);
	message->header.type =
		(coap_msg_type_t)((raw_message[byte_index] >> 4) & 0x03);
	message->header.token_len = (raw_message[byte_index] & 0x0F);
	byte_index++;

	message->header.code = (coap_msg_code_t)raw_message[byte_index];
	byte_index++;

	message->header.id = raw_message[byte_index++] << 8;
	message->header.id += raw_message[byte_index++];

	/* Parse the token, if any. */
	for (u8_t index = 0; (byte_index < message_len) &&
			     (index < message->header.token_len); index++) {
		message->token[index] = raw_message[byte_index++];
	}

	message->options_count = 0;
	message->options_delta = 0;

	/* Parse the options if any. */
	while ((byte_index < message_len) &&
	       (raw_message[byte_index] != COAP_PAYLOAD_MARKER)) {
		u32_t err_code;
		u16_t byte_count = 0;

		err_code = decode_option(&raw_message[byte_index], message,
					 &byte_count);
		if (err_code != 0) {
			return err_code;
		}

		message->options_count += 1;

		byte_index += byte_count;
	}

	/* If there any more bytes to parse this would be the payload. */
	if (byte_index < message_len) {
		/* Verify that we have a payload marker. */
		if (raw_message[byte_index] == COAP_PAYLOAD_MARKER) {
			byte_index++;
		} else {
			return EINVAL;
		}

		message->payload_len = message_len - byte_index;
		message->payload = (u8_t *)&raw_message[byte_index];
	}

	return 0;
}

u32_t coap_message_encode(coap_message_t *message, u8_t *buffer, u16_t *length)
{
	NULL_PARAM_CHECK(length);
	NULL_PARAM_CHECK(message);

	/* calculated size */
	u16_t total_packet_size = 4;

	if (message->payload_len > 0) {
		total_packet_size += message->payload_len;
		total_packet_size += COAP_PAYLOAD_MARKER_SIZE;
	}

	if (message->header.token_len > 8) {
		return EINVAL;
	}
	total_packet_size += message->header.token_len;
	total_packet_size += message->options_len;

	/* If this was a length check, return after setting the length in
	 * the output parameter.
	 */
	if (*length == 0) {
		*length = total_packet_size;
		return 0;
	}

	/* Check that the buffer provided is sufficient. */
	if (*length < total_packet_size) {
		return EMSGSIZE;
	}

	if (((message->payload_len > 0 && message->payload == NULL)) ||
	    (buffer ==  NULL)) {
		return EINVAL;
	}

	/* Start filling the bytes. */
	u16_t byte_index = 0;

	/* TODO: Verify the values of the header fields.
	 * if (version > 1)
	 * if (message->type > COAP_TYPE_RST)
	 * if (message->token_len > 8)
	 */

	buffer[byte_index] = (((message->header.version & 0x3) << 6) |
			     ((message->header.type & 0x3) << 4)) |
			     (message->header.token_len & 0x0F);
	byte_index++;

	buffer[byte_index] = message->header.code;
	byte_index++;

	buffer[byte_index++] = (message->header.id & 0xFF00) >> 8;
	buffer[byte_index++] = (message->header.id & 0x00FF);

	memcpy(&buffer[byte_index], message->token, message->header.token_len);
	byte_index += message->header.token_len;

	/* memcpy(&buffer[byte_index], &message->data[0], message->options_len);
	 */
	for (u8_t i = 0; i < message->options_count; i++) {
		u32_t err_code;
		u16_t byte_count = 0;

		err_code = encode_option(&buffer[byte_index],
					 &message->options[i],
					 &byte_count);
		if (err_code == 0) {
			byte_index += byte_count;
		} else {
			/* Throw an error. */
		}
	}

	if (message->payload_len > 0 && message->payload != NULL) {
		buffer[byte_index++] = 0xFF;
		memcpy(&buffer[byte_index], message->payload,
		       message->payload_len);
	}

	*length = total_packet_size;

	return 0;
}

u32_t coap_message_opt_empty_add(coap_message_t *message, u16_t option_num)
{
	OPTION_INDEX_AVAIL_CHECK(message->options_count);

	u32_t err_code;
	u16_t encoded_len = 0;
	u8_t current_option_index = message->options_count;

	message->options[current_option_index].number =
					option_num - message->options_delta;
	message->options[current_option_index].length = encoded_len;

	/* Set accumulated option delta for next option. */
	message->options_delta = option_num;

	/* Calculate option size */
	u16_t option_byte_count = 0;

	/* do a length check to encode_option to get the header length. */
	err_code = encode_option(NULL, &message->options[current_option_index],
				 &option_byte_count);

	/* Accumulate expected size of all options with headers. */
	message->options_len += option_byte_count;

	message->options_count += 1;

	return err_code;
}

u32_t coap_message_opt_uint_add(coap_message_t *message, u16_t option_num,
				u32_t data)
{
	OPTION_INDEX_AVAIL_CHECK(message->options_count);

	u32_t err_code;
	u16_t encoded_len = message->data_len - message->options_offset;
	u8_t current_option_index = message->options_count;
	u8_t *next_option_data = &message->data[message->options_offset];

	/* If the value of the option is 0, do not encode the 0, as this can
	 * be omitted. (RFC7252 3.2)
	 */
	if (data == 0) {
		encoded_len = 0;
	} else {
		err_code = coap_opt_uint_encode(next_option_data,
						&encoded_len, data);
		if (err_code != 0) {
			return err_code;
		}
	}

	message->options[current_option_index].data = next_option_data;
	message->options[current_option_index].number =
					option_num - message->options_delta;
	message->options[current_option_index].length = encoded_len;

	/* Set accumulated option delta for next option. */
	message->options_delta = option_num;

	/* Calculate option size. */
	u16_t option_byte_count = 0;

	/* Do a length check to encode_option to get the header length. */
	err_code = encode_option(NULL, &message->options[current_option_index],
				 &option_byte_count);

	/* Accumulate expected size of all options with headers. */
	message->options_len += option_byte_count;

	message->options_count += 1;

	/* Increase the pointer offset for the next option data in the scratch
	 * buffer.
	 */
	message->options_offset += encoded_len;

	return err_code;
}

u32_t coap_message_opt_str_add(coap_message_t *message, u16_t option_num,
			       u8_t *data, u16_t length)
{
	OPTION_INDEX_AVAIL_CHECK(message->options_count);

	u32_t err_code;

	u16_t encoded_len = length;
	u8_t current_option_index = message->options_count;
	u8_t *next_option_data = &message->data[message->options_offset];


	err_code = coap_opt_string_encode(next_option_data, &encoded_len,
					  data, length);
	if (err_code != 0) {
		return err_code;
	}

	message->options[current_option_index].data = next_option_data;
	message->options[current_option_index].number =
					option_num - message->options_delta;
	message->options[current_option_index].length = encoded_len;

	/* Set accumulated option delta for next option. */
	message->options_delta = option_num;

	/* Calculate option size */
	u16_t option_byte_count = 0;

	/* do a length check to encode_option to get the header length. */
	err_code = encode_option(NULL, &message->options[current_option_index],
				 &option_byte_count);

	/* Accumulate expected size of all options with headers. */
	message->options_len += option_byte_count;

	message->options_count += 1;
	message->options_offset += encoded_len;

	return err_code;

}

u32_t coap_message_opt_opaque_add(coap_message_t *message, u16_t option_num,
				  u8_t *data, u16_t length)
{
	OPTION_INDEX_AVAIL_CHECK(message->options_count);

	/* Check if it is possible to add a new option of this length. */
	if ((message->data_len - message->options_offset) < length) {
		return EMSGSIZE;
	}

	u32_t err_code = 0;

	u16_t encoded_len = length;
	u8_t current_option_index = message->options_count;
	u8_t *next_option_data = &message->data[message->options_offset];


	memcpy(next_option_data, data, encoded_len);

	message->options[current_option_index].data = next_option_data;
	message->options[current_option_index].number =
					option_num - message->options_delta;
	message->options[current_option_index].length = encoded_len;

	/* Set accumulated option delta for next option. */
	message->options_delta = option_num;

	/* Calculate option size */
	u16_t option_byte_count = 0;

	/* do a length check to encode_option to get the header length. */
	err_code = encode_option(NULL, &message->options[current_option_index],
				 &option_byte_count);

	/* Accumulate expected size of all options with headers. */
	message->options_len += option_byte_count;

	message->options_count += 1;
	message->options_offset += encoded_len;

	return err_code;
}

u32_t coap_message_payload_set(coap_message_t *message, void *payload,
			       u16_t payload_len)
{
	/* Check that there is available memory in the message->data scratch
	 * buffer.
	 */
	if (payload_len > (COAP_MESSAGE_DATA_MAX_SIZE -
						message->options_offset)) {
		return ENOMEM;
	}

	message->payload = &message->data[message->options_offset];
	message->payload_len = payload_len;
	memcpy(message->payload, payload, payload_len);

	return 0;
}

u32_t coap_message_remote_addr_set(coap_message_t *message,
				   struct sockaddr *address)
{
	message->remote = address;

	return 0;
}

u32_t coap_message_opt_index_get(u8_t *index, coap_message_t *message,
				 u16_t option)
{
	NULL_PARAM_CHECK(index);
	NULL_PARAM_CHECK(message);

	u8_t i;

	for (i = 0; i < message->options_count; i++) {
		if (message->options[i].number == option) {
			*index = i;
			return 0;
		}
	}

	return ENOENT;
}

u32_t coap_message_opt_present(coap_message_t *message, u16_t option)
{
	NULL_PARAM_CHECK(message);

	u8_t index;

	for (index = 0; index < message->options_count; index++) {
		if (message->options[index].number == option) {
			return 0;
		}
	}

	return ENOENT;
}

static u32_t bit_to_content_format(coap_content_type_t *ct, u32_t bit)
{
	switch (bit) {
	case COAP_CT_MASK_PLAIN_TEXT:
		*ct = COAP_CT_PLAIN_TEXT;
		break;

	case COAP_CT_MASK_APP_LINK_FORMAT:
		*ct = COAP_CT_APP_LINK_FORMAT;
		break;

	case COAP_CT_MASK_APP_XML:
		*ct = COAP_CT_APP_XML;
		break;

	case COAP_CT_MASK_APP_OCTET_STREAM:
		*ct = COAP_CT_APP_OCTET_STREAM;
		break;

	case COAP_CT_MASK_APP_EXI:
		*ct = COAP_CT_APP_EXI;
		break;

	case COAP_CT_MASK_APP_JSON:
		*ct = COAP_CT_APP_JSON;
		break;

	case COAP_CT_MASK_APP_LWM2M_TLV:
		*ct = COAP_CT_APP_LWM2M_TLV;
		break;

	default:
		return ENOENT;
	}

	return 0;
}

static u32_t content_format_to_bit(coap_content_type_t ct)
{
	u32_t mask = 0;

	switch (ct) {
	case COAP_CT_PLAIN_TEXT:
		mask = COAP_CT_MASK_PLAIN_TEXT;
		break;

	case COAP_CT_APP_LINK_FORMAT:
		mask = COAP_CT_MASK_APP_LINK_FORMAT;
		break;

	case COAP_CT_APP_XML:
		mask = COAP_CT_MASK_APP_XML;
		break;

	case COAP_CT_APP_OCTET_STREAM:
		mask = COAP_CT_MASK_APP_OCTET_STREAM;
		break;

	case COAP_CT_APP_EXI:
		mask = COAP_CT_MASK_APP_EXI;
		break;

	case COAP_CT_APP_JSON:
		mask = COAP_CT_MASK_APP_JSON;
		break;

	case COAP_CT_APP_LWM2M_TLV:
		mask = COAP_CT_MASK_APP_LWM2M_TLV;
		break;

	default:
		break;
	}

	return mask;
}

u32_t coap_message_ct_mask_get(coap_message_t *message, u32_t *mask)
{
	NULL_PARAM_CHECK(message);
	NULL_PARAM_CHECK(mask);

	(*mask) = 0;

	for (u8_t index = 0; index < message->options_count; index++) {
		if (message->options[index].number == COAP_OPT_CONTENT_FORMAT) {
			u32_t value;
			u32_t err_code = coap_opt_uint_decode(
					&value, message->options[index].length,
					message->options[index].data);
			if (err_code == 0) {
				coap_content_type_t ct =
						(coap_content_type_t)value;
				*mask |= content_format_to_bit(ct);
			} else {
				return err_code;
			}
		}
	}
	return 0;
}

u32_t coap_message_accept_mask_get(coap_message_t *message, u32_t *mask)
{
	NULL_PARAM_CHECK(message);
	NULL_PARAM_CHECK(mask);

	(*mask) = 0;

	for (u8_t index = 0; index < message->options_count; index++) {
		if (message->options[index].number == COAP_OPT_ACCEPT) {
			u32_t value;
			u32_t err_code = coap_opt_uint_decode(
					&value, message->options[index].length,
					message->options[index].data);
			if (err_code == 0) {
				coap_content_type_t ct =
						(coap_content_type_t)value;
				(*mask) |= content_format_to_bit(ct);
			} else {
				return err_code;
			}
		}
	}

	return 0;
}

u32_t coap_message_ct_match_select(coap_content_type_t *ct,
				   coap_message_t *message,
				   coap_resource_t *resource)
{
	/* Check ACCEPT options */
	u32_t accept_mask = 0;

	(void)coap_message_accept_mask_get(message, &accept_mask);

	if (accept_mask == 0) {
		/* Default to plain text if option not set. */
		accept_mask = COAP_CT_MASK_PLAIN_TEXT;
	}

	/* Select the first common content-type between the resource and the
	 * CoAP client.
	 */
	u32_t common_ct = resource->ct_support_mask & accept_mask;
	u32_t bit_index;

	for (bit_index = 0; bit_index < 32; bit_index++) {
		if (((common_ct >> bit_index) & 0x1) == 1) {
			break;
		}
	}

	u32_t err_code = bit_to_content_format(ct, 1 << bit_index);

	return err_code;
}
