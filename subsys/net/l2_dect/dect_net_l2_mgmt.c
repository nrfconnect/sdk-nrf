/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>

#include <net/dect/dect_net_l2.h>
#include <net/dect/dect_net_l2_mgmt.h>

#include "dect_net_l2_internal.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_l2_dect_mgmt, CONFIG_NET_L2_DECT_MGMT_LOG_LEVEL);

static const struct dect_nr_hal_api *const get_dect_nr_hal_api(struct net_if *iface,
							       const struct device *dev)
{
	struct dect_nr_hal_api *api;

	if (iface == NULL || dev == NULL) {
		return NULL;
	}
	api = (struct dect_nr_hal_api *)dev->api;

	return api;
}

void dect_mgmt_activate_done_evt(struct net_if *iface, enum dect_status_values status)
{
	struct dect_common_resp_evt evt = {
		.status = status,
	};

	net_mgmt_event_notify_with_info(NET_EVENT_DECT_ACTIVATE_DONE, iface, &evt, sizeof(evt));
}

void dect_mgmt_deactivate_done_evt(struct net_if *iface, enum dect_status_values status)
{
	struct dect_common_resp_evt evt = {
		.status = status,
	};
	net_mgmt_event_notify_with_info(NET_EVENT_DECT_DEACTIVATE_DONE, iface, &evt, sizeof(evt));
}

void dect_mgmt_nw_beacon_start_evt(struct net_if *iface, enum dect_status_values status)
{
	struct dect_common_resp_evt evt = {
		.status = status,
	};
	net_mgmt_event_notify_with_info(NET_EVENT_DECT_NW_BEACON_START_RESULT, iface, &evt,
					sizeof(evt));
}

void dect_mgmt_nw_beacon_stop_evt(struct net_if *iface, enum dect_status_values status)
{
	struct dect_common_resp_evt evt = {
		.status = status,
	};
	net_mgmt_event_notify_with_info(NET_EVENT_DECT_NW_BEACON_STOP_RESULT, iface, &evt,
					sizeof(evt));
}

void dect_mgmt_rssi_scan_result_evt(struct net_if *iface,
				    struct dect_rssi_scan_result_evt result_data)
{
	net_mgmt_event_notify_with_info(NET_EVENT_DECT_RSSI_SCAN_RESULT, iface, &result_data,
					sizeof(struct dect_rssi_scan_result_evt));
}

void dect_mgmt_rssi_scan_done_evt(struct net_if *iface, enum dect_status_values status)
{
	struct dect_common_resp_evt evt = {
		.status = status,
	};

	net_mgmt_event_notify_with_info(NET_EVENT_DECT_RSSI_SCAN_DONE, iface, &evt, sizeof(evt));
}

void dect_mgmt_association_changed_evt(
	struct net_if *iface,
	struct dect_association_changed_evt evt_data)
{
	net_mgmt_event_notify_with_info(NET_EVENT_DECT_ASSOCIATION_CHANGED, iface, &evt_data,
					sizeof(struct dect_association_changed_evt));
}

void dect_mgmt_association_req_failed_mdm_result_evt(struct net_if *iface, uint32_t long_rd_id,
	enum dect_status_values cause)
{
	struct dect_association_changed_evt evt = {
		.long_rd_id = long_rd_id,
		.neighbor_role = DECT_NEIGHBOR_ROLE_PARENT, /* always parent if req failed */
		.association_change_type = DECT_ASSOCIATION_REQ_FAILED_MDM,
		.association_req_failed_mdm.cause = cause,
	};

	net_mgmt_event_notify_with_info(NET_EVENT_DECT_ASSOCIATION_CHANGED, iface, &evt,
					sizeof(struct dect_association_changed_evt));
}

void dect_mgmt_association_req_rejected_result_evt(struct net_if *iface, uint32_t long_rd_id,
	enum dect_association_reject_cause reject_cause,
	enum dect_association_reject_time reject_time)
{
	struct dect_association_changed_evt evt = {
		.long_rd_id = long_rd_id,
		.neighbor_role = DECT_NEIGHBOR_ROLE_PARENT, /* always parent if req failed */
		.association_change_type = DECT_ASSOCIATION_REQ_REJECTED,
		.association_req_rejected.reject_cause = reject_cause,
		.association_req_rejected.reject_time = reject_time,
	};

	net_mgmt_event_notify_with_info(NET_EVENT_DECT_ASSOCIATION_CHANGED, iface, &evt,
					sizeof(struct dect_association_changed_evt));
}

void dect_mgmt_parent_association_created_evt(struct net_if *iface, uint32_t long_rd_id)
{
	struct dect_association_changed_evt evt = {
		.long_rd_id = long_rd_id,
		.neighbor_role = DECT_NEIGHBOR_ROLE_PARENT,
		.association_change_type = DECT_ASSOCIATION_CREATED,
	};

	net_mgmt_event_notify_with_info(NET_EVENT_DECT_ASSOCIATION_CHANGED, iface, &evt,
					sizeof(struct dect_association_changed_evt));
}

void dect_mgmt_child_association_created_evt(struct net_if *iface, uint32_t long_rd_id)
{
	struct dect_association_changed_evt evt = {
		.long_rd_id = long_rd_id,
		.neighbor_role = DECT_NEIGHBOR_ROLE_CHILD,
		.association_change_type = DECT_ASSOCIATION_CREATED,
	};

	net_mgmt_event_notify_with_info(NET_EVENT_DECT_ASSOCIATION_CHANGED, iface, &evt,
					sizeof(struct dect_association_changed_evt));
}

void dect_mgmt_association_released_evt(struct net_if *iface, uint32_t neighbor_long_rd_id,
	enum dect_neighbor_role neighbor_role, bool neighbor_initiated,
	enum dect_association_release_cause cause)
{
	struct dect_association_changed_evt evt = {
		.long_rd_id = neighbor_long_rd_id,
		.neighbor_role = neighbor_role,
		.association_change_type = DECT_ASSOCIATION_RELEASED,
		.association_released.neighbor_initiated = neighbor_initiated,
		.association_released.release_cause = cause,
	};

	net_mgmt_event_notify_with_info(NET_EVENT_DECT_ASSOCIATION_CHANGED, iface, &evt,
					sizeof(struct dect_association_changed_evt));
}

void dect_mgmt_cluster_created_evt(struct net_if *iface,
				   struct dect_cluster_start_resp_evt resp_data)
{
	net_mgmt_event_notify_with_info(NET_EVENT_DECT_CLUSTER_CREATED_RESULT, iface, &resp_data,
					sizeof(struct dect_cluster_start_resp_evt));
}

void dect_mgmt_cluster_stopped_evt(struct net_if *iface, enum dect_status_values status)
{
	struct dect_common_resp_evt evt = {
		.status = status,
	};

	net_mgmt_event_notify_with_info(NET_EVENT_DECT_CLUSTER_STOPPED_RESULT, iface, &evt,
					sizeof(evt));
}

void dect_mgmt_network_status_evt(struct net_if *iface,
				  struct dect_network_status_evt network_status_data)
{
	net_mgmt_event_notify_with_info(NET_EVENT_DECT_NETWORK_STATUS, iface, &network_status_data,
					sizeof(struct dect_network_status_evt));
}

void dect_mgmt_sink_status_evt(struct net_if *iface,
			       struct dect_sink_status_evt sink_status_data)
{
	net_mgmt_event_notify_with_info(NET_EVENT_DECT_SINK_STATUS, iface, &sink_status_data,
					sizeof(struct dect_sink_status_evt));
}

void dect_mgmt_neighbor_list_evt(struct net_if *iface,
				 struct dect_neighbor_list_evt neighbor_list_data)
{
	net_mgmt_event_notify_with_info(NET_EVENT_DECT_NEIGHBOR_LIST, iface, &neighbor_list_data,
					sizeof(struct dect_neighbor_list_evt));
}

void dect_mgmt_neighbor_info_evt(struct net_if *iface,
				 struct dect_neighbor_info_evt neighbor_info_data)
{
	net_mgmt_event_notify_with_info(NET_EVENT_DECT_NEIGHBOR_INFO, iface, &neighbor_info_data,
					sizeof(struct dect_neighbor_info_evt));
}

void dect_mgmt_cluster_info_evt(struct net_if *iface,
				struct dect_cluster_info_evt cluster_info_data)
{
	net_mgmt_event_notify_with_info(NET_EVENT_DECT_CLUSTER_INFO, iface, &cluster_info_data,
					sizeof(struct dect_cluster_info_evt));
}

static void scan_result_cb(struct net_if *iface, enum dect_status_values status,
			   struct dect_scan_result_evt *entry)
{
	if (!iface) {
		return;
	}

	if (!entry) {
		net_mgmt_event_notify_with_info(NET_EVENT_DECT_SCAN_DONE, iface, &status,
						sizeof(enum dect_status_values));
		return;
	}

	net_mgmt_event_notify_with_info(NET_EVENT_DECT_SCAN_RESULT, iface, entry,
					sizeof(struct dect_scan_result_evt));
}

static int dect_mgmt_activate(uint64_t mgmt_request, struct net_if *iface, void *data, size_t len)
{
	const struct device *dev = net_if_get_device(iface);
	const struct dect_nr_hal_api *const api = get_dect_nr_hal_api(iface, dev);

	if (api == NULL || api->activate_req == NULL) {
		return -ENOTSUP;
	}

	return api->activate_req(dev);
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_DECT_ACTIVATE, dect_mgmt_activate);

static int dect_mgmt_deactivate(uint64_t mgmt_request, struct net_if *iface, void *data, size_t len)
{
	const struct device *dev = net_if_get_device(iface);
	const struct dect_nr_hal_api *const api = get_dect_nr_hal_api(iface, dev);

	if (api == NULL || api->deactivate_req == NULL) {
		return -ENOTSUP;
	}

	return api->deactivate_req(dev);
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_DECT_DEACTIVATE, dect_mgmt_deactivate);

static int dect_mgmt_rssi_scan(uint64_t mgmt_request, struct net_if *iface, void *data, size_t len)
{
	const struct device *dev = net_if_get_device(iface);
	const struct dect_nr_hal_api *const api = get_dect_nr_hal_api(iface, dev);
	struct dect_rssi_scan_params *params = data;

	if (len != sizeof(struct dect_rssi_scan_params) || api == NULL || api->rssi_scan == NULL) {
		return -ENOTSUP;
	}

	return api->rssi_scan(dev, params);
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_DECT_RSSI_SCAN, dect_mgmt_rssi_scan);

static int dect_mgmt_scan(uint64_t mgmt_request, struct net_if *iface, void *data, size_t len)
{
	const struct device *dev = net_if_get_device(iface);
	const struct dect_nr_hal_api *const api = get_dect_nr_hal_api(iface, dev);
	struct dect_scan_params *params = data;

	if (len != sizeof(struct dect_scan_params) || api == NULL || api->scan == NULL) {
		return -ENOTSUP;
	}

	return api->scan(dev, params, scan_result_cb);
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_DECT_SCAN, dect_mgmt_scan);

static int dect_mgmt_network_create(uint64_t mgmt_request, struct net_if *iface, void *data,
				    size_t len)
{
	const struct device *dev = net_if_get_device(iface);
	const struct dect_nr_hal_api *const api = get_dect_nr_hal_api(iface, dev);

	if (api == NULL || api->network_create_req == NULL) {
		return -ENOTSUP;
	}

	return api->network_create_req(dev);
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_DECT_NETWORK_CREATE, dect_mgmt_network_create);

static int dect_mgmt_network_remove(uint64_t mgmt_request, struct net_if *iface, void *data,
				    size_t len)
{
	const struct device *dev = net_if_get_device(iface);
	const struct dect_nr_hal_api *const api = get_dect_nr_hal_api(iface, dev);

	if (api == NULL || api->network_remove_req == NULL) {
		return -ENOTSUP;
	}

	return api->network_remove_req(dev);
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_DECT_NETWORK_REMOVE, dect_mgmt_network_remove);

static int dect_mgmt_network_join(uint64_t mgmt_request, struct net_if *iface, void *data,
				  size_t len)
{
	const struct device *dev = net_if_get_device(iface);
	const struct dect_nr_hal_api *const api = get_dect_nr_hal_api(iface, dev);

	if (api == NULL || api->network_join_req == NULL) {
		return -ENOTSUP;
	}

	return api->network_join_req(dev);
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_DECT_NETWORK_JOIN, dect_mgmt_network_join);

static int dect_mgmt_network_unjoin(uint64_t mgmt_request, struct net_if *iface, void *data,
				    size_t len)
{
	const struct device *dev = net_if_get_device(iface);
	const struct dect_nr_hal_api *const api = get_dect_nr_hal_api(iface, dev);

	if (api == NULL || api->network_unjoin_req == NULL) {
		return -ENOTSUP;
	}

	return api->network_unjoin_req(dev);
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_DECT_NETWORK_UNJOIN, dect_mgmt_network_unjoin);

static int dect_mgmt_status_info_get(uint64_t mgmt_request, struct net_if *iface, void *data,
				     size_t len)
{
	const struct device *dev = net_if_get_device(iface);
	const struct dect_nr_hal_api *const api = get_dect_nr_hal_api(iface, dev);
	struct dect_status_info *status_out = data;
	int err;

	if (len != sizeof(struct dect_status_info) || api == NULL || api->status_info_get == NULL) {
		return -ENOTSUP;
	}
	err = api->status_info_get(dev, status_out);
	if (!err) {
		/* Fill the association L2 data */
		dect_net_l2_status_info_fill_association_data(iface, status_out);

		/* Fill the sink L2 data */
		dect_net_l2_status_info_fill_sink_data(iface, status_out);
	}
	return err;
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_DECT_STATUS_INFO_GET, dect_mgmt_status_info_get);

static int dect_mgmt_settings_read(uint64_t mgmt_request, struct net_if *iface, void *data,
				   size_t len)
{
	const struct device *dev = net_if_get_device(iface);
	const struct dect_nr_hal_api *const api = get_dect_nr_hal_api(iface, dev);
	struct dect_settings *settings_out = data;

	if (len != sizeof(struct dect_settings) || api == NULL || api->settings_read == NULL) {
		return -ENOTSUP;
	}

	return api->settings_read(dev, settings_out);
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_DECT_SETTINGS_READ, dect_mgmt_settings_read);

static int dect_mgmt_settings_write(uint64_t mgmt_request, struct net_if *iface, void *data,
				    size_t len)
{
	const struct device *dev = net_if_get_device(iface);
	const struct dect_nr_hal_api *const api = get_dect_nr_hal_api(iface, dev);
	struct dect_settings *settings_in = data;

	if (len != sizeof(struct dect_settings) || api == NULL || api->settings_write == NULL) {
		return -ENOTSUP;
	}
	uint16_t write_scope_bitmap_in = settings_in->cmd_params.write_scope_bitmap;

	if (settings_in->cmd_params.reset_to_driver_defaults) {
		write_scope_bitmap_in = DECT_SETTINGS_WRITE_SCOPE_ALL;
	}
	/* Validate writing scope */
	if (!((write_scope_bitmap_in & DECT_SETTINGS_WRITE_SCOPE_AUTO_START) ||
	      (write_scope_bitmap_in & DECT_SETTINGS_WRITE_SCOPE_REGION) ||
	      (write_scope_bitmap_in & DECT_SETTINGS_WRITE_SCOPE_DEVICE_TYPE) ||
	      (write_scope_bitmap_in & DECT_SETTINGS_WRITE_SCOPE_IDENTITIES) ||
	      (write_scope_bitmap_in & DECT_SETTINGS_WRITE_SCOPE_TX) ||
	      (write_scope_bitmap_in & DECT_SETTINGS_WRITE_SCOPE_POWER_SAVE) ||
	      (write_scope_bitmap_in & DECT_SETTINGS_WRITE_SCOPE_BAND_NBR) ||
	      (write_scope_bitmap_in & DECT_SETTINGS_WRITE_SCOPE_RSSI_SCAN) ||
	      (write_scope_bitmap_in & DECT_SETTINGS_WRITE_SCOPE_CLUSTER) ||
	      (write_scope_bitmap_in & DECT_SETTINGS_WRITE_SCOPE_NW_BEACON) ||
	      (write_scope_bitmap_in & DECT_SETTINGS_WRITE_SCOPE_ASSOCIATION) ||
	      (write_scope_bitmap_in & DECT_SETTINGS_WRITE_SCOPE_NETWORK_JOIN) ||
	      (write_scope_bitmap_in & DECT_SETTINGS_WRITE_SCOPE_SECURITY_CONFIGURATION))) {
		return -EINVAL;
	}

	return api->settings_write(dev, settings_in);
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_DECT_SETTINGS_WRITE, dect_mgmt_settings_write);

static int dect_mgmt_associate_req(uint64_t mgmt_request, struct net_if *iface, void *data,
				   size_t len)
{
	const struct device *dev = net_if_get_device(iface);
	const struct dect_nr_hal_api *const api = get_dect_nr_hal_api(iface, dev);
	struct dect_associate_req_params *params = data;

	if (len != sizeof(struct dect_associate_req_params) || api == NULL ||
	    api->associate_req == NULL) {
		return -ENOTSUP;
	}

	return api->associate_req(dev, params);
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_DECT_ASSOCIATION, dect_mgmt_associate_req);

static int dect_mgmt_associate_release(uint64_t mgmt_request, struct net_if *iface, void *data,
				       size_t len)
{
	const struct device *dev = net_if_get_device(iface);
	const struct dect_nr_hal_api *const api = get_dect_nr_hal_api(iface, dev);
	struct dect_associate_rel_params *params = data;

	if (len != sizeof(struct dect_associate_rel_params) || api == NULL ||
	    api->associate_release == NULL) {
		return -ENOTSUP;
	}

	return api->associate_release(dev, params);
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_DECT_ASSOCIATION_RELEASE,
				  dect_mgmt_associate_release);

static int dect_mgmt_cluster_start_req(uint64_t mgmt_request, struct net_if *iface, void *data,
				       size_t len)
{
	const struct device *dev = net_if_get_device(iface);
	const struct dect_nr_hal_api *const api = get_dect_nr_hal_api(iface, dev);
	struct dect_cluster_start_req_params *params = data;

	if (len != sizeof(struct dect_cluster_start_req_params) || api == NULL ||
	    api->cluster_start_req == NULL) {
		return -ENOTSUP;
	}

	return api->cluster_start_req(dev, params);
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_DECT_CLUSTER_START, dect_mgmt_cluster_start_req);

static int dect_mgmt_cluster_reconfig_req(uint64_t mgmt_request, struct net_if *iface, void *data,
					   size_t len)
{
	const struct device *dev = net_if_get_device(iface);
	const struct dect_nr_hal_api *const api = get_dect_nr_hal_api(iface, dev);
	struct dect_cluster_reconfig_req_params *params = data;

	if (len != sizeof(struct dect_cluster_reconfig_req_params) || api == NULL ||
	    api->cluster_reconfig_req == NULL) {
		return -ENOTSUP;
	}

	return api->cluster_reconfig_req(dev, params);
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_DECT_CLUSTER_RECONFIGURE,
				  dect_mgmt_cluster_reconfig_req);

static int dect_mgmt_cluster_stop_req(uint64_t mgmt_request, struct net_if *iface, void *data,
				      size_t len)
{
	const struct device *dev = net_if_get_device(iface);
	const struct dect_nr_hal_api *const api = get_dect_nr_hal_api(iface, dev);
	struct dect_cluster_stop_req_params *params = data;

	if (len != sizeof(struct dect_cluster_stop_req_params)) {
		return -EINVAL;
	}

	if (api == NULL || api->cluster_stop_req == NULL) {
		return -ENOTSUP;
	}

	return api->cluster_stop_req(dev, params);
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_DECT_CLUSTER_STOP, dect_mgmt_cluster_stop_req);

static int dect_mgmt_nw_beacon_start_req(uint64_t mgmt_request, struct net_if *iface, void *data,
					 size_t len)
{
	const struct device *dev = net_if_get_device(iface);
	const struct dect_nr_hal_api *const api = get_dect_nr_hal_api(iface, dev);
	struct dect_nw_beacon_start_req_params *params = data;

	if (len != sizeof(struct dect_nw_beacon_start_req_params) || api == NULL ||
	    api->nw_beacon_start_req == NULL) {
		return -ENOTSUP;
	}

	return api->nw_beacon_start_req(dev, params);
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_DECT_NW_BEACON_START, dect_mgmt_nw_beacon_start_req);

static int dect_mgmt_nw_beacon_stop_req(uint64_t mgmt_request, struct net_if *iface, void *data,
					size_t len)
{
	const struct device *dev = net_if_get_device(iface);
	const struct dect_nr_hal_api *const api = get_dect_nr_hal_api(iface, dev);
	struct dect_nw_beacon_stop_req_params *params = data;

	if (len != sizeof(struct dect_nw_beacon_stop_req_params) || api == NULL ||
	    api->nw_beacon_stop_req == NULL) {
		return -ENOTSUP;
	}

	return api->nw_beacon_stop_req(dev, params);
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_DECT_NW_BEACON_STOP, dect_mgmt_nw_beacon_stop_req);

static int dect_mgmt_neighbor_list_req(uint64_t mgmt_request, struct net_if *iface, void *data,
				       size_t len)
{
	const struct device *dev = net_if_get_device(iface);
	const struct dect_nr_hal_api *const api = get_dect_nr_hal_api(iface, dev);

	if (len != 0 || api == NULL || api->neighbor_list_req == NULL) {
		return -ENOTSUP;
	}

	return api->neighbor_list_req(dev);
}
NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_DECT_NEIGHBOR_LIST, dect_mgmt_neighbor_list_req);

static int dect_mgmt_neighbor_info_req(uint64_t mgmt_request, struct net_if *iface, void *data,
				       size_t len)
{
	const struct device *dev = net_if_get_device(iface);
	const struct dect_nr_hal_api *const api = get_dect_nr_hal_api(iface, dev);
	struct dect_neighbor_info_req_params *params = data;

	if (len != sizeof(struct dect_neighbor_info_req_params) || api == NULL ||
	    api->neighbor_info_req == NULL) {
		return -ENOTSUP;
	}

	return api->neighbor_info_req(dev, params);
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_DECT_NEIGHBOR_INFO, dect_mgmt_neighbor_info_req);

static int dect_mgmt_cluster_info_req(uint64_t mgmt_request, struct net_if *iface, void *data,
				      size_t len)
{
	const struct device *dev = net_if_get_device(iface);
	const struct dect_nr_hal_api *const api = get_dect_nr_hal_api(iface, dev);

	if (api == NULL || api->cluster_info_req == NULL) {
		return -ENOTSUP;
	}

	return api->cluster_info_req(dev);
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_DECT_CLUSTER_INFO, dect_mgmt_cluster_info_req);
