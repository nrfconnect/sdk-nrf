/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include "slm_util.h"
#include "slm_at_host.h"

LOG_LEVEL_SET(CONFIG_SLM_LOG_LEVEL);

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
 * @brief Get float value from string value input in AT command.
 */
int util_string_to_float_get(const struct at_param_list *list, size_t index, float *value)
{
	int ret;
	char str[32];
	size_t len = sizeof(str);

	ret = util_string_get(list, index, str, &len);
	if (ret) {
		return ret;
	}

	*value = strtof(str, NULL);

	return 0;
}

/**
 * @brief Get double value from string value input in AT command.
 */
int util_string_to_double_get(const struct at_param_list *list, size_t index, double *value)
{
	int ret;
	char str[32];
	size_t len = sizeof(str);

	ret = util_string_get(list, index, str, &len);
	if (ret) {
		return ret;
	}

	*value = strtod(str, NULL);

	return 0;
}

/**
 * @brief use AT command to get IPv4 and/or IPv6 address for specified PDN
 */
void util_get_ip_addr(int cid, char *addr4, char *addr6)
{
	int ret;
	char cmd[128];
	char tmp[sizeof(struct in6_addr)];
	char addr1[NET_IPV6_ADDR_LEN] = { 0 };
	char addr2[NET_IPV6_ADDR_LEN] = { 0 };

	sprintf(cmd, "AT+CGPADDR=%d", cid);
	/** parse +CGPADDR: <cid>,<PDP_addr_1>,<PDP_addr_2>
	 * PDN type "IP": PDP_addr_1 is <IPv4>, max 16(INET_ADDRSTRLEN), '.' and digits
	 * PDN type "IPV6": PDP_addr_1 is <IPv6>, max 46(INET6_ADDRSTRLEN),':', digits, 'A'~'F'
	 * PDN type "IPV4V6": <IPv4>,<IPv6> or <IPV4> or <IPv6>
	 */
	ret = nrf_modem_at_scanf(cmd, "+CGPADDR: %*d,\"%46[.:0-9A-F]\",\"%46[:0-9A-F]\"",
				 addr1, addr2);
	if (ret <= 0) {
		return;
	}
	if (addr4 != NULL && inet_pton(AF_INET, addr1, tmp) == 1) {
		strcpy(addr4, addr1);
	} else if (addr6 != NULL && inet_pton(AF_INET6, addr1, tmp) == 1) {
		strcpy(addr6, addr1);
		return;
	}
	/* parse second IP string, IPv6 only */
	if (addr6 == NULL) {
		return;
	}
	if (ret > 1 && inet_pton(AF_INET6, addr2, tmp) == 1) {
		strcpy(addr6, addr2);
	}
}

/**
 * @brief Convert string to integer
 */
int util_str_to_int(const char *str_buf, int base, int *output)
{
	int temp;
	char *end_ptr = NULL;

	errno = 0;
	temp = strtol(str_buf, &end_ptr, base);

	if (end_ptr == str_buf || *end_ptr != '\0' ||
	    ((temp == LONG_MAX || temp == LONG_MIN) && errno == ERANGE)) {
		return -ENODATA;
	}

	*output = temp;
	return 0;
}

/**
 * @brief Resolve remote host by hostname or IP address
 */
#define PORT_MAX_SIZE    5 /* 0xFFFF = 65535 */
#define PDN_ID_MAX_SIZE  2 /* 0..10 */

int util_resolve_host(int cid, const char *host, uint16_t port, int family,
	LOG_INSTANCE_PTR_DECLARE(log_inst), struct sockaddr *sa)
{
	int err;
	char service[PORT_MAX_SIZE + PDN_ID_MAX_SIZE + 2];
	struct addrinfo *ai = NULL;
	struct addrinfo hints = {
		.ai_flags  = AI_NUMERICSERV | AI_PDNSERV,
		.ai_family = family
	};

	if (sa == NULL) {
		return DNS_EAI_AGAIN;
	}

	/* "service" shall be formatted as follows: "port:pdn_id" */
	snprintf(service, sizeof(service), "%hu:%d", port, cid);
	err = getaddrinfo(host, service, &hints, &ai);
	if (!err) {
		*sa = *(ai->ai_addr);
		freeaddrinfo(ai);

		if (sa->sa_family != AF_INET && sa->sa_family != AF_INET6) {
			err = DNS_EAI_ADDRFAMILY;
		}
	}

	if (err) {
		const char *errstr;

		if (err == DNS_EAI_SYSTEM) {
			errstr = strerror(errno);
			err = errno;
		} else {
			errstr = gai_strerror(err);
		}
		LOG_INST_ERR(log_inst, "getaddrinfo() error (%d): %s", err, errstr);
	}
	return err;
}
