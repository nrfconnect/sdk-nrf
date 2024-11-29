/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/sys/byteorder.h>

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <modem/nrf_modem_lib.h>

#include "desh_print.h"

#include "dect_app_time.h"
#include "dect_common.h"
#include "dect_common_utils.h"
#include "dect_common_settings.h"

#include "dect_phy_api_scheduler.h"

#include "dect_phy_ctrl.h"
#include "dect_phy_mac_nbr.h"
#include "dect_phy_mac_nbr_bg_scan.h"

struct dect_phy_mac_nbr_bg_scan_metrics_data {
	uint32_t scan_started_ok_count;
	uint32_t scan_start_fail_count;
	uint32_t scan_info_updated_count;
	uint32_t scan_info_time_shift_updated_count;
	int64_t scan_info_time_shift_last_value;
};

struct dect_phy_mac_nbr_bg_scan_data {
	bool running;
	struct dect_phy_mac_nbr_bg_scan_params params;
	struct dect_phy_mac_nbr_bg_scan_metrics_data metrics;

	uint64_t last_updated_rcv_time_mdm_ticks;
};

static struct dect_phy_mac_nbr_bg_scan_data nbr_bg_scan_data[DECT_PHY_MAC_MAX_NEIGBORS];

static struct dect_phy_mac_nbr_bg_scan_data *dect_mac_nbr_bg_scan_free_list_item_get(void)
{
	for (int i = 0; i < DECT_PHY_MAC_MAX_NEIGBORS; i++) {
		if (!nbr_bg_scan_data[i].running) {
			return &nbr_bg_scan_data[i];
		}
	}
	return NULL;
}

static struct dect_phy_mac_nbr_bg_scan_data *
dect_mac_nbr_bg_scan_list_item_by_phy_op_handle_get(uint32_t phy_op_handle)
{
	for (int i = 0; i < DECT_PHY_MAC_MAX_NEIGBORS; i++) {
		if (nbr_bg_scan_data[i].running &&
		    nbr_bg_scan_data[i].params.phy_op_handle == phy_op_handle) {
			return &nbr_bg_scan_data[i];
		}
	}
	return NULL;
}

static struct dect_phy_mac_nbr_bg_scan_data *
dect_mac_nbr_bg_scan_list_item_by_nbr_long_rd_id_get(uint32_t long_rd_id)
{
	for (int i = 0; i < DECT_PHY_MAC_MAX_NEIGBORS; i++) {
		if (nbr_bg_scan_data[i].running &&
		    nbr_bg_scan_data[i].params.target_long_rd_id == long_rd_id) {
			return &nbr_bg_scan_data[i];
		}
	}
	return NULL;
}

static void dect_phy_mac_nbr_bg_scan_scheduler_op_to_mdm_cb(
	struct dect_phy_common_op_completed_params *params, uint64_t frame_time)
{
	if (params->status == NRF_MODEM_DECT_PHY_SUCCESS) {
		dect_phy_ctrl_msgq_non_data_op_add(DECT_PHY_CTRL_OP_DEBUG_OFF);
	}
}

static void dect_phy_mac_nbr_bg_scan_scheduler_op_completed_cb(
	struct dect_phy_common_op_completed_params *params, uint64_t tx_frame_time)
{
	/* If we haven't received from a beacon, stop a bg scan and call a callback */
	struct dect_phy_mac_nbr_bg_scan_data *bg_scan_data =
		dect_mac_nbr_bg_scan_list_item_by_phy_op_handle_get(params->handle);
	if (!bg_scan_data) {
		desh_error("(%s): bg_scan_data not found (handle %d)", (__func__), params->handle);
		return;
	}
	uint64_t time_now = dect_app_modem_time_now();
	int64_t time_from_last_received_ms =
		MODEM_TICKS_TO_MS(time_now - bg_scan_data->last_updated_rcv_time_mdm_ticks);

	if (bg_scan_data->last_updated_rcv_time_mdm_ticks != 0 &&
	    time_from_last_received_ms > DECT_PHY_MAC_NBR_BG_SCAN_MAX_UNREACHABLE_TIME_MS) {
		bg_scan_data->running = false;
		dect_phy_api_scheduler_list_item_remove_by_phy_op_handle(params->handle);
		if (bg_scan_data->params.cb_op_completed) {
			struct dect_phy_mac_nbr_bg_scan_op_completed_info completed_info = {
				.cause = DECT_PHY_MAC_NBR_BG_SCAN_OP_COMPLETED_CAUSE_UNREACHABLE,
				.phy_op_handle = params->handle,
				.target_long_rd_id = bg_scan_data->params.target_long_rd_id,
			};

			bg_scan_data->params.cb_op_completed(&completed_info);
		}
	}
	dect_phy_ctrl_msgq_non_data_op_add(DECT_PHY_CTRL_OP_DEBUG_ON);

	/* Update metrics */
	if (params->status == NRF_MODEM_DECT_PHY_SUCCESS) {
		bg_scan_data->metrics.scan_started_ok_count++;
	} else {
		bg_scan_data->metrics.scan_start_fail_count++;
	}
}

static int dect_phy_mac_nbr_bg_scan_schedule(struct dect_phy_mac_nbr_bg_scan_params *params)
{
	__ASSERT_NO_MSG(params && params->target_nbr);

	int err = 0;
	uint64_t time_now = dect_app_modem_time_now();
	struct dect_phy_settings *current_settings = dect_common_settings_ref_get();
	uint64_t beacon_received = params->target_nbr->time_rcvd_mdm_ticks;
	uint64_t first_possible_rx =
		time_now + (MS_TO_MODEM_TICKS(2 * DECT_PHY_API_SCHEDULER_OP_TIME_WINDOW_MS));
	uint64_t next_beacon_frame_start, beacon_interval_mdm_ticks;
	int32_t beacon_interval_ms = dect_phy_mac_pdu_cluster_beacon_period_in_ms(
		params->target_nbr->beacon_msg.cluster_beacon_period);
	struct dect_phy_api_scheduler_list_item_config *sche_list_item_conf;
	struct dect_phy_api_scheduler_list_item *sche_list_item;

	beacon_interval_mdm_ticks = MS_TO_MODEM_TICKS(beacon_interval_ms);

	sche_list_item = dect_phy_api_scheduler_list_item_alloc_rx_element(&sche_list_item_conf);
	if (!sche_list_item) {
		desh_error("(%s): dect_phy_api_scheduler_list_item_alloc_rx_element failed",
			   (__func__));
		err = -ENOMEM;
		goto err_exit;
	}

	next_beacon_frame_start = beacon_received;
	while (next_beacon_frame_start < first_possible_rx) {
		next_beacon_frame_start += beacon_interval_mdm_ticks;
	}

	sche_list_item->phy_op_handle = params->phy_op_handle;
	sche_list_item->silent_fail = true;
	sche_list_item->priority = DECT_PRIORITY1_RX;

	sche_list_item_conf->rx.mode = NRF_MODEM_DECT_PHY_RX_MODE_CONTINUOUS;
	sche_list_item_conf->rx.network_id = current_settings->common.network_id;

	/* Set filter. Broadcast Beacons (with type 1) are always passing the filter. */
	sche_list_item_conf->rx.filter.is_short_network_id_used = true;
	sche_list_item_conf->rx.filter.short_network_id =
		(uint8_t)(current_settings->common.network_id & 0xFF);
	sche_list_item_conf->rx.filter.receiver_identity = current_settings->common.transmitter_id;

	/* Let there be some advance at both ends */
	sche_list_item_conf->frame_time =
		next_beacon_frame_start - (5 * DECT_RADIO_SLOT_DURATION_IN_MODEM_TICKS);
	sche_list_item_conf->rx.duration = 15 * DECT_RADIO_SLOT_DURATION_IN_MODEM_TICKS;
	sche_list_item_conf->length_slots = 0;
	sche_list_item_conf->length_subslots = 0;
	sche_list_item_conf->start_slot = 0;

	sche_list_item_conf->channel = params->target_nbr->channel;

	/* Note: this is not exactly compliant with the MAC spec (which requires for every beacon
	 * in release 1.x
	 */
	sche_list_item_conf->interval_mdm_ticks = beacon_interval_mdm_ticks * 10;

	sche_list_item_conf->cb_op_completed = dect_phy_mac_nbr_bg_scan_scheduler_op_completed_cb;
	sche_list_item_conf->cb_op_to_mdm = dect_phy_mac_nbr_bg_scan_scheduler_op_to_mdm_cb;
	sche_list_item_conf->cb_pdc_received = NULL;

	if (!dect_phy_api_scheduler_list_item_add(sche_list_item)) {
		desh_error("(%s): dect_phy_api_scheduler_list_item_add failed", (__func__));
		err = -EBUSY;
		dect_phy_api_scheduler_list_item_dealloc(sche_list_item);
	}
err_exit:
	return err;
}

int dect_phy_mac_nbr_bg_scan_start(struct dect_phy_mac_nbr_bg_scan_params *params)
{
	__ASSERT_NO_MSG(params && params->target_nbr);

	if (dect_mac_nbr_bg_scan_list_item_by_phy_op_handle_get(params->phy_op_handle) ||
	    dect_phy_api_scheduler_list_item_running_by_phy_op_handle(params->phy_op_handle)) {
		/* Already running */
		desh_warn("BG scan already running with phy op handle %u", params->phy_op_handle);
		return -EBUSY;
	}
	struct dect_phy_mac_nbr_bg_scan_data *free_bg_scan_data_slot =
		dect_mac_nbr_bg_scan_free_list_item_get();
	int err;

	if (!free_bg_scan_data_slot) {
		desh_error("No free BG scan data slot");
		return -ENOMEM;
	}
	err = dect_phy_mac_nbr_bg_scan_schedule(params);
	if (!err) {
		free_bg_scan_data_slot->running = true;
		free_bg_scan_data_slot->params = *params;
		free_bg_scan_data_slot->last_updated_rcv_time_mdm_ticks = 0;
		memset(&free_bg_scan_data_slot->metrics, 0,
		       sizeof(free_bg_scan_data_slot->metrics));
	}

	return err;
}

int dect_phy_mac_nbr_bg_scan_stop(uint32_t phy_op_handle)
{
	struct dect_phy_mac_nbr_bg_scan_data *bg_scan_data =
		dect_mac_nbr_bg_scan_list_item_by_phy_op_handle_get(phy_op_handle);

	if (!bg_scan_data) {
		desh_warn("BG scan item not found for phy op handle %u", phy_op_handle);
		return -EINVAL;
	}
	bg_scan_data->running = false;
	dect_phy_api_scheduler_list_item_remove_by_phy_op_handle(phy_op_handle);

	if (bg_scan_data->params.cb_op_completed) {
		struct dect_phy_mac_nbr_bg_scan_op_completed_info completed_info = {
			.cause = DECT_PHY_MAC_NBR_BG_SCAN_OP_COMPLETED_CAUSE_USER_INITIATED,
			.phy_op_handle = phy_op_handle,
			.target_long_rd_id = bg_scan_data->params.target_long_rd_id,
		};

		bg_scan_data->params.cb_op_completed(&completed_info);
	}
	return 0;
}

void dect_phy_mac_nbr_bg_scan_rcv_time_shift_update(uint32_t nbr_long_rd_id, uint64_t time_rcvd,
						    int64_t time_shift_mdm_ticks)
{
	/* Update frame_time due to possible time shift */
	struct dect_phy_mac_nbr_bg_scan_data *bg_scan_data =
		dect_mac_nbr_bg_scan_list_item_by_nbr_long_rd_id_get(nbr_long_rd_id);

	if (!bg_scan_data) {
		return;
	}

	if (bg_scan_data->last_updated_rcv_time_mdm_ticks < time_rcvd) {
		bg_scan_data->metrics.scan_info_updated_count++;
		bg_scan_data->last_updated_rcv_time_mdm_ticks = time_rcvd;

		if (time_shift_mdm_ticks == 0) {
			return;
		}
		dect_phy_api_scheduler_list_item_sched_config_frame_time_update_by_phy_op_handle(
			bg_scan_data->params.phy_op_handle, time_shift_mdm_ticks);
		bg_scan_data->metrics.scan_info_time_shift_updated_count++;
		bg_scan_data->metrics.scan_info_time_shift_last_value = time_shift_mdm_ticks;
	}
}

void dect_phy_mac_nbr_bg_scan_status_print_for_target_long_rd_id(uint32_t long_rd_id)
{
	struct dect_phy_mac_nbr_bg_scan_data *bg_scan_data =
		dect_mac_nbr_bg_scan_list_item_by_nbr_long_rd_id_get(long_rd_id);

	desh_print("   Background scan status:");

	if (!bg_scan_data) {
		desh_print("     Running: false\n");
		return;
	}
	uint64_t time_now = dect_app_modem_time_now();
	int64_t time_from_last_received_ms =
		MODEM_TICKS_TO_MS(time_now - bg_scan_data->last_updated_rcv_time_mdm_ticks);

	desh_print("     Running: true");
	desh_print("     Phy op handle:  %u", bg_scan_data->params.phy_op_handle);

	if (bg_scan_data->last_updated_rcv_time_mdm_ticks == 0) {
		desh_print("     Last seen: never");
	} else {
		desh_print("     Last seen:      %lld msecs ago",
				time_from_last_received_ms);
		desh_print("     Last timestamp: %llu mdm ticks",
				bg_scan_data->last_updated_rcv_time_mdm_ticks);
	}
	desh_print("     Metrics:");
	desh_print("       Scan info updated count:            %u",
		bg_scan_data->metrics.scan_info_updated_count);
	desh_print("       Scan started ok count:              %u",
		bg_scan_data->metrics.scan_started_ok_count);
	desh_print("       Scan start fail count:              %u",
		bg_scan_data->metrics.scan_start_fail_count);
	desh_print("       Scan info time shift updated count: %u",
		bg_scan_data->metrics.scan_info_time_shift_updated_count);
	desh_print("       Scan info time shift last value:    %lld",
		bg_scan_data->metrics.scan_info_time_shift_last_value);
}
