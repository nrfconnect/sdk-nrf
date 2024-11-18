/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <zephyr/shell/shell.h>
#include <nrf_modem_dect_phy.h>

#include "desh_print.h"

#include "dect_common.h"
#include "dect_common_utils.h"

#include "dect_common_pdu.h"

#include "dect_phy_perf_pdu.h"
#include "dect_phy_ping_pdu.h"

uint16_t dect_common_pdu_tx_next_seq_nbr_get(void)
{
	static uint16_t seq_nbr = 1;

	return seq_nbr++;
}
const char *dect_common_pdu_message_type_to_string(int type, char *out_str_buff)
{
	struct mapping_tbl_item const mapping_table[] = {
		{DECT_MAC_MESSAGE_TYPE_PING_REQUEST, "ping request"},
		{DECT_MAC_MESSAGE_TYPE_PING_RESPONSE, "ping response"},
		{DECT_MAC_MESSAGE_TYPE_PING_RESULTS_REQ, "ping results request"},
		{DECT_MAC_MESSAGE_TYPE_PING_RESULTS_RESP, "ping results response"},
		{DECT_MAC_MESSAGE_TYPE_PING_HARQ_FEEDBACK, "ping HARQ feedback"},

		{DECT_MAC_MESSAGE_TYPE_PERF_HARQ_FEEDBACK, "perf HARQ feedback"},
		{DECT_MAC_MESSAGE_TYPE_PERF_RESULTS_REQ, "perf results request"},
		{DECT_MAC_MESSAGE_TYPE_PERF_RESULTS_RESP, "perf results response"},
		{DECT_MAC_MESSAGE_TYPE_PERF_TX_DATA, "perf TX data"},
		{-1, NULL}};

	return dect_common_utils_map_to_string(mapping_table, type, out_str_buff);
}
