/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <zephyr/shell/shell.h>

#include <nrf_modem_dect.h>

#include <net/dect/dect_net_l2_mgmt.h>
#include <net/dect/dect_utils.h>
#include "dect_mdm_common.h"
#include "dect_mdm_ctrl.h"
#include "dect_mdm_settings.h"

#include "dect_mdm_rx.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(dect_mdm, CONFIG_DECT_MDM_LOG_LEVEL);

K_MSGQ_DEFINE(dect_mdm_rx_th_op_event_msgq, sizeof(struct dect_mdm_common_op_event_msgq_item),
	      10, 4);

#define DECT_MDM_RX_THREAD_STACK_SIZE CONFIG_DECT_MDM_NRF_RX_THREAD_STACK_SIZE
#define DECT_MDM_RX_THREAD_PRIORITY   5

/* Memory Pool for RX Event Data */

/* Simple memory pool for RX event data - sized for the main structure we need */
#define DECT_RX_EVENT_POOL_BLOCK_SIZE sizeof(struct dect_mdm_ctrl_dlc_rx_data_with_pkt_ptr)

#ifndef CONFIG_DECT_MDM_NRF_RX_EVENT_POOL_COUNT
#define CONFIG_DECT_MDM_NRF_RX_EVENT_POOL_COUNT 10
#endif

K_MEM_SLAB_DEFINE_STATIC(dect_rx_event_slab, DECT_RX_EVENT_POOL_BLOCK_SIZE,
			 CONFIG_DECT_MDM_NRF_RX_EVENT_POOL_COUNT, 4);

static bool dect_mdm_data_rx_with_pkt_ptr(struct dect_mdm_ctrl_dlc_rx_data_with_pkt_ptr *params)
{
	struct net_linkaddr ll_src;
	struct net_linkaddr ll_dst;
	/* Pkt has been allocated and written, now set the addressing part */
	struct net_pkt *rcv_pkt = params->pkt;

	int ret;
	bool handled = false;

	__ASSERT_NO_MSG(rcv_pkt != NULL);

	/* Set ll source and destination addresses based on long RD IDs:
	 * src as received and dst as configured in this device for long rd id
	 */
	struct dect_mdm_settings *set_ptr = dect_mdm_settings_ref_get();

	dect_utils_lib_net_linkaddr_set_from_long_rd_id(&ll_src, params->mdm_params.long_rd_id);
	dect_utils_lib_net_linkaddr_set_from_long_rd_id(
		&ll_dst, set_ptr->net_mgmt_common.identities.transmitter_long_rd_id);

	ret = net_linkaddr_set(net_pkt_lladdr_dst(rcv_pkt), ll_dst.addr, ll_dst.len);
	if (ret < 0) {
		LOG_ERR("%s: cannot set destination link address, ret %d", (__func__), ret);
		net_pkt_unref(rcv_pkt);
		return false;
	}

	ret = net_linkaddr_set(net_pkt_lladdr_src(rcv_pkt), ll_src.addr, ll_src.len);
	if (ret < 0) {
		LOG_ERR("%s: cannot set source link address, ret %d", (__func__), ret);
		net_pkt_unref(rcv_pkt);
		return false;
	}

	ret = net_recv_data(params->iface, rcv_pkt);
	if (ret < 0) {
		LOG_ERR("%s: received packet dropped from %u (%d bytes), ret %d", (__func__),
			params->mdm_params.long_rd_id, params->data_len, ret);
		net_pkt_unref(rcv_pkt);
	} else {
		LOG_DBG("%s: received packet from %u (%d bytes)", (__func__),
			params->mdm_params.long_rd_id, params->data_len);
		handled = true;
	}
	return handled;
}

/* Memory pool allocation helper */
static void *dect_mdm_rx_event_alloc(size_t size)
{
	void *ptr = NULL;

	if (size > DECT_RX_EVENT_POOL_BLOCK_SIZE) {
		printk("%s: Event size %zu exceeds pool block size %d\n", __func__,
			size, DECT_RX_EVENT_POOL_BLOCK_SIZE);
		return NULL;
	}
	if (k_mem_slab_alloc(&dect_rx_event_slab, &ptr, K_NO_WAIT) != 0) {
		printk("Failed to allocate memory from RX event pool\n");
		return NULL;
	}
	return ptr;
}

/* Memory pool free helper */
static void dect_mdm_rx_event_free(void *ptr)
{
	if (ptr == NULL) {
		return;
	}
	k_mem_slab_free(&dect_rx_event_slab, ptr);
}

static void dect_mdm_rx_th_op_handler_thread_fn(void)
{
	struct dect_mdm_common_op_event_msgq_item event;

	while (true) {
		k_msgq_get(&dect_mdm_rx_th_op_event_msgq, &event, K_FOREVER);

		switch (event.id) {
		case DECT_MDM_RX_OP_RX_DATA_WITH_PKT_PTR: {
			struct dect_mdm_ctrl_dlc_rx_data_with_pkt_ptr *params =
				(struct dect_mdm_ctrl_dlc_rx_data_with_pkt_ptr *)event.data;
			bool data_handled = false;

			LOG_DBG("DLC data received to iface %p, transmitter: %u (0x%X), "
				"flow ID: %hhu, data_len: %u",
				params->iface, params->mdm_params.long_rd_id,
				params->mdm_params.long_rd_id, params->mdm_params.flow_id,
				params->data_len);

			data_handled = dect_mdm_data_rx_with_pkt_ptr(params);
			if (!data_handled) {
				LOG_ERR("DECT_MDM_RX_OP_RX_DATA_WITH_PKT_PTR: Cannot pass DLC RX "
					"data upwards in stack (len %d)",
					params->data_len);
			}
			break;
		}
		default:
			LOG_ERR("DECT RX: Unknown event %d received", event.id);
			break;
		}
		/* Free memory back to pool */
		dect_mdm_rx_event_free(event.data);
	}
}

K_THREAD_DEFINE(dect_mdm_rx_th, DECT_MDM_RX_THREAD_STACK_SIZE,
		dect_mdm_rx_th_op_handler_thread_fn, NULL, NULL, NULL,
		K_PRIO_PREEMPT(DECT_MDM_RX_THREAD_PRIORITY), 0, 0);

int dect_mdm_rx_msgq_data_op_add(uint16_t event_id, void *data, size_t data_size)
{
	int ret = 0;
	struct dect_mdm_common_op_event_msgq_item event;

	/* Input validation */
	if (data == NULL || data_size == 0) {
		return -EINVAL;
	}

	/* Allocate from memory pool */
	event.data = dect_mdm_rx_event_alloc(data_size);
	if (event.data == NULL) {
		return -ENOMEM;
	}

	/* Copy data and set up event */
	memcpy(event.data, data, data_size);
	event.id = event_id;

	/* Try to put in message queue */
	ret = k_msgq_put(&dect_mdm_rx_th_op_event_msgq, &event, K_NO_WAIT);
	if (ret) {
		if (ret == -ENOMSG) {
			printk("RX message queue full, dropping event %u\n", event_id);
		} else {
			printk("Failed to put event %d to RX message queue, err: %d\n",
				event_id, ret);
		}
		dect_mdm_rx_event_free(event.data);
		return -ENOBUFS;
	}
	return 0;
}
