/*
 * Copyright (c) 2014 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <logging/log.h>
#define LOG_LEVEL CONFIG_NRF_COAP_LOG_LEVEL
LOG_MODULE_REGISTER(coap_option);

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <net/coap_option.h>

#include "coap.h"

u32_t coap_opt_string_encode(u8_t *encoded, u16_t *length, u8_t *string,
			     u16_t str_len)
{
	NULL_PARAM_CHECK(encoded);
	NULL_PARAM_CHECK(length);
	NULL_PARAM_CHECK(string);

	if (str_len > *length) {
		return EMSGSIZE;
	}

	memcpy(encoded, string, str_len);

	*length = str_len;

	return 0;
}

u32_t coap_opt_string_decode(u8_t *string, u16_t *length, u8_t *encoded)
{
	/* TODO Not implemented. */
	return 0;
}

u32_t coap_opt_uint_encode(u8_t *encoded, u16_t *length, u32_t data)
{
	NULL_PARAM_CHECK(encoded);
	NULL_PARAM_CHECK(length);

	u16_t byte_index = 0;

	if (data <= UINT8_MAX) {
		if (*length < sizeof(u8_t)) {
			return EMSGSIZE;
		}

		encoded[byte_index++] = (u8_t)data;
	} else if (data <= UINT16_MAX) {
		if (*length < sizeof(u16_t)) {
			return EMSGSIZE;
		}

		encoded[byte_index++] = (u8_t)((data & 0xFF00) >> 8);
		encoded[byte_index++] = (u8_t)(data & 0x00FF);
	} else {
		if (*length < sizeof(u32_t)) {
			return EMSGSIZE;
		}

		encoded[byte_index++] = (u8_t)((data & 0xFF000000) >> 24);
		encoded[byte_index++] = (u8_t)((data & 0x00FF0000) >> 16);
		encoded[byte_index++] = (u8_t)((data & 0x0000FF00) >> 8);
		encoded[byte_index++] = (u8_t)(data & 0x000000FF);
	}

	*length = byte_index;

	return 0;
}

u32_t coap_opt_uint_decode(u32_t *data, u16_t length, u8_t *encoded)
{
	NULL_PARAM_CHECK(data);
	NULL_PARAM_CHECK(encoded);

	u8_t byte_index = 0;

	switch (length) {
	case 0: {
		*data = 0;

		break;
	}

	case 1: {
		*data = 0;
		*data |= encoded[byte_index++];

		break;
	}

	case 2:	{
		*data = 0;
		*data |= (encoded[byte_index++] << 8);
		*data |= (encoded[byte_index++]);

		break;
	}

	case 3: {
		*data = 0;
		*data |= (encoded[byte_index++] << 16);
		*data |= (encoded[byte_index++] << 8);
		*data |= (encoded[byte_index++]);

		break;
	}

	case 4:	{
		*data = 0;
		*data |= (encoded[byte_index++] << 24);
		*data |= (encoded[byte_index++] << 16);
		*data |= (encoded[byte_index++] << 8);
		*data |= (encoded[byte_index++]);

		break;
	}

	default:
		return EINVAL;
	}

	return 0;
}
