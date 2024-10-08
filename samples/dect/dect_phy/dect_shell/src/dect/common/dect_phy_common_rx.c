/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "nrf_modem_dect_phy.h"

static uint16_t last_rx_op_channel;

int dect_phy_common_rx_op(const struct nrf_modem_dect_phy_rx_params *rx)
{
	last_rx_op_channel = rx->carrier;
	return nrf_modem_dect_phy_rx(rx);
}

uint16_t dect_phy_common_rx_get_last_rx_op_channel(void)
{
	return last_rx_op_channel;
}
