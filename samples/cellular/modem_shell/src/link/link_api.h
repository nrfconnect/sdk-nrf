/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MOSH_LINK_API_H
#define MOSH_LINK_API_H

#include <modem/pdn.h>

#include <sys/types.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/shell/shell.h>

#include "mosh_defines.h"

/** SNR offset value that is used when mapping to dBs  */
#define LINK_SNR_OFFSET_VALUE 24

#define PDP_TYPE_UNKNOWN 0x00
#define PDP_TYPE_IPV4    0x01
#define PDP_TYPE_IPV6    0x02
#define PDP_TYPE_IP4V6   0x03
#define PDP_TYPE_NONIP   0x04

struct pdp_context_info_array {
	struct pdp_context_info *array;
	size_t size;
};

#define OP_FULL_NAME_STR_MAX_LEN 128
#define OP_SHORT_NAME_STR_MAX_LEN 64
#define OP_CELL_ID_STR_MAX_LEN 32
#define OP_PLMN_STR_MAX_LEN 32
#define OP_TAC_STR_MAX_LEN 8

struct lte_xmonitor_resp_t {
	int32_t pci;
	int32_t rsrp;
	int32_t snr;
	int32_t cell_id;
	uint32_t band;
	int32_t tac;

	char full_name_str[OP_FULL_NAME_STR_MAX_LEN + 1];
	char short_name_str[OP_SHORT_NAME_STR_MAX_LEN + 1];
	char plmn_str[OP_PLMN_STR_MAX_LEN + 1];
	char cell_id_str[OP_CELL_ID_STR_MAX_LEN + 1];
	char tac_str[OP_TAC_STR_MAX_LEN + 1];
};

void link_api_modem_info_get_for_shell(bool connected);

int link_api_xmonitor_read(struct lte_xmonitor_resp_t *resp);

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

#endif /* MOSH_LINK_API_H */
