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
#include "slm_trap_macros.h"
#include <modem/at_parser.h>
#include <modem/lte_lc.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/socket.h>
#include <stdbool.h>
#include <hal/nrf_gpio.h>

extern struct k_work_q slm_work_q; /* SLM's work queue. */

/** @return Whether the modem is in the given functional mode. */
bool slm_is_modem_functional_mode(enum lte_lc_func_mode mode);

/** @brief Puts the modem in minimal function mode. */
int slm_power_off_modem(void);

/** @brief Performs a reset of the SiP. */
FUNC_NORETURN void slm_reset(void);

void slm_enter_idle(void);
void slm_enter_sleep(void);
void slm_enter_shutdown(void);

/** @brief Temporarily sets the indicate pin high. */
int slm_indicate(void);

/** Replacement for @c nrf_modem_at_printf() that cannot be
 *  used so that the AT command interception works properly.
 */
int slm_util_at_printf(const char *fmt, ...);

/** Replacement for @c nrf_modem_at_scanf() that cannot be
 *  used so that the AT command interception works properly.
 */
int slm_util_at_scanf(const char *cmd, const char *fmt, ...);

/** Forwards an AT command to the modem while bypassing interception.
 *  @warning This must only be called from code that needs to bypass
 *  AT command interception, such as from interception callbacks themselves.
 *  @note This is only capable of handling AT responses that are
 *  at most two lines long (including the line that holds the result code).
 *  @return Like @c nrf_modem_at_cmd().
 */
int slm_util_at_cmd_no_intercept(char *buf, size_t len, const char *at_cmd);

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
 * @brief Detect hexdecimal string data type
 *
 * @param[in] data Hexdecimal string arrary to be checked
 * @param[in] data_len Length of array
 *
 * @return true if the input is hexdecimal string array, otherwise false
 */
bool slm_util_hexstr_check(const uint8_t *data, uint16_t data_len);

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
int slm_util_htoa(const uint8_t *hex, uint16_t hex_len, char *ascii, uint16_t ascii_len);

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
int slm_util_atoh(const char *ascii, uint16_t ascii_len, uint8_t *hex, uint16_t hex_len);

/**
 * @brief Get string value from AT command with length check.
 *
 * @p len must be bigger than the string length, or an error is returned.
 * The copied string is null-terminated.
 *
 * @param[in]     parser  AT parser.
 * @param[in]     index   Parameter index in the AT parser.
 * @param[out]    value   Pointer to the buffer where to copy the value.
 * @param[in,out] len     Available space in @p value, returns actual length
 *                        copied into string buffer in bytes, excluding the
 *                        terminating null character.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int util_string_get(struct at_parser *parser, size_t index, char *value, size_t *len);

/**
 * @brief Get float value from string value input in AT command.
 *
 * @note The string cannot be larger than 32.
 *
 * @param[in]     parser  AT parser.
 * @param[in]     index   Parameter index in the AT parser.
 * @param[out]    value   Pointer to the float where to store the value.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int util_string_to_float_get(struct at_parser *parser, size_t index, float *value);

/**
 * @brief Get double value from string value input in AT command.
 *
 * @note The string cannot be larger than 32.
 *
 * @param[in]     parser  AT parser.
 * @param[in]     index   Parameter index in the AT parser.
 * @param[out]    value   Pointer to the double where to store the value.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int util_string_to_double_get(struct at_parser *parser, size_t index, double *value);

/**
 * @brief use AT command to get IPv4 and IPv6 addresses for specified PDN
 *
 * @param[in] cid PDP Context ID as defined in "+CGDCONT" command (0~10).
 * @param[out] addr4 Buffer to hold the IPv4 address. May be NULL.
 * @param[out] addr6 Buffer to hold the IPv6 address. May be NULL.
 */
void util_get_ip_addr(int cid, char addr4[INET_ADDRSTRLEN], char addr6[INET6_ADDRSTRLEN]);

/**
 * @brief convert string to integer
 *
 * @param[in] str A string containing the representation of an integral number.
 * @param[in] base The base, which must be between 2 and 36 inclusive or the special value 0.
 * @param[out] output The converted integral number as a long int value.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int util_str_to_int(const char *str, int base, int *output);

/**
 * @brief Resolve remote host by host name or IP address
 *
 * This function wraps up getaddrinfo() to return first resolved address.
 *
 * @param[in] cid PDP Context ID as defined in "+CGDCONT" command (0~10).
 * @param[in] host Name or IP address of remote host.
 * @param[in] port Service port of remote host.
 * @param[in] family Desired address family for the returned address.
 * @param[out] sa The returned address.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, an errno code or a dns_resolve_status enum value
 *           (defined in `zephyr/net/dns_resolve.h`).
 */
int util_resolve_host(int cid, const char *host, uint16_t port, int family, struct sockaddr *sa);

/**
 * @brief Get peer IP address and port in printable format.
 *
 * @param[in] peer Peer address structure.
 * @param[out] addr IP address of the peer.
 * @param[out] port Port of the peer.
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */

int util_get_peer_addr(struct sockaddr *peer, char addr[static INET6_ADDRSTRLEN], uint16_t *port);
/** @} */

#endif /* SLM_UTIL_ */
