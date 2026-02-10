/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DECT_MDM_H
#define DECT_MDM_H

#include <nrf_modem_dect.h>
#include "dect_mdm_ctrl.h"

/* Upcalls from the driver ctrl module */
void dect_mdm_parent_association_created(uint32_t target_long_rd_id,
	struct nrf_modem_dect_mac_ipv6_address_config_t ipv6_config);
void dect_mdm_parent_association_removed(uint32_t long_rd_id,
					 enum nrf_modem_dect_mac_release_cause rel_cause,
					 bool neighbor_initiated);
void dect_mdm_parent_association_ipv6_config_changed(
	struct nrf_modem_dect_mac_ipv6_address_config_t ipv6_config);

void dect_mdm_child_association_created(uint32_t target_long_rd_id);
void dect_mdm_child_association_removed(uint32_t long_rd_id,
					enum nrf_modem_dect_mac_release_cause rel_cause,
					bool neighbor_initiated);
void dect_mdm_child_association_all_removed(enum nrf_modem_dect_mac_release_cause rel_cause);

#endif /* DECT_MDM_H */
