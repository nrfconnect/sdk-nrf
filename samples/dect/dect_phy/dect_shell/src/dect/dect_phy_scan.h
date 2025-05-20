/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DECT_PHY_SCAN_H
#define DECT_PHY_SCAN_H
#include <zephyr/kernel.h>
#include "dect_phy_shell.h"

enum dect_phy_rssi_scan_data_result_verdict {
	DECT_PHY_RSSI_SCAN_VERDICT_UNKNOWN,
	DECT_PHY_RSSI_SCAN_VERDICT_FREE,
	DECT_PHY_RSSI_SCAN_VERDICT_POSSIBLE,
	DECT_PHY_RSSI_SCAN_VERDICT_BUSY,
};

#define DECT_PHY_RSSI_SCAN_MAX_CHANNELS 30

struct dect_phy_rssi_scan_data_result_measurement_data {
	int8_t measurement_highs[DECT_RADIO_FRAME_SUBSLOT_COUNT];
};

struct dect_phy_rssi_scan_data_result_verdict_type_subslot_count {
	uint16_t free_subslot_count;
	uint16_t possible_subslot_count;
	uint16_t busy_subslot_count;

	uint16_t saturated_subslot_count;
	uint16_t not_measured_subslot_count;

	double free_percent;
	double possible_percent;
};

struct dect_phy_rssi_scan_data_result {
	uint16_t channel;
	enum dect_phy_rssi_scan_data_result_verdict result;
	int8_t rssi_high_level;
	int8_t rssi_low_level;
	uint16_t total_scan_count;

	/* Only with DECT_PHY_RSSI_SCAN_RESULT_VERDICT_TYPE_SUBSLOT_COUNT */
	struct dect_phy_rssi_scan_data_result_verdict_type_subslot_count subslot_count_type_results;
};

struct dect_phy_rssi_scan_channel_results {
	enum nrf_modem_dect_phy_err phy_status;

	uint16_t free_channels[DECT_PHY_RSSI_SCAN_MAX_CHANNELS];
	uint8_t free_channels_count;

	uint16_t possible_channels[DECT_PHY_RSSI_SCAN_MAX_CHANNELS];
	uint8_t possible_channels_count;

	uint16_t busy_channels[DECT_PHY_RSSI_SCAN_MAX_CHANNELS];
	uint8_t busy_channels_count;
};

typedef void (*dect_phy_rssi_scan_completed_callback_t)(enum nrf_modem_dect_phy_err phy_status);

void dect_phy_scan_rssi_latest_results_print(void);
int dect_phy_scan_rssi_results_get_by_channel(
	uint16_t channel, struct dect_phy_rssi_scan_data_result *filled_results);
int dect_phy_scan_rssi_channel_results_get(
	struct dect_phy_rssi_scan_channel_results *filled_results);
int dect_phy_scan_rssi_results_based_best_channel_get(void);

int dect_phy_scan_rssi_start(struct dect_phy_rssi_scan_params *params,
			     dect_phy_rssi_scan_completed_callback_t fp_callback);
void dect_phy_scan_rssi_stop(void);
void dect_phy_scan_rssi_rx_th_run(struct dect_phy_rssi_scan_params *cmd_params);
void dect_phy_scan_rssi_finished_handle(enum nrf_modem_dect_phy_err status);
void dect_phy_scan_rssi_cb_handle(enum nrf_modem_dect_phy_err status,
				  struct nrf_modem_dect_phy_rssi_event const *p_result);
int dect_phy_scan_rssi_data_init(struct dect_phy_rssi_scan_params *params);
int dect_phy_scan_rssi_data_reinit_with_current_params(void);


void dect_phy_scan_rx_th_run(struct dect_phy_rx_cmd_params *cmd_params);

const char *dect_phy_rssi_scan_result_verdict_to_string(
	int verdict, char *out_str_buff);

#endif /* DECT_PHY_SCAN_H */
