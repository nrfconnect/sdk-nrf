/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MOSH_LINK_API_H
#define MOSH_LINK_API_H

#include <sys/types.h>
#include <net/net_ip.h>
#include <shell/shell.h>

#include "mosh_defines.h"

#define PDP_TYPE_UNKNOWN 0x00
#define PDP_TYPE_IPV4    0x01
#define PDP_TYPE_IPV6    0x02
#define PDP_TYPE_IP4V6   0x03
#define PDP_TYPE_NONIP   0x04

#define AT_CMD_PDP_CONTEXT_READ_PDP_TYPE_STR_MAX_LEN (6 + 1)
#define AT_CMD_PDP_CONTEXT_READ_IP_ADDR_STR_MAX_LEN (255)

struct pdp_context_info {
	uint32_t cid;
	uint32_t ipv4_mtu;
	uint32_t ipv6_mtu;
	uint32_t pdn_id;
	bool pdn_id_valid;
	bool ctx_active;
	char pdp_type_str[AT_CMD_PDP_CONTEXT_READ_PDP_TYPE_STR_MAX_LEN];
	char apn_str[MOSH_APN_STR_MAX_LEN];
	char pdp_type;
	struct in_addr ip_addr4;
	struct in6_addr ip_addr6;
	struct in_addr dns_addr4_primary;
	struct in_addr dns_addr4_secondary;
	struct in6_addr dns_addr6_primary;
	struct in6_addr dns_addr6_secondary;
};

struct pdp_context_info_array {
	struct pdp_context_info *array;
	size_t size;
};

void link_api_modem_info_get_for_shell(bool connected);

#if defined(CONFIG_AT_CMD)
void link_api_coneval_read_for_shell(void);

int link_api_pdp_contexts_read(struct pdp_context_info_array *pdp_info);

/**
 * Return PDP context info for a given PDN CID.
 *
 * @param[in] pdn_cid PDN CID.
 *
 * @retval pdp_context_info structure. NULL if context info for given CID not found.
 *         Client is responsible for deallocating the memory of the returned pdp_context_info.
 */
struct pdp_context_info *link_api_get_pdp_context_info_by_pdn_cid(int pdn_cid);

/**
 * Get RAI status.
 *
 * @param[out] rai_status True if RAI enabled, false if disabled.
 *                        Will be false also when error occurs.
 *
 * @return 0 if operation was successful, otherwise an error code.
 */
int link_api_rai_status(bool *rai_status);
#endif

#endif /* MOSH_LINK_API_H */
