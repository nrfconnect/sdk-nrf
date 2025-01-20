/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DECT_PHY_COMMON_RX_H
#define DECT_PHY_COMMON_RX_H

#include <nrf_modem_dect_phy.h>
#include "dect_phy_api_scheduler.h"

/* Due to lack of information of a carrier/channel in pdc_cb from libmodem/modem,
 * you need to keep up-to-date information, that is PHY handle - channel mapping.
 */
#define DECT_PHY_COMMON_RX_OP_HANDLES_MAX DECT_PHY_API_SCHEDULER_OP_MAX_COUNT

int dect_phy_common_rx_op(const struct nrf_modem_dect_phy_rx_params *rx);
int dect_phy_common_rx_op_handle_to_channel_get(uint32_t handle, uint16_t *channel_out);

#endif /* DECT_PHY_COMMON_RX_H */
