/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef TETHER_DECT_EVENTS_H_
#define TETHER_DECT_EVENTS_H_

#include <net/dect/dect_net_l2_mgmt.h>

#ifdef __cplusplus
extern "C" {
#endif

extern bool dect_scan_result_received;
extern bool dect_scan_done_received;
extern enum dect_status_values dect_scan_done_status;
extern struct dect_scan_result_evt received_beacon_data;
extern bool beacon_data_valid;

extern bool dect_association_changed_received;
extern bool dect_association_created_received;
extern bool dect_association_failed_received;
extern bool dect_association_released_received;
extern int dect_association_released_count;
extern struct dect_association_changed_evt received_association_data;

extern bool dect_network_status_received;
extern struct dect_network_status_evt received_network_status_data;

extern bool dect_activate_done_received;
extern enum dect_status_values dect_activate_done_status;
extern bool dect_deactivate_done_received;
extern enum dect_status_values dect_deactivate_done_status;

extern bool dect_rssi_scan_result_received;
extern struct dect_rssi_scan_result_evt received_rssi_scan_result_data;
extern bool dect_rssi_scan_done_received;
extern enum dect_status_values dect_rssi_scan_done_status;

void tether_dect_events_reset_flags(void);
int tether_dect_events_init(void);
void tether_dect_events_deinit(void);

extern struct k_sem tether_activation_done_sem;

#ifdef __cplusplus
}
#endif

#endif /* TETHER_DECT_EVENTS_H_ */
