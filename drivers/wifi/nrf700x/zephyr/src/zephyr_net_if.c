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
LOG_MODULE_DECLARE(wifi_nrf, CONFIG_WIFI_LOG_LEVEL);

#include "net_private.h"

#include "util.h"
#include "fmac_api.h"
#include "shim.h"
#include "zephyr_fmac_main.h"
#include "zephyr_wpa_supp_if.h"
#include "zephyr_net_if.h"


extern char *net_sprint_ll_addr_buf(const uint8_t *ll, uint8_t ll_len,
				    char *buf, int buflen);

static struct net_mgmt_event_callback ip_maddr4_cb;
static struct net_mgmt_event_callback ip_maddr6_cb;

#ifdef CONFIG_NRF700X_DATA_TX
void wifi_nrf_if_rx_frm(void *os_vif_ctx, void *frm)
{
	struct wifi_nrf_vif_ctx_zep *vif_ctx_zep = os_vif_ctx;
	struct net_if *iface = vif_ctx_zep->zep_net_if_ctx;
	struct net_pkt *pkt;
	int status;
	struct wifi_nrf_ctx_zep *rpu_ctx_zep = vif_ctx_zep->rpu_ctx_zep;
	struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx = rpu_ctx_zep->rpu_ctx;
	struct rpu_host_stats *host_stats = &fmac_dev_ctx->host_stats;

	pkt = net_pkt_from_nbuf(iface, frm);
	if (!pkt) {
		LOG_DBG("Failed to allocate net_pkt");
		host_stats->total_rx_drop_pkts++;
		return;
	}

	status = net_recv_data(iface, pkt);

	if (status < 0) {
		LOG_DBG("RCV Packet dropped by NET stack: %d", status);
		host_stats->total_rx_drop_pkts++;
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

	if (vif_ctx_zep->zep_net_if_ctx) {
		if (carr_state == WIFI_NRF_FMAC_IF_CARR_STATE_ON) {
			net_eth_carrier_on(vif_ctx_zep->zep_net_if_ctx);
		} else if (carr_state == WIFI_NRF_FMAC_IF_CARR_STATE_OFF) {
			net_eth_carrier_off(vif_ctx_zep->zep_net_if_ctx);
		}
	}
	LOG_DBG("%s: Carrier state: %d\n", __func__, carr_state);

	status = WIFI_NRF_STATUS_SUCCESS;

out:
	return status;
}

static bool is_eapol(struct net_pkt *pkt)
{
	struct net_eth_hdr *hdr;
	uint16_t ethertype;

	hdr = NET_ETH_HDR(pkt);
	ethertype = ntohs(hdr->type);

	return ethertype == NET_ETH_PTYPE_EAPOL;
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
		goto out;
	}

	rpu_ctx_zep = vif_ctx_zep->rpu_ctx_zep;

	if ((vif_ctx_zep->if_carr_state != WIFI_NRF_FMAC_IF_CARR_STATE_ON) ||
	    (!vif_ctx_zep->authorized && !is_eapol(pkt))) {
		goto out;
	}

	ret = wifi_nrf_fmac_start_xmit(rpu_ctx_zep->rpu_ctx,
				       vif_ctx_zep->vif_idx,
				       net_pkt_to_nbuf(pkt));
#else
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

void wifi_nrf_if_init_zep(struct net_if *iface)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	const struct device *dev = NULL;
	struct wifi_nrf_vif_ctx_zep *vif_ctx_zep = NULL;
	struct wifi_nrf_ctx_zep *rpu_ctx_zep = NULL;
	struct nrf_wifi_umac_add_vif_info add_vif_info;

	if (!iface) {
		LOG_ERR("%s: Invalid parameters\n",
			__func__);
		return;
	}

	dev = net_if_get_device(iface);

	if (!dev) {
		LOG_ERR("%s: Invalid dev\n",
			__func__);
		return;
	}

	if (!device_is_ready(dev)) {
		LOG_ERR("%s: Device %s is not ready\n",
			__func__, dev->name);
		return;
	}

	vif_ctx_zep = dev->data;

	if (!vif_ctx_zep) {
		LOG_ERR("%s: vif_ctx_zep is NULL\n",
			__func__);
		return;
	}

	rpu_ctx_zep = vif_ctx_zep->rpu_ctx_zep;

	if (!rpu_ctx_zep) {
		LOG_ERR("%s: rpu_ctx_zep is NULL\n",
			__func__);
		return;
	}

	vif_ctx_zep->zep_net_if_ctx = iface;
	vif_ctx_zep->zep_dev_ctx = dev;

	memset(&add_vif_info,
	       0,
	       sizeof(add_vif_info));

	add_vif_info.iftype = NRF_WIFI_IFTYPE_STATION;

	memcpy(add_vif_info.ifacename,
	       dev->name,
	       strlen(dev->name));

	vif_ctx_zep->vif_idx = wifi_nrf_fmac_add_vif(rpu_ctx_zep->rpu_ctx,
						     vif_ctx_zep,
						     &add_vif_info);

	rpu_ctx_zep->vif_ctx_zep[vif_ctx_zep->vif_idx].if_type =
		add_vif_info.iftype;

	if (vif_ctx_zep->vif_idx >= MAX_NUM_VIFS) {
		LOG_ERR("%s: FMAC returned invalid interface index\n",
			__func__);
		k_free(vif_ctx_zep);
		return;
	}

	status = wifi_nrf_fmac_otp_mac_addr_get(rpu_ctx_zep->rpu_ctx,
						vif_ctx_zep->vif_idx,
						vif_ctx_zep->mac_addr.addr);

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		LOG_ERR("%s: Fetching of MAC address from OTP failed\n",
			__func__);
		return;
	}

	ethernet_init(iface);
	net_if_carrier_off(iface);

	net_if_set_link_addr(iface,
			     vif_ctx_zep->mac_addr.addr,
			     sizeof(vif_ctx_zep->mac_addr.addr),
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


int wifi_nrf_if_start_zep(const struct device *dev)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct wifi_nrf_vif_ctx_zep *vif_ctx_zep = NULL;
	struct wifi_nrf_ctx_zep *rpu_ctx_zep = NULL;
	struct nrf_wifi_umac_chg_vif_state_info vif_info;
	char *mac_addr = NULL;
	unsigned int mac_addr_len = 0;
	int ret = -1;

	if (!dev) {
		LOG_ERR("%s: Invalid parameters\n",
			__func__);
		goto out;
	}

	if (!device_is_ready(dev)) {
		LOG_ERR("%s: Device %s is not ready\n",
			__func__, dev->name);
		goto out;
	}

	vif_ctx_zep = dev->data;

	if (!vif_ctx_zep) {
		LOG_ERR("%s: vif_ctx_zep is NULL\n",
			__func__);
		goto out;
	}

	rpu_ctx_zep = vif_ctx_zep->rpu_ctx_zep;

	if (!rpu_ctx_zep) {
		LOG_ERR("%s: rpu_ctx_zep is NULL\n",
			__func__);
		goto out;
	}

	/* Disallow if a valid mac address has not been configured for the interface
	 * either from the OTP or by the user
	 */
	mac_addr = net_if_get_link_addr(vif_ctx_zep->zep_net_if_ctx)->addr;
	mac_addr_len = net_if_get_link_addr(vif_ctx_zep->zep_net_if_ctx)->len;

	if (!nrf_wifi_utils_is_mac_addr_valid(mac_addr)) {
		LOG_ERR("%s: Invalid MAC address: %s\n",
			__func__,
			net_sprint_ll_addr(mac_addr,
					   mac_addr_len));
		goto out;
	}

	status = wifi_nrf_fmac_set_vif_macaddr(rpu_ctx_zep->rpu_ctx,
					       vif_ctx_zep->vif_idx,
					       mac_addr);

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		LOG_ERR("%s: MAC address change failed\n",
			__func__);
		goto out;
	}

	memset(&vif_info,
	       0,
	       sizeof(vif_info));

	vif_info.state = WIFI_NRF_FMAC_IF_OP_STATE_UP;

	memcpy(vif_info.ifacename,
	       dev->name,
	       strlen(dev->name));

	status = wifi_nrf_fmac_chg_vif_state(rpu_ctx_zep->rpu_ctx,
					     vif_ctx_zep->vif_idx,
					     &vif_info);

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		LOG_ERR("%s: wifi_nrf_fmac_chg_vif_state failed\n",
			__func__);
		goto out;
	}

#ifdef CONFIG_WPA_SUPP
	wifi_nrf_wpa_supp_event_mac_chgd(vif_ctx_zep);
#endif /* CONFIG_WPA_SUPP */

#ifdef CONFIG_NRF_WIFI_LOW_POWER
	status = wifi_nrf_fmac_set_power_save(rpu_ctx_zep->rpu_ctx,
					      vif_ctx_zep->vif_idx,
					      NRF_WIFI_PS_ENABLED);

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		LOG_ERR("%s: wifi_nrf_fmac_set_power_save failed\n",
			__func__);
		goto out;
	}
#endif /* CONFIG_NRF_WIFI_LOW_POWER */

	vif_ctx_zep->if_op_state = WIFI_NRF_FMAC_IF_OP_STATE_UP;
	return 0;
out:
	return ret;
}


int wifi_nrf_if_stop_zep(const struct device *dev)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct wifi_nrf_vif_ctx_zep *vif_ctx_zep = NULL;
	struct wifi_nrf_ctx_zep *rpu_ctx_zep = NULL;
	struct nrf_wifi_umac_chg_vif_state_info vif_info;
	int ret = -1;

	if (!dev) {
		LOG_ERR("%s: Invalid parameters\n",
			__func__);
		goto out;
	}

	vif_ctx_zep = dev->data;

	if (!vif_ctx_zep) {
		LOG_ERR("%s: vif_ctx_zep is NULL\n",
			__func__);
		goto out;
	}

	rpu_ctx_zep = vif_ctx_zep->rpu_ctx_zep;

	if (!rpu_ctx_zep) {
		LOG_ERR("%s: rpu_ctx_zep is NULL\n",
			__func__);
		goto out;
	}

#ifdef CONFIG_NRF_WIFI_LOW_POWER
	status = wifi_nrf_fmac_set_power_save(rpu_ctx_zep->rpu_ctx,
					      vif_ctx_zep->vif_idx,
					      NRF_WIFI_PS_DISABLED);

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		LOG_ERR("%s: wifi_nrf_fmac_set_power_save failed\n",
			__func__);
		goto out;
	}
#endif /* CONFIG_NRF_WIFI_LOW_POWER */

	memset(&vif_info,
	       0,
	       sizeof(vif_info));

	vif_info.state = WIFI_NRF_FMAC_IF_OP_STATE_DOWN;

	memcpy(vif_info.ifacename,
	       dev->name,
	       strlen(dev->name));

	status = wifi_nrf_fmac_chg_vif_state(rpu_ctx_zep->rpu_ctx,
					     vif_ctx_zep->vif_idx,
					     &vif_info);

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		LOG_ERR("%s: wifi_nrf_fmac_chg_vif_state failed\n",
			__func__);
		goto out;
	}

	vif_ctx_zep->if_op_state = WIFI_NRF_FMAC_IF_OP_STATE_DOWN;
	ret = 0;
out:
	return ret;
}


int wifi_nrf_if_set_config_zep(const struct device *dev,
			       enum ethernet_config_type type,
			       const struct ethernet_config *config)
{
	struct wifi_nrf_vif_ctx_zep *vif_ctx_zep = NULL;
	struct wifi_nrf_ctx_zep *rpu_ctx_zep = vif_ctx_zep->rpu_ctx_zep;
	int ret = -1;

	if (!dev) {
		LOG_ERR("%s: Invalid parameters\n",
			__func__);
		goto out;
	}

	vif_ctx_zep = dev->data;

	if (!vif_ctx_zep) {
		LOG_ERR("%s: vif_ctx_zep is NULL\n",
			__func__);
		goto out;
	}

	rpu_ctx_zep = vif_ctx_zep->rpu_ctx_zep;

	if (!rpu_ctx_zep) {
		LOG_ERR("%s: rpu_ctx_zep is NULL\n",
			__func__);
		goto out;
	}

	if (type == ETHERNET_CONFIG_TYPE_MAC_ADDRESS) {
		if (!nrf_wifi_utils_is_mac_addr_valid(config->mac_address.addr)) {
			LOG_ERR("%s: Invalid MAC address %s\n",
				__func__,
				net_sprint_ll_addr(config->mac_address.addr,
						   sizeof(config->mac_address.addr)));
			goto out;
		}

		memcpy(vif_ctx_zep->mac_addr.addr,
		       config->mac_address.addr,
		       sizeof(vif_ctx_zep->mac_addr.addr));

		net_if_set_link_addr(vif_ctx_zep->zep_net_if_ctx,
				     vif_ctx_zep->mac_addr.addr,
				     sizeof(vif_ctx_zep->mac_addr.addr),
				     NET_LINK_ETHERNET);
	}

	ret = 0;
out:
	return ret;
}

#ifdef CONFIG_NET_STATISTICS_ETHERNET
struct net_stats_eth *wifi_nrf_eth_stats_get(const struct device *dev)
{
	struct wifi_nrf_vif_ctx_zep *vif_ctx_zep = NULL;

	if (!dev) {
		LOG_ERR("%s Device not found\n", __func__);
		goto out;
	}

	vif_ctx_zep = dev->data;

	if (!vif_ctx_zep) {
		LOG_ERR("%s: vif_ctx_zep is NULL\n", __func__);
		goto out;
	}

	return &vif_ctx_zep->eth_stats;
out:
	return NULL;
}
#endif /* CONFIG_NET_STATISTICS_ETHERNET */

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
	zstats->errors.tx = stats.host.total_tx_drop_pkts;
	zstats->errors.rx = stats.host.total_rx_drop_pkts;
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
