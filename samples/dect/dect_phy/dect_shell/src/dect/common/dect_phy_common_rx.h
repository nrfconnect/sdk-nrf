/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DECT_PHY_COMMON_RX_H
#define DECT_PHY_COMMON_RX_H

#include "nrf_modem_dect_phy.h"

/* Due to lack of information of a carrier/channel in pdc_cb from libmodem/modem,
 * a small wrapper for RX operations is needed to have a glue to which channel pdc_cb is for.
 */
int dect_phy_common_rx_op(const struct nrf_modem_dect_phy_rx_params *rx);
uint16_t dect_phy_common_rx_get_last_rx_op_channel(void);

#endif /* DECT_PHY_COMMON_RX_H */
