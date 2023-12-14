/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
/* Purpose: parser interface intended for usage inside control module */

#ifndef PTT_PARSER_INTERNAL_H__
#define PTT_PARSER_INTERNAL_H__

#include "ptt_types.h"
#include "ptt_errors.h"

/** @brief Converts string to integer and checks upper and lower boundaries
 *
 *  Works with decimal values and hexadecimal values with '0x' or 0X' prefix.
 *  Will return error if converted value fails boundary check or cannot be
 *  stored in long type.
 *
 *  @param[IN]  str       - pointer to NULL terminated string
 *  @param[OUT] out_value - pointer to store out value of procedure
 *  @param[IN]  base        - base used in conversion. Zero for auto selection
 *  @param[IN]  min         - lower boundary of expected integer
 *  @param[IN]  max         - higher boundary of expected integer
 *
 *  @return enum ptt_ret returns PTT_RET_SUCCESS, or error code
 */
enum ptt_ret ptt_parser_string_to_int(const char *str, int32_t *out_value, uint8_t base,
				      int32_t min, int32_t max);

/** @brief Converts string to uint8_t, will be successful for all value that fit in one byte
 *
 *  This is a wrapper for @ref ptt_parser_string_to_int with min = INT8_MIN and max = UINT8_MAX.
 *  Used to get one byte from input regardless of it sign.
 *
 *  @param[IN]  str       - pointer to NULL terminated string
 *  @param[OUT] out_value - pointer to store out value of procedure
 *  @param[IN]  base        - base used in conversion. Zero for auto selection
 *
 *  @return enum ptt_ret returns PTT_RET_SUCCESS, or error code
 */
enum ptt_ret ptt_parser_string_to_byte(const char *str, uint8_t *out_value, uint8_t base);

/** @brief Converts string to int32_t
 *
 *  This is a wrapper for @ref ptt_parser_string_to_int with min = INT32_MIN and max = INT32_MAX.
 *
 *  @param[IN]  str       - pointer to NULL terminated string
 *  @param[OUT] out_value - pointer to store out value of procedure
 *  @param[IN]  base        - base used in conversion. Zero for auto selection
 *
 *  @return enum ptt_ret returns PTT_RET_SUCCESS, or error code
 */
enum ptt_ret ptt_parser_string_to_int32(const char *str, int32_t *out_value, uint8_t base);

/** @brief Converts string to int8_t
 *
 *  This is a wrapper for @ref ptt_parser_string_to_int with min = INT8_MIN and max = INT8_MAX.
 *
 *  @param[IN]  str       - pointer to NULL terminated string
 *  @param[OUT] out_value - pointer to store out value of procedure
 *  @param[IN]  base        - base used in conversion. Zero for auto selection
 *
 *  @return enum ptt_ret returns PTT_RET_SUCCESS, or error code
 */
enum ptt_ret ptt_parser_string_to_int8(const char *str, int8_t *out_value, uint8_t base);

/** @brief Converts string to uint8_t
 *
 *  This is a wrapper for @ref ptt_parser_string_to_int with min = 0 and max = UINT8_MAX.
 *
 *  @param[IN]  str       - pointer to NULL terminated string
 *  @param[OUT] out_value - pointer to store out value of procedure
 *  @param[IN]  base        - base used in conversion. Zero for auto selection
 *
 *  @return enum ptt_ret returns PTT_RET_SUCCESS, or error code
 */
enum ptt_ret ptt_parser_string_to_uint8(const char *str, uint8_t *out_value, uint8_t base);

/** @brief Converts string to int16_t
 *
 *  This is a wrapper for @ref ptt_parser_string_to_int with min = INT16_MIN and max = INT16_MAX.
 *
 *  @param[IN]  str       - pointer to NULL terminated string
 *  @param[OUT] out_value - pointer to store out value of procedure
 *  @param[IN]  base        - base used in conversion. Zero for auto selection
 *
 *  @return enum ptt_ret returns PTT_RET_SUCCESS, or error code
 */
enum ptt_ret ptt_parser_string_to_int16(const char *str, int16_t *out_value, uint8_t base);

/** @brief Converts string to uint16_t
 *
 *  This is a wrapper for @ref ptt_parser_string_to_int with min = 0 and max = UINT16_MAX.
 *
 *  @param[IN]  str       - pointer to NULL terminated string
 *  @param[OUT] out_value - pointer to store out value of procedure
 *  @param[IN]  base        - base used in conversion. Zero for auto selection
 *
 *  @return enum ptt_ret returns PTT_RET_SUCCESS, or error code
 */
enum ptt_ret ptt_parser_string_to_uint16(const char *str, uint16_t *out_value, uint8_t base);

/** @brief Converts string of hexadecimal values to array of uint8_t
 *
 *  Detects and cut out prefix '0x' or '0X'. Returns error if string can not be
 *  stored fully in provided array, has invalid symbols or is empty.
 *
 *  @param[IN]  hex_str     - pointer to NULL terminated string of hexadecimal values
 *  @param[OUT] out_value   - pointer to allocated array long enough to store max_len values
 *  @param[IN]  max_len       - maximum available space in array out_value
 *  @param[OUT] written_len - pointer to length of converted array, can be NULL,
 *								contains incorrect value on error
 *
 *  @return enum ptt_ret returns PTT_RET_SUCCESS, or error code
 */
enum ptt_ret ptt_parser_hex_string_to_uint8_array(const char *hex_str, uint8_t *out_value,
						  uint8_t max_len, uint8_t *written_len);

#endif /* PTT_PARSER_INTERNAL_H__ */
