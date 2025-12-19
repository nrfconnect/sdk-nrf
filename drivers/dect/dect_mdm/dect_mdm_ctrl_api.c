/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include <nrf_errno.h>
#include <nrf_modem_dect.h>

#include <net/dect/dect_net_l2_mgmt.h>

#include "dect_mdm_common.h"
#include "dect_mdm_utils.h"
#include "dect_mdm_settings.h"
#include "dect_mdm_ctrl.h"
#include "dect_mdm_ctrl_internal.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(dect_mdm, CONFIG_DECT_MDM_LOG_LEVEL);

/* Public API functions - These provide the external interface to the control module */

int dect_mdm_ctrl_api_cluster_channel_get(void)
{
	struct dect_mdm_ctrl_data *data = dect_mdm_ctrl_internal_data_get();
	int channel;

	__ASSERT_NO_MSG(data != NULL);

	if (dect_mdm_ctrl_get_ft_cluster_state() != CTRL_FT_CLUSTER_STATE_STARTED) {
		return -EINVAL;
	}

	CTRL_DATA_LOCK();
	channel = data->configure_params.channel;
	CTRL_DATA_UNLOCK();

	return channel;
}

bool dect_mdm_ctrl_api_nw_beacon_running(void)
{
	return (dect_mdm_ctrl_get_ft_nw_beacon_state() == CTRL_FT_NW_BEACON_STATE_STARTED);
}

bool dect_mdm_ctrl_api_mdm_activated(void)
{
	return (dect_mdm_ctrl_get_mdm_activation_state() == CTRL_MDM_ACTIVATED);
}
int dect_mdm_ctrl_api_mdm_configure_n_activate(void)
{
	struct dect_mdm_ctrl_data *data = dect_mdm_ctrl_internal_data_get();
	int ret;

	__ASSERT_NO_MSG(data != NULL);

	if (dect_mdm_ctrl_get_mdm_activation_state() != CTRL_MDM_DEACTIVATED) {
		LOG_ERR("%s: Modem is not deactivated, cannot configure", __func__);
		return -EINVAL;
	}

	ret = dect_mdm_ctrl_internal_modem_configure_req_from_settings();
	if (ret != 0) {
		LOG_ERR("%s: error in configure, error: %d", __func__, ret);
	} else {
		CTRL_DATA_LOCK();
		data->mdm_activation_state = CTRL_MDM_ACTIVATE_REQ;
		CTRL_DATA_UNLOCK();
	}
	return ret;
}

int dect_mdm_ctrl_api_mdm_deactivate_cmd(void)
{
	int ret = dect_mdm_ctrl_internal_mdm_deactivate_req();

	if (ret != 0) {
		LOG_ERR("%s: error in deactivate, error: %d", __func__, ret);
	} else {
		struct dect_mdm_ctrl_data *data = dect_mdm_ctrl_internal_data_get();

		__ASSERT_NO_MSG(data != NULL);
		CTRL_DATA_LOCK();
		data->mdm_activation_state = CTRL_MDM_DEACTIVATE_REQ;
		CTRL_DATA_UNLOCK();
	}
	return ret;
}

int dect_mdm_ctrl_api_mdm_reactivate(void)
{
	struct dect_mdm_ctrl_data *data = dect_mdm_ctrl_internal_data_get();

	__ASSERT_NO_MSG(data != NULL);

	if (dect_mdm_ctrl_get_mdm_activation_state() != CTRL_MDM_ACTIVATED) {
		LOG_DBG("Modem is not activated, cannot reactivate");
		return -EINVAL;
	}

	CTRL_DATA_LOCK();
	bool connected = (data->ft_cluster_state != CTRL_FT_CLUSTER_STATE_NONE ||
			  data->ass_config.pt_association_state != CTRL_PT_ASSOCIATION_STATE_NONE);
	CTRL_DATA_UNLOCK();

	if (connected) {
		LOG_DBG("Modem is connected, cannot reactivate");
		return -EINVAL;
	}

	k_sem_reset(&dect_mdm_ctrl_reactivate_sema);

	int ret = dect_mdm_ctrl_internal_mdm_deactivate_req();

	if (ret != 0) {
		LOG_ERR("%s: error in deactivate, error: %d", __func__, ret);
	} else {
		CTRL_DATA_LOCK();
		data->mdm_activation_state = CTRL_MDM_REACTIVATING_DEACTIVATE;
		CTRL_DATA_UNLOCK();
		LOG_INF("Modem deactivation requested, waiting for configure/reactivation");
	}

	/* Wait for reactivation */
	ret = k_sem_take(&dect_mdm_ctrl_reactivate_sema, K_SECONDS(3));
	if (ret != 0) {
		LOG_ERR("%s: timeout in wait for reactivation, error: %d", __func__, ret);
	}
	return ret;
}

int dect_mdm_ctrl_api_rssi_scan_start_cmd(struct nrf_modem_dect_mac_rssi_scan_params *params)
{
	struct dect_mdm_ctrl_data *data = dect_mdm_ctrl_internal_data_get();

	__ASSERT_NO_MSG(data != NULL);

	CTRL_DATA_LOCK();
	if (data->rssi_scan_data.cmd_on_going) {
		LOG_ERR("RSSI scan already on going");
		CTRL_DATA_UNLOCK();
		return -EALREADY;
	}
	CTRL_DATA_UNLOCK();

	return dect_mdm_ctrl_internal_msgq_data_op_add(
		DECT_MDM_CTRL_OP_RSSI_START_REQ_CMD,
		params,
		sizeof(struct nrf_modem_dect_mac_rssi_scan_params));
}

int dect_mdm_ctrl_api_nw_scan_cmd(struct nrf_modem_dect_mac_network_scan_params *params,
				  dect_scan_result_cb_t cb)
{
	struct dect_mdm_ctrl_data *data = dect_mdm_ctrl_internal_data_get();
	int err;

	__ASSERT_NO_MSG(data != NULL);

	CTRL_DATA_LOCK();
	if (data->scan_data.on_going) {
		LOG_ERR("Network scan already on going");
		CTRL_DATA_UNLOCK();
		return -EALREADY;
	}
	CTRL_DATA_UNLOCK();

	err = nrf_modem_dect_mac_network_scan(params);

	if (!err) {
		CTRL_DATA_LOCK();
		data->scan_data.on_going = true;
		data->scan_data.scan_result_cb = cb;
		data->scan_data.scan_params = *params;
		memset(data->scan_data.cluster_channels, 0,
		       sizeof(data->scan_data.cluster_channels));
		data->scan_data.current_cluster_channel_index = 0;
		CTRL_DATA_UNLOCK();
	}
	return err;
}

int dect_mdm_ctrl_api_tx_cmd(struct dect_mdm_ctrl_api_tx_cmd_params *params)
{
	struct nrf_modem_dect_dlc_data_tx_params tx_params = {
		.transaction_id = params->transaction_id,
		.flow_id = params->flow_id,
		.long_rd_id = params->long_rd_id,
		.data = params->data,
		.data_len = params->data_len,
	};
	int ret;
	struct dect_mdm_ctrl_data *data = dect_mdm_ctrl_internal_data_get();

	__ASSERT_NO_MSG(data != NULL);

	CTRL_DATA_LOCK();
	if (data->tx_mdm_flow_ctrl_on) {
		LOG_DBG("Flow control is enabled, tx not allowed");
		CTRL_DATA_UNLOCK();
		return -EACCES;
	}
#if defined(CONFIG_DECT_MDM_NRF_TX_FLOW_CTRL_BASED_ON_MDM_TX_DLC_REQS)
	int arr_index = params->transaction_id - DECT_MDM_DATA_TX_HANDLE_START;

	if (data->total_unacked_tx_data_amount + params->data_len >
	    CONFIG_NRF_MODEM_LIB_SHMEM_TX_SIZE) {
		LOG_WRN("Too much unacked TX data: %d bytes - continue but flow ctrl might occur",
			data->total_unacked_tx_data_amount);
	}
	if (data->total_unacked_req_amount >= DECT_MDM_DATA_TX_HANDLE_COUNT) {
		LOG_WRN("Too many unacked TX requests: %d", data->total_unacked_req_amount);
		CTRL_DATA_UNLOCK();
		return -ENOMEM;
	}
	if (arr_index < 0 || arr_index >= DECT_MDM_DLC_DATA_INFO_MAX_COUNT) {
		LOG_ERR("Invalid transaction ID: %d", params->transaction_id);
		CTRL_DATA_UNLOCK();
		return -EINVAL;
	}
	if (data->dlc_data_tx_infos[arr_index].req_on_going) {
		LOG_WRN("Transaction ID %d already in use", params->transaction_id);
		CTRL_DATA_UNLOCK();
		return -EBUSY;
	}
	data->dlc_data_tx_infos[arr_index].transaction_id = params->transaction_id;
	data->dlc_data_tx_infos[arr_index].data_len = params->data_len;
	data->dlc_data_tx_infos[arr_index].req_on_going = true;
#endif
	CTRL_DATA_UNLOCK();

	ret = nrf_modem_dect_dlc_data_tx(&tx_params);
	if (ret) {
		if (ret == -NRF_ENOMEM) {
			ret = -ENOMEM;
			LOG_WRN("nrf_modem_dect_dlc_data_tx returned NRF_ENOMEM");
		} else {
			LOG_ERR("nrf_modem_dect_dlc_data_tx returned error: %d", ret);
		}
#if defined(CONFIG_DECT_MDM_NRF_TX_FLOW_CTRL_BASED_ON_MDM_TX_DLC_REQS)
		CTRL_DATA_LOCK();
		data->dlc_data_tx_infos[arr_index].req_on_going = false;
		CTRL_DATA_UNLOCK();
#endif
	} else {
#if defined(CONFIG_DECT_MDM_NRF_TX_FLOW_CTRL_BASED_ON_MDM_TX_DLC_REQS)
		CTRL_DATA_LOCK();
		data->total_unacked_tx_data_amount += params->data_len;
		data->total_unacked_req_amount++;
		CTRL_DATA_UNLOCK();
#endif
	}

	return ret;
}

int dect_mdm_ctrl_api_cluster_start_req_cmd(struct dect_cluster_start_req_params *params)
{
	struct dect_mdm_settings *set_ptr = dect_mdm_settings_ref_get();

	if (set_ptr->net_mgmt_common.device_type & DECT_DEVICE_TYPE_PT) {
		LOG_ERR("Cluster start not allowed for PT");
		return -EINVAL;
	}

	if (dect_mdm_ctrl_get_ft_cluster_state() != CTRL_FT_CLUSTER_STATE_NONE) {
		LOG_ERR("Cluster already started/starting");
		return -EALREADY;
	}

	/* Validate params */
	if (params->channel != DECT_CLUSTER_CHANNEL_ANY &&
	    dect_common_utils_channel_is_supported_by_band(
		set_ptr->net_mgmt_common.band_nbr, params->channel) == false) {
		LOG_ERR("Channel %d not supported by band %d", params->channel,
			set_ptr->net_mgmt_common.band_nbr);
		return -EINVAL;
	}
	return dect_mdm_ctrl_internal_msgq_data_op_add(
		DECT_MDM_CTRL_OP_CLUSTER_START_REQ, params,
		sizeof(struct dect_cluster_start_req_params));
}

int dect_mdm_ctrl_api_cluster_reconfig_req_cmd(struct dect_cluster_reconfig_req_params *params)
{
	struct dect_mdm_settings *set_ptr = dect_mdm_settings_ref_get();

	/* Validate params */
	if (params->channel != DECT_CLUSTER_CHANNEL_ANY &&
	    dect_common_utils_channel_is_supported_by_band(
		    set_ptr->net_mgmt_common.band_nbr, params->channel) == false) {
		LOG_ERR("Channel %d not supported by band %d", params->channel,
			set_ptr->net_mgmt_common.band_nbr);
		return -EINVAL;
	}

	if (dect_mdm_ctrl_get_ft_cluster_state() != CTRL_FT_CLUSTER_STATE_STARTED) {
		LOG_ERR("Cluster not started");
		return -EINVAL;
	}

	return dect_mdm_ctrl_internal_msgq_data_op_add(
		DECT_MDM_CTRL_OP_CLUSTER_RECONFIG_REQ,
		params,
		sizeof(struct dect_cluster_reconfig_req_params));
}

int dect_mdm_ctrl_api_cluster_reconfig_start_for_ipv6_prefix_cfg_changed(void)
{
	if (dect_mdm_ctrl_get_ft_cluster_state() != CTRL_FT_CLUSTER_STATE_STARTED) {
		LOG_DBG("Cluster not started");
		return -EINVAL;
	}

	return dect_mdm_ctrl_internal_msgq_non_data_op_add(
		DECT_MDM_CTRL_OP_CLUSTER_IPV6_PREFIX_CHANGE_RECONFIG_REQ);
}

int dect_mdm_ctrl_api_cluster_info_req_cmd(void)
{
	int err = nrf_modem_dect_mac_cluster_info();

	if (err) {
		LOG_ERR("%s: error intitiating the request for cluster info: %d", __func__, err);
	}

	return err;
}

int dect_mdm_ctrl_api_nw_beacon_start_req_cmd(struct dect_nw_beacon_start_req_params *params)
{
	struct dect_mdm_settings *set_ptr = dect_mdm_settings_ref_get();

	if (dect_mdm_ctrl_internal_nw_beacon_common_can_be_started(params->channel) == false) {
		return -EINVAL;
	}

	if (dect_mdm_ctrl_get_ft_cluster_state() != CTRL_FT_CLUSTER_STATE_STARTED) {
		LOG_ERR("Cluster not started");
		return -EINVAL;
	}
	if (dect_mdm_ctrl_get_ft_nw_beacon_state() != CTRL_FT_NW_BEACON_STATE_NONE) {
		LOG_ERR("Network beacon already started/starting");
		return -EINVAL;
	}

	for (int i = 0; i < params->additional_ch_count; i++) {
		if (dect_common_utils_channel_is_supported_by_band(
			    set_ptr->net_mgmt_common.band_nbr, params->additional_ch_list[i]) ==
		    false) {
			LOG_ERR("Additional channel %d not supported by band %d",
				params->additional_ch_list[i], set_ptr->net_mgmt_common.band_nbr);
			return -EINVAL;
		}
	}

	return dect_mdm_ctrl_internal_msgq_data_op_add(
		DECT_MDM_CTRL_OP_MDM_NW_BEACON_START,
		params,
		sizeof(struct dect_nw_beacon_start_req_params));
}

int dect_mdm_ctrl_api_nw_beacon_stop_req_cmd(struct dect_nw_beacon_stop_req_params *params)
{
	ARG_UNUSED(params);
	enum ctrl_ft_nw_beacon_state nw_beacon_state = dect_mdm_ctrl_get_ft_nw_beacon_state();

	if (nw_beacon_state == CTRL_FT_NW_BEACON_STATE_NONE ||
	    nw_beacon_state == CTRL_FT_NW_BEACON_STATE_STOPPING) {
		LOG_ERR("Network beacon already stopped or stopping");
		return -EALREADY;
	}

	return dect_mdm_ctrl_internal_msgq_non_data_op_add(DECT_MDM_CTRL_OP_MDM_NW_BEACON_STOP);
}

int dect_mdm_ctrl_api_network_create_req_cmd(void)
{
	struct dect_mdm_settings *set_ptr = dect_mdm_settings_ref_get();
	struct dect_mdm_ctrl_data *data = dect_mdm_ctrl_internal_data_get();
	uint16_t nw_beacon_channel;

	__ASSERT_NO_MSG(data != NULL);

	if (dect_mdm_ctrl_get_mdm_activation_state() != CTRL_MDM_ACTIVATED) {
		LOG_ERR("%s: modem not activated, cannot create network", __func__);
		return -EINVAL;
	}
	CTRL_DATA_LOCK();
	if (data->ft_cluster_state != CTRL_FT_CLUSTER_STATE_NONE) {
		LOG_WRN("%s: cluster already started/starting", __func__);
		CTRL_DATA_UNLOCK();
		return -EALREADY;
	}
	if (data->configure_params.auto_start) {
		LOG_ERR("%s: auto start already enabled", __func__);
		CTRL_DATA_UNLOCK();
		return -EALREADY;
	}
	if (data->ft_network_state != CTRL_FT_NETWORK_STATE_NONE) {
		LOG_WRN("%s: network already started/starting", __func__);
		CTRL_DATA_UNLOCK();
		return -EALREADY;
	}
	CTRL_DATA_UNLOCK();

	nw_beacon_channel = set_ptr->net_mgmt_common.nw_beacon.channel;

	if (nw_beacon_channel != DECT_NW_BEACON_CHANNEL_NOT_USED &&
	    dect_mdm_ctrl_internal_nw_beacon_common_can_be_started(
		    set_ptr->net_mgmt_common.nw_beacon.channel) == false) {
		LOG_ERR("%s: change settings - invalid channel %d for network beacon in band %d",
			__func__,
			nw_beacon_channel, set_ptr->net_mgmt_common.band_nbr);
		return -EINVAL;
	}

	CTRL_DATA_LOCK();
	data->ft_network_state = CTRL_FT_NETWORK_STATE_STARTING;
	CTRL_DATA_UNLOCK();

	return dect_mdm_ctrl_internal_msgq_non_data_op_add(DECT_MDM_CTRL_OP_AUTO_START);
}

bool dect_mdm_ctrl_api_network_remove_req_cmd_allowed(void)
{
	struct dect_mdm_ctrl_data *data = dect_mdm_ctrl_internal_data_get();

	__ASSERT_NO_MSG(data != NULL);

	CTRL_DATA_LOCK();
	bool allowed = !(data->ft_network_state == CTRL_FT_NETWORK_STATE_NONE &&
			 data->ft_cluster_state == CTRL_FT_CLUSTER_STATE_NONE);
	CTRL_DATA_UNLOCK();

	return allowed;
}

int dect_mdm_ctrl_api_network_remove_req_cmd(void)
{
	struct dect_mdm_ctrl_data *data = dect_mdm_ctrl_internal_data_get();
	int ret = dect_mdm_ctrl_internal_mdm_deactivate_req();

	__ASSERT_NO_MSG(data != NULL);

	if (ret != 0) {
		LOG_ERR("%s: error in deactivate, error: %d", __func__, ret);
	} else {
		CTRL_DATA_LOCK();
		data->mdm_activation_state = CTRL_MDM_REACTIVATING_DEACTIVATE;
		CTRL_DATA_UNLOCK();
	}
	return ret;
}

int dect_mdm_ctrl_api_network_join_req_cmd(void)
{
	struct dect_mdm_ctrl_data *data = dect_mdm_ctrl_internal_data_get();

	__ASSERT_NO_MSG(data != NULL);

	if (dect_mdm_ctrl_get_mdm_activation_state() != CTRL_MDM_ACTIVATED) {
		LOG_ERR("%s: Modem not activated, cannot join network", __func__);
		return -EINVAL;
	}
	if (dect_mdm_ctrl_get_pt_association_state() == CTRL_PT_ASSOCIATION_STATE_ASSOCIATED) {
		LOG_ERR("%s: Already associated", __func__);
		return -EALREADY;
	}
	CTRL_DATA_LOCK();
	if (data->configure_params.auto_start) {
		LOG_ERR("%s: Auto start already enabled", __func__);
		CTRL_DATA_UNLOCK();
		return -EALREADY;
	}
	CTRL_DATA_UNLOCK();

	return dect_mdm_ctrl_internal_msgq_non_data_op_add(DECT_MDM_CTRL_OP_AUTO_START);
}

int dect_mdm_ctrl_api_network_unjoin_req_cmd(void)
{
	struct dect_mdm_ctrl_data *data = dect_mdm_ctrl_internal_data_get();
	uint32_t parent_long_rd_id;

	__ASSERT_NO_MSG(data != NULL);

	if (dect_mdm_ctrl_get_pt_association_state() != CTRL_PT_ASSOCIATION_STATE_ASSOCIATED) {
		LOG_ERR("Not joined");
		return -EALREADY;
	}
	CTRL_DATA_LOCK();
	parent_long_rd_id = data->ass_config.parent_long_rd_id;
	CTRL_DATA_UNLOCK();

	int err = dect_mdm_ctrl_api_associate_release_cmd(
		parent_long_rd_id,
		NRF_MODEM_DECT_MAC_RELEASE_CAUSE_CONNECTION_TERMINATION);

	if (err) {
		LOG_ERR("dect_mdm_ctrl_api_associate_release_cmd returned err: %d", err);
	}
	return err;
}

int dect_mdm_ctrl_api_neighbor_list_req_cmd(void)
{
	int err = nrf_modem_dect_mac_neighbor_list();

	if (err) {
		LOG_ERR("Error intitiating the request for neighbor list: %d", err);
	}

	return err;
}

int dect_mdm_ctrl_api_neighbor_info_req_cmd(
	struct nrf_modem_dect_mac_neighbor_info_params *params)
{
	int err = nrf_modem_dect_mac_neighbor_info(params);

	if (err) {
		LOG_ERR("%s: error intitiating the request for neighbor info: %d", __func__, err);
	}

	return err;
}

int dect_mdm_ctrl_api_associate_req_cmd(struct nrf_modem_dect_mac_association_params *params)
{
	int err = nrf_modem_dect_mac_association(params);

	if (err) {
		LOG_ERR("%s: initiation association failed, err: %d", __func__, err);
	}

	return err;
}

int dect_mdm_ctrl_api_associate_release_cmd(
	uint32_t long_rd_id, enum nrf_modem_dect_mac_release_cause rel_cause)
{
	struct nrf_modem_dect_mac_association_release_params params = {
		.release_cause = rel_cause,
		.long_rd_id = long_rd_id,
	};
	int err = nrf_modem_dect_mac_association_release(&params);
	struct dect_mdm_ctrl_data *data = dect_mdm_ctrl_internal_data_get();

	__ASSERT_NO_MSG(data != NULL);

	if (err) {
		LOG_ERR("%s: initiation association release failed, err: %d", __func__, err);
	}

	CTRL_DATA_LOCK();
	data->last_rel_cause = rel_cause;
	CTRL_DATA_UNLOCK();

	return err;
}

int dect_mdm_ctrl_api_init(struct net_if *iface)
{
	return dect_mdm_ctrl_internal_init(iface);
}

struct nrf_modem_dect_mac_capability_ntf_cb_params
	*dect_mdm_ctrl_api_mdm_capabilities_ref_get(void)
{
	struct dect_mdm_ctrl_data *data = dect_mdm_ctrl_internal_data_get();

	__ASSERT_NO_MSG(data != NULL);

	return &data->mdm_capas;
}
