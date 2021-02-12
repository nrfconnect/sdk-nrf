/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SLM_UTIL_
#define SLM_UTIL_

/**@file slm_util.h
 *
 * @brief Utility functions for serial LTE modem
 * @{
 */

#include <zephyr/types.h>
#include <ctype.h>
#include <stdbool.h>
#include <modem/at_cmd.h>
#include <modem/at_cmd_parser.h>

#define INVALID_SOCKET	-1
#define INVALID_PORT	-1
#define INVALID_SEC_TAG	-1
#define INVALID_ROLE	-1

#define TCPIP_MAX_URL	128

/**
 * @brief Compare string ignoring case
 *
 * @param str1 First string
 * @param str2 Second string
 *
 * @return true If two commands match, false if not.
 */
bool slm_util_casecmp(const char *str1, const char *str2);

/**
 * @brief Compare name of AT command ignoring case
 *
 * @param cmd Command string received from UART
 * @param slm_cmd Propreiatry command supported by SLM
 *
 * @return true If two commands match, false if not.
 */
bool slm_util_cmd_casecmp(const char *cmd, const char *slm_cmd);

/**
 * @brief Detect hexdecimal data type
 *
 * @param[in] hex Hex arrary to be encoded
 * @param[in] hex_len Length of hex array
 *
 * @return true if the input is hexdecimal array, otherwise false
 */
bool slm_util_hex_check(const uint8_t *hex, uint16_t hex_len);

/**
 * @brief Encode hex array to hexdecimal string (ASCII text)
 *
 * @param[in]  hex Hex arrary to be encoded
 * @param[in]  hex_len Length of hex array
 * @param[out] ascii encoded hexdecimal string
 * @param[in]  ascii_len reserved buffer size
 *
 * @return actual size of ascii string if the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int slm_util_htoa(const uint8_t *hex, uint16_t hex_len,
		char *ascii, uint16_t ascii_len);

/**
 * @brief Decode hexdecimal string (ASCII text) to hex array
 *
 * @param[in]  ascii encoded hexdecimal string
 * @param[in]  ascii_len size of hexdecimal string
 * @param[out] hex decoded hex arrary
 * @param[in]  hex_len reserved size of hex array
 *
 * @return actual size of hex array if the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int slm_util_atoh(const char *ascii, uint16_t ascii_len,
		uint8_t *hex, uint16_t hex_len);


/**@brief Check whether a string has valid IPv4 address or not
 *
 * @param address URL text string
 * @param length size of URL string
 *
 * @return true if text string is IPv4 address, false otherwise
 */
bool check_for_ipv4(const char *address, uint8_t length);


/**brief use AT command to get IPv4 address
 *
 * @param[in] address buffer to hold the IPv4 address
 *
 * @return true if IPv4 address obtained, false otherwise
 */
bool util_get_ipv4_addr(char *address);

/**
 * @brief Get string value from AT command with length check.
 *
 * @p len must be bigger than the string length, or an error is returned.
 * The copied string is null-terminated.
 *
 * @param[in]     list    Parameter list.
 * @param[in]     index   Parameter index in the list.
 * @param[out]    value   Pointer to the buffer where to copy the value.
 * @param[in,out] len     Available space in @p value, returns actual length
 *                        copied into string buffer in bytes, excluding the
 *                        terminating null character.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int util_string_get(const struct at_param_list *list, size_t index,
			 char *value, size_t *len);

/** @} */

#endif /* SLM_UTIL_ */
