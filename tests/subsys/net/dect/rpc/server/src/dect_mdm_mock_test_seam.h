/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DECT_MDM_MOCK_TEST_SEAM_H_
#define DECT_MDM_MOCK_TEST_SEAM_H_

#include <net/dect/dect_net_l2.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define DECT_MDM_MOCK_FW_VERSION "Not available"

extern int dect_mdm_mock_send_call_count;
extern int dect_mdm_mock_network_join_call_count;
extern int dect_mdm_mock_network_unjoin_call_count;
extern uint32_t dect_mdm_mock_last_send_long_rd_id;
extern size_t dect_mdm_mock_last_send_len;
extern uint8_t dect_mdm_mock_last_send_data[1280];

/** Replace net_mgmt status until dect_mdm_mock_status_info_clear(). */
void dect_mdm_mock_status_info_set(const struct dect_status_info *status);

/** Restore default mock status (activated + DECT_MDM_MOCK_FW_VERSION). */
void dect_mdm_mock_status_info_clear(void);

#endif /* DECT_MDM_MOCK_TEST_SEAM_H_ */
