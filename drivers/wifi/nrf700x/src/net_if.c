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
LOG_MODULE_DECLARE(wifi_nrf, CONFIG_WIFI_NRF700X_LOG_LEVEL);

#include "net_private.h"

#include "util.h"
#include "fmac_api.h"
#include "fmac_util.h"
#include "shim.h"
#include "fmac_main.h"
#include "wpa_supp_if.h"
#include "net_if.h"


extern struct nrf_wifi_drv_priv_zep rpu_drv_priv_zep;
#if defined(CONFIG_NRF700X_DATA_TX) || defined(CONFIG_NRF700X_STA_MODE)
static struct nrf_wifi_ctx_zep *rpu_ctx = &rpu_drv_priv_zep.rpu_ctx_zep;
#endif /* CONFIG_NRF700X_DATA_TX || CONFIG_NRF700X_STA_MODE */

extern char *net_sprint_ll_addr_buf(const uint8_t *ll, uint8_t ll_len,
				    char *buf, int buflen);

#ifdef CONFIG_NRF700X_STA_MODE
static struct net_mgmt_event_callback ip_maddr4_cb;
static struct net_mgmt_event_callback ip_maddr6_cb;
#endif /* CONFIG_NRF700X_STA_MODE */

#ifdef CONFIG_NRF700X_DATA_TX
static void nrf_wifi_net_iface_work_handler(struct k_work *work)
{
	struct nrf_wifi_vif_ctx_zep *vif_ctx_zep = CONTAINER_OF(work,
								struct nrf_wifi_vif_ctx_zep,
								nrf_wifi_net_iface_work);

	if (!vif_ctx_zep) {
		LOG_ERR("%s: vif_ctx_zep is NULL", __func__);
		return;
	}

	if (!vif_ctx_zep->zep_net_if_ctx) {
		LOG_ERR("%s: zep_net_if_ctx is NULL", __func__);
		return;
	}

	if (vif_ctx_zep->if_carr_state == NRF_WIFI_FMAC_IF_CARR_STATE_ON) {
		net_if_dormant_off(vif_ctx_zep->zep_net_if_ctx);
	} else if (vif_ctx_zep->if_carr_state == NRF_WIFI_FMAC_IF_CARR_STATE_OFF) {
		net_if_dormant_on(vif_ctx_zep->zep_net_if_ctx);
	}
}

void nrf_wifi_if_rx_frm(void *os_vif_ctx, void *frm)
{
	struct nrf_wifi_vif_ctx_zep *vif_ctx_zep = os_vif_ctx;
	struct net_if *iface = vif_ctx_zep->zep_net_if_ctx;
	struct net_pkt *pkt;
	int status;
	struct nrf_wifi_ctx_zep *rpu_ctx_zep = vif_ctx_zep->rpu_ctx_zep;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = rpu_ctx_zep->rpu_ctx;
	struct nrf_wifi_fmac_dev_ctx_def *def_dev_ctx = NULL;
	struct rpu_host_stats *host_stats = NULL;

	def_dev_ctx = wifi_dev_priv(fmac_dev_ctx);
	host_stats = &def_dev_ctx->host_stats;
	host_stats->total_rx_pkts++;

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

enum nrf_wifi_status nrf_wifi_if_carr_state_chg(void *os_vif_ctx,
						enum nrf_wifi_fmac_if_carr_state carr_state)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_vif_ctx_zep *vif_ctx_zep = NULL;

	if (!os_vif_ctx) {
		LOG_ERR("%s: Invalid parameters",
			__func__);
		goto out;
	}

	vif_ctx_zep = os_vif_ctx;

	vif_ctx_zep->if_carr_state = carr_state;

	LOG_DBG("%s: Carrier state: %d", __func__, carr_state);

	k_work_submit(&vif_ctx_zep->nrf_wifi_net_iface_work);

	status = NRF_WIFI_STATUS_SUCCESS;

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

enum ethernet_hw_caps nrf_wifi_if_caps_get(const struct device *dev)
{
	enum ethernet_hw_caps caps = (ETHERNET_LINK_10BASE_T |
			ETHERNET_LINK_100BASE_T | ETHERNET_LINK_1000BASE_T);

#ifdef CONFIG_NRF700X_TCP_IP_CHECKSUM_OFFLOAD
	caps |= ETHERNET_HW_TX_CHKSUM_OFFLOAD |
		ETHERNET_HW_RX_CHKSUM_OFFLOAD;
#endif /* CONFIG_NRF700X_TCP_IP_CHECKSUM_OFFLOAD */
	return caps;
}

int nrf_wifi_if_send(const struct device *dev,
		     struct net_pkt *pkt)
{
	int ret = -1;
#ifdef CONFIG_NRF700X_DATA_TX
	struct nrf_wifi_vif_ctx_zep *vif_ctx_zep = NULL;
	struct nrf_wifi_ctx_zep *rpu_ctx_zep = NULL;
	struct net_if *iface = NULL;

	if (!dev || !pkt) {
		LOG_ERR("%s: vif_ctx_zep is NULL", __func__);
		goto out;
	}

	iface = net_pkt_iface(pkt);
	if (!iface) {
		LOG_ERR("%s: iface is NULL", __func__);
		goto out;
	}

	net_if_lock(iface);

	vif_ctx_zep = dev->data;

	if (!vif_ctx_zep) {
		LOG_ERR("%s: vif_ctx_zep is NULL", __func__);
		goto unlock;
	}

	rpu_ctx_zep = vif_ctx_zep->rpu_ctx_zep;

	if (!rpu_ctx_zep->rpu_ctx) {
		goto unlock;
	}

	if ((vif_ctx_zep->if_carr_state != NRF_WIFI_FMAC_IF_CARR_STATE_ON) ||
	    (!vif_ctx_zep->authorized && !is_eapol(pkt))) {
		goto unlock;
	}

	ret = nrf_wifi_fmac_start_xmit(rpu_ctx_zep->rpu_ctx,
				       vif_ctx_zep->vif_idx,
				       net_pkt_to_nbuf(pkt));


unlock:
	net_if_unlock(iface);
#else
	ARG_UNUSED(dev);
	ARG_UNUSED(pkt);
	goto out;
#endif /* CONFIG_NRF700X_DATA_TX */

out:
	return ret;
}

#ifdef CONFIG_NRF700X_STA_MODE
static void ip_maddr_event_handler(struct net_mgmt_event_callback *cb,
				   uint32_t mgmt_event,
				   struct net_if *iface)
{
	struct nrf_wifi_vif_ctx_zep *vif_ctx_zep = NULL;
	struct net_eth_addr mac_addr;
	struct nrf_wifi_umac_mcast_cfg *mcast_info = NULL;
	enum nrf_wifi_status status;
	uint8_t mac_string_buf[sizeof("xx:xx:xx:xx:xx:xx")];

	if (mgmt_event != NET_EVENT_IPV4_MADDR_ADD &&
	    mgmt_event != NET_EVENT_IPV4_MADDR_DEL &&
	    mgmt_event != NET_EVENT_IPV6_MADDR_ADD &&
	    mgmt_event != NET_EVENT_IPV6_MADDR_DEL) {
		return;
	}

	vif_ctx_zep = nrf_wifi_get_vif_ctx(iface);
	if (!vif_ctx_zep) {
		return;
	}

	mcast_info = k_calloc(sizeof(*mcast_info), sizeof(char));

	if (!mcast_info) {
		LOG_ERR("%s: Unable to allocate memory of size %d "
			"for mcast_info", __func__, sizeof(*mcast_info));
		return;
	}

	if ((mgmt_event == NET_EVENT_IPV4_MADDR_ADD) ||
	    (mgmt_event == NET_EVENT_IPV4_MADDR_DEL)) {
		if ((cb->info == NULL) ||
		    (cb->info_length != sizeof(struct in_addr))) {
			goto out;
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
			goto out;
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

	status = nrf_wifi_fmac_set_mcast_addr(rpu_ctx->rpu_ctx,
					      vif_ctx_zep->vif_idx,
					      mcast_info);

	if (status == NRF_WIFI_STATUS_FAIL) {
		LOG_ERR("%s: nrf_wifi_fmac_set_multicast failed	for"
			" mac addr=%s",
			__func__,
			net_sprint_ll_addr_buf(mac_addr.addr,
					       WIFI_MAC_ADDR_LEN, mac_string_buf,
					       sizeof(mac_string_buf)));
	}
out:
	k_free(mcast_info);
}
#endif /* CONFIG_NRF700X_STA_MODE */

void nrf_wifi_if_init_zep(struct net_if *iface)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	const struct device *dev = NULL;
	struct nrf_wifi_vif_ctx_zep *vif_ctx_zep = NULL;
	struct nrf_wifi_ctx_zep *rpu_ctx_zep = NULL;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;
	bool fmac_dev_added = false;
	struct ethernet_context *eth_ctx = net_if_l2_data(iface);

	if (!iface) {
		LOG_ERR("%s: Invalid parameters",
			__func__);
		return;
	}

	dev = net_if_get_device(iface);

	if (!dev) {
		LOG_ERR("%s: Invalid dev",
			__func__);
		return;
	}

	if (!device_is_ready(dev)) {
		LOG_ERR("%s: Device %s is not ready",
			__func__, dev->name);
		return;
	}

	vif_ctx_zep = dev->data;

	if (!vif_ctx_zep) {
		LOG_ERR("%s: vif_ctx_zep is NULL",
			__func__);
		return;
	}

	rpu_ctx_zep = vif_ctx_zep->rpu_ctx_zep;

	if (!rpu_ctx_zep) {
		LOG_ERR("%s: rpu_ctx_zep is NULL",
			__func__);
		return;
	}

	vif_ctx_zep->zep_net_if_ctx = iface;
	vif_ctx_zep->zep_dev_ctx = dev;

	/* This is needed to populate MAC address from OTP */
	if (nrf_wifi_fmac_get_num_vifs(&rpu_drv_priv_zep.fmac_priv) == 0) {
		status = nrf_wifi_fmac_dev_add_zep(&rpu_drv_priv_zep);

		if (status != NRF_WIFI_STATUS_SUCCESS) {
			LOG_ERR("%s: nrf_wifi_fmac_dev_add_zep failed",
				__func__);
			return;
		}

		fmac_dev_added = true;
		LOG_DBG("%s: FMAC device added", __func__);
	}

	fmac_dev_ctx = rpu_ctx_zep->rpu_ctx;

	if (!fmac_dev_ctx) {
		LOG_ERR("%s: fmac_dev_ctx is NULL",
			__func__);
		return;
	}

	if (strlen(CONFIG_WIFI_FIXED_MAC_ADDRESS) > 0) {
		ARG_UNUSED(status);
		char fixed_mac_addr[WIFI_MAC_ADDR_LEN];
		int ret;

		ret = net_bytes_from_str(fixed_mac_addr,
				WIFI_MAC_ADDR_LEN,
				CONFIG_WIFI_FIXED_MAC_ADDRESS);

		if (ret < 0) {
			LOG_ERR("%s: Failed to parse MAC address: %s",
				__func__,
				CONFIG_WIFI_FIXED_MAC_ADDRESS);
			goto dev_rem;
		}

		if (nrf_wifi_utils_is_mac_addr_valid(fmac_dev_ctx->fpriv->opriv,
						     fixed_mac_addr)) {
			memcpy(vif_ctx_zep->mac_addr.addr,
				fixed_mac_addr,
				WIFI_MAC_ADDR_LEN);
		} else {
			LOG_ERR("%s: Invalid MAC address: %s",
				__func__,
				net_sprint_ll_addr(fixed_mac_addr,
						WIFI_MAC_ADDR_LEN));
			goto dev_rem;
		}
	} else {
		status = nrf_wifi_fmac_otp_mac_addr_get(fmac_dev_ctx,
					vif_ctx_zep->vif_idx,
					vif_ctx_zep->mac_addr.addr);

		if (status != NRF_WIFI_STATUS_SUCCESS) {
			LOG_ERR("%s: Fetching of MAC address from OTP failed",
				__func__);
			goto dev_rem;
		}
	}

	eth_ctx->eth_if_type = L2_ETH_IF_TYPE_WIFI;
	ethernet_init(iface);
	net_eth_carrier_on(iface);
	net_if_dormant_on(iface);

	net_if_set_link_addr(iface,
			     vif_ctx_zep->mac_addr.addr,
			     WIFI_MAC_ADDR_LEN,
			     NET_LINK_ETHERNET);
#ifdef CONFIG_NRF700X_STA_MODE
	net_mgmt_init_event_callback(&ip_maddr4_cb,
		     ip_maddr_event_handler,
		     NET_EVENT_IPV4_MADDR_ADD | NET_EVENT_IPV4_MADDR_DEL);
	net_mgmt_add_event_callback(&ip_maddr4_cb);

	net_mgmt_init_event_callback(&ip_maddr6_cb,
			     ip_maddr_event_handler,
			     NET_EVENT_IPV6_MADDR_ADD | NET_EVENT_IPV6_MADDR_DEL);
	net_mgmt_add_event_callback(&ip_maddr6_cb);
#endif /* CONFIG_NRF700X_STA_MODE */
#ifdef CONFIG_NRF700X_DATA_TX
	k_work_init(&vif_ctx_zep->nrf_wifi_net_iface_work,
		    nrf_wifi_net_iface_work_handler);
#endif /* CONFIG_NRF700X_DATA_TX */

#if !defined(CONFIG_NRF_WIFI_IF_AUTO_START)
	net_if_flag_set(iface, NET_IF_NO_AUTO_START);
#endif /* CONFIG_NRF_WIFI_IF_AUTO_START */

	return;
dev_rem:
	/* Free only if we added above i.e., for 1st VIF */
	if (fmac_dev_added) {
		nrf_wifi_fmac_dev_rem_zep(&rpu_drv_priv_zep);
	}
}

int nrf_wifi_if_start_zep(const struct device *dev)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_vif_ctx_zep *vif_ctx_zep = NULL;
	struct nrf_wifi_ctx_zep *rpu_ctx_zep = NULL;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;
	struct nrf_wifi_umac_chg_vif_state_info vif_info;
	struct nrf_wifi_umac_add_vif_info add_vif_info;
	char *mac_addr = NULL;
	unsigned int mac_addr_len = 0;
	int ret = -1;
	bool fmac_dev_added = false;

	if (!dev) {
		LOG_ERR("%s: Invalid parameters",
			__func__);
		goto out;
	}

	if (!device_is_ready(dev)) {
		LOG_ERR("%s: Device %s is not ready",
			__func__, dev->name);
		goto out;
	}

	vif_ctx_zep = dev->data;

	if (!vif_ctx_zep) {
		LOG_ERR("%s: vif_ctx_zep is NULL",
			__func__);
		goto out;
	}

	rpu_ctx_zep = vif_ctx_zep->rpu_ctx_zep;

	if (!rpu_ctx_zep) {
		LOG_ERR("%s: rpu_ctx_zep is NULL",
			__func__);
		goto out;
	}

	if (!rpu_ctx_zep->rpu_ctx) {
		status = nrf_wifi_fmac_dev_add_zep(&rpu_drv_priv_zep);

		if (status != NRF_WIFI_STATUS_SUCCESS) {
			LOG_ERR("%s: nrf_wifi_fmac_dev_add_zep failed",
				__func__);
			goto out;
		}
		fmac_dev_added = true;
		LOG_DBG("%s: FMAC device added", __func__);
	}

	fmac_dev_ctx = rpu_ctx_zep->rpu_ctx;

	memset(&add_vif_info,
	       0,
	       sizeof(add_vif_info));

	/* TODO: This should be configurable */
	add_vif_info.iftype = NRF_WIFI_IFTYPE_STATION;

	memcpy(add_vif_info.ifacename,
	       dev->name,
	       strlen(dev->name));

	vif_ctx_zep->vif_idx = nrf_wifi_fmac_add_vif(fmac_dev_ctx,
						     vif_ctx_zep,
						     &add_vif_info);

	rpu_ctx_zep->vif_ctx_zep[vif_ctx_zep->vif_idx].if_type =
		add_vif_info.iftype;

	if (vif_ctx_zep->vif_idx >= MAX_NUM_VIFS) {
		LOG_ERR("%s: FMAC returned invalid interface index",
			__func__);
		goto dev_rem;
	}

	/* Disallow if a valid mac address has not been configured for the interface
	 * either from the OTP or by the user
	 */
	mac_addr = net_if_get_link_addr(vif_ctx_zep->zep_net_if_ctx)->addr;
	mac_addr_len = net_if_get_link_addr(vif_ctx_zep->zep_net_if_ctx)->len;

	if (!nrf_wifi_utils_is_mac_addr_valid(fmac_dev_ctx->fpriv->opriv,
					      mac_addr)) {
		LOG_ERR("%s: Invalid MAC address: %s",
			__func__,
			net_sprint_ll_addr(mac_addr,
					   mac_addr_len));
		goto dev_rem;
	}

	status = nrf_wifi_fmac_set_vif_macaddr(rpu_ctx_zep->rpu_ctx,
					       vif_ctx_zep->vif_idx,
					       mac_addr);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		LOG_ERR("%s: MAC address change failed",
			__func__);
		goto dev_rem;
	}

	memset(&vif_info,
	       0,
	       sizeof(vif_info));

	vif_info.state = NRF_WIFI_FMAC_IF_OP_STATE_UP;

	memcpy(vif_info.ifacename,
	       dev->name,
	       strlen(dev->name));

	status = nrf_wifi_fmac_chg_vif_state(rpu_ctx_zep->rpu_ctx,
					     vif_ctx_zep->vif_idx,
					     &vif_info);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		LOG_ERR("%s: nrf_wifi_fmac_chg_vif_state failed",
			__func__);
		goto dev_rem;
	}

#ifdef CONFIG_NRF700X_STA_MODE
	nrf_wifi_wpa_supp_event_mac_chgd(vif_ctx_zep);

#ifdef CONFIG_NRF_WIFI_LOW_POWER
	status = nrf_wifi_fmac_set_power_save(rpu_ctx_zep->rpu_ctx,
					      vif_ctx_zep->vif_idx,
					      NRF_WIFI_PS_ENABLED);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		LOG_ERR("%s: nrf_wifi_fmac_set_power_save failed",
			__func__);
		goto dev_rem;
	}
#endif /* CONFIG_NRF_WIFI_LOW_POWER */
#endif /* CONFIG_NRF700X_STA_MODE */

	vif_ctx_zep->if_op_state = NRF_WIFI_FMAC_IF_OP_STATE_UP;

	return 0;
dev_rem:
	/* Free only if we added above i.e., for 1st VIF */
	if (fmac_dev_added) {
		nrf_wifi_fmac_dev_rem_zep(&rpu_drv_priv_zep);
	}
out:
	return ret;
}


int nrf_wifi_if_stop_zep(const struct device *dev)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_vif_ctx_zep *vif_ctx_zep = NULL;
	struct nrf_wifi_ctx_zep *rpu_ctx_zep = NULL;
	struct nrf_wifi_umac_chg_vif_state_info vif_info;
	int ret = -1;

	if (!dev) {
		LOG_ERR("%s: Invalid parameters",
			__func__);
		goto out;
	}

	vif_ctx_zep = dev->data;

	if (!vif_ctx_zep) {
		LOG_ERR("%s: vif_ctx_zep is NULL",
			__func__);
		goto out;
	}

	rpu_ctx_zep = vif_ctx_zep->rpu_ctx_zep;

	if (!rpu_ctx_zep) {
		LOG_ERR("%s: rpu_ctx_zep is NULL",
			__func__);
		goto out;
	}

#ifdef CONFIG_NRF700X_STA_MODE
#ifdef CONFIG_NRF_WIFI_LOW_POWER
	status = nrf_wifi_fmac_set_power_save(rpu_ctx_zep->rpu_ctx,
					      vif_ctx_zep->vif_idx,
					      NRF_WIFI_PS_DISABLED);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		LOG_ERR("%s: nrf_wifi_fmac_set_power_save failed",
			__func__);
		goto out;
	}
#endif /* CONFIG_NRF_WIFI_LOW_POWER */
#endif /* CONFIG_NRF700X_STA_MODE */

	memset(&vif_info,
	       0,
	       sizeof(vif_info));

	vif_info.state = NRF_WIFI_FMAC_IF_OP_STATE_DOWN;

	memcpy(vif_info.ifacename,
	       dev->name,
	       strlen(dev->name));

	status = nrf_wifi_fmac_chg_vif_state(rpu_ctx_zep->rpu_ctx,
					     vif_ctx_zep->vif_idx,
					     &vif_info);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		LOG_ERR("%s: nrf_wifi_fmac_chg_vif_state failed",
			__func__);
		goto out;
	}

	status = nrf_wifi_fmac_del_vif(rpu_ctx_zep->rpu_ctx,
				       vif_ctx_zep->vif_idx);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		LOG_ERR("%s: nrf_wifi_fmac_del_vif failed",
			__func__);
		goto out;
	}

	vif_ctx_zep->if_op_state = NRF_WIFI_FMAC_IF_OP_STATE_DOWN;

	if (nrf_wifi_fmac_get_num_vifs(rpu_ctx_zep->rpu_ctx) == 0) {
		nrf_wifi_fmac_dev_rem_zep(&rpu_drv_priv_zep);
	}
	ret = 0;
out:
	return ret;
}


int nrf_wifi_if_set_config_zep(const struct device *dev,
			       enum ethernet_config_type type,
			       const struct ethernet_config *config)
{
	struct nrf_wifi_vif_ctx_zep *vif_ctx_zep = NULL;
	struct nrf_wifi_ctx_zep *rpu_ctx_zep = vif_ctx_zep->rpu_ctx_zep;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;
	int ret = -1;

	if (!dev) {
		LOG_ERR("%s: Invalid parameters",
			__func__);
		goto out;
	}

	vif_ctx_zep = dev->data;

	if (!vif_ctx_zep) {
		LOG_ERR("%s: vif_ctx_zep is NULL",
			__func__);
		goto out;
	}

	rpu_ctx_zep = vif_ctx_zep->rpu_ctx_zep;

	if (!rpu_ctx_zep) {
		LOG_ERR("%s: rpu_ctx_zep is NULL",
			__func__);
		goto out;
	}

	fmac_dev_ctx = rpu_ctx_zep->rpu_ctx;

	if (!fmac_dev_ctx) {
		LOG_ERR("%s: fmac_dev_ctx is NULL",
			__func__);
		goto out;
	}

	if (type == ETHERNET_CONFIG_TYPE_MAC_ADDRESS) {
		if (!nrf_wifi_utils_is_mac_addr_valid(fmac_dev_ctx->fpriv->opriv,
						      config->mac_address.addr)) {
			LOG_ERR("%s: Invalid MAC address %s",
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
struct net_stats_eth *nrf_wifi_eth_stats_get(const struct device *dev)
{
	struct nrf_wifi_vif_ctx_zep *vif_ctx_zep = NULL;

	if (!dev) {
		LOG_ERR("%s Device not found", __func__);
		goto out;
	}

	vif_ctx_zep = dev->data;

	if (!vif_ctx_zep) {
		LOG_ERR("%s: vif_ctx_zep is NULL", __func__);
		goto out;
	}

	return &vif_ctx_zep->eth_stats;
out:
	return NULL;
}
#endif /* CONFIG_NET_STATISTICS_ETHERNET */

#ifdef CONFIG_NET_STATISTICS_WIFI
int nrf_wifi_stats_get(const struct device *dev, struct net_stats_wifi *zstats)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_ctx_zep *rpu_ctx_zep = NULL;
	struct nrf_wifi_vif_ctx_zep *vif_ctx_zep = NULL;
	struct rpu_op_stats stats;
	int ret = -1;

	if (!dev) {
		LOG_ERR("%s Device not found", __func__);
		goto out;
	}

	if (!zstats) {
		LOG_ERR("%s Stats buffer not allocated", __func__);
		goto out;
	}

	vif_ctx_zep = dev->data;

	if (!vif_ctx_zep) {
		LOG_ERR("%s: vif_ctx_zep is NULL", __func__);
		goto out;
	}

	rpu_ctx_zep = vif_ctx_zep->rpu_ctx_zep;

	if (!rpu_ctx_zep) {
		LOG_ERR("%s: rpu_ctx_zep is NULL", __func__);
		goto out;
	}

	memset(&stats, 0, sizeof(struct rpu_op_stats));
	status = nrf_wifi_fmac_stats_get(rpu_ctx_zep->rpu_ctx, 0, &stats);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		LOG_ERR("%s: nrf_wifi_fmac_stats_get failed", __func__);
		goto out;
	}

	zstats->pkts.tx = stats.host.total_tx_pkts;
	zstats->pkts.rx = stats.host.total_rx_pkts;
	zstats->errors.tx = stats.host.total_tx_drop_pkts;
	zstats->errors.rx = stats.host.total_rx_drop_pkts +
			stats.fw.umac.interface_data_stats.rx_checksum_error_count;
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
