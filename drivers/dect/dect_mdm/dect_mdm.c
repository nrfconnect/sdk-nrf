/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>
#include <zephyr/sys/byteorder.h>

#include <net/dect/dect_utils.h>

#if defined(CONFIG_MODEM_INFO)
#include <modem/modem_info.h>
#endif

#include <net/dect/dect_net_l2.h>
#include <net/dect/dect_net_l2_mgmt.h>

#include "dect_mdm_ctrl.h"
#include "dect_mdm_settings.h"
#include "dect_mdm_sink.h"
#include "dect_mdm.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dect_mdm, CONFIG_DECT_MDM_LOG_LEVEL);

struct dect_mdm_association_data {
	bool in_use;
	uint32_t target_long_rd_id;
};

struct dect_mdm_mac_dev_context {
	struct net_if *iface;

	uint8_t link_addr[8];
	uint32_t parent_long_rd_id;

	struct dect_mdm_association_data
		child_associations[CONFIG_DECT_CLUSTER_MAX_CHILD_ASSOCIATION_COUNT];
	struct dect_mdm_association_data parent_associations[1];
};

static struct dect_mdm_mac_dev_context dect_mdm_mac_dev_context_data;

static K_MUTEX_DEFINE(send_buf_lock); /* Only one TX requests at a time */

#if defined(CONFIG_DECT_MDM_NRF_TX_FLOW_CTRL_BASED_ON_MDM_TX_DLC_REQS)
/* We are using handle as array index */
BUILD_ASSERT(DECT_MDM_DATA_TX_HANDLE_COUNT == DECT_MDM_DLC_DATA_INFO_MAX_COUNT,
	     "Mismatch in DECT MAC data tx handle range and DECT_MDM_DLC_DATA_INFO_MAX_COUNT");
#endif
#define DECT_MDM_TX_TRY_MAX_COUNT 20
#define DECT_MDM_TX_RETRY_SLEEP_MS 50

/* Sanity checks between Zephyr and nrf api */
BUILD_ASSERT((NRF_MODEM_DECT_MAC_MAX_CHANNELS_IN_RSSI_SCAN == DECT_MAX_CHANNELS_IN_RSSI_SCAN),
	     "NRF_MODEM_DECT_MAC_MAX_CHANNELS_IN_RSSI_SCAN != "
	     "DECT_MAX_CHANNELS_IN_RSSI_SCAN");
BUILD_ASSERT((NRF_MODEM_DECT_MAC_MAX_CHANNELS_IN_NETWORK_SCAN_REQ ==
	      DECT_MAX_CHANNELS_IN_NETWORK_SCAN_REQ),
	     "NRF_MODEM_DECT_MAC_MAX_CHANNELS_IN_NETWORK_SCAN_REQ != "
	     "DECT_MAX_CHANNELS_IN_NETWORK_SCAN_REQ");
BUILD_ASSERT((NRF_MODEM_DECT_MAC_MAX_ADDITIONAL_NW_BEACON_CHANNELS ==
	      DECT_MAX_ADDITIONAL_NW_BEACON_CHANNELS),
	     "NRF_MODEM_DECT_MAC_MAX_ADDITIONAL_NW_BEACON_CHANNELS != "
	     "DECT_MAX_ADDITIONAL_NW_BEACON_CHANNELS");

BUILD_ASSERT((NRF_MODEM_DECT_MAC_INTEGRITY_KEY_LENGTH == DECT_INTEGRITY_KEY_LENGTH),
	     "NRF_MODEM_DECT_MAC_INTEGRITY_KEY_LENGTH != "
	     "DECT_INTEGRITY_KEY_LENGTH");
BUILD_ASSERT((NRF_MODEM_DECT_MAC_CIPHER_KEY_LENGTH == DECT_CIPHER_KEY_LENGTH),
	     "NRF_MODEM_DECT_MAC_CIPHER_KEY_LENGTH != "
	     "DECT_CIPHER_KEY_LENGTH");
BUILD_ASSERT((NRF_MODEM_DECT_MAC_SECURITY_MODE_NONE ==
	      (enum nrf_modem_dect_mac_security_mode)DECT_SECURITY_MODE_NONE),
	     "NRF_MODEM_DECT_MAC_SECURITY_MODE_NONE != "
	     "DECT_SECURITY_MODE_NONE");
BUILD_ASSERT((NRF_MODEM_DECT_MAC_SECURITY_MODE_1 ==
	      (enum nrf_modem_dect_mac_security_mode)DECT_SECURITY_MODE_1),
	     "NRF_MODEM_DECT_MAC_SECURITY_MODE_1 != "
	     "DECT_SECURITY_MODE_1");

/* Sanity check between dect_association_release_cause and nrf_modem_dect_mac_release_cause*/
BUILD_ASSERT((DECT_RELEASE_CAUSE_CONNECTION_TERMINATION ==
	      (enum dect_association_release_cause)
		      NRF_MODEM_DECT_MAC_RELEASE_CAUSE_CONNECTION_TERMINATION),
	     "DECT_RELEASE_CAUSE_CONNECTION_TERMINATION != "
	     "NRF_MODEM_DECT_MAC_RELEASE_CAUSE_CONNECTION_TERMINATION");
BUILD_ASSERT((DECT_RELEASE_CAUSE_MOBILITY ==
	      (enum dect_association_release_cause)NRF_MODEM_DECT_MAC_RELEASE_CAUSE_MOBILITY),
	     "DECT_RELEASE_CAUSE_MOBILITY != "
	     "NRF_MODEM_DECT_MAC_RELEASE_CAUSE_MOBILITY");
BUILD_ASSERT(
	(DECT_RELEASE_CAUSE_LONG_INACTIVITY ==
	 (enum dect_association_release_cause)NRF_MODEM_DECT_MAC_RELEASE_CAUSE_LONG_INACTIVITY),
	"DECT_RELEASE_CAUSE_LONG_INACTIVITY != "
	"NRF_MODEM_DECT_MAC_RELEASE_CAUSE_LONG_INACTIVITY");
BUILD_ASSERT((DECT_RELEASE_CAUSE_INCOMPATIBLE_CONFIGURATION ==
	      (enum dect_association_release_cause)
		      NRF_MODEM_DECT_MAC_RELEASE_CAUSE_INCOMPATIBLE_CONFIGURATION),
	     "DECT_RELEASE_CAUSE_INCOMPATIBLE_CONFIGURATION != "
	     "NRF_MODEM_DECT_MAC_RELEASE_CAUSE_INCOMPATIBLE_CONFIGURATION");
BUILD_ASSERT((DECT_RELEASE_CAUSE_INSUFFICIENT_HW_RESOURCES ==
	      (enum dect_association_release_cause)
		      NRF_MODEM_DECT_MAC_RELEASE_CAUSE_INSUFFICIENT_HW_RESOURCES),
	     "DECT_RELEASE_CAUSE_INSUFFICIENT_HW_RESOURCES != "
	     "NRF_MODEM_DECT_MAC_RELEASE_CAUSE_INSUFFICIENT_HW_RESOURCES");
BUILD_ASSERT((DECT_RELEASE_CAUSE_INSUFFICIENT_RADIO_RESOURCES ==
	      (enum dect_association_release_cause)
		      NRF_MODEM_DECT_MAC_RELEASE_CAUSE_INSUFFICIENT_RADIO_RESOURCES),
	     "DECT_RELEASE_CAUSE_INSUFFICIENT_RADIO_RESOURCES != "
	     "NRF_MODEM_DECT_MAC_RELEASE_CAUSE_INSUFFICIENT_RADIO_RESOURCES");
BUILD_ASSERT(
	(DECT_RELEASE_CAUSE_BAD_RADIO_QUALITY ==
	 (enum dect_association_release_cause)NRF_MODEM_DECT_MAC_RELEASE_CAUSE_BAD_RADIO_QUALITY),
	"DECT_RELEASE_CAUSE_BAD_RADIO_QUALITY != "
	"NRF_MODEM_DECT_MAC_RELEASE_CAUSE_BAD_RADIO_QUALITY");
BUILD_ASSERT((DECT_RELEASE_CAUSE_SECURITY_ERROR ==
	      (enum dect_association_release_cause)NRF_MODEM_DECT_MAC_RELEASE_CAUSE_SECURITY_ERROR),
	     "DECT_RELEASE_CAUSE_SECURITY_ERROR != "
	     "NRF_MODEM_DECT_MAC_RELEASE_CAUSE_SECURITY_ERROR");
BUILD_ASSERT((DECT_RELEASE_CAUSE_OTHER_ERROR ==
	      (enum dect_association_release_cause)NRF_MODEM_DECT_MAC_RELEASE_CAUSE_OTHER_ERROR),
	     "DECT_RELEASE_CAUSE_OTHER_ERROR != "
	     "NRF_MODEM_DECT_MAC_RELEASE_CAUSE_OTHER_ERROR");
BUILD_ASSERT((DECT_RELEASE_CAUSE_OTHER_REASON ==
	      (enum dect_association_release_cause)NRF_MODEM_DECT_MAC_RELEASE_CAUSE_OTHER_REASON),
	     "DECT_RELEASE_CAUSE_OTHER_REASON != "
	     "NRF_MODEM_DECT_MAC_RELEASE_CAUSE_OTHER_REASON");
BUILD_ASSERT((DECT_RELEASE_CAUSE_CUSTOM_RACH_RESOURCE_FAILURE ==
	      (enum dect_association_release_cause)DECT_MAC_RELEASE_CAUSE_RACH_RESOURCE_FAILURE),
	     "DECT_RELEASE_CAUSE_CUSTOM_RACH_RESOURCE_FAILURE != "
	     "DECT_MAC_RELEASE_CAUSE_RACH_RESOURCE_FAILURE");

/* Sanity check for the 1st and for the last error cause values */
BUILD_ASSERT((DECT_STATUS_OK == (enum dect_status_values)NRF_MODEM_DECT_MAC_STATUS_OK),
	     "NRF_MODEM_DECT_MAC_STATUS_OK != DECT_STATUS_OK");
BUILD_ASSERT((DECT_STATUS_TX_TIMEOUT ==
	      (enum dect_status_values)NRF_MODEM_DECT_MAC_STATUS_DLC_DISCARD_TIMER_EXPIRED),
	     "DECT_STATUS_TX_TIMEOUT != NRF_MODEM_DECT_MAC_STATUS_DLC_DISCARD_TIMER_EXPIRED");

static uint8_t *dect_mdm_current_link_addr_create(struct dect_mdm_mac_dev_context *ctx)
{
	/* MAC addr based on set long rd id */
	struct dect_mdm_settings *set_ptr = dect_mdm_settings_ref_get();

	memset(ctx->link_addr, 0, sizeof(ctx->link_addr));

	if (set_ptr->net_mgmt_common.device_type & DECT_DEVICE_TYPE_FT) {
		uint32_t rd_id = set_ptr->net_mgmt_common.identities.transmitter_long_rd_id;

		sys_put_be32(rd_id, &ctx->link_addr[0]);
		sys_put_be32(rd_id, &ctx->link_addr[4]);
	} else {
		__ASSERT_NO_MSG(set_ptr->net_mgmt_common.device_type & DECT_DEVICE_TYPE_PT);
		if (ctx->parent_long_rd_id == 0) {
			/* Sink part to be set later when associated */
			sys_put_be32(set_ptr->net_mgmt_common.identities.transmitter_long_rd_id,
				     &ctx->link_addr[4]);
		} else {
			sys_put_be32(ctx->parent_long_rd_id, &ctx->link_addr[0]);
			sys_put_be32(set_ptr->net_mgmt_common.identities.transmitter_long_rd_id,
				     &ctx->link_addr[4]);
		}
	}
	return ctx->link_addr;
}

static struct dect_mdm_association_data *
dect_mdm_child_association_list_item_get(uint32_t target_long_rd_id)
{
	struct dect_mdm_mac_dev_context *ctx = &dect_mdm_mac_dev_context_data;

	LOG_DBG("%s: target_long_rd_id %u", (__func__), target_long_rd_id);

	for (int i = 0; i < ARRAY_SIZE(ctx->child_associations); i++) {
		if (ctx->child_associations[i].in_use &&
		    ctx->child_associations[i].target_long_rd_id == target_long_rd_id) {
			return &ctx->child_associations[i];
		}
	}
	return NULL;
}

static struct dect_mdm_association_data *
dect_mdm_child_association_list_add(uint32_t target_long_rd_id)
{
	struct dect_mdm_mac_dev_context *ctx = &dect_mdm_mac_dev_context_data;
	struct dect_mdm_association_data *ass_list_item = NULL;

	/* Return existing item if already in list */
	ass_list_item = dect_mdm_child_association_list_item_get(target_long_rd_id);
	if (ass_list_item != NULL) {
		return ass_list_item;
	}

	/* Add to list */
	for (int i = 0; i < ARRAY_SIZE(ctx->child_associations); i++) {
		if (!ctx->child_associations[i].in_use) {
			ctx->child_associations[i].in_use = true;
			ctx->child_associations[i].target_long_rd_id = target_long_rd_id;
			return &ctx->child_associations[i];
		}
	}
	return NULL;
}

static void dect_mdm_child_association_list_remove(uint32_t target_long_rd_id)
{
	struct dect_mdm_mac_dev_context *ctx = &dect_mdm_mac_dev_context_data;

	LOG_DBG("%s: target_long_rd_id %u", (__func__), target_long_rd_id);

	for (int i = 0; i < ARRAY_SIZE(ctx->child_associations); i++) {
		if (ctx->child_associations[i].in_use &&
		    ctx->child_associations[i].target_long_rd_id == target_long_rd_id) {
			ctx->child_associations[i].in_use = false;
			break;
		}
	}
}

static void dect_mdm_parent_created_association_list_addressing_handle(
	struct dect_mdm_association_data *ass_list_item)
{
	struct dect_mdm_mac_dev_context *ctx = &dect_mdm_mac_dev_context_data;
	struct net_if *iface = dect_mdm_mac_dev_context_data.iface;
	uint8_t *link_addr;
	int ret;

	/* Parent added: update our link addr */
	link_addr = dect_mdm_current_link_addr_create(ctx);
	ret = net_if_set_link_addr(iface, link_addr, 8, NET_LINK_UNKNOWN);
	if (ret) {
		LOG_WRN("%s: cannot re-set link addr: %d", (__func__), ret);
	}
}

static struct dect_mdm_association_data *
dect_mdm_parent_association_list_add(uint32_t target_long_rd_id)
{
	struct dect_mdm_mac_dev_context *ctx = &dect_mdm_mac_dev_context_data;

	LOG_DBG("%s: target_long_rd_id %u", (__func__), target_long_rd_id);
	for (int i = 0; i < ARRAY_SIZE(ctx->parent_associations); i++) {
		if (!ctx->parent_associations[i].in_use) {
			ctx->parent_associations[i].in_use = true;
			ctx->parent_associations[i].target_long_rd_id = target_long_rd_id;

			return &ctx->parent_associations[i];
		}
	}
	return NULL;
}

static struct dect_mdm_association_data *
dect_mdm_parent_association_list_item_get(uint32_t target_long_rd_id)
{
	struct dect_mdm_mac_dev_context *ctx = &dect_mdm_mac_dev_context_data;

	if (target_long_rd_id) {
		for (int i = 0; i < ARRAY_SIZE(ctx->parent_associations); i++) {
			if (ctx->parent_associations[i].in_use &&
			ctx->parent_associations[i].target_long_rd_id == target_long_rd_id) {
				return &ctx->parent_associations[i];
			}
		}
	} else {
		/* If target_long_rd_id is zero, return the first in-use item */
		for (int i = 0; i < ARRAY_SIZE(ctx->parent_associations); i++) {
			if (ctx->parent_associations[i].in_use) {
				return &ctx->parent_associations[i];
			}
		}
	}

	return NULL;
}

static void dect_mdm_parent_association_list_remove(uint32_t target_long_rd_id)
{
	struct dect_mdm_mac_dev_context *ctx = &dect_mdm_mac_dev_context_data;

	LOG_DBG("dect_mdm_parent_association_remove: target_long_rd_id %u", target_long_rd_id);
	for (int i = 0; i < ARRAY_SIZE(ctx->parent_associations); i++) {
		if (ctx->parent_associations[i].in_use &&
		    ctx->parent_associations[i].target_long_rd_id == target_long_rd_id) {
			ctx->parent_associations[i].in_use = false;
			ctx->parent_associations[i].target_long_rd_id = 0;
			break;
		}
	}
}

static void dect_mdm_iface_init(struct net_if *iface)
{
	struct dect_mdm_mac_dev_context *ctx = net_if_get_device(iface)->data;
	uint8_t *link_addr;
	int err;

	err = dect_mdm_settings_init();
	if (err) {
		LOG_ERR("%s: cannot initialize settings: %d", (__func__), err);
		return;
	}
	ctx->parent_long_rd_id = 0;
	link_addr = dect_mdm_current_link_addr_create(ctx);

	ctx->iface = iface;

	dect_mdm_ctrl_api_init(iface);

	err = net_if_set_link_addr(iface, link_addr, 8, NET_LINK_UNKNOWN);
	if (err) {
		LOG_ERR("%s: cannot set link addr: %d", (__func__), err);
		return;
	}
	err = net_if_set_name(ctx->iface, CONFIG_DECT_MDM_DEVICE_NAME);
	if (err) {
		LOG_WRN("%s: could not set interface name: err %d", (__func__), err);
	}

	if (IS_ENABLED(CONFIG_DECT_MDM_NET_IF_NO_AUTO_START)) {
		net_if_flag_set(iface, NET_IF_NO_AUTO_START);
	}
	struct dect_mdm_settings *set_ptr = dect_mdm_settings_ref_get();

	dect_net_l2_init(iface, &set_ptr->net_mgmt_common);
}

static int dect_mdm_init(const struct device *dev)
{
	LOG_INF("nRF91 DECT NR+ initialized");
	return 0;
}

static int dect_mdm_hal_mdm_activate(const struct device *dev)
{
	return dect_mdm_ctrl_api_mdm_configure_n_activate();
}

static int dect_mdm_hal_mdm_deactivate(const struct device *dev)
{
	return dect_mdm_ctrl_api_mdm_deactivate_cmd();
}

static int dect_mdm_hal_rssi_scan(const struct device *dev,
				  struct dect_rssi_scan_params *params)
{
	struct dect_mdm_settings *set_ptr = dect_mdm_settings_ref_get();
	struct nrf_modem_dect_mac_rssi_scan_params scan_params = {
		.channel_scan_length = params->frame_count_to_scan,
		.num_channels = params->channel_count,
		.band = params->band,
		.threshold_min = set_ptr->net_mgmt_common.rssi_scan.free_threshold_dbm,
		.threshold_max = set_ptr->net_mgmt_common.rssi_scan.busy_threshold_dbm,
	};
	int err;

	if (params->band != 0) {
		scan_params.num_channels = 0;
	}
	for (int i = 0; i < params->channel_count; i++) {
		scan_params.channel_list[i] = params->channel_list[i];
	}

	err = dect_mdm_ctrl_api_rssi_scan_start_cmd(&scan_params);
	if (err) {
		LOG_ERR("Error initiating RSSI scan: err %d", err);
	} else {
		LOG_INF("RSSI scan initiated");
	}

	return err;
}

static int dect_mdm_hal_scan(const struct device *dev, struct dect_scan_params *params,
			     dect_scan_result_cb_t cb)
{
	struct nrf_modem_dect_mac_network_scan_params scan_params = {
		.scan_time = params->channel_scan_time_ms,
		.network_id_filter_mode = NRF_MODEM_DECT_MAC_NW_ID_FILTER_MODE_NONE,
		.num_channels = params->channel_count,
		.band = params->band,
	};
	int err;

	__ASSERT_NO_MSG(cb != NULL);
	if (params->channel_count > NRF_MODEM_DECT_MAC_MAX_CHANNELS_IN_NETWORK_SCAN_REQ) {
		LOG_ERR("Channel count %d exceeds maximum %d", params->channel_count,
			NRF_MODEM_DECT_MAC_MAX_CHANNELS_IN_NETWORK_SCAN_REQ);
		return -EINVAL;
	}

	if (params->channel_scan_time_ms < 1 || params->channel_scan_time_ms > 60000) {
		LOG_ERR("Channel scan time %d is out of range (1-60000 ms)",
			params->channel_scan_time_ms);
		return -EINVAL;
	}

	if (params->band != 0) {
		scan_params.num_channels = 0;
	}

	for (int i = 0; i < params->channel_count; i++) {
		scan_params.channel_list[i] = params->channel_list[i];
	}

	err = dect_mdm_ctrl_api_nw_scan_cmd(&scan_params, cb);
	if (err) {
		LOG_ERR("%s: Error initiating in Network scan: err %d", (__func__), err);
	} else {
		LOG_INF("Network scan initiated");
	}

	return err;
}

int dect_mdm_hal_associate_req(const struct device *dev,
			       struct dect_associate_req_params *params)
{
	int ret;
	struct nrf_modem_dect_mac_association_params mdm_params;
	struct dect_mdm_settings *set_ptr = dect_mdm_settings_ref_get();
	struct nrf_modem_dect_mac_tx_flow_config flow_config[1];

	/* Association req only with PT devices */
	if (!(set_ptr->net_mgmt_common.device_type & DECT_DEVICE_TYPE_PT)) {
		LOG_ERR("%s: Association request only supported for PT devices", (__func__));
		return -EINVAL;
	}

	mdm_params.long_rd_id = params->target_long_rd_id;
	mdm_params.network_id = set_ptr->net_mgmt_common.identities.network_id;
	mdm_params.num_flows = 1;

	mdm_params.info_triggers.num_beacon_rx_failures =
		set_ptr->net_mgmt_common.association.max_beacon_rx_failures;

	flow_config[0].flow_id = 1;
	flow_config[0].dlc_service_type = NRF_MODEM_DECT_DLC_SERVICE_TYPE_3;
	flow_config[0].dlc_sdu_lifetime = NRF_MODEM_DECT_DLC_SDU_LIFETIME_60_S;
	mdm_params.tx_flow_configs = flow_config;

	ret = dect_mdm_ctrl_api_associate_req_cmd(&mdm_params);
	if (ret) {
		LOG_ERR("%s: error in Associate: err %d", (__func__), ret);
	} else {
		LOG_INF("%s: association initiated", (__func__));
	}
	return ret;
}

int dect_mdm_hal_associate_release(const struct device *dev,
				   struct dect_associate_rel_params *params)
{
	int ret;

	ret = dect_mdm_ctrl_api_associate_release_cmd(
		params->target_long_rd_id, NRF_MODEM_DECT_MAC_RELEASE_CAUSE_CONNECTION_TERMINATION);
	if (ret) {
		LOG_ERR("%s: error in Association Release: err %d", (__func__), ret);
	} else {
		LOG_INF("%s: association release initiated", (__func__));
	}
	return ret;
}

int dect_mdm_hal_cluster_start_req(const struct device *dev,
				   struct dect_cluster_start_req_params *params)
{
	return dect_mdm_ctrl_api_cluster_start_req_cmd(params);
}

int dect_mdm_hal_cluster_reconfig_req(const struct device *dev,
				      struct dect_cluster_reconfig_req_params *params)
{
	return dect_mdm_ctrl_api_cluster_reconfig_req_cmd(params);
}

int dect_mdm_hal_nw_beacon_start_req(const struct device *dev,
				     struct dect_nw_beacon_start_req_params *params)
{
	return dect_mdm_ctrl_api_nw_beacon_start_req_cmd(params);
}

int dect_mdm_hal_nw_beacon_stop_req(const struct device *dev,
				    struct dect_nw_beacon_stop_req_params *params)
{
	return dect_mdm_ctrl_api_nw_beacon_stop_req_cmd(params);
}

static int dect_mdm_hal_status_info_get(const struct device *dev,
					struct dect_status_info *status_info_out)
{
	int i, tmp_count;
	struct dect_mdm_mac_dev_context *ctx = &dect_mdm_mac_dev_context_data;
	struct dect_mdm_settings *set_ptr = dect_mdm_settings_ref_get();

	if (status_info_out == NULL) {
		LOG_ERR("%s: no status info pointer", (__func__));
		return -EINVAL;
	}
	memset(status_info_out, 0, sizeof(*status_info_out));
	status_info_out->mdm_activated = dect_mdm_ctrl_api_mdm_activated();

	tmp_count = 0;

	/* Fill children status*/
	for (i = 0; i < ARRAY_SIZE(ctx->child_associations); i++) {
		if (ctx->child_associations[i].in_use) {
			status_info_out->child_associations[tmp_count].long_rd_id =
				ctx->child_associations[i].target_long_rd_id;
			tmp_count++;
		}
	}
	status_info_out->child_count = tmp_count;
	tmp_count = 0;

	/* Fill parent info status*/
	for (i = 0; i < ARRAY_SIZE(ctx->parent_associations); i++) {
		if (ctx->parent_associations[i].in_use) {
			status_info_out->parent_associations[tmp_count].long_rd_id =
				ctx->parent_associations[i].target_long_rd_id;
			tmp_count++;
		}
	}
	status_info_out->parent_count = tmp_count;

	status_info_out->cluster_channel = 0;
	status_info_out->cluster_running = false;

	if (set_ptr->net_mgmt_common.device_type & DECT_DEVICE_TYPE_FT) {
		int cluster_channel = dect_mdm_ctrl_api_cluster_channel_get();

		if (cluster_channel > 0) {
			status_info_out->cluster_channel = cluster_channel;
			status_info_out->cluster_running = true;
		}
	}
	status_info_out->nw_beacon_running = dect_mdm_ctrl_api_nw_beacon_running();

	strncpy(status_info_out->fw_version_str, "Not available",
		sizeof(status_info_out->fw_version_str) - 1);
#if defined(CONFIG_MODEM_INFO)
	char info_str[MODEM_INFO_MAX_RESPONSE_SIZE + 1];
	int ret;

	ret = modem_info_string_get(MODEM_INFO_FW_VERSION, info_str, sizeof(info_str));
	if (ret >= 0 && strlen(info_str) < sizeof(status_info_out->fw_version_str)) {
		strcpy(status_info_out->fw_version_str, info_str);
	}
#endif
	status_info_out->br_net_iface = NULL;
	status_info_out->br_global_ipv6_addr_prefix_set = false;
	status_info_out->br_global_ipv6_addr_prefix_len = 0;
	status_info_out->br_global_ipv6_addr_prefix = (struct in6_addr) {
		.s6_addr = { 0 }
	};

#if defined(CONFIG_NET_L2_DECT_BR)
	struct dect_mdm_ipv6_prefix br_global_prefix;

	status_info_out->br_global_ipv6_addr_prefix_set = false;
	status_info_out->br_net_iface = NULL;
	if (dect_mdm_sink_ipv6_prefix_get(&br_global_prefix)) {
		status_info_out->br_global_ipv6_addr_prefix_set = true;
		net_ipaddr_copy(&status_info_out->br_global_ipv6_addr_prefix,
				&br_global_prefix.prefix);
		status_info_out->br_global_ipv6_addr_prefix_len = br_global_prefix.len;
	}
#endif
	return 0;
}

static int dect_mdm_hal_settings_read(const struct device *dev, struct dect_settings *settings_out)
{
	struct dect_mdm_settings *set_ptr = dect_mdm_settings_ref_get();

	*settings_out = set_ptr->net_mgmt_common;

	return 0;
}

static int dect_mdm_hal_settings_write(const struct device *dev, struct dect_settings *settings_in)
{
	struct dect_mdm_settings_write_status ret_status;
	uint16_t write_scope_bitmap_in =
		settings_in->cmd_params.write_scope_bitmap;

	settings_in->cmd_params.failure_scope_bitmap_out = 0;
	if (settings_in->cmd_params.reset_to_driver_defaults) {
		ret_status = dect_mdm_settings_defaults_set();
	} else {
		struct dect_mdm_settings settings = {
			.net_mgmt_common = *settings_in,
		};

		ret_status = dect_mdm_settings_write(&settings);
	}
	if (ret_status.status == 0) {
		struct dect_mdm_settings *current_settings = dect_mdm_settings_ref_get();

		/* Inform L2 about new settings */
		dect_net_l2_settings_changed(
			dect_mdm_mac_dev_context_data.iface,
			&current_settings->net_mgmt_common);
		if (ret_status.reactivate && dect_mdm_ctrl_api_mdm_reactivate()) {
			LOG_DBG("Couldn't reconfigure/activate modem to apply new settings - "
				"reactivate is needed");
		} else {
			/* Not connected: we have a chance to update link addr already now */
			if ((write_scope_bitmap_in & DECT_SETTINGS_WRITE_SCOPE_IDENTITIES) ||
			    (write_scope_bitmap_in & DECT_SETTINGS_WRITE_SCOPE_DEVICE_TYPE) ||
			    (write_scope_bitmap_in & DECT_SETTINGS_WRITE_SCOPE_ALL)) {
				/* Identities changed: update our link addr */
				uint8_t *link_addr;
				int ret;
				struct dect_mdm_mac_dev_context *ctx =
					&dect_mdm_mac_dev_context_data;

				link_addr = dect_mdm_current_link_addr_create(ctx);
				ret = net_if_set_link_addr(ctx->iface, link_addr, 8,
							  NET_LINK_UNKNOWN);
				if (ret) {
					LOG_WRN("%s: cannot re-set link addr: %d", (__func__),
						ret);
				}
			}
		}
	} else {
		settings_in->cmd_params.failure_scope_bitmap_out = ret_status.failure_scope_bitmap;
	}
	return ret_status.status;
}

static int dect_mdm_hal_driver_send(const struct device *dev, struct net_pkt *pkt)
{
	__ASSERT_NO_MSG(pkt != NULL);

	struct dect_mdm_ctrl_api_tx_cmd_params tx_params;
	uint32_t target_long_rd_id = 0;
	static uint32_t transaction_id = DECT_MDM_DATA_TX_HANDLE_START;
	int ret;
	int data_len = 0;

	if (IS_ENABLED(CONFIG_NET_IPV6) && net_pkt_family(pkt) == AF_INET6) {
		target_long_rd_id = dect_utils_lib_dst_long_rd_id_get_from_pkt_dst_addr(pkt);

		if (!target_long_rd_id) {
			LOG_ERR("AF_INET6: No target long rd id in packet dst address");
			return -EINVAL;
		}
	} else if (IS_ENABLED(CONFIG_NET_SOCKETS_PACKET) && net_pkt_family(pkt) == AF_PACKET) {

		target_long_rd_id = dect_utils_lib_dst_long_rd_id_get_from_pkt_dst_addr(pkt);

		if (!target_long_rd_id) {
			LOG_ERR("AF_PACKET: No target long rd id in packet dst address");
			return -EINVAL;
		}
	} else {
		LOG_ERR("%s: packet type %d not supported", (__func__), net_pkt_family(pkt));
		return -EINVAL;
	}

	LOG_DBG("dect_mdm_hal_driver_send: target_long_rd_id %u", target_long_rd_id);

	k_mutex_lock(&send_buf_lock, K_FOREVER);
	data_len = net_pkt_get_len(pkt);

	if (data_len > NRF91_DECT_UL_BUFFER_SIZE) {
		LOG_ERR("Packet too large: %d", data_len);
		k_mutex_unlock(&send_buf_lock);
		return -EINVAL;
	}

	ret = net_pkt_read(pkt, tx_params.data, data_len);
	if (ret < 0) {
		LOG_ERR("%s: cannot read packet: %d, from pkt %p, data_len %d\n", __func__, ret,
			pkt, data_len);
	} else {
		int retry_count = 0;
		bool data_sent = false;

		tx_params.data_len = data_len;
		tx_params.long_rd_id = target_long_rd_id;
		tx_params.flow_id = 1;
		tx_params.transaction_id = transaction_id++;

		while (retry_count < DECT_MDM_TX_TRY_MAX_COUNT &&
		       data_sent == false) {
			ret = dect_mdm_ctrl_api_tx_cmd(&tx_params);
			if (ret == -ENOMEM || ret == -EACCES) {
				k_sleep(K_MSEC(DECT_MDM_TX_RETRY_SLEEP_MS));
				retry_count++;
			} else if (ret == -EBUSY) {
				tx_params.transaction_id = transaction_id++;
				retry_count++;
			} else {
				data_sent = true;
			}
			if (transaction_id > DECT_MDM_DATA_TX_HANDLE_END) {
				transaction_id = DECT_MDM_DATA_TX_HANDLE_START;
			}
		}

		if (data_sent && ret == 0) {
			LOG_DBG("Packet sending initiated to %d (%d bytes) after %d retries",
				target_long_rd_id, data_len, retry_count);
		} else {
			LOG_ERR("Error (%d) when sending packet to rd id %u: retries %d",
				ret, target_long_rd_id, retry_count);
		}
	}
	k_mutex_unlock(&send_buf_lock);
	/* If something went wrong, then we need to return negative value to
	 * net_if.c:net_if_tx() so that the net_pkt will get released.
	 */

	return ret;
}

static int dect_mdm_hal_neighbor_list_req(const struct device *dev)
{
	return dect_mdm_ctrl_api_neighbor_list_req_cmd();
}

static int dect_mdm_hal_neighbor_info_req(const struct device *dev,
					       struct dect_neighbor_info_req_params *params)
{
	struct nrf_modem_dect_mac_neighbor_info_params mdm_params = {
		.long_rd_id = params->long_rd_id,
	};

	return dect_mdm_ctrl_api_neighbor_info_req_cmd(&mdm_params);
}

static int dect_mdm_hal_cluster_info_req(const struct device *dev)
{
	return dect_mdm_ctrl_api_cluster_info_req_cmd();
}

static int dect_mdm_hal_network_create_req(const struct device *dev)
{
	struct dect_mdm_settings *set_ptr = dect_mdm_settings_ref_get();

	if (set_ptr->net_mgmt_common.device_type & DECT_DEVICE_TYPE_FT) {
		return dect_mdm_ctrl_api_network_create_req_cmd();
	} else {
		return -ENOTSUP;
	}
}

static int dect_mdm_hal_network_remove_req(const struct device *dev)
{
	struct dect_mdm_settings *set_ptr = dect_mdm_settings_ref_get();
	struct dect_mdm_mac_dev_context *ctx = &dect_mdm_mac_dev_context_data;
	int i, ret;

	if (set_ptr->net_mgmt_common.device_type & DECT_DEVICE_TYPE_FT) {
		if (dect_mdm_ctrl_api_network_remove_req_cmd_allowed() == false) {
			LOG_ERR("%s: Network remove not allowed", (__func__));
			return -EPERM;
		}
		for (i = 0; i < ARRAY_SIZE(ctx->child_associations); i++) {
			if (ctx->child_associations[i].in_use) {
				/* We are shooting all without waiting an answer */
				ret = dect_mdm_ctrl_api_associate_release_cmd(
					ctx->child_associations[i].target_long_rd_id,
					NRF_MODEM_DECT_MAC_RELEASE_CAUSE_CONNECTION_TERMINATION);
				if (ret) {
					LOG_ERR("%s: error in Association Release: err %d",
						(__func__), ret);
				} else {
					LOG_INF("%s: association release initiated", (__func__));
				}
			}
		}
		k_sleep(K_MSEC(2000)); /* Wait for a while that all association are released */
		return dect_mdm_ctrl_api_network_remove_req_cmd();
	} else {
		return -ENOTSUP;
	}
}

static int dect_mdm_hal_network_join_req(const struct device *dev)
{
	struct dect_mdm_settings *set_ptr = dect_mdm_settings_ref_get();

	if (set_ptr->net_mgmt_common.device_type & DECT_DEVICE_TYPE_PT) {
		return dect_mdm_ctrl_api_network_join_req_cmd();
	} else {
		return -ENOTSUP;
	}
}

static int dect_mdm_hal_network_unjoin_req(const struct device *dev)
{
	struct dect_mdm_settings *set_ptr = dect_mdm_settings_ref_get();

	if (set_ptr->net_mgmt_common.device_type & DECT_DEVICE_TYPE_PT) {
		return dect_mdm_ctrl_api_network_unjoin_req_cmd();
	} else {
		return -ENOTSUP;
	}
}

static const struct dect_nr_hal_api dect_mdm_api = {
	.iface_api.init = dect_mdm_iface_init,
	.activate_req = dect_mdm_hal_mdm_activate,
	.deactivate_req = dect_mdm_hal_mdm_deactivate,
	.send = dect_mdm_hal_driver_send,
	.rssi_scan = dect_mdm_hal_rssi_scan,
	.scan = dect_mdm_hal_scan,
	.associate_req = dect_mdm_hal_associate_req,
	.associate_release = dect_mdm_hal_associate_release,
	.cluster_start_req = dect_mdm_hal_cluster_start_req,
	.cluster_reconfig_req = dect_mdm_hal_cluster_reconfig_req,
	.nw_beacon_start_req = dect_mdm_hal_nw_beacon_start_req,
	.nw_beacon_stop_req = dect_mdm_hal_nw_beacon_stop_req,
	.settings_read = dect_mdm_hal_settings_read,
	.settings_write = dect_mdm_hal_settings_write,
	.status_info_get = dect_mdm_hal_status_info_get,
	.neighbor_list_req = dect_mdm_hal_neighbor_list_req,
	.neighbor_info_req = dect_mdm_hal_neighbor_info_req,
	.cluster_info_req = dect_mdm_hal_cluster_info_req,
	.network_create_req = dect_mdm_hal_network_create_req,
	.network_remove_req = dect_mdm_hal_network_remove_req,
	.network_join_req = dect_mdm_hal_network_join_req,
	.network_unjoin_req = dect_mdm_hal_network_unjoin_req,
};

NET_DEVICE_INIT(nrf91_dect_mdm_driver, CONFIG_DECT_MDM_DEVICE_NAME, dect_mdm_init, NULL,
		&dect_mdm_mac_dev_context_data, NULL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		&dect_mdm_api, DECT_L2, NET_L2_GET_CTX_TYPE(DECT_L2), DECT_MTU);

#if defined(CONFIG_NET_CONNECTION_MANAGER)
#include <net/dect/dect_net_l2_conn_mgr.h>
CONNECTIVITY_DECT_MGMT_BIND(nrf91_dect_mdm_driver);
#endif

void dect_mdm_parent_association_created(uint32_t target_long_rd_id,
	struct nrf_modem_dect_mac_ipv6_address_config_t ipv6_config)
{
	struct dect_mdm_mac_dev_context *ctx = &dect_mdm_mac_dev_context_data;
	struct net_if *iface = dect_mdm_mac_dev_context_data.iface;
	struct dect_mdm_association_data *ass_data_item;
	struct dect_network_status_evt network_status_data = {
		.network_status = DECT_NETWORK_STATUS_JOINED,
	};

	ass_data_item = dect_mdm_parent_association_list_add(target_long_rd_id);
	if (ass_data_item == NULL) {
		LOG_ERR("Cannot add parent to association list - releasing association");
		goto association_release;
	}
	struct dect_net_ipv6_prefix_config ipv6_prefix_config = {
		.prefix.s6_addr = { 0 },
		.prefix_len = 0, /* As a default no prefix */
	};

	if (ipv6_config.type != NRF_MODEM_DECT_MAC_IPV6_ADDRESS_TYPE_NONE) {
		__ASSERT_NO_MSG(ipv6_config.type == NRF_MODEM_DECT_MAC_IPV6_ADDRESS_TYPE_PREFIX ||
				ipv6_config.type == NRF_MODEM_DECT_MAC_IPV6_ADDRESS_TYPE_FULL);
		ipv6_prefix_config.prefix_len =
			(ipv6_config.type == NRF_MODEM_DECT_MAC_IPV6_ADDRESS_TYPE_PREFIX) ? 8 : 16;
		memcpy(&ipv6_prefix_config.prefix.s6_addr,
		       &ipv6_config.address, ipv6_prefix_config.prefix_len);
	}

	__ASSERT_NO_MSG(ctx->parent_long_rd_id == 0); /* Only one parent supported */
	ctx->parent_long_rd_id = target_long_rd_id;

	dect_mdm_parent_created_association_list_addressing_handle(ass_data_item);

	dect_net_l2_parent_association_created(
		iface, target_long_rd_id, &ipv6_prefix_config);

	/* We send also network status event eventhough join hasn't been necessarily called */
	dect_mgmt_network_status_evt(iface, network_status_data);

	return;

association_release:
	dect_mdm_parent_association_list_remove(target_long_rd_id);
	dect_mdm_ctrl_api_associate_release_cmd(
		target_long_rd_id, NRF_MODEM_DECT_MAC_RELEASE_CAUSE_INSUFFICIENT_HW_RESOURCES);
}

void dect_mdm_child_association_created(uint32_t target_long_rd_id)
{
	struct dect_mdm_association_data *association_list_item;
	struct net_if *iface = dect_mdm_mac_dev_context_data.iface;

	association_list_item = dect_mdm_child_association_list_add(target_long_rd_id);
	if (association_list_item == NULL) {
		LOG_ERR("Cannot add child (long rd id %u) to association list - "
			"releasing association",
			target_long_rd_id);
		goto association_release;
	}
	dect_net_l2_child_association_created(iface, target_long_rd_id);
	return;

association_release:
	dect_mdm_child_association_list_remove(target_long_rd_id);
	dect_mdm_ctrl_api_associate_release_cmd(
		target_long_rd_id,
		NRF_MODEM_DECT_MAC_RELEASE_CAUSE_INSUFFICIENT_HW_RESOURCES);
}

void dect_mdm_parent_association_ipv6_config_changed(
	struct nrf_modem_dect_mac_ipv6_address_config_t ipv6_config)
{
	struct net_if *iface = dect_mdm_mac_dev_context_data.iface;
	struct dect_net_ipv6_prefix_config ipv6_prefix_config = {
		.prefix.s6_addr = { 0 },
		.prefix_len = 0, /* As a default no prefix */
	};

	struct dect_mdm_association_data *ass_list_item =
		dect_mdm_parent_association_list_item_get(0);

	if (!ass_list_item) {
		LOG_WRN("%s: no parent association", (__func__));
		return;
	}

	if (ipv6_config.type != NRF_MODEM_DECT_MAC_IPV6_ADDRESS_TYPE_NONE) {
		__ASSERT_NO_MSG(ipv6_config.type == NRF_MODEM_DECT_MAC_IPV6_ADDRESS_TYPE_PREFIX ||
				ipv6_config.type == NRF_MODEM_DECT_MAC_IPV6_ADDRESS_TYPE_FULL);
		ipv6_prefix_config.prefix_len =
			(ipv6_config.type == NRF_MODEM_DECT_MAC_IPV6_ADDRESS_TYPE_PREFIX) ? 8 : 16;
		memcpy(&ipv6_prefix_config.prefix.s6_addr,
		       &ipv6_config.address, sizeof(ipv6_config.address));
	}
	dect_net_l2_parent_ipv6_config_changed(
		iface, ass_list_item->target_long_rd_id, &ipv6_prefix_config);
}

void dect_mdm_parent_association_removed(uint32_t long_rd_id,
	enum nrf_modem_dect_mac_release_cause rel_cause, bool neighbor_initiated)
{
	struct net_if *iface = dect_mdm_mac_dev_context_data.iface;

	dect_net_l2_association_removed(
		iface, long_rd_id, (enum dect_association_release_cause)rel_cause,
		neighbor_initiated);

	/* We are orphan now. The only possible parent is gone. */
	uint8_t *link_addr;
	int ret;

	/* We send also network status event eventhough unjoin hasn't been necessarily called */
	struct dect_network_status_evt network_status_data = {
		.network_status = DECT_NETWORK_STATUS_UNJOINED,
	};

	dect_mgmt_network_status_evt(iface, network_status_data);

	/* Parent removed: remove current link addr  */
	__ASSERT_NO_MSG(dect_mdm_mac_dev_context_data.parent_long_rd_id != 0);
	dect_mdm_mac_dev_context_data.parent_long_rd_id = 0;

	link_addr = dect_mdm_current_link_addr_create(&dect_mdm_mac_dev_context_data);
	ret = net_if_set_link_addr(iface, link_addr, 8, NET_LINK_UNKNOWN);
	if (ret) {
		LOG_ERR("%s: cannot set link addr: %d", (__func__), ret);
	}

	struct dect_mdm_association_data *ass_list_item =
		dect_mdm_parent_association_list_item_get(long_rd_id);

	if (!ass_list_item) {
		LOG_WRN("%s: no association with %u in a parent list", (__func__), long_rd_id);
		return;
	}

	/* ...and finally, remove from our list */
	dect_mdm_parent_association_list_remove(long_rd_id);
}

void dect_mdm_child_association_removed(uint32_t long_rd_id,
	enum nrf_modem_dect_mac_release_cause rel_cause, bool neighbor_initiated)
{
	struct net_if *iface = dect_mdm_mac_dev_context_data.iface;
	struct dect_mdm_association_data *ass_list_item =
		dect_mdm_child_association_list_item_get(long_rd_id);

	dect_net_l2_association_removed(
		iface, long_rd_id, (enum dect_association_release_cause)rel_cause,
		neighbor_initiated);

	if (ass_list_item) {
		dect_mdm_child_association_list_remove(long_rd_id);
	}
}

void dect_mdm_child_association_all_removed(enum nrf_modem_dect_mac_release_cause rel_cause)
{
	struct dect_mdm_mac_dev_context *ctx = &dect_mdm_mac_dev_context_data;

	for (int i = 0; i < ARRAY_SIZE(ctx->child_associations); i++) {
		if (ctx->child_associations[i].in_use) {
			dect_net_l2_association_removed(
				ctx->iface,
				ctx->child_associations[i].target_long_rd_id,
				(enum dect_association_release_cause)rel_cause,
				false);
			dect_mdm_child_association_list_remove(
				ctx->child_associations[i].target_long_rd_id);
		}
	}
}
