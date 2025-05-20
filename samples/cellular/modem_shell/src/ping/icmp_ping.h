/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MOSH_ICMP_PING_H
#define MOSH_ICMP_PING_H

/**@file icmp_ping.h
 *
 * @brief ICMP ping.
 * @{
 */

#include <zephyr/types.h>
#include "mosh_defines.h"

#define ICMP_IPV4_HDR_LEN	20
#define ICMP_IPV6_HDR_LEN	40

#define ICMP_MAX_ADDR		128
#define ICMP_DEFAULT_LINK_MTU	1500
#define ICMP_HDR_LEN		8

/* Max payload lengths: */
#define ICMP_IPV4_MAX_LEN	(ICMP_DEFAULT_LINK_MTU - ICMP_IPV4_HDR_LEN - ICMP_HDR_LEN)
#define ICMP_IPV6_MAX_LEN	(ICMP_DEFAULT_LINK_MTU - ICMP_IPV6_HDR_LEN - ICMP_HDR_LEN)

#define ICMP_PARAM_LENGTH_DEFAULT   0
#define ICMP_PARAM_COUNT_DEFAULT    4
#define ICMP_PARAM_TIMEOUT_DEFAULT  3000
#define ICMP_PARAM_INTERVAL_DEFAULT 1000

/**@ ICMP Ping command arguments */
struct icmp_ping_shell_cmd_argv {
	struct k_poll_signal *kill_signal;
	char *print_buf;
	int print_buf_len;

	char target_name[ICMP_MAX_ADDR + 1];
	struct addrinfo *src;
	struct addrinfo *dest;
	struct in_addr current_addr4;
	struct in6_addr current_addr6;

	char current_apn_str[MOSH_APN_STR_MAX_LEN];
	char current_pdp_type;
	uint32_t mtu;
	uint32_t len;
	uint32_t timeout;
	int32_t cid;
	int32_t pdn_id_for_cid;
	uint32_t count;
	uint32_t interval;
	bool force_ipv6;
	bool rai;

	int64_t pdn_ctx_info_read_uptime;
};

/**
 * @brief ICMP initiator.
 *
 * @param ping_args Ping parameters.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int icmp_ping_start(struct icmp_ping_shell_cmd_argv *ping_args);

/**
 * @brief Set ICMP ping defaults.
 *
 * @param ping_args Ping parameters.
 *
 */
void icmp_ping_cmd_defaults_set(struct icmp_ping_shell_cmd_argv *ping_args);

#endif /* MOSH_ICMP_PING_H */
