/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief File containing netowrk stack interface specific definitions for the
 * Zephyr OS layer of the Wi-Fi driver.
 */

#include <stdlib.h>

#include <zephyr/logging/log.h>

#include "fmac_api.h"
#include "shim.h"
#include "zephyr_fmac_main.h"
#include "zephyr_net_if.h"

LOG_MODULE_DECLARE(wifi_nrf, CONFIG_WIFI_LOG_LEVEL);

void wifi_nrf_if_rx_frm(void *os_vif_ctx, void *frm)
{
	struct wifi_nrf_vif_ctx_zep *vif_ctx_zep;
	struct net_if *iface;
	struct net_pkt *pkt;
	uint8_t status;

	vif_ctx_zep = os_vif_ctx;

	iface = vif_ctx_zep->zep_net_if_ctx;

	pkt = net_pkt_from_nbuf(iface, frm);

	status = net_recv_data(iface, pkt);

	if (status < 0) {
		LOG_ERR("RCV Packet dropped by NET stack: %d", status);
		net_pkt_unref(pkt);
	}
}

enum wifi_nrf_status wifi_nrf_if_state_chg(void *os_vif_ctx, enum wifi_nrf_fmac_if_state if_state)
{
	struct wifi_nrf_vif_ctx_zep *vif_ctx_zep;

	vif_ctx_zep = os_vif_ctx;

	vif_ctx_zep->if_state = if_state;

	return WIFI_NRF_STATUS_SUCCESS;
}

void wifi_nrf_if_init(struct net_if *iface)
{
	struct wifi_nrf_vif_ctx_zep *vif_ctx_zep = NULL;
	const struct device *dev = NULL;
	struct wifi_nrf_ctx_zep *rpu_ctx_zep = NULL;

	dev = net_if_get_device(iface);

	vif_ctx_zep = dev->data;
	rpu_ctx_zep = vif_ctx_zep->rpu_ctx_zep;

	if (!rpu_ctx_zep || !vif_ctx_zep) {
		LOG_ERR("%s: vif_ctx_zep is NULL\n", __func__);
		return;
	}

	vif_ctx_zep->zep_net_if_ctx = iface;

	ethernet_init(iface);

	net_if_set_link_addr(iface, (unsigned char *)&rpu_ctx_zep->mac_addr,
			     sizeof(rpu_ctx_zep->mac_addr), NET_LINK_ETHERNET);
}

enum ethernet_hw_caps wifi_nrf_if_caps_get(const struct device *dev)
{
	return (ETHERNET_LINK_10BASE_T | ETHERNET_LINK_100BASE_T | ETHERNET_LINK_1000BASE_T);
}

int wifi_nrf_if_send(const struct device *dev, struct net_pkt *pkt)
{
	struct wifi_nrf_vif_ctx_zep *vif_ctx_zep = NULL;
	struct wifi_nrf_ctx_zep *rpu_ctx_zep = NULL;

	vif_ctx_zep = dev->data;
	rpu_ctx_zep = vif_ctx_zep->rpu_ctx_zep;

	if (!vif_ctx_zep) {
		LOG_ERR("%s: vif_ctx_zep is NULL\n", __func__);
		net_pkt_unref(pkt);
		return -1;
	}

	if (vif_ctx_zep->if_state != WIFI_NRF_FMAC_IF_STATE_UP) {
		return 0;
	}

	return wifi_nrf_fmac_start_xmit(rpu_ctx_zep->rpu_ctx, vif_ctx_zep->vif_idx,
					net_pkt_to_nbuf(pkt));
}
