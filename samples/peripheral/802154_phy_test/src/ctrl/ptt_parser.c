/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
/* Purpose: converting string of ASCII codes to integers */

#include "ptt_parser_internal.h"

#include <stddef.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>

#define PARSER_DECIMAL_BASE 10
#define PARSER_HEXADECIMAL_BASE 16
#define PARSER_BASE_NOT_FOUND 0xffu

static enum ptt_ret ptt_parser_hex_string_next_byte(const char *hex_str, uint8_t *out_num_p);

enum ptt_ret ptt_parser_string_to_int(const char *str, int32_t *out_value_p, uint8_t base,
				      int32_t min, int32_t max)
{
	if (str == NULL) {
		return PTT_RET_NULL_PTR;
	}

	if (out_value_p == NULL) {
		return PTT_RET_NULL_PTR;
	}

	enum ptt_ret ret = PTT_RET_SUCCESS;
	char *end_p;
	long out_num = strtol(str, &end_p, base);

	/* "In particular, if *nptr is not '\0' but **endptr is '\0' on return, the entire string
	 * is valid." <- from man strtol
	 */
	/* also lets discard LONG_MIN/LONG_MAX values which show strtol failed */
	if (('\0' != *end_p) || (out_num < min) || (out_num > max) || (out_num == LONG_MIN) ||
	    (out_num == LONG_MAX)) {
		ret = PTT_RET_INVALID_VALUE;
	} else {
		*out_value_p = out_num;
	}

	return ret;
}

inline enum ptt_ret ptt_parser_string_to_int32(const char *str, int32_t *out_value, uint8_t base)
{
	return ptt_parser_string_to_int(str, out_value, base, INT32_MIN, INT32_MAX);
}

inline enum ptt_ret ptt_parser_string_to_byte(const char *str, uint8_t *out_value_p, uint8_t base)
{
	int32_t out_value;
	enum ptt_ret ret = ptt_parser_string_to_int(str, &out_value, base, INT8_MIN, UINT8_MAX);

	if (ret == PTT_RET_SUCCESS) {
		*out_value_p = out_value;
	}

	return ret;
}

inline enum ptt_ret ptt_parser_string_to_int8(const char *str, int8_t *out_value_p, uint8_t base)
{
	int32_t out_value;
	enum ptt_ret ret = ptt_parser_string_to_int(str, &out_value, base, INT8_MIN, INT8_MAX);

	if (ret == PTT_RET_SUCCESS) {
		*out_value_p = out_value;
	}

	return ret;
}

inline enum ptt_ret ptt_parser_string_to_uint8(const char *str, uint8_t *out_value_p, uint8_t base)
{
	int32_t out_value;
	enum ptt_ret ret = ptt_parser_string_to_int(str, &out_value, base, 0, UINT8_MAX);

	if (ret == PTT_RET_SUCCESS) {
		*out_value_p = out_value;
	}

	return ret;
}

inline enum ptt_ret ptt_parser_string_to_int16(const char *str, int16_t *out_value_p, uint8_t base)
{
	int32_t out_value;
	enum ptt_ret ret = ptt_parser_string_to_int(str, &out_value, base, INT16_MIN, INT16_MAX);

	if (ret == PTT_RET_SUCCESS) {
		*out_value_p = out_value;
	}

	return ret;
}

inline enum ptt_ret ptt_parser_string_to_uint16(const char *str, uint16_t *out_value_p,
						uint8_t base)
{
	int32_t out_value;
	enum ptt_ret ret = ptt_parser_string_to_int(str, &out_value, base, 0, UINT16_MAX);

	if (ret == PTT_RET_SUCCESS) {
		*out_value_p = out_value;
	}

	return ret;
}

enum ptt_ret ptt_parser_hex_string_to_uint8_array(const char *hex_str, uint8_t *out_value_p,
						  uint8_t max_len, uint8_t *written_len)
{
	if (hex_str == NULL) {
		return PTT_RET_NULL_PTR;
	}

	if (out_value_p == NULL) {
		return PTT_RET_NULL_PTR;
	}

	if (max_len == 0) {
		return PTT_RET_NO_FREE_SLOT;
	}

	/* cut away prefix, if present */
	if ((hex_str[0] == '0') && ((hex_str[1] == 'X') || (hex_str[1] == 'x'))) {
		hex_str += 2;
	}

	/* one byte is represented by 2 ASCII symbols => we expect even number of elements */
	if ((strlen(hex_str) % 2) || (!strlen(hex_str))) {
		return PTT_RET_INVALID_VALUE;
	}

	enum ptt_ret ret = PTT_RET_SUCCESS;
	uint8_t byte_cnt = 0;

	while ((ret == PTT_RET_SUCCESS) && strlen(hex_str)) {
		if (byte_cnt < max_len) {
			ret = ptt_parser_hex_string_next_byte(hex_str, &(out_value_p[byte_cnt]));
			hex_str += 2;
			++byte_cnt;
		} else {
			ret = PTT_RET_NO_FREE_SLOT;
		}
	}

	if (written_len != NULL) {
		*written_len = byte_cnt;
	}

	return ret;
}

static enum ptt_ret ptt_parser_hex_string_next_byte(const char *hex_str, uint8_t *out_num_p)
{
	enum ptt_ret ret = PTT_RET_SUCCESS;
	char hex_byte[3] = { 0 };

	hex_byte[0] = hex_str[0];
	hex_byte[1] = hex_str[1];
	hex_byte[2] = '\0';

	char *end_p;
	long out_num = strtol(hex_byte, &end_p, PARSER_HEXADECIMAL_BASE);

	/* "In particular, if *nptr is not '\0' but **endptr is '\0' on return, the entire string
	 * is valid." <- from man strtol
	 */
	if ((*end_p != '\0') || (out_num < INT8_MIN) || (out_num > UINT8_MAX)) {
		ret = PTT_RET_INVALID_VALUE;
	} else {
		*out_num_p = (uint8_t)out_num;
	}

	return ret;
}
