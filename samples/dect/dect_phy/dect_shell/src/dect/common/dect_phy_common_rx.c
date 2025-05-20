/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <nrf_modem_dect_phy.h>

#include "dect_phy_common_rx.h"

#define DECT_PHY_COMMON_RX_HANDLE_CH_MAP_TABLE_COLUMNS 2 /* phy handle + channel */
#define DECT_PHY_COMMON_RX_HANDLE_COLUMN_INDEX         0
#define DECT_PHY_COMMON_RX_CHANNEL_COLUMN_INDEX	       1

/* Phy handle to channel mapping table */
static uint16_t phy_handle_to_channel_map[
	DECT_PHY_COMMON_RX_OP_HANDLES_MAX][DECT_PHY_COMMON_RX_HANDLE_CH_MAP_TABLE_COLUMNS];
static uint16_t current_phy_handle_insert_index;

static void dect_phy_common_rx_op_handle_to_channel_set(uint32_t handle, uint16_t channel)
{
	int insert_index;
	bool already_in_table = false;

	if (current_phy_handle_insert_index == DECT_PHY_COMMON_RX_OP_HANDLES_MAX) {
		current_phy_handle_insert_index = 0;
	}
	insert_index = current_phy_handle_insert_index;

	for (int i = 0; i < DECT_PHY_COMMON_RX_OP_HANDLES_MAX; i++) {
		if (phy_handle_to_channel_map[i][
			DECT_PHY_COMMON_RX_HANDLE_COLUMN_INDEX] == handle) {
			insert_index = i;
			already_in_table = true;
			break;
		}
	}

	phy_handle_to_channel_map[insert_index][DECT_PHY_COMMON_RX_HANDLE_COLUMN_INDEX] = handle;
	phy_handle_to_channel_map[insert_index][DECT_PHY_COMMON_RX_CHANNEL_COLUMN_INDEX] = channel;

	if (!already_in_table) {
		current_phy_handle_insert_index++;
	}
}

int dect_phy_common_rx_op_handle_to_channel_get(uint32_t handle, uint16_t *channel_out)
{
	for (int i = 0; i < DECT_PHY_COMMON_RX_OP_HANDLES_MAX; i++) {
		if (phy_handle_to_channel_map[i][
			DECT_PHY_COMMON_RX_HANDLE_COLUMN_INDEX] == handle) {
			*channel_out =
				phy_handle_to_channel_map[i][
					DECT_PHY_COMMON_RX_CHANNEL_COLUMN_INDEX];
			return 0;
		}
	}
	return -1;
}

int dect_phy_common_rx_op(const struct nrf_modem_dect_phy_rx_params *rx)
{
	dect_phy_common_rx_op_handle_to_channel_set(rx->handle, rx->carrier);

	return nrf_modem_dect_phy_rx(rx);
}
