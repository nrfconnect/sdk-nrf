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

extern char *net_sprint_ll_addr_buf(const uint8_t *ll, uint8_t ll_len,
				    char *buf, int buflen);

static struct net_mgmt_event_callback ip_maddr4_cb;
static struct net_mgmt_event_callback ip_maddr6_cb;

#ifdef CONFIG_NRF700X_DATA_TX
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

enum wifi_nrf_status wifi_nrf_if_carr_state_chg(void *os_vif_ctx,
						enum wifi_nrf_fmac_if_carr_state carr_state)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct wifi_nrf_vif_ctx_zep *vif_ctx_zep = NULL;

	if (!os_vif_ctx) {
		LOG_ERR("%s: Invalid parameters\n",
			__func__);
		goto out;
	}

	vif_ctx_zep = os_vif_ctx;

	vif_ctx_zep->if_carr_state = carr_state;

	status = WIFI_NRF_STATUS_SUCCESS;

out:
	return status;
}
#endif /* CONFIG_NRF700X_DATA_TX */

enum ethernet_hw_caps wifi_nrf_if_caps_get(const struct device *dev)
{
	return (ETHERNET_LINK_10BASE_T | ETHERNET_LINK_100BASE_T | ETHERNET_LINK_1000BASE_T);
}

int wifi_nrf_if_send(const struct device *dev,
		     struct net_pkt *pkt)
{
	int ret = -1;
#ifdef CONFIG_NRF700X_DATA_TX
	struct wifi_nrf_vif_ctx_zep *vif_ctx_zep = NULL;
	struct wifi_nrf_ctx_zep *rpu_ctx_zep = NULL;

	if (!dev || !pkt) {
		LOG_ERR("%s: vif_ctx_zep is NULL\n", __func__);
		goto out;
	}

	vif_ctx_zep = dev->data;

	if (!vif_ctx_zep) {
		LOG_ERR("%s: vif_ctx_zep is NULL\n", __func__);
		net_pkt_unref(pkt);
		goto out;
	}

	rpu_ctx_zep = vif_ctx_zep->rpu_ctx_zep;

	if (vif_ctx_zep->if_carr_state != WIFI_NRF_FMAC_IF_CARR_STATE_ON) {
		ret = 0;
		goto out;
	}

	ret = wifi_nrf_fmac_start_xmit(rpu_ctx_zep->rpu_ctx,
				       vif_ctx_zep->vif_idx,
				       net_pkt_to_nbuf(pkt));
#else
	net_pkt_unref(pkt);
	goto out;
#endif /* CONFIG_NRF700X_DATA_TX */

out:
	return ret;
}

static void ip_maddr_event_handler(struct net_mgmt_event_callback *cb,
				   uint32_t mgmt_event,
				   struct net_if *iface)
{
	struct wifi_nrf_vif_ctx_zep *vif_ctx_zep = NULL;
	struct wifi_nrf_ctx_zep *rpu_ctx_zep = NULL;
	const struct device *dev = NULL;
	struct net_eth_addr mac_addr;
	struct nrf_wifi_umac_mcast_cfg *mcast_info = NULL;
	enum wifi_nrf_status status;
	uint8_t mac_string_buf[sizeof("xx:xx:xx:xx:xx:xx")];

	dev = net_if_get_device(iface);

	if (!dev) {
		LOG_ERR("%s: dev is NULL\n", __func__);
		return;
	}

	vif_ctx_zep = dev->data;
	rpu_ctx_zep = vif_ctx_zep->rpu_ctx_zep;

	mcast_info = k_calloc(sizeof(*mcast_info), sizeof(char));

	if (!mcast_info) {
		LOG_ERR("%s: Unable to allocate memory of size %d "
			"for mcast_info\n", __func__, sizeof(*mcast_info));
		return;
	}

	if ((mgmt_event == NET_EVENT_IPV4_MADDR_ADD) ||
	    (mgmt_event == NET_EVENT_IPV4_MADDR_DEL)) {
		if ((cb->info == NULL) ||
		    (cb->info_length != sizeof(struct in_addr))) {
			return;
		}

		net_eth_ipv4_mcast_to_mac_addr((const struct in_addr *)cb->info,
						&mac_addr);

		if (mgmt_event == NET_EVENT_IPV4_MADDR_ADD) {
			mcast_info->type = MCAST_ADDR_ADD;
		} else {
			mcast_info->type = MCAST_ADDR_DEL;
		}

	} else if ((mgmt_event == NET_EVENT_IPV6_MADDR_ADD) ||
		   (mgmt_event == NET_EVENT_IPV6_MADDR_DEL)) {
		if ((cb->info == NULL) ||
		    (cb->info_length != sizeof(struct in6_addr))) {
			return;
		}

		net_eth_ipv6_mcast_to_mac_addr(
				(const struct in6_addr *)cb->info,
				&mac_addr);

		if (mgmt_event == NET_EVENT_IPV6_MADDR_ADD) {
			mcast_info->type = MCAST_ADDR_ADD;
		} else {
			mcast_info->type = MCAST_ADDR_DEL;
		}
	}

	memcpy(((char *)(mcast_info->mac_addr)),
	       &mac_addr,
	       NRF_WIFI_ETH_ADDR_LEN);

	status = wifi_nrf_fmac_set_mcast_addr(rpu_ctx_zep->rpu_ctx,
					      vif_ctx_zep->vif_idx,
					      mcast_info);

	if (status == WIFI_NRF_STATUS_FAIL) {
		LOG_ERR("%s: nrf_wifi_fmac_set_multicast failed	for"
			" mac addr=%s\n",
			__func__,
			net_sprint_ll_addr_buf(mac_addr.addr,
					       WIFI_MAC_ADDR_LEN, mac_string_buf,
					       sizeof(mac_string_buf)));
	}

	k_free(mcast_info);
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

	net_if_set_link_addr(iface,
			     (unsigned char *)&vif_ctx_zep->mac_addr,
			     sizeof(vif_ctx_zep->mac_addr),
			     NET_LINK_ETHERNET);

	net_mgmt_init_event_callback(&ip_maddr4_cb,
		     ip_maddr_event_handler,
		     NET_EVENT_IPV4_MADDR_ADD | NET_EVENT_IPV4_MADDR_DEL);
	net_mgmt_add_event_callback(&ip_maddr4_cb);

	net_mgmt_init_event_callback(&ip_maddr6_cb,
			     ip_maddr_event_handler,
			     NET_EVENT_IPV6_MADDR_ADD | NET_EVENT_IPV6_MADDR_DEL);
	net_mgmt_add_event_callback(&ip_maddr6_cb);

}

#ifdef CONFIG_NET_STATISTICS_WIFI
int wifi_nrf_stats_get(const struct device *dev, struct net_stats_wifi *zstats)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct wifi_nrf_ctx_zep *rpu_ctx_zep = NULL;
	struct wifi_nrf_vif_ctx_zep *vif_ctx_zep = NULL;
	struct rpu_op_stats stats;
	int ret = -1;

	if (!dev) {
		LOG_ERR("%s Device not found\n", __func__);
		goto out;
	}

	if (!zstats) {
		LOG_ERR("%s Stats buffer not allocated\n", __func__);
		goto out;
	}

	vif_ctx_zep = dev->data;

	if (!vif_ctx_zep) {
		LOG_ERR("%s: vif_ctx_zep is NULL\n", __func__);
		goto out;
	}

	rpu_ctx_zep = vif_ctx_zep->rpu_ctx_zep;

	if (!rpu_ctx_zep) {
		LOG_ERR("%s: rpu_ctx_zep is NULL\n", __func__);
		goto out;
	}

	memset(&stats, 0, sizeof(struct rpu_op_stats));
	status = wifi_nrf_fmac_stats_get(rpu_ctx_zep->rpu_ctx, &stats);

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		LOG_ERR("%s: wifi_nrf_fmac_stats_get failed\n", __func__);
		goto out;
	}

	zstats->pkts.tx = stats.host.total_tx_pkts;
	zstats->pkts.rx = stats.host.total_rx_pkts;
	zstats->bytes.received = stats.fw.umac.interface_data_stats.rx_bytes;
	zstats->bytes.sent = stats.fw.umac.interface_data_stats.tx_bytes;
	zstats->sta_mgmt.beacons_rx = stats.fw.umac.interface_data_stats.rx_beacon_success_count;
	zstats->sta_mgmt.beacons_miss = stats.fw.umac.interface_data_stats.rx_beacon_miss_count;
	zstats->broadcast.rx = stats.fw.umac.interface_data_stats.rx_broadcast_pkt_count;
	zstats->broadcast.tx = stats.fw.umac.interface_data_stats.tx_broadcast_pkt_count;
	zstats->multicast.rx = stats.fw.umac.interface_data_stats.rx_multicast_pkt_count;
	zstats->multicast.tx = stats.fw.umac.interface_data_stats.tx_multicast_pkt_count;

	ret = 0;

out:
	return ret;
}
#endif /* CONFIG_NET_STATISTICS_WIFI */
