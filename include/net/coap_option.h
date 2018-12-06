/*
 * Copyright (c) 2014 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/** @file coap_option.h
 *
 * @defgroup iot_sdk_coap_option CoAP Option
 * @ingroup iot_sdk_coap
 * @{
 * @brief Nordic's CoAP Option APIs.
 */

#ifndef COAP_OPTION_H__
#define COAP_OPTION_H__

/*
 * +-----+---+---+---+---+----------------+--------+--------+----------+
 * | No. | C | U | N | R | Name           | Format | Length | Default  |
 * +-----+---+---+---+---+----------------+--------+--------+----------+
 * |   1 | x |   |   | x | If-Match       | opaque | 0-8    | (none)   |
 * |   3 | x | x | - |   | Uri-Host       | string | 1-255  | (see     |
 * |     |   |   |   |   |                |        |        | below)   |
 * |   4 |   |   |   | x | ETag           | opaque | 1-8    | (none)   |
 * |   5 | x |   |   |   | If-None-Match  | empty  | 0      | (none)   |
 * |   7 | x | x | - |   | Uri-Port       | uint   | 0-2    | (see     |
 * |     |   |   |   |   |                |        |        | below)   |
 * |   8 |   |   |   | x | Location-Path  | string | 0-255  | (none)   |
 * |  11 | x | x | - | x | Uri-Path       | string | 0-255  | (none)   |
 * |  12 |   |   |   |   | Content-Format | uint   | 0-2    | (none)   |
 * |  14 |   | x | - |   | Max-Age        | uint   | 0-4    | 60       |
 * |  15 | x | x | - | x | Uri-Query      | string | 0-255  | (none)   |
 * |  17 | x |   |   |   | Accept         | uint   | 0-2    | (none)   |
 * |  20 |   |   |   | x | Location-Query | string | 0-255  | (none)   |
 * |  23 | x | x | - | - | Block2         | uint   | 0-3    | (none)   |
 * |  27 | x | x | - | - | Block1         | uint   | 0-3    | (none)   |
 * |  28 |   |   | x |   | Size2          | uint   | 0-4    | (none)   |
 * |  35 | x | x | - |   | Proxy-Uri      | string | 1-1034 | (none)   |
 * |  39 | x | x | - |   | Proxy-Scheme   | string | 1-255  | (none)   |
 * |  60 |   |   | x |   | Size1          | uint   | 0-4    | (none)   |
 * +-----+---+---+---+---+----------------+--------+--------+----------+
 */

#include <stdint.h>

#include <net/coap_api.h>


#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	COAP_OPT_FORMAT_EMPTY   = 0,
	COAP_OPT_FORMAT_STRING  = 1,
	COAP_OPT_FORMAT_OPAQUE  = 2,
	COAP_OPT_FORMAT_UINT    = 3
} coap_opt_format_t;

/**@brief Encode zero-terminated string into utf-8 encoded string.
 *
 * @param[out]   encoded Pointer to buffer that will be used to fill the
 *                       encoded string into.
 * @param[inout] length  Length of the buffer provided. Will also be used to
 *                       return the size of the used buffer.
 * @param[in]    string  String to encode.
 * @param[in]    str_len Length of the string to encode.
 *
 * @retval 0        Indicates that encoding was successful.
 * @retval EINVAL   If encoded or length or string pointer is NULL.
 * @retval EMSGSIZE Indicates that the buffer provided was not sufficient to
 *                  successfully encode the data.
 */
u32_t coap_opt_string_encode(u8_t *encoded, u16_t *length, u8_t *string,
			     u16_t str_len);

/**@brief Decode a utf-8 string into a zero-terminated string.
 *
 * @param[out]   string  Pointer to the string buffer where the decoded
 *                       string will be placed.
 * @param[inout] length  length of the encoded string. Returns the size of the
 *                       buffer used in bytes.
 * @param[in]    encoded Buffer to decode.
 *
 * @retval 0 Indicates that decoding was successful.
 */
u32_t coap_opt_string_decode(u8_t *string, u16_t *length, u8_t *encoded);

/**@brief Encode a uint value into a u8_t buffer in network byte order.
 *
 * @param[out]   encoded Pointer to buffer that will be used to fill the
 *                       encoded uint into.
 * @param[inout] length  Length of the buffer provided. Will also be used to
 *                       return the size of the used buffer.
 * @param[in]    data    uint value which could be anything from 1 to 4 bytes.
 *
 * @retval 0        Indicates that encoding was successful.
 * @retval EINVAL   If encoded or length pointer is NULL.
 * @retval EMSGSIZE Indicates that the buffer provided was not sufficient to
 *                  successfully encode the data.
 */
u32_t coap_opt_uint_encode(u8_t *encoded, u16_t *length, u32_t data);

/**@brief Decode a uint encoded value in network byte order to a u32_t value.
 *
 * @param[out]   data    Pointer to the u32_t value where the decoded uint will
 *                       be placed.
 * @param[inout] length  Size of the encoded value.
 * @param[in]    encoded uint value to be decoded into a u32_t value.
 *
 * @retval 0      Indicates that decoding was successful.
 * @retval EINVAL If data or encoded pointer is NULL.
 */
u32_t coap_opt_uint_decode(u32_t *data, u16_t length, u8_t *encoded);

#ifdef __cplusplus
}
#endif

#endif /* COAP_OPTION_H__ */

/** @} */
