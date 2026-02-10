/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_pkt.h>
#include <stdint.h>
#include <stdio.h>

#include <modem/nrf_modem_lib.h>
#include <nrf_modem_dect.h>

#include <net/dect/dect_net_l2_mgmt.h>

#include "dect_mdm_common.h"
#include "dect_mdm_rx.h"
#include "dect_mdm_ctrl.h"
#include "dect_mdm_ctrl_internal.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(dect_mdm, CONFIG_DECT_MDM_LOG_LEVEL);

/* External control data (defined in dect_mdm_ctrl.c) */
extern struct dect_mdm_ctrl_data ctrl_data;

/* Modem callback functions */

static void
dect_mdm_ctrl_mdm_cfun_cb(struct nrf_modem_dect_mac_control_functional_mode_cb_params *params)
{
	dect_mdm_ctrl_internal_msgq_data_op_add(DECT_MDM_CTRL_OP_MDM_CFUN_RESP, &params->status,
						  sizeof(params->status));
}

static void
dect_mdm_ctrl_mdm_configure_cb(struct nrf_modem_dect_mac_control_configure_cb_params *params)
{
	dect_mdm_ctrl_internal_msgq_data_op_add(DECT_MDM_CTRL_OP_MDM_CONFIGURE_RESP,
						  &params->status, sizeof(params->status));
}

static void
dect_mdm_ctrl_mdm_systemmode_cb(struct nrf_modem_dect_mac_control_systemmode_cb_params *params)
{
	k_sem_give(&dect_mdm_libmodem_api_sema);
}

static void
dect_mdm_ctrl_mdm_capability_ntf_cb(struct nrf_modem_dect_mac_capability_ntf_cb_params *params)
{
	dect_mdm_ctrl_internal_msgq_data_op_add(
		DECT_MDM_CTRL_OP_MDM_CAPABILITIES, params,
		sizeof(struct nrf_modem_dect_mac_capability_ntf_cb_params));
}

static void dect_mdm_ctrl_mdm_rssi_scan_cb(struct nrf_modem_dect_mac_rssi_scan_cb_params *params)
{
	dect_mdm_ctrl_internal_msgq_data_op_add(DECT_MDM_CTRL_OP_MDM_RSSI_COMPLETE,
						  &params->status, sizeof(params->status));
}

static void
dect_mdm_ctrl_mdm_rssi_scan_ntf_cb(struct nrf_modem_dect_mac_rssi_scan_ntf_cb_params *params)
{
	struct dect_mdm_ctrl_rssi_measurement_data_evt evt_data;

	__ASSERT_NO_MSG(params->rssi_meas_array_size == DECT_MDM_RSSI_MEAS_ARR_SIZE);
	evt_data.rssi_result.channel = params->channel;
	evt_data.rssi_result.busy_percentage = params->busy_percentage;

	for (int i = 0; i < params->rssi_meas_array_size; i++) {
		if (i >= DECT_MDM_RSSI_MEAS_ARR_SIZE) {
			printk("Too many RSSI measurements, max %d", DECT_MDM_RSSI_MEAS_ARR_SIZE);
			break;
		}
		evt_data.rssi_result.busy[i] = params->busy[i];
		evt_data.rssi_result.possible[i] = params->possible[i];
		evt_data.rssi_result.free[i] = params->free[i];
	}

	dect_mdm_ctrl_internal_msgq_data_op_add(
		DECT_MDM_CTRL_OP_MDM_RSSI_RESULT, &evt_data,
		sizeof(struct dect_mdm_ctrl_rssi_measurement_data_evt));
}

static void
dect_mdm_ctrl_mdm_rssi_scan_stop_cb(struct nrf_modem_dect_mac_rssi_scan_stop_cb_params *params)
{
	dect_mdm_ctrl_internal_msgq_data_op_add(DECT_MDM_CTRL_OP_MDM_RSSI_STOPPED,
						  &params->status, sizeof(params->status));
}

static void dect_mdm_ctrl_mdm_cluster_configure_cb(
	struct nrf_modem_dect_mac_cluster_configure_cb_params *params)
{
	dect_mdm_ctrl_internal_msgq_data_op_add(DECT_MDM_CTRL_OP_CLUSTER_CONFIG_RESP,
						  &params->status, sizeof(params->status));
}

static void dect_mdm_ctrl_mdm_cluster_ch_load_change_ntf_cb(
	struct nrf_modem_dect_mac_cluster_ch_load_change_ntf_cb_params *params)
{
	dect_mdm_ctrl_internal_msgq_data_op_add(
		DECT_MDM_CTRL_OP_CLUSTER_CH_LOAD_CHANGED, params,
		sizeof(struct nrf_modem_dect_mac_cluster_ch_load_change_ntf_cb_params));
}

static void dect_mdm_ctrl_mdm_neighbor_inactivity_ntf_cb(
	struct nrf_modem_dect_mac_neighbor_inactivity_ntf_cb_params *params)
{
	dect_mdm_ctrl_internal_msgq_data_op_add(
		DECT_MDM_CTRL_OP_NEIGHBOR_INACTIVITY, params,
		sizeof(struct nrf_modem_dect_mac_neighbor_inactivity_ntf_cb_params));
}

static void dect_mdm_ctrl_mdm_network_beacon_configure_cb(
	struct nrf_modem_dect_mac_network_beacon_configure_cb_params *params)
{
	dect_mdm_ctrl_internal_msgq_data_op_add(
		DECT_MDM_CTRL_OP_MDM_NW_BEACON_START_OR_STOP_DONE, &params->status,
		sizeof(params->status));
}

static void
dect_mdm_ctrl_mdm_association_ntf_cb(struct nrf_modem_dect_mac_association_ntf_cb_params *params)
{
	dect_mdm_ctrl_internal_msgq_data_op_add(
		DECT_MDM_CTRL_OP_MDM_ASSOCIATION_IND, params,
		sizeof(struct nrf_modem_dect_mac_association_ntf_cb_params));
}

static void dect_mdm_ctrl_mdm_association_release_ntf_cb(
	struct nrf_modem_dect_mac_association_release_ntf_cb_params *params)
{
	dect_mdm_ctrl_internal_msgq_data_op_add(
		DECT_MDM_CTRL_OP_MDM_ASSOCIATION_RELEASE_IND, params,
		sizeof(struct nrf_modem_dect_mac_association_release_ntf_cb_params));
}

static void
dect_mdm_ctrl_mdm_network_scan_cb(struct nrf_modem_dect_mac_network_scan_cb_params *params)
{
	dect_mdm_ctrl_internal_msgq_data_op_add(
		DECT_MDM_CTRL_OP_MDM_NW_SCAN_COMPLETE, params,
		sizeof(struct nrf_modem_dect_mac_network_scan_cb_params));
}

static void dect_mdm_ctrl_mdm_cluster_beacon_ntf_cb(
	struct nrf_modem_dect_mac_cluster_beacon_ntf_cb_params *params)
{
	dect_mdm_ctrl_internal_msgq_data_op_add(
		DECT_MDM_CTRL_OP_MDM_CLUSTER_BEACON_RCVD, params,
		sizeof(struct nrf_modem_dect_mac_cluster_beacon_ntf_cb_params));
}

static void dect_mdm_ctrl_mdm_network_beacon_ntf_cb(
	struct nrf_modem_dect_mac_network_beacon_ntf_cb_params *params)
{
	dect_mdm_ctrl_internal_msgq_data_op_add(
		DECT_MDM_CTRL_OP_MDM_NW_BEACON_RCVD, params,
		sizeof(struct nrf_modem_dect_mac_network_beacon_ntf_cb_params));
}

static void dect_mdm_ctrl_mdm_network_scan_stop_cb(
	struct nrf_modem_dect_mac_network_scan_stop_cb_params *params)
{
	dect_mdm_ctrl_internal_msgq_data_op_add(DECT_MDM_CTRL_OP_MDM_NW_SCAN_STOPPED,
						  &params->status, sizeof(params->status));
}

static void dect_mdm_ctrl_mdm_cluster_beacon_receive_cb(
	struct nrf_modem_dect_mac_cluster_beacon_receive_cb_params *params)
{
	dect_mdm_ctrl_internal_msgq_data_op_add(DECT_MDM_CTRL_OP_MDM_CLUSTER_RCV_COMPLETE,
						  &params->cluster_status[0],
						  sizeof(params->cluster_status[0]));
}

static void
dect_mdm_ctrl_mdm_association_cb(struct nrf_modem_dect_mac_association_cb_params *params)
{
	dect_mdm_ctrl_internal_msgq_data_op_add(
		DECT_MDM_CTRL_OP_MDM_ASSOCIATION_RESP, params,
		sizeof(struct nrf_modem_dect_mac_association_cb_params));
}

static void dect_mdm_ctrl_mdm_association_release_cb(
	struct nrf_modem_dect_mac_association_release_cb_params *params)
{
	dect_mdm_ctrl_internal_msgq_data_op_add(
		DECT_MDM_CTRL_OP_MDM_ASSOCIATION_RELEASE_RESP, params,
		sizeof(struct nrf_modem_dect_mac_association_release_cb_params));
}

static void
dect_mdm_ctrl_mdm_dlc_data_rx_ntf_cb(struct nrf_modem_dect_dlc_data_rx_ntf_cb_params *params)
{
	struct net_pkt *rcv_pkt;
	int ret;

	/* Allocate net_pkt using Zephyr's internal memory pools - ISR safe with K_NO_WAIT */
	/* Note: ctrl_data.iface is read without mutex here. This is safe because:
	 * 1. This callback may be called from ISR context where mutex cannot be used
	 * 2. ctrl_data.iface is set once during initialization (protected by mutex)
	 *    and never changes afterward
	 */
	rcv_pkt = net_pkt_rx_alloc_with_buffer(ctrl_data.iface, params->data_len, AF_UNSPEC, 0,
					       K_NO_WAIT);
	if (!rcv_pkt) {
		printk("%s: RX packet allocation failed in ISR (len=%d), dropping\n", __func__,
		       params->data_len);
		return;
	}

	/* Write data to packet */
	ret = net_pkt_write(rcv_pkt, params->data, params->data_len);
	if (ret < 0) {
		printk("%s: Failed to write RX data to packet (len=%d), err=%d\n", __func__,
		       params->data_len, ret);
		net_pkt_unref(rcv_pkt);
		return;
	}

	/* Prepare data for RX thread processing */
	struct dect_mdm_ctrl_dlc_rx_data_with_pkt_ptr mdm_dlc_data_with_pkt_ptr_params;

	mdm_dlc_data_with_pkt_ptr_params.mdm_params = *params;
	mdm_dlc_data_with_pkt_ptr_params.data_len = params->data_len;
	/* See comment above: iface is safe to read without mutex in ISR context */
	mdm_dlc_data_with_pkt_ptr_params.iface = ctrl_data.iface;
	mdm_dlc_data_with_pkt_ptr_params.pkt = rcv_pkt;

	/* Queue for processing in RX thread */
	ret = dect_mdm_rx_msgq_data_op_add(
		DECT_MDM_RX_OP_RX_DATA_WITH_PKT_PTR, (void *)&mdm_dlc_data_with_pkt_ptr_params,
		sizeof(struct dect_mdm_ctrl_dlc_rx_data_with_pkt_ptr));
	if (ret) {
		printk("%s: Failed to queue RX data for processing, err=%d\n", __func__, ret);
		net_pkt_unref(rcv_pkt); /* Clean up packet if queueing fails */
		return;
	}
}

static void dect_mdm_ctrl_mdm_dlc_data_tx_cb(struct nrf_modem_dect_dlc_data_tx_cb_params *params)
{
	struct dect_mdm_ctrl_dlc_data_tx_resp_evt evt_data;

	evt_data.status = params->status;
	evt_data.long_rd_id = params->long_rd_id;
	evt_data.flow_id = params->flow_id;
	evt_data.num_acked_data = 1;
	evt_data.acked_data[0].transaction_id = params->transaction_id;

	dect_mdm_ctrl_internal_msgq_data_op_add(
		DECT_MDM_CTRL_OP_MDM_DLC_DATA_RESP, &evt_data,
		sizeof(struct dect_mdm_ctrl_dlc_data_tx_resp_evt));
}

static void
dect_mdm_ctrl_mdm_cluster_info_ntf_cb(struct nrf_modem_dect_mac_cluster_info_cb_params *params)
{
	dect_mdm_ctrl_internal_msgq_data_op_add(
		DECT_MDM_CTRL_OP_MDM_CLUSTER_INFO, params,
		sizeof(struct nrf_modem_dect_mac_cluster_info_cb_params));
}

static void
dect_mdm_ctrl_mdm_neighbor_info_cb(struct nrf_modem_dect_mac_neighbor_info_cb_params *params)
{
	dect_mdm_ctrl_internal_msgq_data_op_add(
		DECT_MDM_CTRL_OP_MDM_NEIGHBOR_INFO, params,
		sizeof(struct nrf_modem_dect_mac_neighbor_info_cb_params));
}

BUILD_ASSERT(DECT_MDM_MAX_NEIGHBOR_LIST_COUNT == DECT_L2_MAX_NEIGHBOR_LIST_ITEM_COUNT,
	     "Neighbor list size mismatch between DECT L2 and DECT NRF91");
static void
dect_mdm_ctrl_mdm_neighbor_list_cb(struct nrf_modem_dect_mac_neighbor_list_cb_params *params)
{
	struct dect_mdm_ctrl_neighbor_list_resp_evt evt_data;

	evt_data.status = params->status;
	evt_data.num_neighbors = params->num_neighbors;
	for (int i = 0; i < params->num_neighbors; i++) {
		if (i >= DECT_MDM_MAX_NEIGHBOR_LIST_COUNT) {
			printk("Too many neighbors, only first %d will be used, "
			       "long RD ID %u dropped first\n",
			       DECT_MDM_MAX_NEIGHBOR_LIST_COUNT, params->long_rd_ids[i]);
			evt_data.num_neighbors = DECT_MDM_MAX_NEIGHBOR_LIST_COUNT;
			break;
		}
		evt_data.neighbor_long_rd_ids[i] = params->long_rd_ids[i];
	}

	dect_mdm_ctrl_internal_msgq_data_op_add(
		DECT_MDM_CTRL_OP_MDM_NEIGHBOR_LIST, &evt_data,
		sizeof(struct dect_mdm_ctrl_neighbor_list_resp_evt));
}

static void dect_mdm_ctrl_mdm_flow_control_ntf_cb(
	struct nrf_modem_dect_dlc_flow_control_ntf_cb_params *params)
{
	dect_mdm_ctrl_internal_msgq_data_op_add(
		DECT_MDM_CTRL_OP_MDM_FLOW_CONTROL, params,
		sizeof(struct nrf_modem_dect_dlc_flow_control_ntf_cb_params));
}

static void
dect_mdm_ctrl_mdm_dlc_data_discard_cb(struct nrf_modem_dect_dlc_data_discard_cb_params *params)
{
	printk("Data discard completed, status: %hu\n", params->status);
}

static void dect_mdm_ctrl_mdm_cluster_beacon_receive_stop_cb(
	struct nrf_modem_dect_mac_cluster_beacon_receive_stop_cb_params *params)
{
	printk("Cluster beacon stop completed, status: %hu\n", params->status);
}

static void dect_mdm_ctrl_mdm_neighbor_paging_failure_ntf_cb(
	struct nrf_modem_dect_mac_neighbor_paging_failure_ntf_cb_params *params)
{
	dect_mdm_ctrl_internal_msgq_data_op_add(
		DECT_MDM_CTRL_OP_NEIGHBOR_PAGING_FAILURE, params,
		sizeof(struct nrf_modem_dect_mac_neighbor_paging_failure_ntf_cb_params));
}

static void dect_mdm_ctrl_mdm_cluster_beacon_rx_fail_ntf_cb(
	struct nrf_modem_dect_mac_cluster_beacon_rx_failure_ntf_cb_params *params)
{
	dect_mdm_ctrl_internal_msgq_data_op_add(
		DECT_MDM_CTRL_OP_CLUSTER_BEACON_RX_FAILURE, params,
		sizeof(struct nrf_modem_dect_mac_cluster_beacon_rx_failure_ntf_cb_params));
}

static void dect_mdm_ctrl_mdm_ipv6_config_changed_ntf_cb(
	struct nrf_modem_dect_mac_ipv6_config_update_ntf_cb_params *params)
{
	dect_mdm_ctrl_internal_msgq_data_op_add(
		DECT_MDM_CTRL_OP_MDM_IPV6_CONFIG_CHANGED, params,
		sizeof(struct nrf_modem_dect_mac_ipv6_config_update_ntf_cb_params));
}

/* Callback structures */

static const struct nrf_modem_dect_mac_ntf_callbacks mac_ntf_callbacks = {
	.association_ntf = dect_mdm_ctrl_mdm_association_ntf_cb,
	.association_release_ntf = dect_mdm_ctrl_mdm_association_release_ntf_cb,
	.cluster_ch_load_change_ntf = dect_mdm_ctrl_mdm_cluster_ch_load_change_ntf_cb,
	.neighbor_inactivity_ntf = dect_mdm_ctrl_mdm_neighbor_inactivity_ntf_cb,
	.neighbor_paging_failure_ntf = dect_mdm_ctrl_mdm_neighbor_paging_failure_ntf_cb,
	.rssi_scan_ntf = dect_mdm_ctrl_mdm_rssi_scan_ntf_cb,
	.cluster_beacon_ntf = dect_mdm_ctrl_mdm_cluster_beacon_ntf_cb,
	.network_beacon_ntf = dect_mdm_ctrl_mdm_network_beacon_ntf_cb,
	.dlc_data_rx_ntf = dect_mdm_ctrl_mdm_dlc_data_rx_ntf_cb,
	.capability_ntf = dect_mdm_ctrl_mdm_capability_ntf_cb,
	.cluster_beacon_rx_failure_ntf = dect_mdm_ctrl_mdm_cluster_beacon_rx_fail_ntf_cb,
	.dlc_flow_control_ntf = dect_mdm_ctrl_mdm_flow_control_ntf_cb,
	.ipv6_config_update_ntf = dect_mdm_ctrl_mdm_ipv6_config_changed_ntf_cb,
};

static const struct nrf_modem_dect_mac_op_callbacks mac_op_callbacks = {
	.control_functional_mode = dect_mdm_ctrl_mdm_cfun_cb,
	.control_configure = dect_mdm_ctrl_mdm_configure_cb,
	.control_systemmode = dect_mdm_ctrl_mdm_systemmode_cb,
	.association = dect_mdm_ctrl_mdm_association_cb,
	.association_release = dect_mdm_ctrl_mdm_association_release_cb,
	.cluster_beacon_receive = dect_mdm_ctrl_mdm_cluster_beacon_receive_cb,
	.cluster_beacon_receive_stop = dect_mdm_ctrl_mdm_cluster_beacon_receive_stop_cb,
	.cluster_configure = dect_mdm_ctrl_mdm_cluster_configure_cb,
	.cluster_info = dect_mdm_ctrl_mdm_cluster_info_ntf_cb,
	.neighbor_info = dect_mdm_ctrl_mdm_neighbor_info_cb,
	.neighbor_list = dect_mdm_ctrl_mdm_neighbor_list_cb,
	.dlc_data_tx = dect_mdm_ctrl_mdm_dlc_data_tx_cb,
	.dlc_data_discard = dect_mdm_ctrl_mdm_dlc_data_discard_cb,
	.network_beacon_configure = dect_mdm_ctrl_mdm_network_beacon_configure_cb,
	.network_scan = dect_mdm_ctrl_mdm_network_scan_cb,
	.network_scan_stop = dect_mdm_ctrl_mdm_network_scan_stop_cb,
	.rssi_scan = dect_mdm_ctrl_mdm_rssi_scan_cb,
	.rssi_scan_stop = dect_mdm_ctrl_mdm_rssi_scan_stop_cb,
};

/* Modem initialization */

static void dect_mdm_ctrl_mdm_mac_init(void)
{
	LOG_INF("Starting DECT NR+ Stack initialization");

	int ret = nrf_modem_dect_mac_callback_set(&mac_op_callbacks, &mac_ntf_callbacks);

	if (ret != 0) {
		printk("Error in callback set, error: %d\n", ret);
	}

	ret = nrf_modem_dect_control_systemmode_set(NRF_MODEM_DECT_MODE_MAC);
	if (ret != 0) {
		printk("Error in systemmode set, error: %d\n", ret);
	}

	/* Wait that sysmode is set */
	ret = k_sem_take(&dect_mdm_libmodem_api_sema, K_SECONDS(2));

	if (ret) {
		printk("(%s): nrf_modem_dect_control_systemmode_set() timeout.\n", (__func__));
	}
	CTRL_DATA_LOCK();
	ctrl_data.mdm_activation_state = CTRL_MDM_DEACTIVATED;
	struct net_if *iface = ctrl_data.iface;

	CTRL_DATA_UNLOCK();
	net_if_carrier_off(iface);

	(void)dect_mdm_ctrl_internal_modem_configure_req_from_settings();

	ret = k_sem_take(&dect_mdm_libmodem_api_sema, K_SECONDS(2));
	if (ret) {
		printk("(%s): modem configure timeout.\n", (__func__));
	}
}
#if defined(CONFIG_TEST)
void dect_mdm_ctrl_mdm_on_modem_lib_init(int ret, void *ctx)
#else
NRF_MODEM_LIB_ON_INIT(dect_mdm_ctrl_api_init_hook, dect_mdm_ctrl_mdm_on_modem_lib_init, NULL);
static void dect_mdm_ctrl_mdm_on_modem_lib_init(int ret, void *ctx)
#endif
{
	ARG_UNUSED(ret);
	ARG_UNUSED(ctx);

	dect_mdm_ctrl_mdm_mac_init();
}
