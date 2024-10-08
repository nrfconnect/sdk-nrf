/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <zephyr/shell/shell.h>
#include "desh_print.h"
#include "nrf_modem_dect_phy.h"

#include "dect_common_utils.h"
#include "dect_phy_common_rx.h"
#include "dect_phy_api_scheduler.h"
#include "dect_app_time.h"
#include "dect_common_settings.h"

#include "dect_phy_ctrl.h"
#include "dect_phy_scan.h"
#include "dect_phy_rx.h"

enum dect_phy_rssi_scan_data_result_verdict {
	DECT_PHY_RSSI_SCAN_VERDICT_FREE,
	DECT_PHY_RSSI_SCAN_VERDICT_POSSIBLE,
	DECT_PHY_RSSI_SCAN_VERDICT_BUSY,
};
struct dect_phy_rssi_scan_data_result {
	uint16_t channel;
	enum dect_phy_rssi_scan_data_result_verdict result;
	int8_t rssi_high_level;
	int8_t rssi_low_level;
	uint16_t total_scan_count;
};

struct dect_phy_rssi_scan_data {
	bool on_going;

	/* Input params */
	struct dect_phy_rssi_scan_params cmd_params;

	/* Current scanning state */
	uint64_t start_time_mdm_ticks;
	uint32_t total_time_subslots;
	uint16_t current_scan_count; /* Scan count per channel */
	uint16_t current_saturated_count;
	uint16_t current_not_measured_count;
	uint16_t current_meas_fail_count;

	int8_t rssi_high_level;
	int8_t rssi_low_level;
	uint16_t current_channel; /* Currently scanned channel */

	/* Results */
	uint16_t results_index;
	struct dect_phy_rssi_scan_data_result results[30];

	uint8_t current_free_channels_index;
	uint16_t free_channels[30];

	uint8_t current_possible_channels_index;
	uint16_t possible_channels[30];

	uint8_t current_busy_channels_index;
	uint16_t busy_channels[30];

	dect_phy_rssi_scan_completed_callback_t fp_callback;
} rssi_scan_data;

void dect_phy_scan_rssi_rx_th_run(struct dect_phy_rssi_scan_params *cmd_params)
{
	struct nrf_modem_dect_phy_rssi_params rssi_params = {
		.start_time = 0,
		.handle = DECT_PHY_COMMON_RSSI_SCAN_HANDLE,
		.carrier = rssi_scan_data.current_channel,
		.duration = rssi_scan_data.total_time_subslots,
		.reporting_interval = NRF_MODEM_DECT_PHY_RSSI_INTERVAL_24_SLOTS,
	};
	uint64_t mdm_time_now = dect_app_modem_time_now();
	int ret = 0;
	struct dect_phy_settings *curr_settings = dect_common_settings_ref_get();

	if (rssi_scan_data.start_time_mdm_ticks == 0) {
		/* 1st time*/
		int factor = 1;

		if (!dect_phy_api_scheduler_done_list_is_empty()) {
			/* Something cooking there, let's wait more */
			factor = 2;
		}
		if (cmd_params->suspend_scheduler) {
			/* Wait for a while to get modem emptied of already scheduled stuff */
			rssi_params.start_time =
				mdm_time_now +
				(factor *
				 MS_TO_MODEM_TICKS(DECT_PHY_API_SCHEDULER_OP_TIME_WINDOW_MS + 100));
			rssi_scan_data.start_time_mdm_ticks = rssi_params.start_time;
		} else {
			rssi_scan_data.start_time_mdm_ticks = mdm_time_now;
		}
	} else {
		if (cmd_params->suspend_scheduler && !dect_phy_api_scheduler_done_list_is_empty()) {
			rssi_params.start_time =
				mdm_time_now +
				(US_TO_MODEM_TICKS(curr_settings->scheduler.scheduling_delay_us));
		}
	}

	/* Override set start time to zero if mdm api was reinited then no other
	 * operations on the way
	 */
	if (cmd_params->reinit_mdm_api) {
		rssi_params.start_time = 0;
	}

	ret = nrf_modem_dect_phy_rssi(&rssi_params);
	if (ret) {
		desh_error("nrf_modem_dect_phy_rssi failed %d", ret);
		goto rssi_scan_done;
	}
	return;

rssi_scan_done:
	rssi_scan_data.on_going = false;
	if (rssi_scan_data.fp_callback) {
		struct dect_phy_rssi_channel_scan_result_t callback_results;

		if (ret) {
			callback_results.phy_status =
				NRF_MODEM_DECT_PHY_ERR_NO_MEMORY; /* Not success */
		} else {
			callback_results.phy_status = NRF_MODEM_DECT_PHY_SUCCESS;
		}
		dect_phy_scan_rssi_results_get(&callback_results);
		rssi_scan_data.fp_callback(&callback_results);
	} else {
		desh_print("RSSI scan DONE.");
	}
}

int dect_phy_scan_rssi_results_get(struct dect_phy_rssi_channel_scan_result_t *filled_results)
{
	filled_results->free_channels_count = rssi_scan_data.current_free_channels_index;
	memcpy(filled_results->free_channels, rssi_scan_data.free_channels,
	       filled_results->free_channels_count * sizeof(uint16_t));

	filled_results->possible_channels_count = rssi_scan_data.current_possible_channels_index;
	memcpy(filled_results->possible_channels, rssi_scan_data.possible_channels,
	       filled_results->possible_channels_count * sizeof(uint16_t));

	filled_results->busy_channels_count = rssi_scan_data.current_busy_channels_index;
	memcpy(filled_results->busy_channels, rssi_scan_data.busy_channels,
	       filled_results->busy_channels_count * sizeof(uint16_t));

	return 0;
}

void dect_phy_scan_rssi_latest_results_print(void)
{
	int i;
	char color[10];

	if (rssi_scan_data.results_index == 0) {
		desh_print("No RSSI scanning results.");
	} else {
		desh_print("Latest RSSI scanning results:");
		for (i = 0; i < rssi_scan_data.results_index; i++) {
			desh_print("  channel           %d", rssi_scan_data.results[i].channel);
			desh_print("  scanning count    %d",
				   rssi_scan_data.results[i].total_scan_count);
			if (rssi_scan_data.results[i].result == DECT_PHY_RSSI_SCAN_VERDICT_BUSY) {
				sprintf(color, "%s", ANSI_COLOR_RED);
			} else if (rssi_scan_data.results[i].result ==
				   DECT_PHY_RSSI_SCAN_VERDICT_POSSIBLE) {
				sprintf(color, "%s", ANSI_COLOR_YELLOW);
			} else {
				sprintf(color, "%s", ANSI_COLOR_GREEN);
			}

			desh_print("%s  highest RSSI      %d%s", color,
				   rssi_scan_data.results[i].rssi_high_level, ANSI_RESET_ALL);
			desh_print("  lowest RSSI       %d\n",
				   rssi_scan_data.results[i].rssi_low_level);
		}
		desh_print("RSSI scanning results listing done.");
	}
}

void dect_phy_scan_rssi_stop(void)
{
	rssi_scan_data.on_going = false;
	desh_print("RSSI scan stopping.");
}

void dect_phy_scan_rssi_data_init(struct dect_phy_rssi_scan_params *params)
{
	memset(&rssi_scan_data, 0, sizeof(struct dect_phy_rssi_scan_data));
	rssi_scan_data.rssi_high_level = -127;
	rssi_scan_data.rssi_low_level = 1;

	if (params) {
		rssi_scan_data.total_time_subslots = MS_TO_SUBSLOTS(params->scan_time_ms);
		rssi_scan_data.current_channel = params->channel;
		rssi_scan_data.cmd_params = *params;
	}
}

static void dect_phy_scan_rssi_scan_params_get(struct dect_phy_rssi_scan_params *params)
{
	*params = rssi_scan_data.cmd_params;
}

void dect_phy_scan_rssi_data_reinit_with_current_params(void)
{
	struct dect_phy_rssi_scan_params rssi_params;

	dect_phy_scan_rssi_scan_params_get(&rssi_params);
	dect_phy_scan_rssi_data_init(&rssi_params);
}

int dect_phy_scan_rssi_start(struct dect_phy_rssi_scan_params *params,
			     dect_phy_rssi_scan_completed_callback_t fp_callback)
{
	struct dect_phy_settings *curr_settings = dect_common_settings_ref_get();

	dect_phy_scan_rssi_data_init(params);

	rssi_scan_data.fp_callback = fp_callback;
	rssi_scan_data.on_going = true;

	if (params->channel == 0) {
		/* All channels on set band */
		rssi_scan_data.current_channel =
			dect_common_utils_channel_min_on_band(curr_settings->common.band_nbr);
	}

	/* Initiate rssi scanning in a thread */
	int ret = dect_phy_rx_phy_measure_rssi_op_add(&rssi_scan_data.cmd_params);

	if (ret) {
		desh_error("(%s): dect_phy_rx_phy_measure_rssi_op_add failed: %d", (__func__), ret);
	} else {
		desh_print("RSSI scan duration: scan_time_ms %d (subslots %d)",
			   params->scan_time_ms, rssi_scan_data.total_time_subslots);
	}

	return 0;
}

void dect_phy_scan_rssi_finished_handle(enum nrf_modem_dect_phy_err status)
{
	if (status == NRF_MODEM_DECT_PHY_SUCCESS) {
		char color[10];
		bool stop_scanning = false;
		bool result_skipped = false;
		bool nbr_channel = dect_phy_ctrl_nbr_is_in_channel(rssi_scan_data.current_channel);
		struct dect_phy_settings *curr_settings = dect_common_settings_ref_get();

		/* print a summary for this round */
		desh_print("RSSI scanning results:");
		desh_print("  channel                               %d",
			   rssi_scan_data.current_channel);
		if (nbr_channel) {
			desh_print("    neighbor has been seen in this channel");
		}
		desh_print("  total scanning count                  %d",
			   rssi_scan_data.current_scan_count);
		if (rssi_scan_data.current_saturated_count) {
			desh_print("    saturations                         %d",
				   rssi_scan_data.current_saturated_count);
		}
		if (rssi_scan_data.current_not_measured_count) {
			desh_print("    not measured                        %d",
				   rssi_scan_data.current_not_measured_count);
		}
		if (rssi_scan_data.current_meas_fail_count) {
			desh_print("    measurement failed                  %d",
				   rssi_scan_data.current_meas_fail_count);
		}

		rssi_scan_data.results[rssi_scan_data.results_index].channel =
			rssi_scan_data.current_channel;
		rssi_scan_data.results[rssi_scan_data.results_index].rssi_high_level =
			rssi_scan_data.rssi_high_level;
		rssi_scan_data.results[rssi_scan_data.results_index].rssi_low_level =
			rssi_scan_data.rssi_low_level;
		rssi_scan_data.results[rssi_scan_data.results_index].total_scan_count =
			rssi_scan_data.current_scan_count;

		if (rssi_scan_data.rssi_high_level > rssi_scan_data.cmd_params.busy_rssi_limit) {
			sprintf(color, "%s", ANSI_COLOR_RED);
			rssi_scan_data.results[rssi_scan_data.results_index].result =
				DECT_PHY_RSSI_SCAN_VERDICT_BUSY;
			rssi_scan_data.busy_channels[rssi_scan_data.current_busy_channels_index++] =
				rssi_scan_data.current_channel;

		} else if (rssi_scan_data.rssi_high_level <=
			   rssi_scan_data.cmd_params.free_rssi_limit) {
			sprintf(color, "%s", ANSI_COLOR_GREEN);
			stop_scanning = rssi_scan_data.cmd_params.stop_on_1st_free_channel;
			if ((rssi_scan_data.current_channel ==
			     rssi_scan_data.cmd_params.dont_stop_on_this_channel) ||
			    (nbr_channel && rssi_scan_data.cmd_params.dont_stop_on_nbr_channels)) {
				stop_scanning = false;
				result_skipped = true;
				desh_print("    result skipped");
			} else {
				rssi_scan_data.results[rssi_scan_data.results_index].result =
					DECT_PHY_RSSI_SCAN_VERDICT_FREE;
				rssi_scan_data.free_channels
					[rssi_scan_data.current_free_channels_index++] =
					rssi_scan_data.current_channel;
			}

		} else {
			sprintf(color, "%s", ANSI_COLOR_YELLOW);
			rssi_scan_data.results[rssi_scan_data.results_index].result =
				DECT_PHY_RSSI_SCAN_VERDICT_POSSIBLE;

			rssi_scan_data.possible_channels
				[rssi_scan_data.current_possible_channels_index++] =
				rssi_scan_data.current_channel;
		}

		desh_print("%s  highest RSSI                          %d%s", color,
			   rssi_scan_data.rssi_high_level, ANSI_RESET_ALL);
		desh_print("  lowest RSSI                           %d",
			   rssi_scan_data.rssi_low_level);

		if (result_skipped) {
			desh_print("    result skipped");
		} else {
			rssi_scan_data.results_index++;
		}

		/* But are we all done yet? */
		if (!rssi_scan_data.on_going) {
			/* Scanning stopped */
			goto rssi_scan_done;
		}
		if (!stop_scanning &&
		    (rssi_scan_data.cmd_params.channel == 0 &&
		     rssi_scan_data.current_channel != dect_common_utils_channel_max_on_band(
							       curr_settings->common.band_nbr))) {
			/* Start a new round in next channel */

			rssi_scan_data.start_time_mdm_ticks = dect_app_modem_time_now();
			rssi_scan_data.rssi_high_level = -127;
			rssi_scan_data.rssi_low_level = 1;
			rssi_scan_data.current_scan_count = 0;

			rssi_scan_data.current_channel =
				dect_common_utils_get_next_channel_in_band_range(
					curr_settings->common.band_nbr,
					rssi_scan_data.current_channel,
					rssi_scan_data.cmd_params.only_allowed_channels);

			if (!rssi_scan_data.current_channel) {
				goto rssi_scan_done;
			}

			/* Initiate rssi scanning in a thread */
			int ret = dect_phy_rx_phy_measure_rssi_op_add(&rssi_scan_data.cmd_params);

			if (ret) {
				desh_error("(%s): dect_phy_rx_phy_measure_rssi_op_add failed: %d",
					   (__func__), ret);
			}

		} else {
			goto rssi_scan_done;
		}
	} else {
		char tmp_str[128] = {0};

		dect_common_utils_modem_phy_err_to_string(
			status, NRF_MODEM_DECT_PHY_TEMP_NOT_MEASURED, tmp_str);
		desh_warn("RSSI scanning failed: %s", tmp_str);
		goto rssi_scan_done;
	}
	return;

rssi_scan_done:
	if (rssi_scan_data.fp_callback) {
		struct dect_phy_rssi_channel_scan_result_t callback_results;

		callback_results.phy_status = status;
		callback_results.free_channels_count = rssi_scan_data.current_free_channels_index;
		memcpy(callback_results.free_channels, rssi_scan_data.free_channels,
		       callback_results.free_channels_count * sizeof(uint16_t));

		callback_results.possible_channels_count =
			rssi_scan_data.current_possible_channels_index;
		memcpy(callback_results.possible_channels, rssi_scan_data.possible_channels,
		       callback_results.possible_channels_count * sizeof(uint16_t));

		callback_results.busy_channels_count = rssi_scan_data.current_busy_channels_index;
		memcpy(callback_results.busy_channels, rssi_scan_data.busy_channels,
		       callback_results.busy_channels_count * sizeof(uint16_t));

		rssi_scan_data.fp_callback(&callback_results);
	} else {
		if (rssi_scan_data.on_going) {
			desh_print("RSSI scan DONE.");
		}
	}
	rssi_scan_data.on_going = false;
}

void dect_phy_scan_rssi_cb_handle(enum nrf_modem_dect_phy_err status,
				  struct nrf_modem_dect_phy_rssi_meas const *p_result)
{
	if (status == NRF_MODEM_DECT_PHY_SUCCESS) {
		__ASSERT_NO_MSG(p_result != NULL);

		/* Store response */
		for (int i = 0; i < p_result->meas_len; i++) {
			int8_t curr_meas = p_result->meas[i];

			if (curr_meas > 0) {
				rssi_scan_data.current_saturated_count++;
			} else if (curr_meas == NRF_MODEM_DECT_PHY_RSSI_NOT_MEASURED) {
				rssi_scan_data.current_not_measured_count++;
			} else {
				if (curr_meas > rssi_scan_data.rssi_high_level) {
					rssi_scan_data.rssi_high_level = curr_meas;
				}
				if (curr_meas < rssi_scan_data.rssi_low_level) {
					rssi_scan_data.rssi_low_level = curr_meas;
				}
			}
		}
	} else {
		rssi_scan_data.current_meas_fail_count++;
	}
	rssi_scan_data.current_scan_count++;

	/* For on a command that have set RSSI measurement reporting interval during RX.
	 * RX op is having always an reporting interval of NRF_MODEM_DECT_PHY_RSSI_INTERVAL_24_SLOTS
	 * (10msec) from modem, and that's why factor of 100 to get to seconds.
	 */
	if (rssi_scan_data.cmd_params.interval_secs &&
	    (rssi_scan_data.current_scan_count ==
	     (rssi_scan_data.cmd_params.interval_secs * 100))) {
		dect_phy_ctrl_msgq_non_data_op_add(DECT_PHY_CTRL_OP_PHY_API_MDM_RSSI_CB_ON_RX_OP);
	}
}

/**************************************************************************************************/

void dect_phy_scan_rx_th_run(struct dect_phy_rx_cmd_params *params)
{
	struct nrf_modem_dect_phy_rx_params rx_op = {
		.handle = params->handle,
		.start_time = 0,
		.mode = NRF_MODEM_DECT_PHY_RX_MODE_CONTINUOUS,
		.link_id = NRF_MODEM_DECT_PHY_LINK_UNSPECIFIED,
		.carrier = params->channel,
		.network_id = params->network_id,
		.rssi_level = params->expected_rssi_level,
		.duration = SECONDS_TO_MODEM_TICKS(params->duration_secs),
		.rssi_interval = NRF_MODEM_DECT_PHY_RSSI_INTERVAL_OFF,
	};
	int ret;

	if (params->rssi_interval_secs) {
		rx_op.rssi_interval = NRF_MODEM_DECT_PHY_RSSI_INTERVAL_24_SLOTS;
	}

	if (params->suspend_scheduler) {
		/* Modem is not necessarily idle thus start_time zero cannot be used. */
		/* Wait for a while to get modem emptied of already scheduled stuff */

		uint64_t time_now = dect_app_modem_time_now();

		rx_op.start_time =
			time_now +
			(2 * MS_TO_MODEM_TICKS(DECT_PHY_API_SCHEDULER_OP_TIME_WINDOW_MS));
	}

	rx_op.filter = params->filter;

	printk("Starting RX: channel %d, rssi_level %d, duration %d secs.\n", params->channel,
	       params->expected_rssi_level, params->duration_secs);

	ret = dect_phy_common_rx_op(&rx_op);
	if (ret) {
		desh_error("nrf_modem_dect_phy_rx failed %d", ret);
	}
}

/**************************************************************************************************/
