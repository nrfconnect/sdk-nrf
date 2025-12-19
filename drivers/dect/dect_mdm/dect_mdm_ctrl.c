/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/sys/byteorder.h>

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <zephyr/shell/shell.h>

#include <modem/nrf_modem_lib.h>
#include <nrf_modem_at.h>

#include <nrf_errno.h>
#include <nrf_modem_dect.h>

#include <net/dect/dect_utils.h>
#include <net/dect/dect_net_l2_mgmt.h>

#include "dect_mdm_common.h"
#include "dect_mdm_utils.h"
#include "dect_mdm_settings.h"
#include "dect_mdm.h"
#include "dect_mdm_rx.h"
#include "dect_mdm_sink.h"
#include "dect_mdm_ctrl.h"
#include "dect_mdm_ctrl_internal.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(dect_mdm, CONFIG_DECT_MDM_LOG_LEVEL);

#include "net_private.h" /* For net_sprint_ipv6_addr */

K_SEM_DEFINE(dect_mdm_libmodem_api_sema, 0, 1);
K_SEM_DEFINE(dect_mdm_ctrl_reactivate_sema, 0, 1);

K_MUTEX_DEFINE(dect_mdm_ctrl_data_mtx);

/* Scan time constants (milliseconds) */
#define DECT_MDM_CTRL_FT_NW_SCAN_TIME_MS 2100
#define DECT_MDM_CTRL_PT_NW_SCAN_TIME_MS 2200

/* RSSI scan result selection constants */
#define DECT_MDM_CTRL_BEST_POSSIBLE_SUBSLOTS_INIT 10000U

/* States of all commands */
/* Control data instance - struct definition is in dect_mdm_ctrl_internal.h */
struct dect_mdm_ctrl_data ctrl_data;

/* Forward declarations for handler functions */
static void handle_mdm_capabilities(struct dect_mdm_common_op_event_msgq_item *event);
static void handle_mdm_configure_resp(struct dect_mdm_common_op_event_msgq_item *event);
static void handle_mdm_cfun_resp(struct dect_mdm_common_op_event_msgq_item *event);
static void handle_mdm_activated(struct dect_mdm_common_op_event_msgq_item *event);
static void handle_mdm_deactivated(struct dect_mdm_common_op_event_msgq_item *event);
static void handle_cluster_reconfig_req(struct dect_mdm_common_op_event_msgq_item *event);
static void
handle_cluster_ipv6_prefix_change_reconfig_req(struct dect_mdm_common_op_event_msgq_item *event);
static void handle_cluster_start_req(struct dect_mdm_common_op_event_msgq_item *event);
static void handle_cluster_config_resp(struct dect_mdm_common_op_event_msgq_item *event);
static void handle_mdm_ipv6_config_changed(struct dect_mdm_common_op_event_msgq_item *event);
static void handle_cluster_ch_load_changed(struct dect_mdm_common_op_event_msgq_item *event);
static void handle_neighbor_inactivity(struct dect_mdm_common_op_event_msgq_item *event);
static void handle_cluster_beacon_rx_failure(struct dect_mdm_common_op_event_msgq_item *event);
static void handle_neighbor_paging_failure(struct dect_mdm_common_op_event_msgq_item *event);
static void handle_mdm_flow_control(struct dect_mdm_common_op_event_msgq_item *event);
static void handle_mdm_association_ind(struct dect_mdm_common_op_event_msgq_item *event);
static void handle_mdm_association_resp(struct dect_mdm_common_op_event_msgq_item *event);
static void handle_mdm_association_release_resp(struct dect_mdm_common_op_event_msgq_item *event);
static void handle_mdm_association_release_ind(struct dect_mdm_common_op_event_msgq_item *event);
static void handle_auto_start(struct dect_mdm_common_op_event_msgq_item *event);
static void handle_rssi_start_req_cmd(struct dect_mdm_common_op_event_msgq_item *event);
static void handle_rssi_start_req_ch_selection(struct dect_mdm_common_op_event_msgq_item *event);
static void handle_mdm_rssi_result(struct dect_mdm_common_op_event_msgq_item *event);
static void handle_mdm_rssi_complete(struct dect_mdm_common_op_event_msgq_item *event);
static void handle_mdm_rssi_stopped(struct dect_mdm_common_op_event_msgq_item *event);
static void handle_mdm_nw_beacon_start(struct dect_mdm_common_op_event_msgq_item *event);
static void
handle_mdm_nw_beacon_start_or_stop_done(struct dect_mdm_common_op_event_msgq_item *event);
static void handle_mdm_nw_beacon_stop(struct dect_mdm_common_op_event_msgq_item *event);
static void handle_mdm_cluster_beacon_rcvd(struct dect_mdm_common_op_event_msgq_item *event);
static void handle_mdm_nw_beacon_rcvd(struct dect_mdm_common_op_event_msgq_item *event);
static void handle_mdm_nw_scan_complete(struct dect_mdm_common_op_event_msgq_item *event);
static void handle_mdm_nw_scan_stopped(struct dect_mdm_common_op_event_msgq_item *event);
static void handle_mdm_cluster_rcv_complete(struct dect_mdm_common_op_event_msgq_item *event);
static void handle_mdm_cluster_info(struct dect_mdm_common_op_event_msgq_item *event);
static void handle_mdm_neighbor_info(struct dect_mdm_common_op_event_msgq_item *event);
static void handle_mdm_neighbor_list(struct dect_mdm_common_op_event_msgq_item *event);
static void handle_mdm_dlc_data_resp(struct dect_mdm_common_op_event_msgq_item *event);

static void dect_mdm_ctrl_rssi_scan_data_init(bool actual_command)
{
	CTRL_DATA_LOCK();
	ctrl_data.rssi_scan_data.cmd_on_going = actual_command;
	memset(&ctrl_data.rssi_scan_data.results, 0, sizeof(ctrl_data.rssi_scan_data.results));
	ctrl_data.rssi_scan_data.current_results_index = 0;
	CTRL_DATA_UNLOCK();
}

/* Strategic getters for most commonly accessed single fields */

enum ctrl_mdm_activation_state dect_mdm_ctrl_get_mdm_activation_state(void)
{
	enum ctrl_mdm_activation_state state;

	CTRL_DATA_LOCK();
	state = ctrl_data.mdm_activation_state;
	CTRL_DATA_UNLOCK();

	return state;
}

enum ctrl_ft_cluster_state dect_mdm_ctrl_get_ft_cluster_state(void)
{
	enum ctrl_ft_cluster_state state;

	CTRL_DATA_LOCK();
	state = ctrl_data.ft_cluster_state;
	CTRL_DATA_UNLOCK();

	return state;
}

enum ctrl_ft_nw_beacon_state dect_mdm_ctrl_get_ft_nw_beacon_state(void)
{
	enum ctrl_ft_nw_beacon_state state;

	CTRL_DATA_LOCK();
	state = ctrl_data.ft_nw_beacon_state;
	CTRL_DATA_UNLOCK();

	return state;
}

enum ctrl_pt_association_state dect_mdm_ctrl_get_pt_association_state(void)
{
	enum ctrl_pt_association_state state;

	CTRL_DATA_LOCK();
	state = ctrl_data.ass_config.pt_association_state;
	CTRL_DATA_UNLOCK();

	return state;
}
/* Helper function to build RSSI scan parameters from settings */
static void
dect_mdm_ctrl_build_rssi_scan_params(struct nrf_modem_dect_mac_rssi_scan_params *params,
				       const struct dect_mdm_settings *settings,
				       uint16_t requested_channel, bool is_reconfig)
{
	params->channel_scan_length = settings->net_mgmt_common.rssi_scan.time_per_channel_ms / 10;
	params->threshold_min = settings->net_mgmt_common.rssi_scan.free_threshold_dbm;
	params->threshold_max = settings->net_mgmt_common.rssi_scan.busy_threshold_dbm;
	params->num_channels = 0;
	params->band = settings->net_mgmt_common.band_nbr;

	if (requested_channel != DECT_CLUSTER_CHANNEL_ANY) {
		params->num_channels = 1;
		params->channel_list[0] = requested_channel;
	}

	if (is_reconfig) {
		/* Only one frame RSSI measurement if reconfiguring */
		params->channel_scan_length = 1;
	}
}

static bool dect_mdm_ctrl_rssi_scan_data_results_best_get(
	struct dect_mdm_ctrl_rssi_scan_result_data *rssi_data_out)
{
	bool cont = false;
	const struct dect_mdm_ctrl_rssi_scan_result_data *current_best = NULL;
	int max_index;
	struct dect_mdm_ctrl_rssi_scan_result_data best_result;

	CTRL_DATA_LOCK();
	max_index = ctrl_data.rssi_scan_data.current_results_index;

	for (int i = 0; i < max_index; i++) {
		const struct dect_mdm_ctrl_rssi_scan_result_data *res =
			&ctrl_data.rssi_scan_data.results[i];

		if (res->channel == 0) {
			continue;
		}
		/* Min requirement */
		if (res->scan_suitable_percent_ok) {
			cont = true;
			break;
		}
	}
	if (!cont) {
		CTRL_DATA_UNLOCK();
		return false;
	}

	/* MAC spec, ch. 5.1.2:
	 * after measuring the supported channels,
	 * the RD should select an operating channel(s) or consecutive operating
	 * channels in the following manner:
	 */
	/* 1st: if any channel where all subslots are "free", is found:
	 * Note: we have some extra checks here to search the best of "all free"
	 */
	for (int i = 0; i < max_index; i++) {
		const struct dect_mdm_ctrl_rssi_scan_result_data *res =
			&ctrl_data.rssi_scan_data.results[i];

		if (res->all_subslots_free && res->busy_percentage == 0 &&
		    res->another_cluster_detected_in_channel == false) {
			current_best = res;
			goto exit;
		}
	}
	for (int i = 0; i < max_index; i++) {
		const struct dect_mdm_ctrl_rssi_scan_result_data *res =
			&ctrl_data.rssi_scan_data.results[i];

		if (res->all_subslots_free && res->busy_percentage_ok &&
		    res->another_cluster_detected_in_channel == false) {
			current_best = res;
			goto exit;
		}
	}
	for (int i = 0; i < max_index; i++) {
		const struct dect_mdm_ctrl_rssi_scan_result_data *res =
			&ctrl_data.rssi_scan_data.results[i];

		if (res->all_subslots_free && res->busy_percentage == 0) {
			current_best = res;
			goto exit;
		}
	}
	for (int i = 0; i < max_index; i++) {
		const struct dect_mdm_ctrl_rssi_scan_result_data *res =
			&ctrl_data.rssi_scan_data.results[i];

		if (res->all_subslots_free && res->another_cluster_detected_in_channel == false) {
			current_best = res;
			goto exit;
		}
	}
	for (int i = 0; i < max_index; i++) {
		const struct dect_mdm_ctrl_rssi_scan_result_data *res =
			&ctrl_data.rssi_scan_data.results[i];

		if (res->all_subslots_free) {
			current_best = res;
			goto exit;
		}
	}

	/* 2nd: select the channel that has the lowest number of "busy" subslots */
	uint8_t min_busy_subslots = UINT8_MAX;

	for (int i = 0; i < max_index; i++) {
		const struct dect_mdm_ctrl_rssi_scan_result_data *res =
			&ctrl_data.rssi_scan_data.results[i];

		if (res->busy_subslot_cnt < min_busy_subslots) {
			min_busy_subslots = res->busy_subslot_cnt;
			current_best = res;
		}
	}
	uint16_t same_best_busy_subslots_count = 0;
	uint16_t best_possible_subslots_count = DECT_MDM_CTRL_BEST_POSSIBLE_SUBSLOTS_INIT;

	for (int i = 0; i < max_index; i++) {
		const struct dect_mdm_ctrl_rssi_scan_result_data *res =
			&ctrl_data.rssi_scan_data.results[i];

		if (res->busy_subslot_cnt == min_busy_subslots) {
			same_best_busy_subslots_count++;
		}
	}
	/* if multiple channels or consecutive channels have the same number of "busy" subslots: */
	if (same_best_busy_subslots_count > 1) {
		/* 2b: then select the channel that has the lowest number of "possible" subslots */
		for (int i = 0; i < max_index; i++) {
			const struct dect_mdm_ctrl_rssi_scan_result_data *res =
				&ctrl_data.rssi_scan_data.results[i];

			if (res->busy_subslot_cnt == min_busy_subslots) {
				if (res->possible_subslot_cnt < best_possible_subslots_count) {
					best_possible_subslots_count = res->possible_subslot_cnt;
					current_best = res;
				}
			}
		}
	}
exit:
	if (current_best != NULL) {
		best_result = *current_best;
		CTRL_DATA_UNLOCK();
		*rssi_data_out = best_result;
		return true;
	}
	CTRL_DATA_UNLOCK();
	return false;
}
static bool dect_mdm_ctrl_cluster_channels_list_channel_exists(uint16_t channel)
{
	bool found = false;

	CTRL_DATA_LOCK();
	for (int i = 0; i < DECT_MDM_CTRL_CLUSTER_SCAN_DATA_MAX_CHANNELS; i++) {
		if (ctrl_data.scan_data.cluster_channels[i].channel == channel) {
			found = true;
			break;
		}
	}
	CTRL_DATA_UNLOCK();
	return found;
}

static bool dect_mdm_ctrl_cluster_channels_list_best_channel_get(
	struct dect_mdm_ctrl_cluster_channel_list_item *best_channel_data_out)
{
	CTRL_DATA_LOCK();
	if (ctrl_data.scan_data.current_cluster_channel_index == 0 &&
	    ctrl_data.scan_data.cluster_channels[0].channel == 0) {
		CTRL_DATA_UNLOCK();
		LOG_WRN("No cluster channels stored");
		return false;
	}
	int8_t best_rssi = ctrl_data.scan_data.cluster_channels[0].rssi_2;
	uint16_t best_channel = ctrl_data.scan_data.cluster_channels[0].channel;
	uint32_t best_long_rd_id = ctrl_data.scan_data.cluster_channels[0].long_rd_id;

	for (int i = 1; i < ctrl_data.scan_data.current_cluster_channel_index; i++) {
		if (ctrl_data.scan_data.cluster_channels[i].rssi_2 > best_rssi) {
			best_rssi = ctrl_data.scan_data.cluster_channels[i].rssi_2;
			best_channel = ctrl_data.scan_data.cluster_channels[i].channel;
			best_long_rd_id = ctrl_data.scan_data.cluster_channels[i].long_rd_id;
		}
	}
	CTRL_DATA_UNLOCK();
	best_channel_data_out->rssi_2 = best_rssi;
	best_channel_data_out->channel = best_channel;
	best_channel_data_out->long_rd_id = best_long_rd_id;
	LOG_DBG("Best channel found: %d (RSSI: %d, long rd id %u)", best_channel, best_rssi,
		best_long_rd_id);
	return true;
}
K_MSGQ_DEFINE(dect_mdm_ctrl_msgq, sizeof(struct dect_mdm_common_op_event_msgq_item),
	1000, 4);

int dect_mdm_ctrl_internal_msgq_non_data_op_add(enum dect_mdm_ctrl_op event_id)
{
	int ret = 0;
	struct dect_mdm_common_op_event_msgq_item event;

	event.id = event_id;
	event.data = NULL;
	ret = k_msgq_put(&dect_mdm_ctrl_msgq, &event, K_NO_WAIT);
	if (ret) {
		/* event.data is NULL for non-data ops, no need to free */
		return -ENOBUFS;
	}
	return 0;
}

int dect_mdm_ctrl_internal_msgq_data_op_add(enum dect_mdm_ctrl_op event_id, void *data,
					    size_t data_size)
{
	int ret = 0;
	struct dect_mdm_common_op_event_msgq_item event;

	event.data = k_malloc(data_size);
	if (event.data == NULL) {
		return -ENOMEM;
	}
	memcpy(event.data, data, data_size);
	event.id = event_id;

	ret = k_msgq_put(&dect_mdm_ctrl_msgq, &event, K_NO_WAIT);
	if (ret) {
		k_free(event.data);
		return -ENOBUFS;
	}
	return 0;
}
struct dect_mdm_ctrl_data *dect_mdm_ctrl_internal_data_get(void)
{
	return &ctrl_data;
}
static int dect_mdm_ctrl_configure_cmd(struct dect_mdm_ctrl_configure_params *params)
{
	struct dect_mdm_settings *set_ptr = dect_mdm_settings_ref_get();

	CTRL_DATA_LOCK();
	ctrl_data.configure_params = *params;
	CTRL_DATA_UNLOCK();

	struct nrf_modem_dect_control_configure_params config = {
		.max_tx_power = dect_utils_lib_dbm_to_phy_tx_power(params->tx_pwr),
		.expected_mcs1_rx_rssi_level = CONFIG_DECT_MDM_NRF_RX_MCS1_EXPECTED_RSSI_LEVEL,
		.max_mcs = params->tx_mcs,
		.long_rd_id = params->long_rd_id,
		.phy_band_group_index = params->band_group_index,
		.power_save = params->power_save,
		.security.mode = set_ptr->net_mgmt_common.sec_conf.mode,
		.stats_averaging_length = 2,
	};
	int err;

	if (set_ptr->net_mgmt_common.sec_conf.mode != DECT_SECURITY_MODE_NONE) {
		memcpy(config.security.integrity_key,
		       set_ptr->net_mgmt_common.sec_conf.integrity_key,
		       DECT_INTEGRITY_KEY_LENGTH);
		memcpy(config.security.cipher_key, set_ptr->net_mgmt_common.sec_conf.cipher_key,
		       DECT_CIPHER_KEY_LENGTH);
	}
	err = nrf_modem_dect_control_configure(&config);
	if (err) {
		LOG_ERR("nrf_modem_dect_control_configure() failed, error: %d", err);
	}
	return err;
}

int dect_mdm_ctrl_internal_modem_configure_req_from_settings(void)
{
	struct dect_mdm_ctrl_configure_params params;
	struct dect_mdm_settings *set_ptr = dect_mdm_settings_ref_get();
	int ret;

	if (dect_mdm_ctrl_get_mdm_activation_state() != CTRL_MDM_DEACTIVATED) {
		LOG_ERR("%s: Modem is not deactivated, cannot configure", (__func__));
		return -EINVAL;
	}

	params.long_rd_id = set_ptr->net_mgmt_common.identities.transmitter_long_rd_id;
	params.network_id = set_ptr->net_mgmt_common.identities.network_id;
	params.band = set_ptr->net_mgmt_common.band_nbr;
	params.band_group_index = 0;
	if (params.band == 4) {
		params.band_group_index = 1;
	}

	params.debug = false;

	params.tx_mcs = set_ptr->net_mgmt_common.tx.max_mcs;
	params.auto_start = false;
	params.auto_activate = set_ptr->net_mgmt_common.auto_start.activate;
	params.channel = 0;
	params.power_save = set_ptr->net_mgmt_common.power_save;
	params.tx_pwr = set_ptr->net_mgmt_common.tx.max_power_dbm;

	CTRL_DATA_LOCK();
	/* Init reconfigure params */
	ctrl_data.ft_cluster_reconfig_params.channel = 0;
	ctrl_data.ft_cluster_reconfig_params.max_beacon_tx_power_dbm =
		set_ptr->net_mgmt_common.cluster.max_beacon_tx_power_dbm;
	ctrl_data.ft_cluster_reconfig_params.max_cluster_power_dbm =
		set_ptr->net_mgmt_common.cluster.max_cluster_power_dbm;
	ctrl_data.ft_cluster_reconfig_params.period =
		set_ptr->net_mgmt_common.cluster.beacon_period;
	CTRL_DATA_UNLOCK();

	ret = dect_mdm_ctrl_configure_cmd(&params);
	if (ret != 0) {
		LOG_ERR("Error in configure: %d", ret);
	}
	return ret;
}
static int dect_mdm_ctrl_mdm_activate_req(void)
{
	/* Activate modem by setting full functional mode */
	char tmp_str[DECT_MDM_UTILS_STR_BUFF_SIZE] = {0};
	int err = nrf_modem_dect_control_functional_mode_set(
		NRF_MODEM_DECT_CONTROL_FUNCTIONAL_MODE_ACTIVATE);

	if (err) {
		dect_mdm_utils_modem_mac_err_to_string(err, tmp_str, sizeof(tmp_str));
		LOG_ERR("%s: error in CFUN set: err %s (%d)", (__func__), tmp_str, err);
	}
	return err;
}

int dect_mdm_ctrl_internal_mdm_deactivate_req(void)
{
	/* Deactivate modem by setting min functional mode */
	char tmp_str[DECT_MDM_UTILS_STR_BUFF_SIZE] = {0};
	int err = nrf_modem_dect_control_functional_mode_set(
		NRF_MODEM_DECT_CONTROL_FUNCTIONAL_MODE_DEACTIVATE);

	if (err) {
		dect_mdm_utils_modem_mac_err_to_string(err, tmp_str, sizeof(tmp_str));
		LOG_ERR("%s: error in CFUN set: err %s (%d)", (__func__), tmp_str, err);
	}
	return err;
}

bool dect_mdm_ctrl_internal_nw_beacon_common_can_be_started(uint16_t channel)
{
	struct dect_mdm_settings *set_ptr = dect_mdm_settings_ref_get();

	if (set_ptr->net_mgmt_common.device_type & DECT_DEVICE_TYPE_PT) {
		LOG_ERR("Network beacon can not be started for PT");
		return false;
	}

	if (dect_common_utils_channel_is_supported_by_band(set_ptr->net_mgmt_common.band_nbr,
							   channel) == false) {
		LOG_ERR("Channel %d not supported by band %d", channel,
			set_ptr->net_mgmt_common.band_nbr);
		return false;
	}

	return true;
}

static void dect_mdm_ctrl_trigger_association(void)
{
	CTRL_DATA_LOCK();
	uint32_t network_id = ctrl_data.ass_config.network_id;
	uint32_t parent_long_rd_id = ctrl_data.ass_config.parent_long_rd_id;

	CTRL_DATA_UNLOCK();

	LOG_INF("Association triggered towards:");
	LOG_INF("  network id (32bit).............................%u (0x%08x)", network_id,
		network_id);
	LOG_INF("  transmitter id (long RD ID)....................%u (0x%08x)", parent_long_rd_id,
		parent_long_rd_id);

	struct nrf_modem_dect_mac_tx_flow_config flow_config[1] = {{
		.flow_id = 1,
		.dlc_service_type = NRF_MODEM_DECT_DLC_SERVICE_TYPE_3,
		.dlc_sdu_lifetime = NRF_MODEM_DECT_DLC_SDU_LIFETIME_60_S,
	}};
	struct dect_mdm_settings *set_ptr = dect_mdm_settings_ref_get();
	struct nrf_modem_dect_mac_association_params params = {
		.long_rd_id = parent_long_rd_id,
		.network_id = network_id,
		.num_flows = 1,
		.tx_flow_configs = flow_config,
		.info_triggers.num_beacon_rx_failures =
			set_ptr->net_mgmt_common.association.max_beacon_rx_failures,
	};
	int err = nrf_modem_dect_mac_association(&params);

	if (err) {
		LOG_ERR("nrf_modem_dect_mac_association failed: err %d", err);
	}
}

static void handle_mdm_capabilities(struct dect_mdm_common_op_event_msgq_item *event)
{
	struct nrf_modem_dect_mac_capability_ntf_cb_params *evt_data =
		(struct nrf_modem_dect_mac_capability_ntf_cb_params *)event->data;

	LOG_INF("Modem Capabilities:");
	LOG_INF("  max_mcs: %hhu", evt_data->max_mcs);
	for (int i = 0; i < evt_data->num_band_info_elems; i++) {
		LOG_INF("    band[%hhu]:                   %hhu", i,
			evt_data->band_info_elems[i].band);
		LOG_DBG("    band_group_index[%hhu]:       %hhu", i,
			evt_data->band_info_elems[i].band_group_index);
		LOG_INF("    power_class[%hhu]:            %hhu", i,
			evt_data->band_info_elems[i].power_class);
		LOG_INF("    min_carrier[%hhu]:            %hu", i,
			evt_data->band_info_elems[i].min_carrier);
		LOG_INF("    max_carrier[%hhu]:            %hu", i,
			evt_data->band_info_elems[i].max_carrier);
	}
	/* Store capabilities */
	CTRL_DATA_LOCK();
	ctrl_data.mdm_capas = *evt_data;
	CTRL_DATA_UNLOCK();
}

static void handle_mdm_configure_resp(struct dect_mdm_common_op_event_msgq_item *event)
{
	char tmp_str[DECT_MDM_UTILS_STR_BUFF_SIZE] = {0};
	enum nrf_modem_dect_mac_err *status = (enum nrf_modem_dect_mac_err *)event->data;

	if (*status != NRF_MODEM_DECT_MAC_STATUS_OK) {
		dect_mdm_utils_modem_mac_err_to_string(*status, tmp_str, sizeof(tmp_str));

		LOG_ERR("Error in configure: err %s (%d)", tmp_str, *status);
		return;
	}
	k_sem_give(&dect_mdm_libmodem_api_sema);

	enum ctrl_mdm_activation_state activation_state = dect_mdm_ctrl_get_mdm_activation_state();
	bool should_activate = (activation_state == CTRL_MDM_ACTIVATE_REQ ||
				activation_state == CTRL_MDM_REACTIVATING_CONFIGURE);

	if (!should_activate) {
		/* Check auto_activate if state doesn't require activation */
		CTRL_DATA_LOCK();
		should_activate = ctrl_data.configure_params.auto_activate;
		CTRL_DATA_UNLOCK();
	}

	if (should_activate) {
		LOG_DBG("Modem configured: "
			"activating to full functional mode");

		if (dect_mdm_ctrl_mdm_activate_req()) {
			CTRL_DATA_LOCK();
			ctrl_data.mdm_activation_state = CTRL_MDM_DEACTIVATED;
			CTRL_DATA_UNLOCK();
			LOG_ERR("Error in initiating modem activation");
		}
	} else {
		CTRL_DATA_LOCK();
		LOG_INF("Modem configured without auto start - deactivated");
		ctrl_data.mdm_activation_state = CTRL_MDM_DEACTIVATED;
		CTRL_DATA_UNLOCK();
	}
}

static void handle_mdm_cfun_resp(struct dect_mdm_common_op_event_msgq_item *event)
{
	char tmp_str[DECT_MDM_UTILS_STR_BUFF_SIZE] = {0};
	enum nrf_modem_dect_mac_err *status = (enum nrf_modem_dect_mac_err *)event->data;

	CTRL_DATA_LOCK();
	if (*status != NRF_MODEM_DECT_MAC_STATUS_OK) {
		/* Operation wasn't successful -> no change to activation state */
		enum ctrl_mdm_activation_state activation_state = ctrl_data.mdm_activation_state;
		struct net_if *iface = ctrl_data.iface;

		CTRL_DATA_UNLOCK();
		dect_mdm_utils_modem_mac_err_to_string(*status, tmp_str, sizeof(tmp_str));
		if (activation_state == CTRL_MDM_ACTIVATE_REQ) {
			dect_mgmt_activate_done_evt(
				iface, dect_mdm_utils_modem_status_to_net_mgmt_status(*status));

		} else {
			__ASSERT_NO_MSG(activation_state == CTRL_MDM_DEACTIVATE_REQ ||
					activation_state == CTRL_MDM_DEACTIVATED);
			dect_mgmt_deactivate_done_evt(
				iface, dect_mdm_utils_modem_status_to_net_mgmt_status(*status));
		}
		LOG_ERR("Error in activate/CFUN req: err %s (%d)", tmp_str, *status);
		return;
	}

	if ((ctrl_data.configure_params.auto_activate &&
	     (ctrl_data.mdm_activation_state != CTRL_MDM_DEACTIVATE_REQ &&
	      ctrl_data.mdm_activation_state != CTRL_MDM_REACTIVATING_DEACTIVATE)) ||
	    ctrl_data.mdm_activation_state == CTRL_MDM_ACTIVATE_REQ ||
	    ctrl_data.mdm_activation_state == CTRL_MDM_REACTIVATING_CONFIGURE) {
		CTRL_DATA_UNLOCK();
		dect_mdm_ctrl_internal_msgq_non_data_op_add(DECT_MDM_CTRL_OP_MDM_ACTIVATED);
	} else {
		__ASSERT_NO_MSG(ctrl_data.mdm_activation_state == CTRL_MDM_DEACTIVATE_REQ ||
				ctrl_data.mdm_activation_state == CTRL_MDM_DEACTIVATED ||
				ctrl_data.mdm_activation_state == CTRL_MDM_REACTIVATING_DEACTIVATE);
		CTRL_DATA_UNLOCK();
		dect_mdm_ctrl_internal_msgq_non_data_op_add(DECT_MDM_CTRL_OP_MDM_DEACTIVATED);
	}
}

static void handle_mdm_activated(struct dect_mdm_common_op_event_msgq_item *event)
{
	ARG_UNUSED(event);

	CTRL_DATA_LOCK();
	bool reactivating = (ctrl_data.mdm_activation_state == CTRL_MDM_REACTIVATING_CONFIGURE);
	struct net_if *iface = ctrl_data.iface;

	if (!reactivating) {
		CTRL_DATA_UNLOCK();
		net_if_carrier_on(iface);
		dect_mgmt_activate_done_evt(iface, DECT_STATUS_OK);
		CTRL_DATA_LOCK();
	}
	ctrl_data.mdm_activation_state = CTRL_MDM_ACTIVATED;
	ctrl_data.tx_mdm_flow_ctrl_on = false;
#if defined(CONFIG_DECT_MDM_NRF_TX_FLOW_CTRL_BASED_ON_MDM_TX_DLC_REQS)
	ctrl_data.total_unacked_tx_data_amount = 0;
	memset(ctrl_data.dlc_data_tx_infos, 0, sizeof(ctrl_data.dlc_data_tx_infos));
#endif
	CTRL_DATA_UNLOCK();
	k_sem_give(&dect_mdm_ctrl_reactivate_sema);
	LOG_INF("Modem activated - ready for commands");
}

static void handle_mdm_deactivated(struct dect_mdm_common_op_event_msgq_item *event)
{
	ARG_UNUSED(event);
	bool reactivate;
	struct net_if *iface;
	uint32_t parent_long_rd_id;

	CTRL_DATA_LOCK();
	reactivate =
		(ctrl_data.mdm_activation_state == CTRL_MDM_REACTIVATING_DEACTIVATE) ? true : false;

	ctrl_data.ft_nw_beacon_state = CTRL_FT_NW_BEACON_STATE_NONE;
	ctrl_data.configure_params.channel = 0;
	if (ctrl_data.ft_network_state != CTRL_FT_NETWORK_STATE_NONE ||
	    ctrl_data.ft_cluster_state != CTRL_FT_CLUSTER_STATE_NONE) {
		if (ctrl_data.mdm_activation_state != CTRL_MDM_DEACTIVATE_REQ) {
			/* If not deactivated by user, then we need to
			 * reactivate the modem to be able to remove network,
			 */
			reactivate = true;
		}
		iface = ctrl_data.iface;
		CTRL_DATA_UNLOCK();

		struct dect_network_status_evt network_status_data = {
			.network_status = DECT_NETWORK_STATUS_REMOVED,
		};
		dect_mdm_child_association_all_removed(
			NRF_MODEM_DECT_MAC_RELEASE_CAUSE_OTHER_REASON);
		dect_mgmt_network_status_evt(iface, network_status_data);

		CTRL_DATA_LOCK();
	} else if (ctrl_data.ass_config.pt_association_state ==
		   CTRL_PT_ASSOCIATION_STATE_ASSOCIATED) {
		parent_long_rd_id = ctrl_data.ass_config.parent_long_rd_id;
		CTRL_DATA_UNLOCK();

		dect_mdm_parent_association_removed(
			parent_long_rd_id, NRF_MODEM_DECT_MAC_RELEASE_CAUSE_OTHER_REASON, false);

		CTRL_DATA_LOCK();
	}
	ctrl_data.mdm_activation_state = CTRL_MDM_DEACTIVATED;
	ctrl_data.ft_cluster_state = CTRL_FT_CLUSTER_STATE_NONE;
	ctrl_data.ft_network_state = CTRL_FT_NETWORK_STATE_NONE;
	ctrl_data.ft_requested_cluster_channel = DECT_CLUSTER_CHANNEL_ANY;
	ctrl_data.ft_cluster_reconfig_params.channel = DECT_CLUSTER_CHANNEL_ANY;
	ctrl_data.ft_cluster_reconfig_ongoing = false;
	ctrl_data.configure_params.auto_start = false;
	ctrl_data.ass_config.pt_association_state = CTRL_PT_ASSOCIATION_STATE_NONE;

	if (reactivate) {
		iface = ctrl_data.iface;
		CTRL_DATA_UNLOCK();
		LOG_DBG("Deactivated. Next configure before reactivate");
		/* Reactivate modem to be able to give commands still */
		if (dect_mdm_ctrl_api_mdm_configure_n_activate()) {
			LOG_ERR("Error in initiating modem configure and activate");
			CTRL_DATA_LOCK();
		} else {
			CTRL_DATA_LOCK();
			ctrl_data.mdm_activation_state = CTRL_MDM_REACTIVATING_CONFIGURE;
			CTRL_DATA_UNLOCK();
			/* we do not want to send deactivate evt when reactivating */
			return;
		}
	}
	iface = ctrl_data.iface;
	CTRL_DATA_UNLOCK();
	LOG_INF("Modem state: deactivated");
	net_if_carrier_off(iface);
	dect_mgmt_deactivate_done_evt(iface, DECT_STATUS_OK);
}

static void handle_cluster_reconfig_req(struct dect_mdm_common_op_event_msgq_item *event)
{
	struct dect_cluster_reconfig_req_params *params =
		(struct dect_cluster_reconfig_req_params *)event->data;

	CTRL_DATA_LOCK();
	ctrl_data.ft_cluster_state = CTRL_FT_CLUSTER_STATE_STARTING;

	ctrl_data.ft_requested_cluster_channel = params->channel;
	ctrl_data.ft_cluster_reconfig_params = *params;

	ctrl_data.ft_cluster_reconfig_ongoing = true;

	/* Store current channel in case of cluster_reconfig failure */
	ctrl_data.ft_cluster_reconfig_prev_cluster_channel = ctrl_data.configure_params.channel;

	ctrl_data.configure_params.channel = 0;
	CTRL_DATA_UNLOCK();

	dect_mdm_ctrl_internal_msgq_non_data_op_add(DECT_MDM_CTRL_OP_AUTO_START);
}

static void
handle_cluster_ipv6_prefix_change_reconfig_req(struct dect_mdm_common_op_event_msgq_item *event)
{
	ARG_UNUSED(event);

	CTRL_DATA_LOCK();
	ctrl_data.ft_cluster_state = CTRL_FT_CLUSTER_STATE_STARTING;

	/* Keep current channel, just reconfig ipv6 cfg  */
	ctrl_data.ft_requested_cluster_channel = ctrl_data.configure_params.channel;
	ctrl_data.ft_cluster_reconfig_prev_cluster_channel = ctrl_data.configure_params.channel;

	ctrl_data.ft_cluster_reconfig_ongoing = true;
	CTRL_DATA_UNLOCK();

	dect_mdm_ctrl_internal_msgq_non_data_op_add(DECT_MDM_CTRL_OP_AUTO_START);
}

static void handle_cluster_start_req(struct dect_mdm_common_op_event_msgq_item *event)
{
	struct dect_cluster_start_req_params *params =
		(struct dect_cluster_start_req_params *)event->data;

	CTRL_DATA_LOCK();
	ctrl_data.ft_cluster_state = CTRL_FT_CLUSTER_STATE_STARTING;
	ctrl_data.ft_requested_cluster_channel = params->channel;
	CTRL_DATA_UNLOCK();

	dect_mdm_ctrl_internal_msgq_non_data_op_add(DECT_MDM_CTRL_OP_AUTO_START);
}

static void handle_cluster_config_resp(struct dect_mdm_common_op_event_msgq_item *event)
{
	char tmp_str[DECT_MDM_UTILS_STR_BUFF_SIZE] = {0};
	struct nrf_modem_dect_mac_cluster_configure_cb_params *params = event->data;
	struct dect_mdm_settings *set_ptr = dect_mdm_settings_ref_get();
	struct dect_cluster_start_resp_evt resp_evt = {
		.status = params->status,
		.cluster_channel = 0,
	};
	struct dect_network_status_evt network_status_data;
	bool reconfig_ongoing;
	enum ctrl_ft_cluster_state cluster_state;
	struct net_if *iface;
	uint16_t cluster_channel;

	CTRL_DATA_LOCK();
	ctrl_data.ft_requested_cluster_channel = DECT_CLUSTER_CHANNEL_ANY;

	if (params->status != NRF_MODEM_DECT_MAC_STATUS_OK) {
		reconfig_ongoing = ctrl_data.ft_cluster_reconfig_ongoing;
		cluster_state = ctrl_data.ft_cluster_state;
		if (reconfig_ongoing && cluster_state == CTRL_FT_CLUSTER_STATE_STARTING &&
			ctrl_data.ft_cluster_reconfig_prev_cluster_channel != 0) {
			/* Reconfig failure, existing cluster config
			 * still running
			 */
			ctrl_data.ft_cluster_state = CTRL_FT_CLUSTER_STATE_STARTED;
			ctrl_data.ft_cluster_reconfig_ongoing = false;
			ctrl_data.ft_requested_cluster_channel = DECT_CLUSTER_CHANNEL_ANY;
			ctrl_data.configure_params.channel =
				ctrl_data.ft_cluster_reconfig_prev_cluster_channel;
			CTRL_DATA_UNLOCK();
			LOG_WRN("Reconfig failure, existing cluster config still running");
			return;
		}

		network_status_data.network_status = DECT_NETWORK_STATUS_FAILURE;
		network_status_data.dect_err_cause =
			dect_mdm_utils_modem_status_to_net_mgmt_status(params->status);

		dect_mdm_utils_modem_mac_err_to_string(
			params->status, tmp_str, sizeof(tmp_str));
		LOG_ERR("Error in Cluster config: err %s (%d)", tmp_str, params->status);

		ctrl_data.ft_cluster_state = CTRL_FT_CLUSTER_STATE_NONE;
		ctrl_data.ft_network_state = CTRL_FT_NETWORK_STATE_NONE;
		iface = ctrl_data.iface;
		CTRL_DATA_UNLOCK();
		dect_mgmt_network_status_evt(iface, network_status_data);
		goto send_events;
	}
	LOG_INF("Cluster configured");

	__ASSERT_NO_MSG(set_ptr->net_mgmt_common.device_type & DECT_DEVICE_TYPE_FT);

	/* Lock is still held from line 906 */
	ctrl_data.ft_cluster_state = CTRL_FT_CLUSTER_STATE_STARTED;
	cluster_channel = ctrl_data.configure_params.channel;
	resp_evt.cluster_channel = cluster_channel;
	ctrl_data.ft_cluster_reconfig_ongoing = false;
	iface = ctrl_data.iface;
	ctrl_data.configure_params.auto_start = false;
	CTRL_DATA_UNLOCK();
send_events:
	dect_mgmt_cluster_created_evt(iface, resp_evt);
	CTRL_DATA_LOCK();
	if (ctrl_data.ft_network_state == CTRL_FT_NETWORK_STATE_STARTING) {
		CTRL_DATA_UNLOCK();
		if (set_ptr->net_mgmt_common.nw_beacon.channel !=
		    DECT_NW_BEACON_CHANNEL_NOT_USED) {
			/* Network beacon start if configured */
			struct dect_nw_beacon_start_req_params beacon_params = {
				.channel = set_ptr->net_mgmt_common.nw_beacon.channel,
				.additional_ch_count = 0,
			};

			dect_mdm_ctrl_internal_msgq_data_op_add(
				DECT_MDM_CTRL_OP_MDM_NW_BEACON_START, &beacon_params,
				sizeof(struct dect_nw_beacon_start_req_params));
		} else {
			CTRL_DATA_LOCK();
			ctrl_data.ft_network_state = CTRL_FT_NETWORK_STATE_CREATED;
			iface = ctrl_data.iface;
			CTRL_DATA_UNLOCK();
			LOG_INF("Network created - network beacon not configured");
			network_status_data.network_status = DECT_NETWORK_STATUS_CREATED;
			dect_mgmt_network_status_evt(iface, network_status_data);
		}
	} else {
		CTRL_DATA_UNLOCK();
	}
}

static void handle_mdm_ipv6_config_changed(struct dect_mdm_common_op_event_msgq_item *event)
{
	struct nrf_modem_dect_mac_ipv6_config_update_ntf_cb_params *params = event->data;
	enum ctrl_pt_association_state pt_assoc_state;

	LOG_WRN("IPv6 config changed, type: %d", params->ipv6_config.type);
	CTRL_DATA_LOCK();
	pt_assoc_state = ctrl_data.ass_config.pt_association_state;
	CTRL_DATA_UNLOCK();

	if (pt_assoc_state == CTRL_PT_ASSOCIATION_STATE_ASSOCIATED) {
		dect_mdm_parent_association_ipv6_config_changed(params->ipv6_config);
	} else {
		LOG_WRN("IPv6 config changed. Not associated - ignored ");
	}
}

static void handle_cluster_ch_load_changed(struct dect_mdm_common_op_event_msgq_item *event)
{
	struct nrf_modem_dect_mac_cluster_ch_load_change_ntf_cb_params *evt_data =
		(struct nrf_modem_dect_mac_cluster_ch_load_change_ntf_cb_params *)event->data;
	struct dect_mdm_settings *set_ptr = dect_mdm_settings_ref_get();
	struct nrf_modem_dect_mac_rssi_result *mdm_rssi_res = &evt_data->rssi_result;
	struct dect_rssi_scan_result_data rssi_data;

	LOG_INF("Cluster channel load changed: channel %u, busy_percentage %d",
		evt_data->rssi_result.channel, evt_data->rssi_result.busy_percentage);

	if (set_ptr->net_mgmt_common.cluster.channel_loaded_percent == 0) {
		/* Channel reselection/reconfigure disabled, no worth to continue */
		return;
	}
	if (dect_mdm_ctrl_get_ft_cluster_state() != CTRL_FT_CLUSTER_STATE_STARTED) {
		/* Cluster not started - cannot reconfigure */
		return;
	}
	int err = dect_mdm_utils_mdm_rssi_results_to_l2_rssi_data(mdm_rssi_res, &rssi_data);

	if (err) {
		LOG_ERR("Error in converting RSSI results to L2 data: %d", err);
		return;
	}
	uint8_t channel_loaded_percent = 100 - rssi_data.scan_suitable_percent;

	if (channel_loaded_percent > set_ptr->net_mgmt_common.cluster.channel_loaded_percent) {
		LOG_WRN("Cluster channel load (%u%%) exceeded threshold (%d%%): "
			"channel %u",
			channel_loaded_percent,
			set_ptr->net_mgmt_common.cluster.channel_loaded_percent,
			evt_data->rssi_result.channel);
		if (set_ptr->net_mgmt_common.device_type & DECT_DEVICE_TYPE_FT) {
			struct dect_cluster_reconfig_req_params reconfig_params = {
				.channel = DECT_CLUSTER_CHANNEL_ANY,
				.max_beacon_tx_power_dbm =
					set_ptr->net_mgmt_common.cluster.max_beacon_tx_power_dbm,
				.max_cluster_power_dbm =
					set_ptr->net_mgmt_common.cluster.max_cluster_power_dbm,
				.period = set_ptr->net_mgmt_common.cluster.beacon_period,
			};

			LOG_INF("FT: starting channel reselection procedure");

			/* Trigger a channel reselection procedure */
			dect_mdm_ctrl_internal_msgq_data_op_add(
				DECT_MDM_CTRL_OP_CLUSTER_RECONFIG_REQ, &reconfig_params,
				sizeof(struct dect_cluster_reconfig_req_params));
		}
	}
}

static void handle_neighbor_inactivity(struct dect_mdm_common_op_event_msgq_item *event)
{
	struct nrf_modem_dect_mac_neighbor_inactivity_ntf_cb_params *evt_data =
		(struct nrf_modem_dect_mac_neighbor_inactivity_ntf_cb_params *)event->data;
	struct dect_mdm_settings *set_ptr = dect_mdm_settings_ref_get();

	LOG_INF("Neighbor inactivity: long_rd_id %u (0x%X)", evt_data->long_rd_id,
		evt_data->long_rd_id);

	/* If inactivity timer is supported and child has been too long as inactive
	 * -> drop it
	 */
	if ((set_ptr->net_mgmt_common.device_type & DECT_DEVICE_TYPE_FT) &&
	    set_ptr->net_mgmt_common.cluster.neighbor_inactivity_disconnect_timer_ms) {
		LOG_WRN("Neighbor inactive for too long - "
			"releasing association: long_rd_id %u (0x%X)",
			evt_data->long_rd_id, evt_data->long_rd_id);

		/* Release association for this child neighbor */
		dect_mdm_ctrl_api_associate_release_cmd(
			evt_data->long_rd_id, NRF_MODEM_DECT_MAC_RELEASE_CAUSE_LONG_INACTIVITY);
	}
}

static void handle_cluster_beacon_rx_failure(struct dect_mdm_common_op_event_msgq_item *event)
{
	struct nrf_modem_dect_mac_cluster_beacon_rx_failure_ntf_cb_params *evt_data =
		(struct nrf_modem_dect_mac_cluster_beacon_rx_failure_ntf_cb_params *)event->data;
	struct dect_mdm_settings *set_ptr = dect_mdm_settings_ref_get();

	LOG_WRN("Max nbr of cluster beacon RX failures (%d): evt long rd id %u (0x%X), "
		"ctrl_data parent long rd id %u (0x%X)",
		set_ptr->net_mgmt_common.association.max_beacon_rx_failures, evt_data->long_rd_id,
		evt_data->long_rd_id, ctrl_data.ass_config.parent_long_rd_id,
		ctrl_data.ass_config.parent_long_rd_id);

	/* OK, configured max amount of cluster beacon RX failures, so we can
	 * start a dissociation process.
	 */
	CTRL_DATA_LOCK();
	if (ctrl_data.ass_config.pt_association_state == CTRL_PT_ASSOCIATION_STATE_ASSOCIATED) {
		__ASSERT_NO_MSG(ctrl_data.ass_config.parent_long_rd_id == evt_data->long_rd_id);
		uint32_t parent_long_rd_id = ctrl_data.ass_config.parent_long_rd_id;

		CTRL_DATA_UNLOCK();
		LOG_INF("Starting releasing the association");
		dect_mdm_ctrl_api_associate_release_cmd(
			parent_long_rd_id, NRF_MODEM_DECT_MAC_RELEASE_CAUSE_LONG_INACTIVITY);
	} else {
		CTRL_DATA_UNLOCK();
	}
}

static void handle_neighbor_paging_failure(struct dect_mdm_common_op_event_msgq_item *event)
{
	/* Message is an indication that a neighbor is tried to be paged for
	 * incoming data but there was no answer. We release the association.
	 */
	struct nrf_modem_dect_mac_neighbor_paging_failure_ntf_cb_params *params =
		(struct nrf_modem_dect_mac_neighbor_paging_failure_ntf_cb_params *)event->data;

	LOG_WRN("Neighbor paging failure: long_rd_id %u (0x%X) - "
		"releasing association",
		params->long_rd_id, params->long_rd_id);

	dect_mdm_ctrl_api_associate_release_cmd(
		params->long_rd_id, NRF_MODEM_DECT_MAC_RELEASE_CAUSE_BAD_RADIO_QUALITY);
}

static void handle_mdm_flow_control(struct dect_mdm_common_op_event_msgq_item *event)
{
	struct nrf_modem_dect_dlc_flow_control_ntf_cb_params *evt_data = event->data;

	CTRL_DATA_LOCK();
	ctrl_data.tx_mdm_flow_ctrl_on =
		evt_data->status == NRF_MODEM_DECT_DLC_FLOW_CTRL_STATUS_ON ? true : false;
	CTRL_DATA_UNLOCK();

	LOG_DBG("Flow control %s:",
		evt_data->status == NRF_MODEM_DECT_DLC_FLOW_CTRL_STATUS_ON ? "ON" : "OFF");
}

static void handle_mdm_association_ind(struct dect_mdm_common_op_event_msgq_item *event)
{
	char tmp_str[DECT_MDM_UTILS_STR_BUFF_SIZE] = {0};
	struct nrf_modem_dect_mac_association_ntf_cb_params *params = event->data;

	if (params->status != NRF_MODEM_DECT_MAC_ASSOCIATION_INDICATION_STATUS_SUCCESS) {
		dect_mdm_utils_modem_association_ind_err_to_string(
			params->status, tmp_str, sizeof(tmp_str));
		LOG_ERR("Association indication with err %s (%d) with long RD ID: "
			"0x%X",
			tmp_str, params->status, params->long_rd_id);
		return;
	}
	LOG_INF("New child: association with long RD ID: %u (0x%X)", params->long_rd_id,
		params->long_rd_id);

	dect_mdm_child_association_created(params->long_rd_id);
}

static void handle_mdm_association_resp(struct dect_mdm_common_op_event_msgq_item *event)
{
	char tmp_str[DECT_MDM_UTILS_STR_BUFF_SIZE] = {0};
	struct nrf_modem_dect_mac_association_cb_params *params = event->data;
#if defined(CONFIG_ASSERT)
	struct dect_mdm_settings *set_ptr = dect_mdm_settings_ref_get();
#endif
	struct dect_network_status_evt network_status_data = {
		.network_status = DECT_NETWORK_STATUS_FAILURE,
		.dect_err_cause = dect_mdm_utils_modem_status_to_net_mgmt_status(params->status),
	};

	/* Association response only with PT devices */
	__ASSERT_NO_MSG(set_ptr->net_mgmt_common.device_type & DECT_DEVICE_TYPE_PT);
	if (params->status != NRF_MODEM_DECT_MAC_STATUS_OK) {
		/* Modem operation failed */
		struct net_if *iface;

		CTRL_DATA_LOCK();
		iface = ctrl_data.iface;
		CTRL_DATA_UNLOCK();

		dect_mgmt_association_req_failed_mdm_result_evt(iface, params->long_rd_id,
								network_status_data.dect_err_cause);
		dect_mgmt_network_status_evt(iface, network_status_data);

		CTRL_DATA_LOCK();
		ctrl_data.configure_params.auto_start = false;
		if (ctrl_data.ass_config.pt_association_state !=
		    CTRL_PT_ASSOCIATION_STATE_ASSOCIATED) {
			ctrl_data.ass_config.pt_association_state = CTRL_PT_ASSOCIATION_STATE_NONE;
		}
		CTRL_DATA_UNLOCK();

		dect_mdm_utils_modem_mac_err_to_string(
			params->status, tmp_str, sizeof(tmp_str));
		LOG_ERR("Modem operation failed for Association Response with "
			"err %s (%d) with long RD ID: 0x%X",
			tmp_str, params->status, params->long_rd_id);
		return;
	}
	CTRL_DATA_LOCK();
	struct net_if *iface = ctrl_data.iface;

	CTRL_DATA_UNLOCK();
	if (!params->flags.has_association_response) {
		dect_mgmt_association_req_rejected_result_evt(iface, params->long_rd_id,
							      DECT_ASSOCIATION_NO_RESPONSE,
							      DECT_ASSOCIATION_REJECT_TIME_60S);

		dect_mgmt_network_status_evt(iface, network_status_data);
		LOG_ERR("Association response with no actual response with long RD "
			"ID: 0x%X",
			params->long_rd_id);
	} else if (!params->association_response.ack_status) {
		/* Association rejected */
		LOG_INF("Association rejected with long RD ID: %u (0x%X), cause %d",
			params->long_rd_id, params->long_rd_id,
			params->association_response.reject_cause);

		dect_mgmt_association_req_rejected_result_evt(
			iface, params->long_rd_id, params->association_response.reject_cause,
			params->association_response.reject_time);
	} else {
		LOG_INF("New parent: association with long RD ID: %u (0x%X)", params->long_rd_id,
			params->long_rd_id);

		CTRL_DATA_LOCK();
		ctrl_data.configure_params.auto_start = false;
		ctrl_data.ass_config.pt_association_state = CTRL_PT_ASSOCIATION_STATE_ASSOCIATED;
		ctrl_data.ass_config.parent_long_rd_id = params->long_rd_id;
		CTRL_DATA_UNLOCK();
		dect_mdm_parent_association_created(params->long_rd_id, params->ipv6_config);
	}
}

static void handle_mdm_association_release_resp(struct dect_mdm_common_op_event_msgq_item *event)
{
	struct nrf_modem_dect_mac_association_release_cb_params *params = event->data;
	struct dect_mdm_settings *set_ptr = dect_mdm_settings_ref_get();

	LOG_INF("Association released with long RD ID: %u (0x%08x)", params->long_rd_id,
		params->long_rd_id);

	CTRL_DATA_LOCK();
	if (ctrl_data.ass_config.pt_association_state == CTRL_PT_ASSOCIATION_STATE_ASSOCIATED) {
		ctrl_data.ass_config.pt_association_state = CTRL_PT_ASSOCIATION_STATE_NONE;
		enum nrf_modem_dect_mac_release_cause rel_cause = ctrl_data.last_rel_cause;

		CTRL_DATA_UNLOCK();
		dect_mdm_parent_association_removed(params->long_rd_id, rel_cause, false);
	} else if (set_ptr->net_mgmt_common.device_type & DECT_DEVICE_TYPE_FT) {
		enum nrf_modem_dect_mac_release_cause rel_cause = ctrl_data.last_rel_cause;

		CTRL_DATA_UNLOCK();
		dect_mdm_child_association_removed(params->long_rd_id, rel_cause, false);
	} else {
		CTRL_DATA_UNLOCK();
	}
}

static void handle_mdm_association_release_ind(struct dect_mdm_common_op_event_msgq_item *event)
{
	struct nrf_modem_dect_mac_association_release_ntf_cb_params *params = event->data;
	struct dect_mdm_settings *set_ptr = dect_mdm_settings_ref_get();

	LOG_INF("Release req received and association released with long RD ID: "
		"%u (0x%X)",
		params->long_rd_id, params->long_rd_id);

	if (set_ptr->net_mgmt_common.device_type & DECT_DEVICE_TYPE_FT) {
		dect_mdm_child_association_removed(params->long_rd_id, params->release_cause,
						     true);
	} else {
		/* Network kicked us out? */
		CTRL_DATA_LOCK();
		ctrl_data.ass_config.pt_association_state = CTRL_PT_ASSOCIATION_STATE_NONE;
		CTRL_DATA_UNLOCK();
		dect_mdm_parent_association_removed(params->long_rd_id, params->release_cause,
						      true);
	}
}

static void handle_auto_start(struct dect_mdm_common_op_event_msgq_item *event)
{
	char tmp_str[DECT_MDM_UTILS_STR_BUFF_SIZE] = {0};
	int err;
	struct dect_mdm_settings *set_ptr = dect_mdm_settings_ref_get();

	ARG_UNUSED(event);
	CTRL_DATA_LOCK();
	ctrl_data.configure_params.auto_start = true;

	/* Clear seen cluster channels */
	memset(ctrl_data.scan_data.cluster_channels, 0,
	       sizeof(ctrl_data.scan_data.cluster_channels));
	ctrl_data.scan_data.current_cluster_channel_index = 0;

	uint16_t ft_requested_channel = ctrl_data.ft_requested_cluster_channel;
	bool reconfig_ongoing = ctrl_data.ft_cluster_reconfig_ongoing;

	CTRL_DATA_UNLOCK();
	if (set_ptr->net_mgmt_common.device_type & DECT_DEVICE_TYPE_FT) {
		struct nrf_modem_dect_mac_network_scan_params params = {
			.network_id_filter_mode = NRF_MODEM_DECT_MAC_NW_ID_FILTER_MODE_NONE,
			.band = set_ptr->net_mgmt_common.band_nbr,
			.scan_time = DECT_MDM_CTRL_FT_NW_SCAN_TIME_MS,
			.num_channels = 0,
		};

		if (ft_requested_channel != DECT_CLUSTER_CHANNEL_ANY) {
			params.num_channels = 1;
			params.channel_list[0] = ft_requested_channel;
		} else {
			uint8_t num_channels = NRF_MODEM_DECT_MAC_MAX_CHANNELS_IN_NETWORK_SCAN_REQ;
			uint16_t channel_list[NRF_MODEM_DECT_MAC_MAX_CHANNELS_IN_NETWORK_SCAN_REQ];

			if (dect_common_utils_use_harmonized_std(
				    set_ptr->net_mgmt_common.band_nbr) &&
			    dect_common_utils_harmonized_band_channel_array_get(
				    set_ptr->net_mgmt_common.band_nbr, channel_list,
				    &num_channels)) {
				/* Per harmonized std: only odd number channels at band #1:
				 * ETSI EN 301 406-2, V3.0.1, ch 4.3.2.3.
				 */
				params.num_channels = num_channels;
				memcpy(params.channel_list, channel_list,
				       num_channels * sizeof(uint16_t));
			}
		}

		if (reconfig_ongoing) {
			/* Do not initiate nw scan if cluster is already running and
			 * doing reconfig
			 */
			err = -EALREADY;
		} else {
			LOG_INF("FT device auto start for cluster start: "
				"starting NW scanning to see where other "
				"clusters are: band %hhu, num_channels %hhu",
				params.band, params.num_channels);
			err = nrf_modem_dect_mac_network_scan(&params);
		}
		if (!err) {
			CTRL_DATA_LOCK();
			ctrl_data.scan_data.on_going = true;
			ctrl_data.scan_data.scan_result_cb = NULL;
			ctrl_data.scan_data.scan_params = params;
			CTRL_DATA_UNLOCK();
		} else {
			/* As a default RSSI params from settings  */
			struct nrf_modem_dect_mac_rssi_scan_params rssi_params;

			__ASSERT_NO_MSG(set_ptr->net_mgmt_common.device_type & DECT_DEVICE_TYPE_FT);
			if (!reconfig_ongoing) {
				LOG_WRN("Error initiating NW scan: err %d - "
					"starting directly RSSI scan",
					err);
			}

			dect_mdm_ctrl_build_rssi_scan_params(
				&rssi_params, set_ptr, ft_requested_channel, reconfig_ongoing);

			dect_mdm_ctrl_internal_msgq_data_op_add(
				DECT_MDM_CTRL_OP_RSSI_START_REQ_CH_SELECTION, &rssi_params,
				sizeof(struct nrf_modem_dect_mac_rssi_scan_params));
		}
	} else {
		LOG_INF("PT device auto start: starting network scan");

		struct nrf_modem_dect_mac_network_scan_params params = {
			.network_id_filter_mode = NRF_MODEM_DECT_MAC_NW_ID_FILTER_MODE_32BIT,
			.network_id_filter = set_ptr->net_mgmt_common.identities.network_id,
			.scan_time = DECT_MDM_CTRL_PT_NW_SCAN_TIME_MS,
			.num_channels = 0,
			.band = set_ptr->net_mgmt_common.band_nbr,
		};

		err = nrf_modem_dect_mac_network_scan(&params);
		if (err) {
			dect_mdm_utils_modem_mac_err_to_string(err, tmp_str, sizeof(tmp_str));
			LOG_ERR("Error in Network scan: err %s (%d)", tmp_str, err);
			CTRL_DATA_LOCK();
			ctrl_data.configure_params.auto_start = false;
			struct net_if *iface = ctrl_data.iface;

			CTRL_DATA_UNLOCK();
			dect_mgmt_network_status_evt(
				iface, (struct dect_network_status_evt){
					       .network_status = DECT_NETWORK_STATUS_FAILURE,
					       .dect_err_cause = DECT_STATUS_OS_ERROR,
					       .os_err_cause = err,
				       });
		} else {
			CTRL_DATA_LOCK();
			ctrl_data.scan_data.on_going = true;
			ctrl_data.scan_data.scan_result_cb = NULL;
			ctrl_data.scan_data.scan_params = params;
			CTRL_DATA_UNLOCK();
		}
	}
}

static void handle_rssi_start_req_cmd(struct dect_mdm_common_op_event_msgq_item *event)
{
	char tmp_str[DECT_MDM_UTILS_STR_BUFF_SIZE] = {0};
	struct nrf_modem_dect_mac_rssi_scan_params *params =
		(struct nrf_modem_dect_mac_rssi_scan_params *)event->data;
	int err;

	LOG_INF("DECT_MDM_CTRL_OP_RSSI_START_REQ_CMD: "
		"channel_scan_length %hu, num_channels %hu, band %hhu, "
		"min_threshold %hhd, max_threshold %hhd",
		params->channel_scan_length, params->num_channels, params->band,
		params->threshold_min, params->threshold_max);

	/* Log channels */
	for (uint32_t i = 0; i < params->num_channels; i++) {
		LOG_INF("  channel[%hhu]: %d", i, params->channel_list[i]);
	}
	err = nrf_modem_dect_mac_rssi_scan(params);
	if (err) {
		dect_mdm_utils_modem_mac_err_to_string(err, tmp_str, sizeof(tmp_str));
		LOG_ERR("Error initiating RSSI scan by params: err %s (%d)", tmp_str, err);
		CTRL_DATA_LOCK();
		struct net_if *iface = ctrl_data.iface;
		enum ctrl_ft_network_state network_state = ctrl_data.ft_network_state;

		CTRL_DATA_UNLOCK();
		dect_mgmt_rssi_scan_done_evt(iface,
					     dect_mdm_utils_modem_status_to_net_mgmt_status(err));
		if (network_state != CTRL_FT_NETWORK_STATE_NONE) {
			CTRL_DATA_LOCK();
			ctrl_data.ft_network_state = CTRL_FT_NETWORK_STATE_NONE;
			iface = ctrl_data.iface;
			CTRL_DATA_UNLOCK();
			dect_mgmt_network_status_evt(
				iface, (struct dect_network_status_evt){
					       .network_status = DECT_NETWORK_STATUS_FAILURE,
					       .dect_err_cause = DECT_STATUS_OS_ERROR,
					       .os_err_cause = err,
				       });
		}
	} else {
		LOG_INF("RSSI scan started");
		dect_mdm_ctrl_rssi_scan_data_init(true);
	}
}

static void handle_rssi_start_req_ch_selection(struct dect_mdm_common_op_event_msgq_item *event)
{
	char tmp_str[DECT_MDM_UTILS_STR_BUFF_SIZE] = {0};
	struct nrf_modem_dect_mac_rssi_scan_params *params =
		(struct nrf_modem_dect_mac_rssi_scan_params *)event->data;
	struct dect_mdm_settings *set_ptr = dect_mdm_settings_ref_get();

	LOG_INF("DECT_MDM_CTRL_OP_RSSI_START_REQ_CH_SELECTION: "
		"channel_scan_length %hu, num_channels %hu, band %hhu, "
		"min_threshold %hhd, max_threshold %hhd",
		params->channel_scan_length, params->num_channels, params->band,
		params->threshold_min, params->threshold_max);

	if (params->num_channels == 0 &&
	    dect_common_utils_use_harmonized_std(set_ptr->net_mgmt_common.band_nbr) == true) {
		/* Per harmonized std: only odd number channels at band #1:
		 * ETSI EN 301 406-2, V3.0.1, ch 4.3.2.3.
		 */
		bool array_filled = false;

		params->num_channels = DECT_MAX_CHANNELS_IN_RSSI_SCAN;
		array_filled = dect_common_utils_harmonized_band_channel_array_get(
			set_ptr->net_mgmt_common.band_nbr, params->channel_list,
			&params->num_channels);
		if (!array_filled) {
			LOG_ERR("RSSI scanning start: "
				"error in channel array filling");
			return;
		}
	}
	int err = nrf_modem_dect_mac_rssi_scan(params);

	if (err) {
		dect_mdm_utils_modem_mac_err_to_string(err, tmp_str, sizeof(tmp_str));
		LOG_ERR("nrf_modem_dect_mac_rssi_scan failed: %s (%d)", tmp_str, err);
		CTRL_DATA_LOCK();
		bool reconfig_ongoing = ctrl_data.ft_cluster_reconfig_ongoing;
		enum ctrl_ft_cluster_state cluster_state = ctrl_data.ft_cluster_state;

		if (reconfig_ongoing && cluster_state == CTRL_FT_CLUSTER_STATE_STARTING) {
			/* Reconfig failure, existing cluster config
			 * still running
			 */
			ctrl_data.ft_cluster_state = CTRL_FT_CLUSTER_STATE_STARTED;
			ctrl_data.configure_params.channel =
				ctrl_data.ft_cluster_reconfig_prev_cluster_channel;
		} else {
			ctrl_data.ft_cluster_state = CTRL_FT_CLUSTER_STATE_NONE;
		}
		ctrl_data.ft_requested_cluster_channel = DECT_CLUSTER_CHANNEL_ANY;
		ctrl_data.ft_cluster_reconfig_ongoing = false;
		struct net_if *iface = ctrl_data.iface;

		CTRL_DATA_UNLOCK();
		dect_mgmt_rssi_scan_done_evt(iface, DECT_STATUS_OS_ERROR);
	} else {
		dect_mdm_ctrl_rssi_scan_data_init(false);
		LOG_INF("RSSI scan started with params: "
			"channel_scan_length %hu, num_channels %hu, band %hhu, "
			"min_threshold %hhd, max_threshold %hhd",
			params->channel_scan_length, params->num_channels, params->band,
			params->threshold_min, params->threshold_max);
	}
}

static void handle_mdm_rssi_result(struct dect_mdm_common_op_event_msgq_item *event)
{
	char tmp_str[DECT_MDM_UTILS_STR_BUFF_SIZE] = {0};
	struct dect_mdm_ctrl_rssi_measurement_data_evt *evt_data =
		(struct dect_mdm_ctrl_rssi_measurement_data_evt *)event->data;
	struct nrf_modem_dect_mac_rssi_result *mdm_rssi_res = &evt_data->rssi_result;
	bool channel_has_another_cluster = false;

	LOG_DBG("RSSI scan results received");

	bool scan_suitable_percent_ok = false;
	bool all_subslots_free = false;
	struct dect_mdm_settings *set_ptr = dect_mdm_settings_ref_get();
	struct dect_rssi_scan_result_evt l2_results_evt;
	struct dect_rssi_scan_result_data *rssi_data = &l2_results_evt.rssi_scan_result;
	int err = dect_mdm_utils_mdm_rssi_results_to_l2_rssi_data(mdm_rssi_res, rssi_data);

	if (err) {
		LOG_ERR("Error in converting RSSI results to L2 data: %d", err);
	}

	/* Check if we have seen another cluster in this channel */
	if (dect_mdm_ctrl_cluster_channels_list_channel_exists(rssi_data->channel)) {
		LOG_DBG("RSSI results: another cluster seen in this"
			" channel %d was seen in nw scanning phase",
			rssi_data->channel);
		channel_has_another_cluster = true;
		rssi_data->another_cluster_detected_in_channel = true;
	}
	all_subslots_free = rssi_data->all_subslots_free;
	if (rssi_data->scan_suitable_percent >=
	    set_ptr->net_mgmt_common.rssi_scan.scan_suitable_percent) {
		scan_suitable_percent_ok = true;
	}

	LOG_INF("RSSI scan results: channel %d, all_subslots_free: %s, "
		"scan_suitable_percent: %d%%, "
		"busy_percentage: %d%%, "
		"another_cluster_detected_in_channel: %s",
		rssi_data->channel, (rssi_data->all_subslots_free) ? "yes" : "no",
		rssi_data->scan_suitable_percent, rssi_data->busy_percentage,
		(rssi_data->another_cluster_detected_in_channel) ? "yes" : "no");

	CTRL_DATA_LOCK();
	struct net_if *iface = ctrl_data.iface;

	CTRL_DATA_UNLOCK();
	dect_mgmt_rssi_scan_result_evt(iface, l2_results_evt);

	/* Store results for internal usage in cluster channel selection */
	CTRL_DATA_LOCK();
	if (ctrl_data.rssi_scan_data.current_results_index <
	    DECT_MDM_CTRL_RSSI_SCAN_RESULTS_MAX_CHANNELS) {
		struct dect_mdm_ctrl_rssi_scan_result_data results_item = {
			.channel = rssi_data->channel,
			.all_subslots_free = all_subslots_free,
			.scan_suitable_percent_ok = scan_suitable_percent_ok,
			.busy_percentage = rssi_data->busy_percentage,
			.busy_percentage_ok =
				(rssi_data->busy_percentage <
				 (100 - set_ptr->net_mgmt_common.rssi_scan.scan_suitable_percent)),
			.another_cluster_detected_in_channel = channel_has_another_cluster,
			.possible_subslot_cnt = rssi_data->possible_subslot_cnt,
			.busy_subslot_cnt = rssi_data->busy_subslot_cnt,
		};

		ctrl_data.rssi_scan_data.results[ctrl_data.rssi_scan_data.current_results_index++] =
			results_item;
	}
	bool cmd_on_going = ctrl_data.rssi_scan_data.cmd_on_going;
	bool auto_start = ctrl_data.configure_params.auto_start;
	enum ctrl_ft_cluster_state ft_cluster_state = ctrl_data.ft_cluster_state;
	uint16_t channel = ctrl_data.configure_params.channel;

	CTRL_DATA_UNLOCK();
	if (cmd_on_going) {
		/* Actual RSSI scanning command was running, let it run */
		return;
	}
	if ((channel_has_another_cluster == false && all_subslots_free &&
	     scan_suitable_percent_ok && rssi_data->busy_percentage == 0) &&
	    (auto_start || ft_cluster_state == CTRL_FT_CLUSTER_STATE_STARTING) && (channel == 0)) {
		/* MAC spec, ch. 5.1.2:
		 * if any channel where all subslots are "free", is found,
		 * then select channel for the cluster & stop the scan.
		 * Additionally, we made this rule stricter, we checked
		 * that no other cluster is present in the channel and
		 * calculated busy percentage is also zero.
		 */
		CTRL_DATA_LOCK();
		ctrl_data.configure_params.channel = mdm_rssi_res->channel;
		CTRL_DATA_UNLOCK();

		err = nrf_modem_dect_mac_rssi_scan_stop();
		if (err) {
			dect_mdm_utils_modem_mac_err_to_string(err, tmp_str, sizeof(tmp_str));
			LOG_ERR("Error in RSSI scan stop: err %s (%d)", tmp_str, err);
		}
	}
}

static void handle_mdm_rssi_stopped(struct dect_mdm_common_op_event_msgq_item *event)
{
	char tmp_str[DECT_MDM_UTILS_STR_BUFF_SIZE] = {0};
	enum nrf_modem_dect_mac_err *status = (enum nrf_modem_dect_mac_err *)event->data;

	if (*status != NRF_MODEM_DECT_MAC_STATUS_OK) {
		dect_mdm_utils_modem_mac_err_to_string(*status, tmp_str, sizeof(tmp_str));

		LOG_ERR("Error in RSSI scan stop: err %s (%d)", tmp_str, *status);
	} else {
		LOG_INF("RSSI scan stopped");
	}
}

static void handle_mdm_rssi_complete(struct dect_mdm_common_op_event_msgq_item *event)
{
	char tmp_str[DECT_MDM_UTILS_STR_BUFF_SIZE] = {0};
	int err;
	enum nrf_modem_dect_mac_err *status = (enum nrf_modem_dect_mac_err *)event->data;
	struct dect_mdm_settings *set_ptr = dect_mdm_settings_ref_get();
	bool reconfig_on_same_channel;
	struct net_if *iface;

	CTRL_DATA_LOCK();
	reconfig_on_same_channel = (ctrl_data.ft_cluster_reconfig_ongoing &&
				    ctrl_data.configure_params.channel ==
					    ctrl_data.ft_cluster_reconfig_prev_cluster_channel);
	iface = ctrl_data.iface;
	CTRL_DATA_UNLOCK();

	if (!reconfig_on_same_channel) {
		/* Reconfig for prefix on going, RSSI scan result is bypassed */
		dect_mgmt_rssi_scan_done_evt(
			iface, dect_mdm_utils_modem_status_to_net_mgmt_status(*status));
	}
	if (*status != NRF_MODEM_DECT_MAC_STATUS_OK && !(reconfig_on_same_channel)) {
		dect_mdm_utils_modem_mac_err_to_string(*status, tmp_str, sizeof(tmp_str));

		LOG_ERR("Error in RSSI scan complete: err %s (%d)", tmp_str, *status);
		CTRL_DATA_LOCK();
		ctrl_data.rssi_scan_data.cmd_on_going = false;

		if (ctrl_data.ft_cluster_state == CTRL_FT_CLUSTER_STATE_STARTING) {
			if (ctrl_data.ft_cluster_reconfig_ongoing) {
				/* RSSI failure during reconfig, prev cluster conf
				 * still running
				 */
				ctrl_data.ft_cluster_state = CTRL_FT_CLUSTER_STATE_STARTED;
				ctrl_data.ft_cluster_reconfig_ongoing = false;
				ctrl_data.ft_requested_cluster_channel = DECT_CLUSTER_CHANNEL_ANY;
				ctrl_data.configure_params.channel =
					ctrl_data.ft_cluster_reconfig_prev_cluster_channel;
				CTRL_DATA_UNLOCK();

				LOG_WRN("RSSI scan failed on reconfig - "
					"keeping cluster running");
				return;
			}
			struct net_if *iface_local = ctrl_data.iface;

			ctrl_data.ft_cluster_state = CTRL_FT_CLUSTER_STATE_NONE;
			ctrl_data.configure_params.channel = 0;
			ctrl_data.ft_requested_cluster_channel = DECT_CLUSTER_CHANNEL_ANY;

			CTRL_DATA_UNLOCK();
			dect_mgmt_cluster_created_evt(
				iface_local,
				(struct dect_cluster_start_resp_evt){
					.status = dect_mdm_utils_modem_status_to_net_mgmt_status(
						*status),
					.cluster_channel = 0,
				});
		} else {
			enum ctrl_ft_network_state ft_network_state = ctrl_data.ft_network_state;
			struct net_if *iface_local = ctrl_data.iface;

			if (ft_network_state != CTRL_FT_NETWORK_STATE_NONE) {
				ctrl_data.ft_network_state = CTRL_FT_NETWORK_STATE_NONE;
			}
			CTRL_DATA_UNLOCK();
			if (ft_network_state != CTRL_FT_NETWORK_STATE_NONE) {
				struct dect_network_status_evt network_status_data = {
					.network_status = DECT_NETWORK_STATUS_FAILURE,
					.dect_err_cause =
						dect_mdm_utils_modem_status_to_net_mgmt_status(
							*status),
				};

				dect_mgmt_network_status_evt(iface_local, network_status_data);
			}
		}
		return;
	}
	LOG_INF("RSSI scan completed");
	CTRL_DATA_LOCK();
	if (ctrl_data.rssi_scan_data.cmd_on_going) {
		ctrl_data.rssi_scan_data.cmd_on_going = false;
		CTRL_DATA_UNLOCK();
		return;
	}
	bool auto_start = ctrl_data.configure_params.auto_start;
	enum ctrl_ft_cluster_state ft_cluster_state = ctrl_data.ft_cluster_state;
	uint16_t channel = ctrl_data.configure_params.channel;

	CTRL_DATA_UNLOCK();

	if (!(auto_start || ft_cluster_state == CTRL_FT_CLUSTER_STATE_STARTING)) {
		/* Not auto start or cluster start req - nothing to do here more */
		return;
	}

	if (channel == 0) {
		struct dect_mdm_ctrl_rssi_scan_result_data best_rssi_meas;

		if (dect_mdm_ctrl_rssi_scan_data_results_best_get(&best_rssi_meas)) {
			CTRL_DATA_LOCK();
			ctrl_data.configure_params.channel = best_rssi_meas.channel;
			CTRL_DATA_UNLOCK();
		} else {
			CTRL_DATA_LOCK();
			if (ctrl_data.ft_cluster_reconfig_ongoing) {
				/* Reconfig failure, existing cluster config
				 * still running
				 */

				LOG_WRN("No free enough channel found in "
					"RSSI scan,"
					" cannot reconfigure cluster");
				ctrl_data.ft_cluster_reconfig_ongoing = false;
				ctrl_data.ft_cluster_state = CTRL_FT_CLUSTER_STATE_STARTED;
				ctrl_data.configure_params.channel =
					ctrl_data.ft_cluster_reconfig_prev_cluster_channel;
				CTRL_DATA_UNLOCK();
				return;
			}
			struct net_if *iface_local = ctrl_data.iface;

			ctrl_data.ft_cluster_state = CTRL_FT_CLUSTER_STATE_NONE;
			ctrl_data.ft_network_state = CTRL_FT_NETWORK_STATE_NONE;
			ctrl_data.ft_requested_cluster_channel = DECT_CLUSTER_CHANNEL_ANY;
			CTRL_DATA_UNLOCK();
			LOG_ERR("No free enough channel found in RSSI scan,"
				" cannot start cluster");
			dect_mgmt_cluster_created_evt(
				iface_local, (struct dect_cluster_start_resp_evt){
						     .status = DECT_STATUS_NO_RESOURCES,
						     .cluster_channel = 0,
					     });
			dect_mgmt_network_status_evt(
				iface_local, (struct dect_network_status_evt){
						     .network_status = DECT_NETWORK_STATUS_FAILURE,
						     .dect_err_cause = DECT_STATUS_NO_RESOURCES,
					     });
			return;
		}
	}

	CTRL_DATA_LOCK();
	bool auto_start_log = ctrl_data.configure_params.auto_start;
	uint16_t channel_log = ctrl_data.configure_params.channel;

	CTRL_DATA_UNLOCK();
	LOG_INF("%s: starting a cluster on a channel %d.",
		auto_start_log ? "auto start" : "cluster_start_req", channel_log);

	/* Start a cluster in a chosen channel */
	CTRL_DATA_LOCK();
	uint16_t cluster_channel = ctrl_data.configure_params.channel;
	uint32_t network_id = ctrl_data.configure_params.network_id;

	CTRL_DATA_UNLOCK();

	struct nrf_modem_dect_mac_cluster_config cluster_config = {
		.flags = {
				.has_max_tx_power = true,
				.has_rach_config = true,
			},
		.count_to_trigger = NRF_MODEM_DECT_MAC_COUNT_TO_TRIGGER_2,
		.relative_quality = NRF_MODEM_DECT_MAC_QUALITY_THRESHOLD_0,
		.min_quality = NRF_MODEM_DECT_MAC_QUALITY_THRESHOLD_0,
		.beacon_tx_power = dect_utils_lib_dbm_to_phy_tx_power(
			set_ptr->net_mgmt_common.cluster.max_beacon_tx_power_dbm),
		.cluster_max_tx_power = dect_utils_lib_dbm_to_phy_tx_power(
			set_ptr->net_mgmt_common.cluster.max_cluster_power_dbm),
		.cluster_beacon_period = set_ptr->net_mgmt_common.cluster.beacon_period,
		.cluster_channel = cluster_channel,
		.network_id = network_id,
		.rach_configuration = {.policy = NRF_MODEM_DECT_MAC_RACH_CONFIG_POLICY_FILL,
				       .common = {
						       .response_window_length = 8,
						       .max_transmission_length = 8,
						       .cw_min_sig = 2,
						       .cw_max_sig = 7,
					       },
				       .config = {.fill = {
								  .percentage = 100,
							  }}},
		.triggers = {
			.busy_threshold = 20,
		}};
	struct nrf_modem_dect_mac_association_config ass_config = {
		.max_num_neighbours =
			set_ptr->net_mgmt_common.cluster.max_num_neighbors,
		.max_num_ft_neighbours = 2,
		.neighbor_info_triggers = {
				.inactivity_timer = set_ptr->net_mgmt_common
					.cluster
					.neighbor_inactivity_disconnect_timer_ms,
			},
		.default_tx_flow_config = {
			{
				.dlc_service_type =
				NRF_MODEM_DECT_DLC_SERVICE_TYPE_3,
				.dlc_sdu_lifetime =
				NRF_MODEM_DECT_DLC_SDU_LIFETIME_60_S,
			},
			{
				.dlc_service_type =
					NRF_MODEM_DECT_DLC_SERVICE_TYPE_3,
				.dlc_sdu_lifetime =
					NRF_MODEM_DECT_DLC_SDU_LIFETIME_INFINITY,
			},
			{
				.priority = 3,
				.dlc_service_type =
					NRF_MODEM_DECT_DLC_SERVICE_TYPE_3,
				.dlc_sdu_lifetime =
					NRF_MODEM_DECT_DLC_SDU_LIFETIME_INFINITY,
			},
			{
				.priority = 4,
				.dlc_service_type =
					NRF_MODEM_DECT_DLC_SERVICE_TYPE_3,
				.dlc_sdu_lifetime =
					NRF_MODEM_DECT_DLC_SDU_LIFETIME_INFINITY,
			},
			{
				.priority = 5,
				.dlc_service_type =
					NRF_MODEM_DECT_DLC_SERVICE_TYPE_3,
				.dlc_sdu_lifetime =
					NRF_MODEM_DECT_DLC_SDU_LIFETIME_INFINITY,
			},
			{
				.priority = 6,
				.dlc_service_type =
					NRF_MODEM_DECT_DLC_SERVICE_TYPE_3,
				.dlc_sdu_lifetime =
					NRF_MODEM_DECT_DLC_SDU_LIFETIME_INFINITY,
			}
		},
	};
	struct nrf_modem_dect_mac_cluster_configure_params params = {
		.cluster_period_start_offset = 0,
		.association_config = &ass_config,
		.cluster_config = &cluster_config,
	};
	/* Set our IPv6 address prefix to modem to be passed to children */
	struct dect_mdm_ipv6_prefix global_prefix;

	if (dect_mdm_sink_ipv6_prefix_get(&global_prefix)) {
		/* Pass prefix to children */
		__ASSERT_NO_MSG(global_prefix.len == 8);
		memcpy(&cluster_config.ipv6_config.address, global_prefix.prefix.s6_addr,
		       global_prefix.len);
		cluster_config.ipv6_config.type = NRF_MODEM_DECT_MAC_IPV6_ADDRESS_TYPE_PREFIX;
	} else {
		/* Using link local (already set to us)*/
		cluster_config.ipv6_config.type = NRF_MODEM_DECT_MAC_IPV6_ADDRESS_TYPE_NONE;
		LOG_WRN("%s: no IPv6 prefix to set - using link local only", __func__);
	}
	CTRL_DATA_LOCK();
	if (ctrl_data.ft_cluster_reconfig_ongoing) {
		/* Use reconfigure params for certain cluster configs */
		cluster_config.beacon_tx_power = dect_utils_lib_dbm_to_phy_tx_power(
			ctrl_data.ft_cluster_reconfig_params.max_beacon_tx_power_dbm);
		cluster_config.cluster_max_tx_power = dect_utils_lib_dbm_to_phy_tx_power(
			ctrl_data.ft_cluster_reconfig_params.max_cluster_power_dbm);
		cluster_config.cluster_beacon_period = ctrl_data.ft_cluster_reconfig_params.period;
	}
	CTRL_DATA_UNLOCK();
	err = nrf_modem_dect_mac_cluster_configure(&params);
	if (err) {
		LOG_ERR("nrf_modem_dect_mac_cluster_configure returned err "
			"%d",
			err);
		CTRL_DATA_LOCK();
		if (ctrl_data.ft_cluster_reconfig_ongoing &&
		    ctrl_data.ft_cluster_state == CTRL_FT_CLUSTER_STATE_STARTING) {
			/* Reconfig failure, existing cluster config
			 * still running
			 */
			ctrl_data.ft_cluster_state = CTRL_FT_CLUSTER_STATE_STARTED;
			ctrl_data.configure_params.channel =
				ctrl_data.ft_cluster_reconfig_prev_cluster_channel;
			CTRL_DATA_UNLOCK();
		} else if (ctrl_data.ft_network_state != CTRL_FT_NETWORK_STATE_NONE) {
			struct net_if *iface_local = ctrl_data.iface;

			ctrl_data.ft_network_state = CTRL_FT_NETWORK_STATE_NONE;
			CTRL_DATA_UNLOCK();
			dect_mgmt_network_status_evt(
				iface_local, (struct dect_network_status_evt){
						     .network_status = DECT_NETWORK_STATUS_FAILURE,
						     .dect_err_cause = DECT_STATUS_OS_ERROR,
						     .os_err_cause = err,
					     });
		} else {
			CTRL_DATA_UNLOCK();
		}
		CTRL_DATA_LOCK();
		ctrl_data.ft_cluster_reconfig_ongoing = false;
		ctrl_data.ft_requested_cluster_channel = DECT_CLUSTER_CHANNEL_ANY;
		CTRL_DATA_UNLOCK();
	}
}

static void handle_mdm_nw_beacon_start(struct dect_mdm_common_op_event_msgq_item *event)
{
	int err;
	struct dect_nw_beacon_start_req_params *params =
		(struct dect_nw_beacon_start_req_params *)event->data;
	struct dect_mdm_settings *set_ptr = dect_mdm_settings_ref_get();

	struct nrf_modem_dect_mac_network_beacon_configure_params beacon_params = {
		.channel = params->channel,
		.num_additional_channels = 0,
		.nw_beacon_period = set_ptr->net_mgmt_common.nw_beacon.beacon_period,
	};

	beacon_params.num_additional_channels = params->additional_ch_count;
	beacon_params.additional_channels = params->additional_ch_list;

	LOG_INF("starting network beacon on a channel %d.", params->channel);

	err = nrf_modem_dect_mac_network_beacon_configure(&beacon_params);
	if (err) {

		LOG_ERR("nrf_modem_dect_mac_network_beacon_configure failed: %d", err);
		CTRL_DATA_LOCK();
		struct net_if *iface_local = ctrl_data.iface;

		ctrl_data.ft_nw_beacon_state = CTRL_FT_NW_BEACON_STATE_NONE;
		CTRL_DATA_UNLOCK();
		dect_mgmt_nw_beacon_start_evt(
			iface_local, dect_mdm_utils_modem_status_to_net_mgmt_status(err));

		CTRL_DATA_LOCK();
		if (ctrl_data.ft_network_state != CTRL_FT_NETWORK_STATE_NONE) {
			struct net_if *iface_local = ctrl_data.iface;

			ctrl_data.ft_network_state = CTRL_FT_NETWORK_STATE_NONE;
			CTRL_DATA_UNLOCK();

			dect_mgmt_network_status_evt(
				iface_local, (struct dect_network_status_evt){
						     .network_status = DECT_NETWORK_STATUS_FAILURE,
						     .dect_err_cause = DECT_STATUS_OS_ERROR,
						     .os_err_cause = err,
					     });
		} else {
			CTRL_DATA_UNLOCK();
		}
	} else {
		CTRL_DATA_LOCK();
		ctrl_data.ft_nw_beacon_state = CTRL_FT_NW_BEACON_STATE_STARTING;
		CTRL_DATA_UNLOCK();
	}
}

static void
handle_mdm_nw_beacon_start_or_stop_done(struct dect_mdm_common_op_event_msgq_item *event)
{
	char tmp_str[DECT_MDM_UTILS_STR_BUFF_SIZE] = {0};
	struct nrf_modem_dect_mac_network_beacon_configure_cb_params *evt_data =
		(struct nrf_modem_dect_mac_network_beacon_configure_cb_params *)event->data;

	CTRL_DATA_LOCK();
	enum ctrl_ft_nw_beacon_state ft_nw_beacon_state = ctrl_data.ft_nw_beacon_state;
	enum ctrl_ft_network_state ft_network_state = ctrl_data.ft_network_state;
	struct net_if *iface = ctrl_data.iface;

	CTRL_DATA_UNLOCK();

	if (evt_data->status != NRF_MODEM_DECT_MAC_STATUS_OK) {
		dect_mdm_utils_modem_mac_err_to_string(
			evt_data->status, tmp_str, sizeof(tmp_str));

		LOG_ERR("Error in network beacon start/stop: err %s (%d)", tmp_str,
			evt_data->status);
		if (ft_nw_beacon_state == CTRL_FT_NW_BEACON_STATE_STOPPING) {
			dect_mgmt_nw_beacon_stop_evt(
				iface,
				dect_mdm_utils_modem_status_to_net_mgmt_status(evt_data->status));
			CTRL_DATA_LOCK();
			ctrl_data.ft_nw_beacon_state = CTRL_FT_NW_BEACON_STATE_NONE;
			CTRL_DATA_UNLOCK();
		} else if (ft_nw_beacon_state == CTRL_FT_NW_BEACON_STATE_STARTING) {
			dect_mgmt_nw_beacon_start_evt(
				iface,
				dect_mdm_utils_modem_status_to_net_mgmt_status(evt_data->status));

			/* Eventhough nw beacon failed, cluster is still running*/
			if (ft_network_state == CTRL_FT_NETWORK_STATE_STARTING) {
				dect_mgmt_network_status_evt(
					iface,
					(struct dect_network_status_evt){
						.network_status = DECT_NETWORK_STATUS_CREATED,
					});
				LOG_WRN("Network beacon start failed, "
					"but cluster is still running");
			}
			CTRL_DATA_LOCK();
			ctrl_data.ft_network_state = CTRL_FT_NETWORK_STATE_CREATED;
			ctrl_data.ft_nw_beacon_state = CTRL_FT_NW_BEACON_STATE_NONE;
			CTRL_DATA_UNLOCK();
		}
	} else {
		if (ft_nw_beacon_state == CTRL_FT_NW_BEACON_STATE_STARTING) {
			LOG_INF("Network beacon started");
			CTRL_DATA_LOCK();
			ctrl_data.ft_nw_beacon_state = CTRL_FT_NW_BEACON_STATE_STARTED;
			ctrl_data.ft_network_state = CTRL_FT_NETWORK_STATE_CREATED;
			iface = ctrl_data.iface;
			CTRL_DATA_UNLOCK();
			dect_mgmt_nw_beacon_start_evt(iface, DECT_STATUS_OK);
			dect_mgmt_network_status_evt(
				iface, (struct dect_network_status_evt){
					       .network_status = DECT_NETWORK_STATUS_CREATED,
				       });
		} else {
			__ASSERT_NO_MSG(ft_nw_beacon_state == CTRL_FT_NW_BEACON_STATE_STOPPING);
			LOG_INF("Network beacon stopped");
			CTRL_DATA_LOCK();
			ctrl_data.ft_nw_beacon_state = CTRL_FT_NW_BEACON_STATE_NONE;
			iface = ctrl_data.iface;
			CTRL_DATA_UNLOCK();
			dect_mgmt_nw_beacon_stop_evt(iface, DECT_STATUS_OK);
		}
	}
}

static void handle_mdm_nw_beacon_stop(struct dect_mdm_common_op_event_msgq_item *event)
{
	char tmp_str[DECT_MDM_UTILS_STR_BUFF_SIZE] = {0};
	int err;
	struct nrf_modem_dect_mac_network_beacon_configure_params beacon_params = {
		.channel = 0,
		.num_additional_channels = 0,
	};

	LOG_INF("Stopping network beacon.");

	err = nrf_modem_dect_mac_network_beacon_configure(&beacon_params);
	if (err) {
		dect_mdm_utils_modem_mac_err_to_string(err, tmp_str, sizeof(tmp_str));
		LOG_ERR("nrf_modem_dect_mac_network_beacon_configure failed: %s "
			"(%d)",
			tmp_str, err);
		CTRL_DATA_LOCK();
		ctrl_data.ft_nw_beacon_state = CTRL_FT_NW_BEACON_STATE_NONE;
		struct net_if *iface = ctrl_data.iface;

		CTRL_DATA_UNLOCK();
		dect_mgmt_nw_beacon_stop_evt(iface, DECT_STATUS_OS_ERROR);
	} else {
		CTRL_DATA_LOCK();
		ctrl_data.ft_nw_beacon_state = CTRL_FT_NW_BEACON_STATE_STOPPING;
		CTRL_DATA_UNLOCK();
	}
}

static bool dect_mdm_ctrl_beacon_has_route_info_ie(
	struct nrf_modem_dect_mac_cluster_beacon_ntf_cb_params *params,
	struct nrf_modem_dect_mac_route_info_ie *route_info_ie_out)
{
	if (params->number_of_ies > 0) {
		for (int i = 0; i < params->number_of_ies; i++) {
			if (params->ies[i].ie_type == NRF_MODEM_DECT_MAC_IE_TYPE_ROUTE_INFO) {
				*route_info_ie_out = params->ies[i].ie.route_info;
				return true;
			}
		}
	}
	return false;
}

static void handle_mdm_cluster_beacon_rcvd(struct dect_mdm_common_op_event_msgq_item *event)
{
	struct nrf_modem_dect_mac_cluster_beacon_ntf_cb_params *params = event->data;
	struct dect_mdm_settings *set_ptr = dect_mdm_settings_ref_get();
	bool add_to_cluster_channels;
	dect_scan_result_cb_t scan_result_cb;
	struct net_if *iface;
	enum ctrl_pt_association_state pt_association_state;
	bool auto_start;
	struct dect_route_info route_info;
	bool has_route_info = false;

	CTRL_DATA_LOCK();
	add_to_cluster_channels = ctrl_data.scan_data.on_going;

	/* Did we already mark this to cluster_channels? */
	for (int i = 0;
	     i < DECT_MDM_CTRL_CLUSTER_SCAN_DATA_MAX_CHANNELS && add_to_cluster_channels; i++) {
		if (ctrl_data.scan_data.cluster_channels[i].channel == params->channel) {
			add_to_cluster_channels = false;
			break;
		}
	}
	/* Store channel and RSSI: used by FT when creating a cluster and
	 * when PT selecting RD for association.
	 */

	if (add_to_cluster_channels) {
		/* Check if we got route info in the beacon */
		struct nrf_modem_dect_mac_route_info_ie route_info_ie = {0};

		ctrl_data.scan_data.cluster_channels[
			ctrl_data.scan_data.current_cluster_channel_index].has_route_info =
			dect_mdm_ctrl_beacon_has_route_info_ie(params, &route_info_ie);
		if (ctrl_data.scan_data.cluster_channels[
				ctrl_data.scan_data.current_cluster_channel_index].has_route_info) {
			ctrl_data.scan_data.cluster_channels[
				ctrl_data.scan_data.current_cluster_channel_index]
					.route_info_ie = route_info_ie;
			route_info.route_cost = route_info_ie.route_cost;
			route_info.application_sequence_number = route_info_ie
				.application_sequence_number;
			route_info.sink_address = route_info_ie.sink_address;
			has_route_info = true;
		}

		ctrl_data.scan_data
			.cluster_channels[ctrl_data.scan_data.current_cluster_channel_index]
			.channel = params->channel;
		ctrl_data.scan_data
			.cluster_channels[ctrl_data.scan_data.current_cluster_channel_index]
			.rssi_2 = params->rx_signal_info.rssi_2;
		ctrl_data.scan_data
			.cluster_channels[ctrl_data.scan_data.current_cluster_channel_index]
			.long_rd_id = params->transmitter_long_rd_id;

		ctrl_data.scan_data.current_cluster_channel_index++;
		__ASSERT_NO_MSG(ctrl_data.scan_data.current_cluster_channel_index <
				ARRAY_SIZE(ctrl_data.scan_data.cluster_channels));
	}
	scan_result_cb = ctrl_data.scan_data.scan_result_cb;
	iface = ctrl_data.iface;
	pt_association_state = ctrl_data.ass_config.pt_association_state;
	auto_start = ctrl_data.configure_params.auto_start;
	CTRL_DATA_UNLOCK();

	if (scan_result_cb) {
		struct dect_scan_result_evt scan_result = {
			.beacon_type = DECT_SCAN_RESULT_TYPE_CLUSTER_BEACON,
			.channel = params->channel,
			.transmitter_short_rd_id = params->transmitter_short_rd_id,
			.transmitter_long_rd_id = params->transmitter_long_rd_id,
			.network_id = params->network_id,
			.rx_signal_info.mcs = params->rx_signal_info.mcs,
			.rx_signal_info.transmit_power = params->rx_signal_info.transmit_power,
			.rx_signal_info.rssi_2 = params->rx_signal_info.rssi_2,
			.rx_signal_info.snr = params->rx_signal_info.snr,
			.has_route_info = has_route_info,
			.route_info = route_info,
		};

		scan_result_cb(iface, 0, &scan_result);
	}
	if (!(set_ptr->net_mgmt_common.device_type & DECT_DEVICE_TYPE_PT)) {
		/* Cluster beacon can be also received if FT device and
		 * if same short nw id and in same channel
		 */
		LOG_DBG("Cluster beacon received, but not PT device!!!");
		return;
	}
	if (pt_association_state != CTRL_PT_ASSOCIATION_STATE_ASSOCIATED &&
	    dect_mdm_utils_cluster_acceptable_for_association(params)) {
		CTRL_DATA_LOCK();
		ctrl_data.ass_config.pt_association_state =
			CTRL_PT_ASSOCIATION_STATE_CLUSTER_BEACON_RECEIVED;
		ctrl_data.ass_config.network_id = params->network_id;
		ctrl_data.ass_config.parent_long_rd_id = params->transmitter_long_rd_id;
		CTRL_DATA_UNLOCK();

		if (auto_start) {
			if (nrf_modem_dect_mac_network_scan_stop() != 0) {
				LOG_ERR("OP_MDM_CLUSTER_BEACON_RCVD: error in "
					"Network scan stop!");
			} else {
				LOG_DBG("nw scan stopping requested");
			}
		}
	}
	CTRL_DATA_LOCK();
	pt_association_state = ctrl_data.ass_config.pt_association_state;
	CTRL_DATA_UNLOCK();
	if (pt_association_state == CTRL_PT_ASSOCIATION_STATE_ASSOCIATED &&
	    set_ptr->net_mgmt_common.association.min_sensitivity_dbm >=
		    params->rx_signal_info.rssi_2) {
		LOG_WRN("Signal (rssi_2 %d) is below the minimum "
			"sensitivity threshold (%d)",
			params->rx_signal_info.rssi_2,
			set_ptr->net_mgmt_common.association.min_sensitivity_dbm);
	}
}

static void handle_mdm_nw_beacon_rcvd(struct dect_mdm_common_op_event_msgq_item *event)
{
	int err;
	struct nrf_modem_dect_mac_network_beacon_ntf_cb_params *params = event->data;
	struct dect_mdm_settings *set_ptr = dect_mdm_settings_ref_get();
	dect_scan_result_cb_t scan_result_cb;
	struct net_if *iface;
	enum ctrl_pt_association_state pt_association_state;
	bool auto_start;
	uint32_t parent_long_rd_id;

	LOG_DBG("Network beacon information:");
	LOG_DBG("  network id (32bit).............................%u (0x%08x)", params->network_id,
		params->network_id);
	LOG_DBG("  transmitter id (long RD ID)....................%u (0x%08x)",
		params->transmitter_long_rd_id, params->transmitter_long_rd_id);
	LOG_DBG("  short RD ID....................................%u (0x%04x)",
		params->transmitter_short_rd_id, params->transmitter_short_rd_id);
	LOG_DBG("  channel number.................................%d", params->channel);
	LOG_DBG("  next_cluster_channel number....................%d",
		params->beacon.next_cluster_channel);

	CTRL_DATA_LOCK();
	scan_result_cb = ctrl_data.scan_data.scan_result_cb;
	iface = ctrl_data.iface;
	CTRL_DATA_UNLOCK();

	if (scan_result_cb) {
		struct dect_scan_result_evt scan_result = {
			.beacon_type = DECT_SCAN_RESULT_TYPE_NW_BEACON,
			.channel = params->channel,
			.transmitter_short_rd_id = params->transmitter_short_rd_id,
			.transmitter_long_rd_id = params->transmitter_long_rd_id,
			.network_id = params->network_id,
			.rx_signal_info.mcs = params->rx_signal_info.mcs,
			.rx_signal_info.transmit_power = params->rx_signal_info.transmit_power,
			.rx_signal_info.rssi_2 = params->rx_signal_info.rssi_2,
			.rx_signal_info.snr = params->rx_signal_info.snr,
		};
		struct dect_network_beacon_data nw_beacon = {
			.next_cluster_channel = params->beacon.next_cluster_channel,
			.current_cluster_channel = params->beacon.current_cluster_channel,
			.num_network_beacon_channels = params->beacon.num_network_beacon_channels,
		};

		for (int i = 0; i < params->beacon.num_network_beacon_channels; i++) {
			nw_beacon.network_beacon_channels[i] =
				params->beacon.network_beacon_channels[i];
		}
		scan_result.network_beacon = nw_beacon;
		scan_result_cb(iface, 0, &scan_result);
	}

	CTRL_DATA_LOCK();
	pt_association_state = ctrl_data.ass_config.pt_association_state;
	auto_start = ctrl_data.configure_params.auto_start;
	CTRL_DATA_UNLOCK();

	if ((set_ptr->net_mgmt_common.device_type & DECT_DEVICE_TYPE_PT) &&
	    pt_association_state != CTRL_PT_ASSOCIATION_STATE_ASSOCIATED) {
		CTRL_DATA_LOCK();
		ctrl_data.ass_config.pt_association_state = CTRL_PT_ASSOCIATION_STATE_CLUSTER_FOUND;
		ctrl_data.ass_config.network_id = params->network_id;
		ctrl_data.ass_config.parent_long_rd_id = params->transmitter_long_rd_id;
		parent_long_rd_id = ctrl_data.ass_config.parent_long_rd_id;
		CTRL_DATA_UNLOCK();

		if (auto_start && (set_ptr->net_mgmt_common.network_join.target_ft_long_rd_id ==
					   DECT_SETT_NETWORK_JOIN_TARGET_FT_ANY ||
				   set_ptr->net_mgmt_common.network_join.target_ft_long_rd_id ==
					   parent_long_rd_id)) {
			err = nrf_modem_dect_mac_network_scan_stop();
			if (err) {
				LOG_ERR("%s: error in stopping nw scan, err: %d", (__func__), err);
			}
		}
	}
}

static void handle_mdm_nw_scan_complete(struct dect_mdm_common_op_event_msgq_item *event)
{
	char tmp_str[DECT_MDM_UTILS_STR_BUFF_SIZE] = {0};
	int err;
	struct nrf_modem_dect_mac_network_scan_cb_params *evt_data =
		(struct nrf_modem_dect_mac_network_scan_cb_params *)event->data;
	struct dect_mdm_settings *set_ptr = dect_mdm_settings_ref_get();

	if (evt_data->status != NRF_MODEM_DECT_MAC_STATUS_OK) {
		dect_mdm_utils_modem_mac_err_to_string(
			evt_data->status, tmp_str, sizeof(tmp_str));

		LOG_ERR("Network scan completed with err %s (%d)", tmp_str, evt_data->status);
	} else {
		LOG_INF("Network scan completed: %d channels scanned",
			evt_data->num_scanned_channels);
	}
	CTRL_DATA_LOCK();
	ctrl_data.scan_data.on_going = false;
	dect_scan_result_cb_t scan_result_cb = ctrl_data.scan_data.scan_result_cb;
	struct net_if *iface = ctrl_data.iface;
	bool auto_start = ctrl_data.configure_params.auto_start;
	enum ctrl_pt_association_state pt_association_state =
		ctrl_data.ass_config.pt_association_state;
	uint32_t parent_long_rd_id = ctrl_data.ass_config.parent_long_rd_id;
	uint32_t network_id = ctrl_data.ass_config.network_id;
	uint16_t ft_requested_cluster_channel = ctrl_data.ft_requested_cluster_channel;

	CTRL_DATA_UNLOCK();
	if (scan_result_cb) {
		scan_result_cb(iface,
			       dect_mdm_utils_modem_status_to_net_mgmt_status(evt_data->status),
			       NULL);
		CTRL_DATA_LOCK();
		ctrl_data.scan_data.scan_result_cb = NULL;
		CTRL_DATA_UNLOCK();
	}
	if (!auto_start) {
		/* We are done here */
		return;
	}
	if (set_ptr->net_mgmt_common.device_type & DECT_DEVICE_TYPE_PT) {
		if (pt_association_state == CTRL_PT_ASSOCIATION_STATE_CLUSTER_FOUND) {
			LOG_INF("auto start: sending cluster beacon receive req, "
				"Long RD ID: 0x%X, NW ID: 0x%X",
				parent_long_rd_id, network_id);

			struct nrf_modem_dect_mac_cluster_beacon_config cluster_config = {
				.long_rd_id = parent_long_rd_id,
				.network_id = network_id,
			};
			struct nrf_modem_dect_mac_cluster_beacon_receive_params params = {
				.num_configs = 1,
				.configs = &cluster_config,
			};

			err = nrf_modem_dect_mac_cluster_beacon_receive(&params);
			if (err) {
				LOG_ERR("Error in Cluster beacon receive: err %d", err);
			}
		} else {
			CTRL_DATA_LOCK();
			pt_association_state = ctrl_data.ass_config.pt_association_state;
			CTRL_DATA_UNLOCK();
			if (pt_association_state ==
			    CTRL_PT_ASSOCIATION_STATE_CLUSTER_BEACON_RECEIVED) {
				dect_mdm_ctrl_trigger_association();
			} else {
				struct dect_mdm_ctrl_cluster_channel_list_item best_channel_data;
				bool best_channel_found =
					dect_mdm_ctrl_cluster_channels_list_best_channel_get(
						&best_channel_data);

				/* MAC spec 5.1.4:
				 * If none of the detected cluster beacons meets
				 * the minimum quality level, the RD may:
				 * - initiate association to the RD providing
				 * the highest RSSI-2 value
				 */
				if (best_channel_found) {
					LOG_INF("Nw scan done: selecting best channel: "
						"Long RD ID: 0x%X, RSSI-2: %d",
						best_channel_data.long_rd_id,
						best_channel_data.rssi_2);
					CTRL_DATA_LOCK();
					ctrl_data.ass_config.pt_association_state =
						CTRL_PT_ASSOCIATION_STATE_CLUSTER_BEACON_RECEIVED;
					ctrl_data.ass_config.network_id =
						set_ptr->net_mgmt_common.identities.network_id;
					ctrl_data.ass_config.parent_long_rd_id =
						best_channel_data.long_rd_id;
					CTRL_DATA_UNLOCK();
					dect_mdm_ctrl_trigger_association();
				} else {
					CTRL_DATA_LOCK();
					struct net_if *iface_local = ctrl_data.iface;

					ctrl_data.configure_params.auto_start = false;
					CTRL_DATA_UNLOCK();
					dect_mgmt_network_status_evt(
						iface_local,
						(struct dect_network_status_evt){
							.network_status =
								DECT_NETWORK_STATUS_FAILURE,
							.dect_err_cause =
								DECT_STATUS_RD_NOT_FOUND,
						});
					LOG_WRN("No cluster found with NW scan");
				}
			}
		}
	} else {
		/* As a default RSSI params from settings  */
		struct nrf_modem_dect_mac_rssi_scan_params rssi_params;

		__ASSERT_NO_MSG(set_ptr->net_mgmt_common.device_type & DECT_DEVICE_TYPE_FT);

		dect_mdm_ctrl_build_rssi_scan_params(&rssi_params, set_ptr,
						       ft_requested_cluster_channel, false);

		dect_mdm_ctrl_internal_msgq_data_op_add(
			DECT_MDM_CTRL_OP_RSSI_START_REQ_CH_SELECTION, &rssi_params,
			sizeof(struct nrf_modem_dect_mac_rssi_scan_params));
	}
}

static void handle_mdm_nw_scan_stopped(struct dect_mdm_common_op_event_msgq_item *event)
{
	char tmp_str[DECT_MDM_UTILS_STR_BUFF_SIZE] = {0};
	enum nrf_modem_dect_mac_err *status = (enum nrf_modem_dect_mac_err *)event->data;

	if (*status != NRF_MODEM_DECT_MAC_STATUS_OK) {
		dect_mdm_utils_modem_mac_err_to_string(*status, tmp_str, sizeof(tmp_str));
		LOG_ERR("Network scan stopping failed with err %s (%d)", tmp_str, *status);
		return;
	}
	LOG_INF("Network scan stopped");
}

static void handle_mdm_cluster_rcv_complete(struct dect_mdm_common_op_event_msgq_item *event)
{
	char tmp_str[DECT_MDM_UTILS_STR_BUFF_SIZE] = {0};
	enum nrf_modem_dect_mac_err *status = (enum nrf_modem_dect_mac_err *)event->data;

	if (*status != NRF_MODEM_DECT_MAC_STATUS_OK) {
		dect_mdm_utils_modem_mac_err_to_string(*status, tmp_str, sizeof(tmp_str));
		LOG_ERR("Cluster beacon rcv failed with err %s (%d)", tmp_str, *status);
		return;
	}
	LOG_INF("Cluster beacon reception completed");

	CTRL_DATA_LOCK();
	bool auto_start = ctrl_data.configure_params.auto_start;
	enum ctrl_pt_association_state pt_association_state =
		ctrl_data.ass_config.pt_association_state;
	CTRL_DATA_UNLOCK();

	if (auto_start) {
		if (pt_association_state == CTRL_PT_ASSOCIATION_STATE_CLUSTER_BEACON_RECEIVED) {
			dect_mdm_ctrl_trigger_association();
		} else {
			LOG_WRN("No cluster found with Cluster beacon receive");
		}
	}
}

static void handle_mdm_cluster_info(struct dect_mdm_common_op_event_msgq_item *event)
{
	char tmp_str[DECT_MDM_UTILS_STR_BUFF_SIZE] = {0};
	struct nrf_modem_dect_mac_cluster_info_cb_params *evt_data = event->data;
	struct dect_cluster_info_evt l2_results_evt = {
		.status = dect_mdm_utils_modem_status_to_net_mgmt_status(evt_data->status),
	};
	struct dect_cluster_status_info *cluster_info = &l2_results_evt.status_info;
	struct dect_rssi_scan_result_data *rssi_result = &cluster_info->rssi_result;

	if (evt_data->status != NRF_MODEM_DECT_MAC_STATUS_OK) {
		dect_mdm_utils_modem_mac_err_to_string(
			evt_data->status, tmp_str, sizeof(tmp_str));
		LOG_ERR("Cluster info rcv failed with err %s (%d)", tmp_str, evt_data->status);
	} else {
		cluster_info->num_association_failures = evt_data->info.num_association_failures;
		cluster_info->num_association_requests = evt_data->info.num_association_requests;
		cluster_info->num_neighbors = evt_data->info.num_neighbors;
		cluster_info->num_ftpt_neighbors = evt_data->info.num_ftpt_neighbors;
		cluster_info->num_rach_rx_pdc = evt_data->info.num_rach_rx_pdc;
		cluster_info->num_rach_rx_pcc_crc_failures =
			evt_data->info.num_rach_rx_pcc_crc_failures;
		cluster_info->rssi_result.busy_percentage =
			evt_data->info.rssi_result.busy_percentage;

		LOG_DBG("Cluster status information:");
		LOG_DBG("  num_association_requests.......................%u",
			evt_data->info.num_association_requests);
		LOG_DBG("  num_association_failures.......................%u",
			evt_data->info.num_association_failures);
		LOG_DBG("  num_neighbors..................................%u",
			evt_data->info.num_neighbors);
		LOG_DBG("  num_ftpt_neighbors.............................%u",
			evt_data->info.num_ftpt_neighbors);
		LOG_DBG("  num_rach_rx_pdc................................%u",
			evt_data->info.num_rach_rx_pdc);
		LOG_DBG("  num_rach_rx_pcc_crc_fail.......................%u",
			evt_data->info.num_rach_rx_pcc_crc_failures);
		LOG_DBG("  channel busy percentage........................%u",
			evt_data->info.rssi_result.busy_percentage);

		struct nrf_modem_dect_mac_rssi_result *mdm_rssi_res = &evt_data->info.rssi_result;
		int err = dect_mdm_utils_mdm_rssi_results_to_l2_rssi_data(mdm_rssi_res,
									    rssi_result);

		if (err) {
			LOG_ERR("Error in converting RSSI results to L2 data: %d", err);
		}
	}
	/* Send L2 evt */
	CTRL_DATA_LOCK();
	struct net_if *iface = ctrl_data.iface;

	CTRL_DATA_UNLOCK();
	dect_mgmt_cluster_info_evt(iface, l2_results_evt);
}

static void handle_mdm_neighbor_info(struct dect_mdm_common_op_event_msgq_item *event)
{
	char tmp_str[DECT_MDM_UTILS_STR_BUFF_SIZE] = {0};
	struct nrf_modem_dect_mac_neighbor_info_cb_params *evt_data = event->data;
	struct dect_neighbor_info_evt l2_results_evt = {
		.long_rd_id = evt_data->long_rd_id,
		.status = dect_mdm_utils_modem_status_to_net_mgmt_status(evt_data->status),
	};

	if (evt_data->status != NRF_MODEM_DECT_MAC_STATUS_OK) {
		dect_mdm_utils_modem_mac_err_to_string(
			evt_data->status, tmp_str, sizeof(tmp_str));
		LOG_ERR("Neighbor info rcv failed with err %s (%d)", tmp_str, evt_data->status);
	} else {
		LOG_DBG("Neighbor status information:");
		LOG_DBG("  Neighbor (long RD ID).......................%u (0x%08x)",
			evt_data->long_rd_id, evt_data->long_rd_id);
		LOG_DBG("  Network ID..................................%u (0x%08x)",
			evt_data->network_id, evt_data->network_id);
		LOG_DBG("  Associated..................................%s",
			evt_data->associated ? "true" : "false");
		LOG_DBG("  FT mode.....................................%s",
			evt_data->ft_mode ? "true" : "false");
		LOG_DBG("  Channel.....................................%d", evt_data->channel);
		LOG_DBG("  Time in ms since neighbor is last seen......%u ms",
			evt_data->time_since_last_rx_ms);
		LOG_DBG("  RX mcs......................................%u",
			evt_data->last_rx_signal_info.mcs);
		LOG_DBG("  RX transmit power...........................%d",
			evt_data->last_rx_signal_info.transmit_power);
		LOG_DBG("  RX RSSI 2...................................%d",
			evt_data->last_rx_signal_info.rssi_2);
		LOG_DBG("  RX SNR.....................................%d",
			evt_data->last_rx_signal_info.snr);
		LOG_DBG("  beacon_average_rx_rssi_2..................%d",
			evt_data->beacon_average_rx_rssi_2);
		LOG_DBG("  beacon_average_rx_snr.......................%d",
			evt_data->beacon_average_rx_snr);
		l2_results_evt.network_id = evt_data->network_id;
		l2_results_evt.associated = evt_data->associated;
		l2_results_evt.ft_mode = evt_data->ft_mode;
		l2_results_evt.channel = evt_data->channel;
		l2_results_evt.time_since_last_rx_ms = evt_data->time_since_last_rx_ms;
		l2_results_evt.last_rx_signal_info.mcs = evt_data->last_rx_signal_info.mcs;
		l2_results_evt.last_rx_signal_info.transmit_power =
			evt_data->last_rx_signal_info.transmit_power;
		l2_results_evt.last_rx_signal_info.rssi_2 = evt_data->last_rx_signal_info.rssi_2;
		l2_results_evt.last_rx_signal_info.snr = evt_data->last_rx_signal_info.snr;
		l2_results_evt.beacon_average_rx_txpower = evt_data->beacon_average_rx_txpower;
		l2_results_evt.beacon_average_rx_rssi_2 = evt_data->beacon_average_rx_rssi_2;
		l2_results_evt.beacon_average_rx_snr = evt_data->beacon_average_rx_snr;

		LOG_DBG("  total_missed_cluster_beacons................%u",
			evt_data->status_info.total_missed_cluster_beacons);
		LOG_DBG("  current_consecutive_missed_cluster_beacons..%u",
			evt_data->status_info.current_consecutive_missed_cluster_beacons);
		LOG_DBG("  num_rx_paging...............................%u",
			evt_data->status_info.num_rx_paging);
		LOG_DBG("  average_rx_mcs..............................%u",
			evt_data->status_info.average_rx_mcs);
		LOG_DBG("  average_rx_txpower..........................%d",
			evt_data->status_info.average_rx_txpower);
		LOG_DBG("  average_rx_rssi_2...........................%d",
			evt_data->status_info.average_rx_rssi_2);
		LOG_DBG("  average_rx_snr..............................%d",
			evt_data->status_info.average_rx_snr);
		LOG_DBG("  average_tx_mcs..............................%u",
			evt_data->status_info.average_tx_mcs);
		LOG_DBG("  average_tx_txpower..........................%d",
			evt_data->status_info.average_tx_txpower);
		LOG_DBG("  num_tx_attempts.............................%u",
			evt_data->status_info.num_tx_attempts);
		LOG_DBG("  num_lbt_failures............................%u",
			evt_data->status_info.num_lbt_failures);
		LOG_DBG("  num_rx_pdc..................................%u",
			evt_data->status_info.num_rx_pdc);
		LOG_DBG("  num_rx_pdc_crc_fail.........................%u",
			evt_data->status_info.num_rx_pdc_crc_failures);
		LOG_DBG("  num_no_response.............................%u",
			evt_data->status_info.num_no_response);
		LOG_DBG("  num_harq_ack................................%u",
			evt_data->status_info.num_harq_ack);
		LOG_DBG("  num_harq_nack...............................%u",
			evt_data->status_info.num_harq_nack);
		LOG_DBG("  num_arq_retx................................%u",
			evt_data->status_info.num_arq_retx);
		LOG_DBG("  inactive_time_ms............................%u ms",
			evt_data->status_info.inactive_time_ms);

		l2_results_evt.status_info.total_missed_cluster_beacons =
			evt_data->status_info.total_missed_cluster_beacons;
		l2_results_evt.status_info.current_consecutive_missed_cluster_beacons =
			evt_data->status_info.current_consecutive_missed_cluster_beacons;
		l2_results_evt.status_info.num_rx_paging = evt_data->status_info.num_rx_paging;
		l2_results_evt.status_info.average_rx_mcs = evt_data->status_info.average_rx_mcs;
		l2_results_evt.status_info.average_rx_txpower =
			evt_data->status_info.average_rx_txpower;
		l2_results_evt.status_info.average_rx_rssi_2 =
			evt_data->status_info.average_rx_rssi_2;
		l2_results_evt.status_info.average_rx_snr = evt_data->status_info.average_rx_snr;
		l2_results_evt.status_info.average_tx_mcs = evt_data->status_info.average_tx_mcs;
		l2_results_evt.status_info.average_tx_txpower =
			evt_data->status_info.average_tx_txpower;
		l2_results_evt.status_info.num_tx_attempts = evt_data->status_info.num_tx_attempts;
		l2_results_evt.status_info.num_lbt_failures =
			evt_data->status_info.num_lbt_failures;
		l2_results_evt.status_info.num_rx_pdc = evt_data->status_info.num_rx_pdc;
		l2_results_evt.status_info.num_rx_pdc_crc_failures =
			evt_data->status_info.num_rx_pdc_crc_failures;
		l2_results_evt.status_info.num_no_response = evt_data->status_info.num_no_response;
		l2_results_evt.status_info.num_harq_ack = evt_data->status_info.num_harq_ack;
		l2_results_evt.status_info.num_harq_nack = evt_data->status_info.num_harq_nack;
		l2_results_evt.status_info.num_arq_retx = evt_data->status_info.num_arq_retx;
		l2_results_evt.status_info.inactive_time_ms =
			evt_data->status_info.inactive_time_ms;
	}

	/* Send L2 evt */
	CTRL_DATA_LOCK();
	struct net_if *iface = ctrl_data.iface;

	CTRL_DATA_UNLOCK();
	dect_mgmt_neighbor_info_evt(iface, l2_results_evt);
}

static void handle_mdm_neighbor_list(struct dect_mdm_common_op_event_msgq_item *event)
{
	char tmp_str[DECT_MDM_UTILS_STR_BUFF_SIZE] = {0};
	struct dect_mdm_ctrl_neighbor_list_resp_evt *evt_data =
		(struct dect_mdm_ctrl_neighbor_list_resp_evt *)event->data;
	struct dect_neighbor_list_evt l2_results_evt;

	l2_results_evt.neighbor_count = evt_data->num_neighbors;
	l2_results_evt.status = dect_mdm_utils_modem_status_to_net_mgmt_status(evt_data->status);

	if (evt_data->status != NRF_MODEM_DECT_MAC_STATUS_OK) {
		dect_mdm_utils_modem_mac_err_to_string(evt_data->status, tmp_str, sizeof(tmp_str));
		LOG_ERR("Neighbor list rcv failed with err %s (%d)", tmp_str, evt_data->status);

		l2_results_evt.neighbor_count = 0;
		CTRL_DATA_LOCK();
		struct net_if *iface = ctrl_data.iface;

		CTRL_DATA_UNLOCK();
		dect_mgmt_neighbor_list_evt(iface, l2_results_evt);
		return;
	}

	LOG_DBG("Neighbors:");
	LOG_DBG("  num_neighbors..................................%u", evt_data->num_neighbors);
	for (uint32_t i = 0; i < evt_data->num_neighbors; i++) {
		LOG_DBG("  Neighbor (long RD ID)..........................%u "
			"(0x%08x)",
			evt_data->neighbor_long_rd_ids[i], evt_data->neighbor_long_rd_ids[i]);
		if (i >= DECT_L2_MAX_NEIGHBOR_LIST_ITEM_COUNT) {
			LOG_ERR("Neighbor list too long, max is %d",
				DECT_L2_MAX_NEIGHBOR_LIST_ITEM_COUNT);
			l2_results_evt.neighbor_count = DECT_L2_MAX_NEIGHBOR_LIST_ITEM_COUNT;
			break;
		}
		l2_results_evt.neighbor_long_rd_ids[i] = evt_data->neighbor_long_rd_ids[i];
	}
	CTRL_DATA_LOCK();
	struct net_if *iface = ctrl_data.iface;

	CTRL_DATA_UNLOCK();
	dect_mgmt_neighbor_list_evt(iface, l2_results_evt);
}

static void handle_mdm_dlc_data_resp(struct dect_mdm_common_op_event_msgq_item *event)
{
	char tmp_str[DECT_MDM_UTILS_STR_BUFF_SIZE] = {0};
#if defined(CONFIG_DECT_MDM_NRF_TX_FLOW_CTRL_BASED_ON_MDM_TX_DLC_REQS)
	struct dect_mdm_ctrl_dlc_data_tx_resp_evt *evt_data = event->data;
	int arr_index;

	if (evt_data->status != NRF_MODEM_DECT_MAC_STATUS_OK) {
		dect_mdm_utils_modem_mac_err_to_string(evt_data->status, tmp_str, sizeof(tmp_str));
		LOG_ERR("DLC data TX response failed to rd id %u with err %s (%d), "
			"transaction id %u",
			evt_data->long_rd_id, tmp_str, evt_data->status,
			evt_data->acked_data[0].transaction_id);
	}

	/* Update total_unacked_tx_data_amount (even if is error) */
	CTRL_DATA_LOCK();
	for (int i = 0; i < evt_data->num_acked_data; i++) {
		arr_index = evt_data->acked_data[i].transaction_id - DECT_MDM_DATA_TX_HANDLE_START;
		if (arr_index < 0 || arr_index >= DECT_MDM_DLC_DATA_INFO_MAX_COUNT) {
			LOG_ERR("Transaction ID %d not found in array",
				evt_data->acked_data[i].transaction_id);
			continue;
		}
		ctrl_data.dlc_data_tx_infos[arr_index].req_on_going = false;
		ctrl_data.total_unacked_tx_data_amount -=
			ctrl_data.dlc_data_tx_infos[arr_index].data_len;
		ctrl_data.total_unacked_req_amount--;
	}
	uint32_t total_unacked_tx_data_amount = ctrl_data.total_unacked_tx_data_amount;
	uint16_t total_unacked_req_amount = ctrl_data.total_unacked_req_amount;

	CTRL_DATA_UNLOCK();
	LOG_DBG("DLC data response (towards RD ID %u): "
		"total %d bytes unacked left, total req count %d",
		evt_data->long_rd_id, total_unacked_tx_data_amount, total_unacked_req_amount);
#endif
}

static void dect_mdm_ctrl_msgq_thread_handler(void)
{
	struct dect_mdm_common_op_event_msgq_item event;

	while (true) {
		k_msgq_get(&dect_mdm_ctrl_msgq, &event, K_FOREVER);

		switch (event.id) {
		case DECT_MDM_CTRL_OP_MDM_CAPABILITIES:
			handle_mdm_capabilities(&event);
			break;
		case DECT_MDM_CTRL_OP_MDM_CONFIGURE_RESP:
			handle_mdm_configure_resp(&event);
			break;
		case DECT_MDM_CTRL_OP_MDM_CFUN_RESP:
			handle_mdm_cfun_resp(&event);
			break;
		case DECT_MDM_CTRL_OP_MDM_ACTIVATED:
			handle_mdm_activated(&event);
			break;
		case DECT_MDM_CTRL_OP_MDM_DEACTIVATED:
			handle_mdm_deactivated(&event);
			break;
		case DECT_MDM_CTRL_OP_CLUSTER_RECONFIG_REQ:
			handle_cluster_reconfig_req(&event);
			break;
		case DECT_MDM_CTRL_OP_CLUSTER_IPV6_PREFIX_CHANGE_RECONFIG_REQ:
			handle_cluster_ipv6_prefix_change_reconfig_req(&event);
			break;
		case DECT_MDM_CTRL_OP_CLUSTER_START_REQ:
			handle_cluster_start_req(&event);
			break;
		case DECT_MDM_CTRL_OP_CLUSTER_CONFIG_RESP:
			handle_cluster_config_resp(&event);
			break;
		case DECT_MDM_CTRL_OP_CLUSTER_CH_LOAD_CHANGED:
			handle_cluster_ch_load_changed(&event);
			break;
		case DECT_MDM_CTRL_OP_NEIGHBOR_INACTIVITY:
			handle_neighbor_inactivity(&event);
			break;
		case DECT_MDM_CTRL_OP_AUTO_START:
			handle_auto_start(&event);
			break;
		case DECT_MDM_CTRL_OP_RSSI_START_REQ_CMD:
			handle_rssi_start_req_cmd(&event);
			break;
		case DECT_MDM_CTRL_OP_RSSI_START_REQ_CH_SELECTION:
			handle_rssi_start_req_ch_selection(&event);
			break;
		case DECT_MDM_CTRL_OP_MDM_RSSI_RESULT:
			handle_mdm_rssi_result(&event);
			break;
		case DECT_MDM_CTRL_OP_MDM_RSSI_COMPLETE:
			handle_mdm_rssi_complete(&event);
			break;
		case DECT_MDM_CTRL_OP_MDM_RSSI_STOPPED:
			handle_mdm_rssi_stopped(&event);
			break;
		case DECT_MDM_CTRL_OP_MDM_NW_BEACON_START:
			handle_mdm_nw_beacon_start(&event);
			break;
		case DECT_MDM_CTRL_OP_MDM_NW_BEACON_START_OR_STOP_DONE:
			handle_mdm_nw_beacon_start_or_stop_done(&event);
			break;
		case DECT_MDM_CTRL_OP_MDM_NW_BEACON_STOP:
			handle_mdm_nw_beacon_stop(&event);
			break;
		case DECT_MDM_CTRL_OP_MDM_CLUSTER_BEACON_RCVD:
			handle_mdm_cluster_beacon_rcvd(&event);
			break;
		case DECT_MDM_CTRL_OP_MDM_NW_BEACON_RCVD:
			handle_mdm_nw_beacon_rcvd(&event);
			break;
		case DECT_MDM_CTRL_OP_MDM_NW_SCAN_COMPLETE:
			handle_mdm_nw_scan_complete(&event);
			break;
		case DECT_MDM_CTRL_OP_MDM_NW_SCAN_STOPPED:
			handle_mdm_nw_scan_stopped(&event);
			break;
		case DECT_MDM_CTRL_OP_MDM_CLUSTER_RCV_COMPLETE:
			handle_mdm_cluster_rcv_complete(&event);
			break;
		case DECT_MDM_CTRL_OP_MDM_CLUSTER_INFO:
			handle_mdm_cluster_info(&event);
			break;
		case DECT_MDM_CTRL_OP_CLUSTER_BEACON_RX_FAILURE:
			handle_cluster_beacon_rx_failure(&event);
			break;
		case DECT_MDM_CTRL_OP_NEIGHBOR_PAGING_FAILURE:
			handle_neighbor_paging_failure(&event);
			break;
		case DECT_MDM_CTRL_OP_MDM_NEIGHBOR_INFO:
			handle_mdm_neighbor_info(&event);
			break;
		case DECT_MDM_CTRL_OP_MDM_NEIGHBOR_LIST:
			handle_mdm_neighbor_list(&event);
			break;
		case DECT_MDM_CTRL_OP_MDM_FLOW_CONTROL:
			handle_mdm_flow_control(&event);
			break;
		case DECT_MDM_CTRL_OP_MDM_DLC_DATA_RESP:
			handle_mdm_dlc_data_resp(&event);
			break;
		case DECT_MDM_CTRL_OP_MDM_ASSOCIATION_IND:
			handle_mdm_association_ind(&event);
			break;
		case DECT_MDM_CTRL_OP_MDM_ASSOCIATION_RESP:
			handle_mdm_association_resp(&event);
			break;
		case DECT_MDM_CTRL_OP_MDM_ASSOCIATION_RELEASE_RESP:
			handle_mdm_association_release_resp(&event);
			break;
		case DECT_MDM_CTRL_OP_MDM_ASSOCIATION_RELEASE_IND:
			handle_mdm_association_release_ind(&event);
			break;
		case DECT_MDM_CTRL_OP_MDM_IPV6_CONFIG_CHANGED:
			handle_mdm_ipv6_config_changed(&event);
			break;

		default:
			LOG_WRN("DECT NRF91 CTRL: Unknown event %u received", event.id);
			break;
		}
		/* Free event data if it was allocated (non-NULL for data ops) */
		if (event.data != NULL) {
			k_free(event.data);
		}
	}
}

#define DECT_MDM_CTRL_STACK_SIZE CONFIG_DECT_MDM_NRF_CTRL_THREAD_STACK_SIZE
#define DECT_MDM_CTRL_PRIORITY   K_PRIO_COOP(9) /* -7 */

K_THREAD_DEFINE(dect_mdm_ctrl_msgq_th, DECT_MDM_CTRL_STACK_SIZE,
		dect_mdm_ctrl_msgq_thread_handler, NULL, NULL, NULL, DECT_MDM_CTRL_PRIORITY, 0,
		0);
int dect_mdm_ctrl_internal_init(struct net_if *iface)
{
	__ASSERT_NO_MSG(iface);
	memset(&ctrl_data, 0, sizeof(struct dect_mdm_ctrl_data));

	CTRL_DATA_LOCK();
	ctrl_data.iface = iface;
	CTRL_DATA_UNLOCK();
	net_if_carrier_off(iface);
	LOG_DBG("dect_mdm_ctrl_internal_init");

	return 0;
}
