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
#include <nrf_modem_dect_phy.h>

#include "desh_print.h"

#include "dect_common_utils.h"
#include "dect_phy_common_rx.h"
#include "dect_phy_api_scheduler.h"
#include "dect_app_time.h"
#include "dect_common_settings.h"

#include "dect_phy_ctrl.h"
#include "dect_phy_scan.h"
#include "dect_phy_rx.h"

struct dect_phy_rssi_scan_data {
	bool on_going;

	/* Input params */
	struct dect_phy_rssi_scan_params cmd_params;

	/* Current scanning state */
	enum nrf_modem_dect_phy_err phy_status;

	uint32_t total_time_subslots;
	uint16_t current_scan_count; /* Scan count per channel */
	uint16_t current_saturated_count;
	uint16_t current_not_measured_count;
	uint16_t current_meas_fail_count;

	int8_t rssi_high_level;
	int8_t rssi_low_level;
	uint16_t current_channel; /* Currently scanned channel */

	uint64_t first_mdm_meas_time_mdm_ticks;

	/* Storage for measurement data extracted to subslot level */
	struct dect_phy_rssi_scan_data_result_measurement_data *measurement_data_ptr;
	uint16_t measurement_data_count;

	/* Results */
	uint16_t results_index;
	struct dect_phy_rssi_scan_data_result results[DECT_PHY_RSSI_SCAN_MAX_CHANNELS];

	uint8_t current_free_channels_index;
	uint16_t free_channels[DECT_PHY_RSSI_SCAN_MAX_CHANNELS];

	uint8_t current_possible_channels_index;
	uint16_t possible_channels[DECT_PHY_RSSI_SCAN_MAX_CHANNELS];

	uint8_t current_busy_channels_index;
	uint16_t busy_channels[DECT_PHY_RSSI_SCAN_MAX_CHANNELS];

	dect_phy_rssi_scan_completed_callback_t fp_callback;
} rssi_scan_data;

static void dect_phy_scan_rssi_on_going_set(bool on_going)
{
	rssi_scan_data.on_going = on_going;

	if (!on_going && rssi_scan_data.cmd_params.result_verdict_type ==
		DECT_PHY_RSSI_SCAN_RESULT_VERDICT_TYPE_SUBSLOT_COUNT) {
		if (rssi_scan_data.measurement_data_ptr) {
			k_free(rssi_scan_data.measurement_data_ptr);
			rssi_scan_data.measurement_data_ptr = NULL;
			rssi_scan_data.measurement_data_count = 0;
		}
	}
}

void dect_phy_scan_rssi_rx_th_run(struct dect_phy_rssi_scan_params *cmd_params)
{
	struct nrf_modem_dect_phy_rssi_params rssi_params = {
		.start_time = 0,
		.handle = DECT_PHY_COMMON_RSSI_SCAN_HANDLE,
		.carrier = rssi_scan_data.current_channel,
		.duration = rssi_scan_data.total_time_subslots,
		.reporting_interval = NRF_MODEM_DECT_PHY_RSSI_INTERVAL_24_SLOTS,
	};
	int ret = 0;
	struct dect_phy_settings *curr_settings = dect_common_settings_ref_get();
	uint64_t latency = dect_phy_ctrl_modem_latency_for_next_op_get(false) +
			(US_TO_MODEM_TICKS(curr_settings->scheduler.scheduling_delay_us));
	uint64_t mdm_time_now = dect_app_modem_time_now();

	rssi_params.start_time = mdm_time_now + latency;

	ret = nrf_modem_dect_phy_rssi(&rssi_params);
	if (ret) {
		desh_error("nrf_modem_dect_phy_rssi failed %d", ret);
		goto rssi_scan_done;
	}
	return;

rssi_scan_done:
	dect_phy_scan_rssi_on_going_set(false);
	if (rssi_scan_data.fp_callback) {
		enum nrf_modem_dect_phy_err phy_status;

		if (ret) {
			phy_status = NRF_MODEM_DECT_PHY_ERR_NO_MEMORY; /* Not success */
		} else {
			phy_status = NRF_MODEM_DECT_PHY_SUCCESS;
		}
		rssi_scan_data.fp_callback(phy_status);
	} else {
		desh_print("RSSI scan DONE.");
	}
}

int dect_phy_scan_rssi_channel_results_get(
	struct dect_phy_rssi_scan_channel_results *filled_results)
{
	if (rssi_scan_data.on_going) {
		return -EBUSY;
	}
	__ASSERT_NO_MSG(filled_results);

	filled_results->phy_status = rssi_scan_data.phy_status;

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

int dect_phy_scan_rssi_results_get_by_channel(
	uint16_t channel, struct dect_phy_rssi_scan_data_result *filled_results)
{
	if (rssi_scan_data.on_going) {
		return -EBUSY;
	}
	__ASSERT_NO_MSG(filled_results);

	for (int i = 0; i < rssi_scan_data.results_index; i++) {
		if (rssi_scan_data.results[i].channel == channel) {
			*filled_results = rssi_scan_data.results[i];
			return 0;
		}
	}
	return -ENODATA;
}

int dect_phy_scan_rssi_results_based_best_channel_get(void)
{
	uint16_t best_channel = 0;
	uint16_t best_busy_subslots = 1000; /* Lower the better */
	int8_t best_rssi_high_level = 127;  /* Lower the better */

	if (rssi_scan_data.on_going) {
		return -EBUSY;
	}
	/* MAC spec, ch. 5.1.2:
	 * after measuring the supported channels,
	 * the RD should select an operating channel(s) or consecutive operating
	 * channels in the following manner:
	 */

	/* 1st: if any channel where all subslots are "free", is found: */
	for (int i = 0; i < rssi_scan_data.results_index; i++) {
		if (rssi_scan_data.results[i].result == DECT_PHY_RSSI_SCAN_VERDICT_FREE) {
			if (rssi_scan_data.results[i].rssi_high_level < best_rssi_high_level) {
				best_rssi_high_level = rssi_scan_data.results[i].rssi_high_level;
				best_channel = rssi_scan_data.results[i].channel;
			}
		}
	}

	if (best_channel) {
		/* then, select the channel. */
		return best_channel;
	}

	if (rssi_scan_data.cmd_params.result_verdict_type !=
	    DECT_PHY_RSSI_SCAN_RESULT_VERDICT_TYPE_SUBSLOT_COUNT) {
		return -1; /* Not found */
	}

	/* 2nd: select the channel that has the lowest number of "busy" subslots */
	for (int i = 0; i < rssi_scan_data.results_index; i++) {
		if (rssi_scan_data.results[i].subslot_count_type_results.busy_subslot_count <
			best_busy_subslots) {
			best_busy_subslots = rssi_scan_data.results[i].subslot_count_type_results
				.busy_subslot_count;
			best_channel = rssi_scan_data.results[i].channel;
		}
	}

	/* 2a: if multiple channels have the same number of "busy" subslots: */
	uint16_t same_best_busy_subslots_count = 0;

	for (int i = 0; i < rssi_scan_data.results_index; i++) {
		if (rssi_scan_data.results[i].subslot_count_type_results.busy_subslot_count ==
			best_busy_subslots) {
			same_best_busy_subslots_count++;
		}
	}

	uint16_t best_possible_subslots_count = 10000; /* Lower the better */

	if (same_best_busy_subslots_count > 1) {
		best_possible_subslots_count = rssi_scan_data.results[0].subslot_count_type_results
			.possible_subslot_count;

		/* 2b: then select the channel that has the lowest number of "possible" subslots */
		for (int i = 0; i < rssi_scan_data.results_index; i++) {
			if (rssi_scan_data.results[i].subslot_count_type_results
				.busy_subslot_count == best_busy_subslots) {
				if (rssi_scan_data.results[i].subslot_count_type_results
					.possible_subslot_count < best_possible_subslots_count) {
					best_possible_subslots_count = rssi_scan_data.results[i]
						.subslot_count_type_results.possible_subslot_count;
					best_channel = rssi_scan_data.results[i].channel;
				}
			}
		}
	}

	return best_channel;
}

void dect_phy_scan_rssi_latest_results_print(void)
{
	int i;
	char color[10];
	char verdict_str[16];
	enum dect_phy_rssi_scan_data_result_verdict verdict;
	struct dect_phy_settings *current_settings = dect_common_settings_ref_get();

	if (rssi_scan_data.results_index == 0) {
		desh_print("No RSSI scanning results.");
	} else {
		desh_print("Latest RSSI scanning results:");
		for (i = 0; i < rssi_scan_data.results_index; i++) {
			desh_print("  channel           %d", rssi_scan_data.results[i].channel);
			desh_print("  scanning count    %d",
				   rssi_scan_data.results[i].total_scan_count);
			if (rssi_scan_data.results[i].rssi_high_level > rssi_scan_data.cmd_params
				.busy_rssi_limit) {
				sprintf(color, "%s", ANSI_COLOR_RED);
			} else if (rssi_scan_data.results[i].rssi_high_level <= rssi_scan_data
					.cmd_params.free_rssi_limit) {
				sprintf(color, "%s", ANSI_COLOR_GREEN);
			} else {
				sprintf(color, "%s", ANSI_COLOR_YELLOW);
			}

			desh_print("%s  highest RSSI      %d%s", color,
				   rssi_scan_data.results[i].rssi_high_level, ANSI_RESET_ALL);
			desh_print("  lowest RSSI       %d\n",
				   rssi_scan_data.results[i].rssi_low_level);
			if (rssi_scan_data.cmd_params.result_verdict_type ==
			    DECT_PHY_RSSI_SCAN_RESULT_VERDICT_TYPE_SUBSLOT_COUNT) {
				verdict = rssi_scan_data.results[i].result;
				dect_phy_rssi_scan_result_verdict_to_string(verdict, verdict_str);

				desh_print("Subslot count based results:");
				desh_print(
					"  total subslots: %d\n"
					"  free subslots: %d, possible subslots: %d, busy subslots: %d",
					rssi_scan_data.total_time_subslots,
					rssi_scan_data.results[i]
						.subslot_count_type_results.free_subslot_count,
					rssi_scan_data.results[i]
						.subslot_count_type_results.possible_subslot_count,
					rssi_scan_data.results[i]
						.subslot_count_type_results.busy_subslot_count);

				desh_print(
					"  Final verdict %s%s%s based on SCAN_SUITABLE %d%%:\n"
					"    free: %.02f%%, possible: %.02f%%",
					color,
					verdict_str,
					ANSI_RESET_ALL,
					current_settings->rssi_scan.type_subslots_params
						.scan_suitable_percent,
					rssi_scan_data.results[i]
						.subslot_count_type_results.free_percent,
					rssi_scan_data.results[i].subslot_count_type_results
						.possible_percent);
			}
		}
		desh_print("Latest RSSI scanning results listing done.");
	}
}

void dect_phy_scan_rssi_stop(void)
{
	dect_phy_scan_rssi_on_going_set(false);
	desh_print("RSSI scan stopping.");
}

int dect_phy_scan_rssi_data_init(struct dect_phy_rssi_scan_params *params)
{
	if (rssi_scan_data.measurement_data_ptr) {
		k_free(rssi_scan_data.measurement_data_ptr);
		rssi_scan_data.measurement_data_count = 0;
	}
	memset(&rssi_scan_data, 0, sizeof(struct dect_phy_rssi_scan_data));
	rssi_scan_data.rssi_high_level = -127;
	rssi_scan_data.rssi_low_level = 1;

	if (params) {
		rssi_scan_data.total_time_subslots = MS_TO_SUBSLOTS(params->scan_time_ms);
		rssi_scan_data.current_channel = params->channel;
		rssi_scan_data.cmd_params = *params;

		if (params->result_verdict_type ==
			DECT_PHY_RSSI_SCAN_RESULT_VERDICT_TYPE_SUBSLOT_COUNT) {
			uint32_t frame_count = params->scan_time_ms / DECT_RADIO_FRAME_DURATION_MS;

			frame_count += 30; /* Add some extra for reinit delay */
			rssi_scan_data.measurement_data_count = frame_count;
			rssi_scan_data.measurement_data_ptr =
			(struct dect_phy_rssi_scan_data_result_measurement_data *)k_calloc(
				frame_count,
				sizeof(struct dect_phy_rssi_scan_data_result_measurement_data));
			if (rssi_scan_data.measurement_data_ptr == NULL) {
				desh_error("No memory for RSSI scan measurement data. Try again "
					   "with shorter scan time.");
				return -ENOMEM;
			}
		}
	}
	return 0;
}

static void dect_phy_scan_rssi_scan_params_get(struct dect_phy_rssi_scan_params *params)
{
	*params = rssi_scan_data.cmd_params;
}

int dect_phy_scan_rssi_data_reinit_with_current_params(void)
{
	struct dect_phy_rssi_scan_params rssi_params;
	int err = 0;

	dect_phy_scan_rssi_scan_params_get(&rssi_params);
	err = dect_phy_scan_rssi_data_init(&rssi_params);
	if (err) {
		desh_error("RSSI scan data reinit failed: err %d.", err);
	}
	return err;
}

int dect_phy_scan_rssi_start(struct dect_phy_rssi_scan_params *params,
			     dect_phy_rssi_scan_completed_callback_t fp_callback)
{
	struct dect_phy_settings *curr_settings = dect_common_settings_ref_get();
	int err = 0;

	err = dect_phy_scan_rssi_data_init(params);
	if (err) {
		desh_error("RSSI scan data init failed: err %d.", err);
		return err;
	}

	rssi_scan_data.fp_callback = fp_callback;
	dect_phy_scan_rssi_on_going_set(true);

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

/**************************************************************************************************/

static void dect_phy_rssi_scan_data_subslot_count_based_results(bool store_verdict)
{
	struct dect_phy_settings *current_settings = dect_common_settings_ref_get();
	char output_str[1024];
	char color[10];
	char final_verdict_str[16];
	bool possible_or_busy_verdict = false;
	double free_percent, possible_percent;
	enum dect_phy_rssi_scan_data_result_verdict final_verdict;
	uint32_t all_measured_subslots = 0;
	uint32_t current_measured_subslot_count;
	int8_t curr_meas;
	bool measured = true;

	/* Print only busy/possible measurements:
	 * i: for measurements
	 * j: for subslots
	 */
	for (int i = 0; i < rssi_scan_data.current_scan_count &&
			i < rssi_scan_data.measurement_data_count && rssi_scan_data.on_going; i++) {
		current_measured_subslot_count = 0;

		output_str[0] = '\0';
		for (int j = 0; j < DECT_RADIO_FRAME_SUBSLOT_COUNT; j++) {
			measured = true;
			curr_meas = rssi_scan_data.measurement_data_ptr[i]
				.measurement_highs[j];

			if (curr_meas > 0) {
				sprintf(color, "%s", ANSI_COLOR_BLUE);
				possible_or_busy_verdict = true; /* Printing these also */
				rssi_scan_data.results[rssi_scan_data.results_index]
					.subslot_count_type_results.saturated_subslot_count++;

				/* Saturated over measurement range -> busy */
				rssi_scan_data.results[rssi_scan_data.results_index]
					.subslot_count_type_results.busy_subslot_count++;
			} else if (curr_meas == NRF_MODEM_DECT_PHY_RSSI_NOT_MEASURED) {
				sprintf(color, "%s", ANSI_COLOR_BLUE);
				measured = false;
				possible_or_busy_verdict = true; /* Printing these also */
				rssi_scan_data.results[rssi_scan_data.results_index]
					.subslot_count_type_results.not_measured_subslot_count++;
			} else if (curr_meas > rssi_scan_data.cmd_params.busy_rssi_limit) {
				rssi_scan_data.results[rssi_scan_data.results_index]
					.subslot_count_type_results.busy_subslot_count++;
				sprintf(color, "%s", ANSI_COLOR_RED);
				possible_or_busy_verdict = true;
			} else if (curr_meas <= rssi_scan_data.cmd_params.free_rssi_limit) {
				rssi_scan_data.results[rssi_scan_data.results_index]
					.subslot_count_type_results.free_subslot_count++;
				sprintf(color, "%s", ANSI_COLOR_GREEN);
			} else {
				rssi_scan_data.results[rssi_scan_data.results_index]
					.subslot_count_type_results.possible_subslot_count++;
				sprintf(color, "%s", ANSI_COLOR_YELLOW);
				possible_or_busy_verdict = true;
			}
			sprintf(output_str + strlen(output_str), "%s%4d%s|%s", color, curr_meas,
				ANSI_COLOR_BLUE, ANSI_RESET_ALL);
			if (j == ((DECT_RADIO_FRAME_SUBSLOT_COUNT / 2) - 1)) {
				sprintf(output_str + strlen(output_str), "\n  ");
			}
			if (measured) {
				current_measured_subslot_count++;
			}
		}

		if (possible_or_busy_verdict && rssi_scan_data.cmd_params
			.type_subslots_params.detail_print) {
			desh_print("Measurement #%d with busy/possible subslots, started %llu, 48 "
				   "subslots:\n  %s",
				   i + 1,
				   rssi_scan_data.first_mdm_meas_time_mdm_ticks +
					(i * DECT_RADIO_FRAME_DURATION_IN_MODEM_TICKS),
				   output_str);
			possible_or_busy_verdict = false;
		}
		all_measured_subslots += current_measured_subslot_count;
	}

	if (all_measured_subslots == 0) {
		desh_warn("No subslots measured.");
		return;
	}

	/* Calculate and store the final verdict */
	free_percent = (double)rssi_scan_data.results[rssi_scan_data.results_index]
			       .subslot_count_type_results.free_subslot_count /
		       all_measured_subslots * 100;
	possible_percent = (double)(rssi_scan_data.results[rssi_scan_data.results_index]
					    .subslot_count_type_results.free_subslot_count +
				    rssi_scan_data.results[rssi_scan_data.results_index]
					    .subslot_count_type_results.possible_subslot_count) /
			   all_measured_subslots * 100;
	if (free_percent >=
	    current_settings->rssi_scan.type_subslots_params.scan_suitable_percent) {
		final_verdict = DECT_PHY_RSSI_SCAN_VERDICT_FREE;
		if (store_verdict == true) {
			rssi_scan_data.results[rssi_scan_data.results_index].result =
				final_verdict;
			rssi_scan_data.free_channels[rssi_scan_data.current_free_channels_index++] =
				rssi_scan_data.current_channel;
		}
	} else if (possible_percent >=
		   current_settings->rssi_scan.type_subslots_params.scan_suitable_percent) {
		final_verdict = DECT_PHY_RSSI_SCAN_VERDICT_POSSIBLE;
		if (store_verdict == true) {
			rssi_scan_data.results[rssi_scan_data.results_index].result =
				final_verdict;
			rssi_scan_data.possible_channels[
				rssi_scan_data.current_possible_channels_index++] =
					rssi_scan_data.current_channel;
		}

	} else {
		final_verdict = DECT_PHY_RSSI_SCAN_VERDICT_BUSY;
		if (store_verdict == true) {
			rssi_scan_data.results[rssi_scan_data.results_index].result =
				final_verdict;
			rssi_scan_data.busy_channels[rssi_scan_data.current_busy_channels_index++] =
				rssi_scan_data.current_channel;
		}
	}

	dect_phy_rssi_scan_result_verdict_to_string(final_verdict, final_verdict_str);


	desh_print("Subslot count based results:");
	desh_print(
		"  total subslots: %d\n"
		"  free subslots: %d, possible subslots: %d, busy subslots: %d\n"
		"  not measured subslots: %d, saturated subslots: %d",
		rssi_scan_data.total_time_subslots,
		rssi_scan_data.results[rssi_scan_data.results_index]
			.subslot_count_type_results.free_subslot_count,
		rssi_scan_data.results[rssi_scan_data.results_index]
			.subslot_count_type_results.possible_subslot_count,
		rssi_scan_data.results[rssi_scan_data.results_index]
			.subslot_count_type_results.busy_subslot_count,
		rssi_scan_data.results[rssi_scan_data.results_index]
			.subslot_count_type_results.not_measured_subslot_count,
		rssi_scan_data.results[rssi_scan_data.results_index]
			.subslot_count_type_results.saturated_subslot_count);

	rssi_scan_data.results[rssi_scan_data.results_index]
			.subslot_count_type_results.free_percent = free_percent;
	rssi_scan_data.results[rssi_scan_data.results_index].subslot_count_type_results
			.possible_percent = possible_percent;

	if (rssi_scan_data.current_scan_count > rssi_scan_data.measurement_data_count) {
		desh_warn("  WARNING: Not all measurements were stored. Received %d, "
			  "Storage size %d",
			rssi_scan_data.current_scan_count, rssi_scan_data.measurement_data_count);
	}
	desh_print(
		"  Final verdict %s%s%s based on SCAN_SUITABLE %d%%:\n"
		"    free: %.02f%%, possible: %.02f%%",
		color,
		final_verdict_str,
		ANSI_RESET_ALL,
		current_settings->rssi_scan.type_subslots_params.scan_suitable_percent,
		free_percent,
		possible_percent);
}

static bool dect_phy_rssi_scan_data_high_low_levels_results(bool *result_skipped)
{
	bool nbr_channel = dect_phy_ctrl_nbr_is_in_channel(rssi_scan_data.current_channel);
	bool stop_scanning = false;
	bool tmp_result_skipped = false;
	char color[10];

	if (rssi_scan_data.rssi_high_level > rssi_scan_data.cmd_params.busy_rssi_limit) {
		sprintf(color, "%s", ANSI_COLOR_RED);
		if (rssi_scan_data.cmd_params.result_verdict_type ==
		    DECT_PHY_RSSI_SCAN_RESULT_VERDICT_TYPE_ALL) {
			rssi_scan_data.results[rssi_scan_data.results_index].result =
				DECT_PHY_RSSI_SCAN_VERDICT_BUSY;
			rssi_scan_data.busy_channels[rssi_scan_data.current_busy_channels_index++] =
				rssi_scan_data.current_channel;
		}
	} else if (rssi_scan_data.rssi_high_level <= rssi_scan_data.cmd_params.free_rssi_limit) {
		sprintf(color, "%s", ANSI_COLOR_GREEN);
		stop_scanning = rssi_scan_data.cmd_params.stop_on_1st_free_channel;
		if ((rssi_scan_data.current_channel ==
		     rssi_scan_data.cmd_params.dont_stop_on_this_channel) ||
		    (nbr_channel && rssi_scan_data.cmd_params.dont_stop_on_nbr_channels)) {
			stop_scanning = false;
			tmp_result_skipped = true;
			desh_print("    result skipped");
		} else if (rssi_scan_data.cmd_params.result_verdict_type ==
			   DECT_PHY_RSSI_SCAN_RESULT_VERDICT_TYPE_ALL) {
			rssi_scan_data.results[rssi_scan_data.results_index].result =
				DECT_PHY_RSSI_SCAN_VERDICT_FREE;
			rssi_scan_data.free_channels[rssi_scan_data.current_free_channels_index++] =
				rssi_scan_data.current_channel;
		}
	} else {
		sprintf(color, "%s", ANSI_COLOR_YELLOW);
		if (rssi_scan_data.cmd_params.result_verdict_type ==
		    DECT_PHY_RSSI_SCAN_RESULT_VERDICT_TYPE_ALL) {
			rssi_scan_data.results[rssi_scan_data.results_index].result =
				DECT_PHY_RSSI_SCAN_VERDICT_POSSIBLE;
			rssi_scan_data.possible_channels
				[rssi_scan_data.current_possible_channels_index++] =
				rssi_scan_data.current_channel;
		}
	}

	desh_print("%s  highest RSSI                          %d%s", color,
		   rssi_scan_data.rssi_high_level, ANSI_RESET_ALL);
	desh_print("  lowest RSSI                           %d", rssi_scan_data.rssi_low_level);

	if (tmp_result_skipped) {
		desh_print("    result skipped");
	}
	*result_skipped = tmp_result_skipped;
	return stop_scanning;
}

static void dect_phy_rssi_scan_data_common_results_print(void)
{
	bool nbr_channel = dect_phy_ctrl_nbr_is_in_channel(rssi_scan_data.current_channel);

	rssi_scan_data.results[rssi_scan_data.results_index].channel =
		rssi_scan_data.current_channel;
	rssi_scan_data.results[rssi_scan_data.results_index].rssi_high_level =
		rssi_scan_data.rssi_high_level;
	rssi_scan_data.results[rssi_scan_data.results_index].rssi_low_level =
		rssi_scan_data.rssi_low_level;
	rssi_scan_data.results[rssi_scan_data.results_index].total_scan_count =
		rssi_scan_data.current_scan_count;

	desh_print("-----------------------------------------------------------------------------");
	desh_print("RSSI scanning results (meas #1 mdm time %llu):",
		rssi_scan_data.first_mdm_meas_time_mdm_ticks);
	desh_print("  channel                               %d", rssi_scan_data.current_channel);
	if (nbr_channel) {
		desh_print("    neighbor has been seen in this channel");
	}
	desh_print("  total scanning count                  %d", rssi_scan_data.current_scan_count);
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
}

void dect_phy_scan_rssi_finished_handle(enum nrf_modem_dect_phy_err status)
{
	rssi_scan_data.phy_status = status;

	if (status == NRF_MODEM_DECT_PHY_SUCCESS) {
		bool stop_scanning = false;
		bool result_skipped = false;
		struct dect_phy_settings *curr_settings = dect_common_settings_ref_get();


		/* print and store a summary for this round */
		dect_phy_rssi_scan_data_common_results_print();
		stop_scanning = dect_phy_rssi_scan_data_high_low_levels_results(&result_skipped);

		if (rssi_scan_data.cmd_params.result_verdict_type ==
			DECT_PHY_RSSI_SCAN_RESULT_VERDICT_TYPE_SUBSLOT_COUNT) {
			dect_phy_rssi_scan_data_subslot_count_based_results(!result_skipped);
		}

		if (!result_skipped) {
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
			rssi_scan_data.first_mdm_meas_time_mdm_ticks = 0;
			rssi_scan_data.rssi_high_level = -127;
			rssi_scan_data.rssi_low_level = 1;
			rssi_scan_data.current_scan_count = 0;
			rssi_scan_data.current_saturated_count = 0;
			rssi_scan_data.current_not_measured_count = 0;
			rssi_scan_data.current_meas_fail_count = 0;

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
	dect_phy_scan_rssi_on_going_set(false);
	if (rssi_scan_data.fp_callback) {
		rssi_scan_data.fp_callback(status);
	} else {
		if (rssi_scan_data.on_going) {
			desh_print("RSSI scan DONE.");
		}
	}
}

/**************************************************************************************************/

void dect_phy_scan_rssi_cb_handle(enum nrf_modem_dect_phy_err status,
				  struct nrf_modem_dect_phy_rssi_event const *p_result)
{
	if (status == NRF_MODEM_DECT_PHY_SUCCESS) {
		__ASSERT_NO_MSG(p_result != NULL);
		int8_t subslot_high = INT8_MIN;
		int8_t subslot_saturated_or_not_measured_value = 30;
		bool subslot_saturated_or_not_measured = false;

		if (rssi_scan_data.first_mdm_meas_time_mdm_ticks == 0) {
			rssi_scan_data.first_mdm_meas_time_mdm_ticks = p_result->meas_start_time;
		}

		/* We have requested NRF_MODEM_DECT_PHY_RSSI_INTERVAL_24_SLOTS
		 * reporting interval.
		 */
		__ASSERT_NO_MSG(p_result->meas_len <= DECT_RADIO_FRAME_SYMBOL_COUNT);

		/* Store response:
		 * i: response meas iterator
		 * j: symbols to subslot iterator
		 * k: stored subslot iterator
		 */
		for (int i = 0, j = 1, k = 0; i < p_result->meas_len; i++) {
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

			if (rssi_scan_data.cmd_params.result_verdict_type !=
				DECT_PHY_RSSI_SCAN_RESULT_VERDICT_TYPE_SUBSLOT_COUNT) {
				continue;
			}

			/* We are decreasing data amount from symbol level to subslot level
			 * So, we store highest RSSI value per subslot
			 */
			if (j <= DECT_RADIO_SUBSLOT_SYMBOL_COUNT) {
				if (curr_meas == NRF_MODEM_DECT_PHY_RSSI_NOT_MEASURED ||
				    curr_meas > 0) {
					subslot_saturated_or_not_measured = true;
					subslot_saturated_or_not_measured_value = curr_meas;
				} else if (curr_meas > subslot_high) {
					subslot_high = curr_meas;
				}
				j++;
			}

			if (j > DECT_RADIO_SUBSLOT_SYMBOL_COUNT) {
				__ASSERT_NO_MSG(k < DECT_RADIO_FRAME_SUBSLOT_COUNT);

				if (subslot_saturated_or_not_measured) {
					/* Some of the measurements were saturated or not measured.
					 * We consider whole subslot as last of either.
					 */
					subslot_high = subslot_saturated_or_not_measured_value;
				}
				/* We have gone through subslot count of measurements
				 * that are for each symbol.
				 * Store highest RSSI value per subslot
				 */
				__ASSERT_NO_MSG(rssi_scan_data.measurement_data_ptr);

				if (rssi_scan_data.current_scan_count <
					rssi_scan_data.measurement_data_count) {
					rssi_scan_data.measurement_data_ptr[
						rssi_scan_data.current_scan_count]
							.measurement_highs[k] = subslot_high;
				}
				j = 1; /* Next subslot */
				k++;
				subslot_high = INT8_MIN;
				subslot_saturated_or_not_measured = false;
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
	struct dect_phy_settings *current_settings;
	uint64_t time_now;
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
	int err;

	current_settings = dect_common_settings_ref_get();
	time_now = dect_app_modem_time_now();

	if (params->rssi_interval_secs) {
		rx_op.rssi_interval = NRF_MODEM_DECT_PHY_RSSI_INTERVAL_24_SLOTS;
	}

	if (params->suspend_scheduler) {
		/* Wait for a while to get modem emptied of already scheduled stuff */
		rx_op.start_time =
			time_now +
			(2 * MS_TO_MODEM_TICKS(DECT_PHY_API_SCHEDULER_OP_TIME_WINDOW_MS));
	} else {
		enum nrf_modem_dect_phy_radio_mode radio_mode;

		uint64_t latency = dect_phy_ctrl_modem_latency_for_next_op_get(false) +
			(3 * US_TO_MODEM_TICKS(current_settings->scheduler.scheduling_delay_us));

		err = dect_phy_ctrl_current_radio_mode_get(&radio_mode);
		if (!err && radio_mode == NRF_MODEM_DECT_PHY_RADIO_MODE_LOW_LATENCY &&
			dect_phy_api_scheduler_list_is_empty()) {
			rx_op.start_time = 0;
		} else {
			rx_op.start_time = time_now + latency;
		}
	}
	rx_op.filter = params->filter;

	desh_print("-----------------------------------------------------------------------------");
	desh_print("Starting RX: channel %d, rssi_level %d, duration %d secs.",
		params->channel, params->expected_rssi_level, params->duration_secs);

	err = dect_phy_common_rx_op(&rx_op);
	if (err) {
		desh_error("nrf_modem_dect_phy_rx failed %d", err);
	}
}

/**************************************************************************************************/

const char *dect_phy_rssi_scan_result_verdict_to_string(int verdict, char *out_str_buff)
{
	if (verdict == DECT_PHY_RSSI_SCAN_VERDICT_FREE) {
		sprintf(out_str_buff, "%sFREE%s", ANSI_COLOR_GREEN, ANSI_RESET_ALL);
	} else if (verdict == DECT_PHY_RSSI_SCAN_VERDICT_POSSIBLE) {
		sprintf(out_str_buff, "%sPOSSIBLE%s", ANSI_COLOR_YELLOW, ANSI_RESET_ALL);
	} else if (verdict == DECT_PHY_RSSI_SCAN_VERDICT_BUSY) {
		sprintf(out_str_buff, "%sBUSY%s", ANSI_COLOR_RED, ANSI_RESET_ALL);
	} else {
		sprintf(out_str_buff, "%sUNKNOWN%s", ANSI_COLOR_BLUE, ANSI_RESET_ALL);
	}
	return out_str_buff;
}
