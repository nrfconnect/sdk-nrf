/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <net/socket.h>
#include <modem/at_cmd_parser.h>
#include "slm_util.h"

/* global variable defined in different files */
extern struct at_param_list at_param_list;

/**
 * @brief Compare string ignoring case
 */
bool slm_util_casecmp(const char *str1, const char *str2)
{
	int str2_len = strlen(str2);

	if (strlen(str1) != str2_len) {
		return false;
	}

	for (int i = 0; i < str2_len; i++) {
		if (toupper((int)*(str1 + i)) != toupper((int)*(str2 + i))) {
			return false;
		}
	}

	return true;
}


/**
 * @brief Compare name of AT command ignoring case
 */
bool slm_util_cmd_casecmp(const char *cmd, const char *slm_cmd)
{
	int i;
	int slm_cmd_len = strlen(slm_cmd);

	if (strlen(cmd) < slm_cmd_len) {
		return false;
	}

	for (i = 0; i < slm_cmd_len; i++) {
		if (toupper((int)*(cmd + i)) != toupper((int)*(slm_cmd + i))) {
			return false;
		}
	}
#if defined(CONFIG_SLM_CR_LF_TERMINATION)
	if (strlen(cmd) > (slm_cmd_len + 2)) {
#else
	if (strlen(cmd) > (slm_cmd_len + 1)) {
#endif
		char ch = *(cmd + i);
		/* With parameter, SET TEST, "="; READ, "?" */
		return ((ch == '=') || (ch == '?'));
	}

	return true;
}

/**
 * @brief Detect hexdecimal string data type
 */
bool slm_util_hexstr_check(const uint8_t *data, uint16_t data_len)
{
	for (int i = 0; i < data_len; i++) {
		char ch = *(data + i);

		if ((ch < '0' || ch > '9') &&
		    (ch < 'A' || ch > 'F') &&
		    (ch < 'a' || ch > 'f')) {
			return false;
		}
	}

	return true;
}

/**
 * @brief Encode hex array to hexdecimal string (ASCII text)
 */
int slm_util_htoa(const uint8_t *hex, uint16_t hex_len, char *ascii, uint16_t ascii_len)
{
	if (hex == NULL || ascii == NULL) {
		return -EINVAL;
	}
	if (ascii_len < (hex_len * 2)) {
		return -EINVAL;
	}

	for (int i = 0; i < hex_len; i++) {
		sprintf(ascii + (i * 2), "%02X", *(hex + i));
	}

	return (hex_len * 2);
}

/**
 * @brief Decode hexdecimal string (ASCII text) to hex array
 */
int slm_util_atoh(const char *ascii, uint16_t ascii_len, uint8_t *hex, uint16_t hex_len)
{
	char hex_str[3];

	if (hex == NULL || ascii == NULL) {
		return -EINVAL;
	}
	if ((ascii_len % 2) > 0) {
		return -EINVAL;
	}
	if (ascii_len > (hex_len * 2)) {
		return -EINVAL;
	}
	if (!slm_util_hexstr_check(ascii, ascii_len)) {
		return -EINVAL;
	}

	hex_str[2] = '\0';
	for (int i = 0; (i * 2) < ascii_len; i++) {
		strncpy(&hex_str[0], ascii + (i * 2), 2);
		*(hex + i) = (uint8_t)strtoul(hex_str, NULL, 16);
	}

	return (ascii_len / 2);
}

/**
 * @brief Get string value from AT command with length check
 */
int util_string_get(const struct at_param_list *list, size_t index, char *value, size_t *len)
{
	int ret;
	size_t size = *len;

	/* at_params_string_get calls "memcpy" instead of "strcpy" */
	ret = at_params_string_get(list, index, value, len);
	if (ret) {
		return ret;
	}
	if (*len < size) {
		*(value + *len) = '\0';
		return 0;
	}

	return -ENOMEM;
}

/**
 * @brief use AT command to get IPv4 and/or IPv6 address
 */
void util_get_ip_addr(char *addr4, char *addr6)
{
	int err;
	char rsp[128];
	char tmp[sizeof(struct in6_addr)];
	char addr[NET_IPV6_ADDR_LEN];
	size_t addr_len;

	err = at_cmd_write("AT+CGPADDR", rsp, sizeof(rsp), NULL);
	if (err) {
		return;
	}
	/** parse +CGPADDR: <cid>,<PDP_addr_1>,<PDP_addr_2>
	 * PDN type "IP": PDP_addr_1 is <IPv4>
	 * PDN type "IPV6": PDP_addr_1 is <IPv6>
	 * PDN type "IPV4V6": <IPv4>,<IPv6> or <IPV4> or <IPv6>
	 */
	err = at_parser_params_from_str(rsp, NULL, &at_param_list);
	if (err) {
		return;
	}

	/* parse first IP string, could be IPv4 or IPv6 */
	addr_len = NET_IPV6_ADDR_LEN;
	err = util_string_get(&at_param_list, 2, addr, &addr_len);
	if (err) {
		return;
	}
	if (addr4 != NULL && inet_pton(AF_INET, addr, tmp) == 1) {
		strcpy(addr4, addr);
	} else if (addr6 != NULL && inet_pton(AF_INET6, addr, tmp) == 1) {
		strcpy(addr6, addr);
		return;
	}
	/* parse second IP string, IPv6 only */
	if (addr6 == NULL) {
		return;
	}
	addr_len = NET_IPV6_ADDR_LEN;
	err = util_string_get(&at_param_list, 3, addr, &addr_len);
	if (err) {
		return;
	}
	if (inet_pton(AF_INET6, addr, tmp) == 1) {
		strcpy(addr6, addr);
	}
	/* only parse addresses from primary PDN */
}
