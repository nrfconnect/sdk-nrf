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
#include <nrf_modem_at.h>
#include <dk_buttons_and_leds.h>

#include "desh_print.h"

#include "dect_common.h"
#include "dect_common_utils.h"
#include "dect_common_settings.h"

#include "dect_phy_api_scheduler.h"
#include "dect_phy_ctrl.h"
#include "dect_phy_scan.h"
#include "dect_phy_rx.h"

#include "dect_phy_mac_cluster_beacon.h"

#include "dect_phy_mac_common.h"
#include "dect_phy_mac.h"
#include "dect_phy_mac_nbr.h"
#include "dect_phy_mac_client.h"
#include "dect_phy_mac_ctrl.h"

extern struct k_work_q dect_phy_ctrl_work_q;
extern struct k_sem rssi_scan_sema;

static struct dect_phy_mac_ctrl_data {
	bool beacon_tx_on_going;

	/* Callbacks from dect_phy_ctrl */
	struct dect_phy_ctrl_ext_callbacks ext_cmd;
} mac_data;

struct dect_phy_mac_ctrl_beacon_starter_data {
	struct k_work work;
	struct dect_phy_mac_beacon_start_params cmd_params;
};
static struct dect_phy_mac_ctrl_beacon_starter_data beacon_starter_work_data;

struct dect_phy_mac_ctrl_beacon_stopper_data {
	struct k_work work;
	enum dect_phy_mac_ctrl_beacon_stop_cause cause;
};
static struct dect_phy_mac_ctrl_beacon_stopper_data beacon_stopper_work_data;

static void dect_phy_mac_ctrl_beacon_start_work_handler(struct k_work *work_item)
{
	struct dect_phy_mac_ctrl_beacon_starter_data *data =
		CONTAINER_OF(work_item, struct dect_phy_mac_ctrl_beacon_starter_data, work);
	int ret = 0;
	uint16_t chosen_channel;
	char started_string[255];

	if (!dect_phy_ctrl_mdm_phy_api_initialized()) {
		desh_error("(%s) Phy api not initialized", (__func__));
		return;
	}

	struct dect_phy_settings *current_settings = dect_common_settings_ref_get();
	struct dect_phy_rssi_scan_params rssi_scan_params = {
		.channel = data->cmd_params.beacon_channel,
		.scan_time_ms = current_settings->rssi_scan.time_per_channel_ms,
		.busy_rssi_limit = current_settings->rssi_scan.busy_threshold,
		.free_rssi_limit = current_settings->rssi_scan.free_threshold,
		.stop_on_1st_free_channel = true,
		.dont_stop_on_this_channel = 0,
		.dont_stop_on_nbr_channels = true,
		.reinit_mdm_api = true,
		.only_allowed_channels = true,
	};
	struct dect_phy_rssi_channel_scan_result_t rssi_results;

	dect_phy_scan_rssi_results_get(&rssi_results); /* TODO: why is this? */
	ret = dect_phy_ctrl_rssi_scan_start(&rssi_scan_params, NULL);
	if (ret) {
		desh_error("Cannot start rssi scan, err %d", ret);
		return;
	}
	desh_print("RSSI scan started.");

	ret = k_sem_take(&rssi_scan_sema, K_SECONDS(120));
	if (ret) {
		desh_error("(%s): No response for RSSI scan or RSSI scan failed.", (__func__));
		return;
	}

	dect_phy_scan_rssi_results_get(&rssi_results);
	if (!data->cmd_params.ch_acc_busy_channel_use && !rssi_results.free_channels_count &&
	    !rssi_results.possible_channels_count) {
		desh_error("No free/possible channels to start a beacon.");
		return;
	}

	chosen_channel = data->cmd_params.beacon_channel;
	sprintf(started_string, "Channel %d was chosen for the beacon.", chosen_channel);

	/* FT/Beacon to select the first FREE channel for a beacon TX.
	 * If no FREE channels, select the first POSSIBLE channel
	 * if no FREE or POSSIBLE channels, then beacon cannot be started.
	 * Note: doesn't do per-subslot measurements like would be needed for ETSI TS 103 636-4.
	 */
	if (rssi_results.free_channels_count) {
		chosen_channel = rssi_results.free_channels[0];
		sprintf(started_string, "\"Free\" channel %d was chosen for the beacon.",
			chosen_channel);
	} else if (rssi_results.possible_channels_count) {
		chosen_channel = rssi_results.possible_channels[0];
		sprintf(started_string, "\"Possible\" channel %d was chosen for the beacon.",
			chosen_channel);
	} else {
		sprintf(started_string, "No \"free\" or \"possible\" channel available "
					"for starting a beacon");
		goto err_exit;
	}

	struct dect_phy_mac_beacon_start_params start_params = data->cmd_params;

	__ASSERT_NO_MSG(chosen_channel != 0);

	start_params.beacon_channel = chosen_channel;

	ret = dect_phy_mac_cluster_beacon_tx_start(&start_params);
	if (ret) {
		desh_error("Failed to start beacon TX: err %d", ret);
	} else {
		mac_data.beacon_tx_on_going = true;
		desh_print("%s", started_string);
		desh_print("Beacon TX started.");
#if defined(CONFIG_DK_LIBRARY)
		dk_set_led_on(DECT_BEACON_ON_STATUS_LED);
#endif
	}
	return;
err_exit:
	mac_data.beacon_tx_on_going = false;
	desh_print("%s", started_string);
	desh_print("Beacon TX couldn't be started.");
}

static void dect_phy_mac_ctrl_th_phy_api_direct_pdc_rx_cb(
	struct dect_phy_commmon_op_pdc_rcv_params *params);

int dect_phy_mac_ctrl_cluster_beacon_start(struct dect_phy_mac_beacon_start_params *params)
{
	int ret;

	if (mac_data.beacon_tx_on_going) {
		desh_error("Beacon already started");
		return -EALREADY;
	}

	mac_data.ext_cmd.direct_rssi_cb = dect_phy_mac_ctrl_cluster_beacon_phy_api_direct_rssi_cb;
	mac_data.ext_cmd.direct_pdc_rcv_cb = dect_phy_mac_ctrl_th_phy_api_direct_pdc_rx_cb;
	ret = dect_phy_ctrl_ext_command_start(mac_data.ext_cmd);
	if (ret) {
		return ret;
	}

	beacon_starter_work_data.cmd_params = *params;
	k_work_submit_to_queue(&dect_phy_ctrl_work_q, &beacon_starter_work_data.work);

	return 0;
}

/**************************************************************************************************/

const char *dect_phy_mac_ctrl_beacon_stop_cause_to_string(int type, char *out_str_buff)
{
	struct mapping_tbl_item const mapping_table[] = {
		{DECT_PHY_MAC_CTRL_BEACON_STOP_CAUSE_USER_INITIATED, "User Initiated"},
		{DECT_PHY_MAC_CTRL_BEACON_STOP_CAUSE_LMS_BEACON_TX,
			"LMS resulted BUSY on beacon tx"},
		{DECT_PHY_MAC_CTRL_BEACON_STOP_CAUSE_LMS_RACH, "LMS resulted BUSY on RACH"},
		{-1, NULL}};

	return dect_common_utils_map_to_string(mapping_table, type, out_str_buff);
}

static void dect_phy_mac_ctrl_beacon_stop_work_handler(struct k_work *work_item)
{
	struct dect_phy_mac_ctrl_beacon_stopper_data *data =
		CONTAINER_OF(work_item, struct dect_phy_mac_ctrl_beacon_stopper_data, work);
	char tmp_str[128] = {0};

	dect_phy_mac_cluster_beacon_tx_stop();
	mac_data.ext_cmd.direct_pdc_rcv_cb = NULL;
	dect_phy_ctrl_ext_command_stop();
	mac_data.beacon_tx_on_going = false;
#if defined(CONFIG_DK_LIBRARY)
	dk_set_led_off(DECT_BEACON_ON_STATUS_LED);
#endif
	desh_print("Beacon TX stopped, cause: %s.",
		dect_phy_mac_ctrl_beacon_stop_cause_to_string(data->cause, tmp_str));
}

void dect_phy_mac_ctrl_cluster_beacon_stop(enum dect_phy_mac_ctrl_beacon_stop_cause cause)
{
	beacon_stopper_work_data.cause = cause;
	k_work_submit_to_queue(&dect_phy_ctrl_work_q, &beacon_stopper_work_data.work);
}

/**************************************************************************************************/

int dect_phy_mac_ctrl_beacon_scan_start(struct dect_phy_mac_beacon_scan_params *params)
{
	int ret = 0;
	struct dect_phy_settings *current_settings = dect_common_settings_ref_get();
	struct dect_phy_rx_cmd_params rx_params = {
		.handle = DECT_PHY_COMMON_RX_CMD_HANDLE,
		.channel = params->channel,
		.network_id = 0,
		.duration_secs = params->duration_secs,
		.suspend_scheduler = params->suspend_scheduler,
		.expected_rssi_level = params->expected_rssi_level,
		.busy_rssi_limit = params->busy_rssi_limit,
		.free_rssi_limit = params->free_rssi_limit,
		.rssi_interval_secs = params->rssi_interval_secs,
	};

	/* Set filter. Broadcast Beacons (with type 1) are always passing filter. */
	rx_params.filter.is_short_network_id_used = true;
	rx_params.filter.short_network_id = (uint8_t)(current_settings->common.network_id & 0xFF);
	rx_params.filter.receiver_identity = current_settings->common.transmitter_id;

	ret = dect_phy_ctrl_rx_start(&rx_params);
	if (ret) {
		desh_error("Cannot start RX, err %d", ret);
	} else {
		desh_print("RX started.");
	}

	return ret;
}

/**************************************************************************************************/

int dect_phy_mac_ctrl_associate(struct dect_phy_mac_associate_params *params)
{
	struct dect_phy_mac_nbr_info_list_item *scan_info =
		dect_phy_mac_nbr_info_get_by_long_rd_id(params->target_long_rd_id);
	int ret;

	if (!scan_info) {
		desh_error("Beacon with long RD ID %u has not been seen in scan results",
			   params->target_long_rd_id);
		return -EINVAL;
	}

	ret = dect_phy_ctrl_ext_command_start(mac_data.ext_cmd);
	if (ret) {
		return ret;
	}

	ret = dect_phy_mac_client_associate(scan_info, params);
	if (ret) {
		desh_error("Cannot start client_associate: %d", ret);
	}

	return ret;
}

/**************************************************************************************************/

int dect_phy_mac_ctrl_dissociate(struct dect_phy_mac_associate_params *params)
{
	struct dect_phy_mac_nbr_info_list_item *scan_info =
		dect_phy_mac_nbr_info_get_by_long_rd_id(params->target_long_rd_id);
	int ret;

	if (!scan_info) {
		desh_error("Beacon with long RD ID %u has not been seen in scan results",
			   params->target_long_rd_id);
		return -EINVAL;
	}

	ret = dect_phy_ctrl_ext_command_start(mac_data.ext_cmd);
	if (ret) {
		return ret;
	}

	ret = dect_phy_mac_client_dissociate(scan_info, params);
	if (ret) {
		desh_error("Cannot start client_dissociate: %d", ret);
	}

	return ret;
}

/**************************************************************************************************/

int dect_phy_mac_ctrl_rach_tx(struct dect_phy_mac_rach_tx_params *params)
{
	struct dect_phy_mac_nbr_info_list_item *scan_info =
		dect_phy_mac_nbr_info_get_by_long_rd_id(params->target_long_rd_id);
	int ret;

	if (!scan_info) {
		desh_error("Beacon with long RD ID %u has not been seen in scan results",
			   params->target_long_rd_id);
		return -EINVAL;
	}

	ret = dect_phy_ctrl_ext_command_start(mac_data.ext_cmd);
	if (ret) {
		return ret;
	}

	ret = dect_phy_mac_client_rach_tx(scan_info, params);
	if (ret) {
		desh_error("Cannot start client_rach_tx: %d\n", ret);
	}

	return ret;
}

/**************************************************************************************************/

static void dect_phy_mac_ctrl_th_phy_api_direct_pdc_rx_cb(
	struct dect_phy_commmon_op_pdc_rcv_params *params)
{
	dect_phy_mac_direct_pdc_handle(params);
}

void dect_phy_mac_ctrl_th_phy_api_mdm_op_complete_cb(
	struct dect_phy_common_op_completed_params *params)
{
	if (params->status != NRF_MODEM_DECT_PHY_SUCCESS) {
		char tmp_str[128] = {0};

		dect_common_utils_modem_phy_err_to_string(params->status, params->temperature,
							  tmp_str);
		if (params->handle == DECT_PHY_MAC_BEACON_TX_HANDLE) {
			desh_error("%s: cannot TX beacon: %s", __func__, tmp_str);
		} else if (params->handle == DECT_PHY_MAC_CLIENT_RA_TX_HANDLE) {
			if (params->status == NRF_MODEM_DECT_PHY_ERR_LBT_CHANNEL_BUSY) {
				desh_warn("%s: cannot TX client data due to LBT, "
					  "channel was busy",
					  __func__, params->status);
			} else {
				desh_error("%s: cannot TX client data: %s", __func__, tmp_str);
			}
		} else {
			desh_error("%s: operation (handle %d) failed with status: %s", __func__,
				   params->handle, tmp_str);
		}
	} else {
		if (params->handle == DECT_PHY_MAC_CLIENT_RA_TX_HANDLE) {
			desh_print("Client data TX completed.");
		} else if (params->handle == DECT_PHY_MAC_CLIENT_ASSOCIATION_TX_HANDLE) {
			desh_print("TX for Association Request completed.");
		} else if (params->handle == DECT_PHY_MAC_CLIENT_ASSOCIATION_RX_HANDLE) {
			desh_print("RX for Association Response completed.");
		} else if (params->handle == DECT_PHY_MAC_BEACON_RA_RESP_TX_HANDLE) {
			desh_print("Beacon TX for Association Resp completed.");
		} else if (params->handle == DECT_PHY_MAC_CLIENT_ASSOCIATION_REL_TX_HANDLE) {
			desh_print("TX for Association Release completed.");
		}
	}

	if (params->handle == DECT_PHY_MAC_CLIENT_RA_TX_HANDLE ||
	    params->handle == DECT_PHY_MAC_CLIENT_ASSOCIATION_RX_HANDLE) {
		dect_phy_ctrl_ext_command_stop();
	}

	/* In any case, let's update SFN for a next round in a scheduler */
	if (params->handle == DECT_PHY_MAC_BEACON_TX_HANDLE) {
		dect_phy_mac_cluster_beacon_update();
	}
}

/**************************************************************************************************/

static int dect_phy_mac_ctrl_init(void)
{
	memset(&mac_data, 0, sizeof(struct dect_phy_mac_ctrl_data));

	/* Register for modem and other needed callbacks served by dect_phy_ctrl */
	mac_data.ext_cmd.direct_pcc_rcv_cb = NULL; /* No HARQ support */
	mac_data.ext_cmd.pcc_rcv_cb = NULL;
	mac_data.ext_cmd.direct_pdc_rcv_cb = NULL;
	mac_data.ext_cmd.pdc_rcv_cb = NULL;
	mac_data.ext_cmd.op_complete_cb = dect_phy_mac_ctrl_th_phy_api_mdm_op_complete_cb;
	mac_data.ext_cmd.sett_changed_cb = NULL;
	mac_data.ext_cmd.channel_has_nbr_cb = NULL; /* MAC got its own dedicated for this */

	k_work_init(&beacon_starter_work_data.work, dect_phy_mac_ctrl_beacon_start_work_handler);
	k_work_init(&beacon_stopper_work_data.work, dect_phy_mac_ctrl_beacon_stop_work_handler);

	return 0;
}

SYS_INIT(dect_phy_mac_ctrl_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
