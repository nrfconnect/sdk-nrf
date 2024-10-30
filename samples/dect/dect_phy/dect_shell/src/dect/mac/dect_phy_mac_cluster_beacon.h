/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DECT_PHY_MAC_CLUSTER_BEACON_H
#define DECT_PHY_MAC_CLUSTER_BEACON_H

#include <zephyr/kernel.h>
#include "dect_phy_mac_common.h"
#include "dect_phy_mac_pdu.h"

#define DECT_PHY_MAC_CLUSTER_BEACON_INTERVAL_MS (2000)

/* Defines for Random Access Resource IE */

#define DECT_PHY_MAC_CLUSTER_BEACON_RA_START_SUBSLOT (12) /* Only even numbers */
#define DECT_PHY_MAC_CLUSTER_BEACON_RA_LENGTH_SLOTS  (10)

/* 'Validity' is random access allocation length in frames */
#define DECT_PHY_MAC_CLUSTER_BEACON_RA_VALIDITY                                                    \
	((DECT_PHY_MAC_CLUSTER_BEACON_INTERVAL_MS / DECT_RADIO_FRAME_DURATION_MS) / 2)

/* 'repetition' in every 2nd frame */
#define DECT_PHY_MAC_CLUSTER_BEACON_RA_REPETITION (2)

/******************************************************************************/

int dect_phy_mac_cluster_beacon_tx_start(struct dect_phy_mac_beacon_start_params *params);
void dect_phy_mac_cluster_beacon_tx_stop(void);

void dect_phy_mac_cluster_beacon_update(void);

void dect_phy_mac_cluster_beacon_status_print(void);

/******************************************************************************/

void dect_phy_mac_cluster_beacon_association_req_handle(
	struct dect_phy_commmon_op_pdc_rcv_params *rcv_params,
	dect_phy_mac_common_header_t *common_header,
	dect_phy_mac_association_req_t *association_req);

#endif /* DECT_PHY_MAC_CLUSTER_BEACON_H */
