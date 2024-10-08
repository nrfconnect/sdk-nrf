/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DECT_PHY_SCAN_H
#define DECT_PHY_SCAN_H
#include <zephyr/kernel.h>

struct dect_phy_rssi_channel_scan_result_t {
	enum nrf_modem_dect_phy_err phy_status;

	uint16_t free_channels[30];
	uint8_t free_channels_count;

	uint16_t possible_channels[30];
	uint8_t possible_channels_count;

	uint16_t busy_channels[30];
	uint8_t busy_channels_count;
};

typedef void (*dect_phy_rssi_scan_completed_callback_t)(
	struct dect_phy_rssi_channel_scan_result_t *p_scan_information);

int dect_phy_scan_rssi_results_get(struct dect_phy_rssi_channel_scan_result_t *filled_results);
int dect_phy_scan_rssi_start(struct dect_phy_rssi_scan_params *params,
			     dect_phy_rssi_scan_completed_callback_t fp_callback);
void dect_phy_scan_rssi_stop(void);
void dect_phy_scan_rssi_rx_th_run(struct dect_phy_rssi_scan_params *cmd_params);
void dect_phy_scan_rssi_finished_handle(enum nrf_modem_dect_phy_err status);
void dect_phy_scan_rssi_cb_handle(enum nrf_modem_dect_phy_err status,
				  struct nrf_modem_dect_phy_rssi_meas const *p_result);
void dect_phy_scan_rssi_data_init(struct dect_phy_rssi_scan_params *params);
void dect_phy_scan_rssi_data_reinit_with_current_params(void);

void dect_phy_scan_rssi_latest_results_print(void);

void dect_phy_scan_rx_th_run(struct dect_phy_rx_cmd_params *cmd_params);

#endif /* DECT_PHY_SCAN_H */
