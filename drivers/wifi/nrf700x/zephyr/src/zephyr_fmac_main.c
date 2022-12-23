/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief File containing FMAC interface specific definitions for the
 * Zephyr OS layer of the Wi-Fi driver.
 */

#include <stdlib.h>

#include <zephyr/kernel.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/logging/log.h>

#include <rpu_fw_patches.h>
#include <fmac_api.h>
#include <zephyr_fmac_main.h>
#ifdef CONFIG_WPA_SUPP
#include <zephyr_wpa_supp_if.h>
#endif /* CONFIG_WPA_SUPP */
#ifndef CONFIG_NRF700X_RADIO_TEST
#include <zephyr_disp_scan.h>
#include <zephyr_twt.h>
#include <zephyr_ps.h>
#endif /* !CONFIG_NRF700X_RADIO_TEST */

LOG_MODULE_DECLARE(wifi_nrf, CONFIG_WIFI_LOG_LEVEL);

struct wifi_nrf_drv_priv_zep rpu_drv_priv_zep;

#ifndef CONFIG_NRF700X_RADIO_TEST
#ifdef CONFIG_NRF700X_DATA_TX

#define MAX_RX_QUEUES 3

#define TOTAL_TX_SIZE (CONFIG_TX_MAX_DATA_SIZE + TX_BUF_HEADROOM)

BUILD_ASSERT(CONFIG_MAX_TX_TOKENS >= 1, "At least one TX token is required");
BUILD_ASSERT(CONFIG_MAX_TX_AGGREGATION <= 16, "Max TX aggregation is 16");
BUILD_ASSERT(CONFIG_RX_NUM_BUFS >= 1, "At least one RX buffer is required");

BUILD_ASSERT(RPU_PKTRAM_SIZE >=
		((CONFIG_MAX_TX_AGGREGATION * CONFIG_MAX_TX_TOKENS * TOTAL_TX_SIZE) +
		(CONFIG_RX_NUM_BUFS * CONFIG_RX_MAX_DATA_SIZE)),
		"Packet RAM overflow in Sheliak");

static const unsigned char aggregation = 1;
static const unsigned char wmm = 1;
static const unsigned char max_num_tx_agg_sessions = 4;
static const unsigned char max_num_rx_agg_sessions = 2;
static const unsigned char reorder_buf_size = 64;
static const unsigned char max_rxampdu_size = MAX_RX_AMPDU_SIZE_64KB;

static const unsigned char max_tx_aggregation = CONFIG_MAX_TX_AGGREGATION;

static const unsigned int rx1_num_bufs = CONFIG_RX_NUM_BUFS / MAX_RX_QUEUES;
static const unsigned int rx2_num_bufs = CONFIG_RX_NUM_BUFS / MAX_RX_QUEUES;
static const unsigned int rx3_num_bufs = CONFIG_RX_NUM_BUFS / MAX_RX_QUEUES;

static const unsigned int rx1_buf_sz = CONFIG_RX_MAX_DATA_SIZE;
static const unsigned int rx2_buf_sz = CONFIG_RX_MAX_DATA_SIZE;
static const unsigned int rx3_buf_sz = CONFIG_RX_MAX_DATA_SIZE;

static const unsigned char rate_protection_type;
#else
/* Reduce buffers to Scan only operation */
static const unsigned int rx1_num_bufs = 2;
static const unsigned int rx2_num_bufs = 2;
static const unsigned int rx3_num_bufs = 2;

static const unsigned int rx1_buf_sz = 1000;
static const unsigned int rx2_buf_sz = 1000;
static const unsigned int rx3_buf_sz = 1000;
#endif

struct wifi_nrf_drv_priv_zep rpu_drv_priv_zep;

void wifi_nrf_event_proc_scan_start_zep(void *if_priv,
					struct nrf_wifi_umac_event_trigger_scan *scan_start_event,
					unsigned int event_len)
{
	struct wifi_nrf_vif_ctx_zep *vif_ctx_zep = NULL;

	vif_ctx_zep = if_priv;

	if (vif_ctx_zep->scan_type == SCAN_DISPLAY) {
		return;
	}

#ifdef CONFIG_WPA_SUPP
	wifi_nrf_wpa_supp_event_proc_scan_start(if_priv);
#endif /* CONFIG_WPA_SUPP */
}


void wifi_nrf_event_proc_scan_done_zep(void *vif_ctx,
				       struct nrf_wifi_umac_event_trigger_scan *scan_done_event,
				       unsigned int event_len)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct wifi_nrf_vif_ctx_zep *vif_ctx_zep = NULL;

	vif_ctx_zep = vif_ctx;

	if (!vif_ctx_zep) {
		LOG_ERR("%s: vif_ctx_zep is NULL\n", __func__);
		return;
	}

	if (vif_ctx_zep->scan_type == SCAN_DISPLAY) {
		status = wifi_nrf_disp_scan_res_get_zep(vif_ctx_zep);

		if (status != WIFI_NRF_STATUS_SUCCESS) {
			LOG_ERR("%s: wifi_nrf_disp_scan_res_get_zep failed\n", __func__);
			return;
		}
#ifdef CONFIG_WPA_SUPP
	} else if (vif_ctx_zep->scan_type == SCAN_CONNECT) {
		wifi_nrf_wpa_supp_event_proc_scan_done(vif_ctx_zep,
						       scan_done_event,
						       event_len,
						       0);
#endif /* CONFIG_WPA_SUPP */
	} else {
		LOG_ERR("%s: Scan type = %d not supported yet\n", __func__, vif_ctx_zep->scan_type);
		return;
	}

	status = WIFI_NRF_STATUS_SUCCESS;
}
#endif /* !CONFIG_NRF700X_RADIO_TEST */


enum wifi_nrf_status wifi_nrf_fmac_dev_add_zep(struct wifi_nrf_drv_priv_zep *drv_priv_zep)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct wifi_nrf_ctx_zep *rpu_ctx_zep = NULL;
	struct wifi_nrf_fmac_fw_info fw_info;
	void *rpu_ctx = NULL;

	rpu_ctx_zep = &drv_priv_zep->rpu_ctx_zep;

	rpu_ctx_zep->drv_priv_zep = drv_priv_zep;

	rpu_ctx = wifi_nrf_fmac_dev_add(drv_priv_zep->fmac_priv, rpu_ctx_zep);

	if (!rpu_ctx) {
		LOG_ERR("%s: wifi_nrf_fmac_dev_add failed\n", __func__);
		rpu_ctx_zep = NULL;
		goto out;
	}

	rpu_ctx_zep->rpu_ctx = rpu_ctx;

	memset(&fw_info, 0, sizeof(fw_info));

	fw_info.lmac_patch_pri.data = wifi_nrf_lmac_patch_pri_bimg;
	fw_info.lmac_patch_pri.size = sizeof(wifi_nrf_lmac_patch_pri_bimg);
	fw_info.lmac_patch_sec.data = wifi_nrf_lmac_patch_sec_bin;
	fw_info.lmac_patch_sec.size = sizeof(wifi_nrf_lmac_patch_sec_bin);
	fw_info.umac_patch_pri.data = wifi_nrf_umac_patch_pri_bimg;
	fw_info.umac_patch_pri.size = sizeof(wifi_nrf_umac_patch_pri_bimg);
	fw_info.umac_patch_sec.data = wifi_nrf_umac_patch_sec_bin;
	fw_info.umac_patch_sec.size = sizeof(wifi_nrf_umac_patch_sec_bin);

	/* Load the FW patches to the RPU */
	status = wifi_nrf_fmac_fw_load(rpu_ctx, &fw_info);

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		LOG_ERR("%s: wifi_nrf_fmac_fw_load failed\n", __func__);
		goto out;
	}
out:
	return status;
}

void wifi_nrf_fmac_dev_rem_zep(struct wifi_nrf_ctx_zep *rpu_ctx_zep)
{
	wifi_nrf_fmac_dev_rem(rpu_ctx_zep->rpu_ctx);
}


#ifndef CONFIG_NRF700X_RADIO_TEST
enum wifi_nrf_status wifi_nrf_fmac_vif_add_zep(struct wifi_nrf_ctx_zep *rpu_ctx_zep,
					       unsigned char vif_idx,
					       const struct device *dev)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct wifi_nrf_vif_ctx_zep *vif_ctx_zep = NULL;
	struct nrf_wifi_umac_add_vif_info add_vif_info;
	struct wifi_nrf_fmac_otp_info otp_info;

	if (vif_idx >= MAX_NUM_VIFS) {
		LOG_ERR("%s: Invalid VIF index (%d)\n",
			__func__,
			vif_idx);

		goto out;
	}

	vif_ctx_zep = &rpu_ctx_zep->vif_ctx_zep[vif_idx];

	vif_ctx_zep->rpu_ctx_zep = rpu_ctx_zep;
	vif_ctx_zep->zep_dev_ctx = dev;

	memset(&add_vif_info,
	       0,
	       sizeof(add_vif_info));

	add_vif_info.iftype = NRF_WIFI_IFTYPE_STATION;

	memcpy(add_vif_info.ifacename,
	       "wlan0",
	       strlen("wlan0"));

	status = wifi_nrf_fmac_otp_info_get(rpu_ctx_zep->rpu_ctx,
					    &otp_info);

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		LOG_ERR("%s:  Fetching of RPU OTP information failed\n",
			__func__);
		goto out;
	}

	if (net_eth_is_addr_unspecified(&vif_ctx_zep->mac_addr)) {
		if (vif_idx == 0) {
			/* Check if MAC address has been programmed in the OTP */
#ifdef CONFIG_NRF700X_REV_A
			if (otp_info.info.mac_address0[0] == 0xFFFFFFFF &&
			    otp_info.info.mac_address0[1] == 0xFFFFFFFF) {
#else
			if (otp_info.flags & (~MAC0_ADDR_FLAG_MASK)) {
#endif /* !CONFIG_NRF700X_REV_A */
				LOG_ERR("%s: MAC address 0 not programmed in OTP !\n",
						      __func__);

				status = WIFI_NRF_STATUS_FAIL;
				goto out;
			}
		} else if (vif_idx == 1) {
#ifdef CONFIG_NRF700X_REV_A
			if (otp_info.info.mac_address1[0] == 0xFFFFFFFF &&
			    otp_info.info.mac_address1[1] == 0xFFFFFFFF) {
#else
			if (otp_info.flags & (~MAC1_ADDR_FLAG_MASK)) {
#endif /* !CONFIG_NRF700X_REV_A */
				LOG_ERR("%s: MAC address 1 not programmed in OTP !\n",
						      __func__);

				status = WIFI_NRF_STATUS_FAIL;
				goto out;
			}
		}

		memcpy(&vif_ctx_zep->mac_addr,
		       otp_info.info.mac_address0,
		       NRF_WIFI_ETH_ADDR_LEN);
	}

	memcpy(add_vif_info.mac_addr,
	       &vif_ctx_zep->mac_addr,
	       sizeof(add_vif_info.mac_addr));

	vif_ctx_zep->vif_idx = wifi_nrf_fmac_add_vif(rpu_ctx_zep->rpu_ctx,
						     vif_ctx_zep,
						     &add_vif_info);

	if (vif_ctx_zep->vif_idx != vif_idx) {
		LOG_ERR("%s: FMAC returned invalid interface index\n",
			__func__);
		k_free(vif_ctx_zep);
		goto out;
	}

	status = WIFI_NRF_STATUS_SUCCESS;
out:
	return status;
}


void wifi_nrf_fmac_vif_rem_zep(struct wifi_nrf_vif_ctx_zep *vif_ctx_zep)
{
	struct wifi_nrf_ctx_zep *rpu_ctx_zep = NULL;

	rpu_ctx_zep = vif_ctx_zep->rpu_ctx_zep;

	wifi_nrf_fmac_del_vif(rpu_ctx_zep->rpu_ctx, vif_ctx_zep->vif_idx);
}


enum wifi_nrf_status wifi_nrf_fmac_vif_state_chg(struct wifi_nrf_vif_ctx_zep *vif_ctx_zep,
						 enum wifi_nrf_fmac_if_op_state state)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct wifi_nrf_ctx_zep *rpu_ctx_zep = NULL;
	struct nrf_wifi_umac_chg_vif_state_info vif_info;

	rpu_ctx_zep = vif_ctx_zep->rpu_ctx_zep;

	memset(&vif_info,
	       0,
	       sizeof(vif_info));

	vif_info.state = state;

	memcpy(vif_info.ifacename,
	       "wlan0",
	       strlen("wlan0"));

	status = wifi_nrf_fmac_chg_vif_state(rpu_ctx_zep->rpu_ctx,
					     vif_ctx_zep->vif_idx,
					     &vif_info);

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		LOG_ERR("%s: wifi_nrf_fmac_chg_vif_state failed\n", __func__);
		goto out;
	}

#ifdef CONFIG_NRF_WIFI_LOW_POWER
	status = wifi_nrf_fmac_set_power_save(rpu_ctx_zep->rpu_ctx,
					      vif_ctx_zep->vif_idx,
					      NRF_WIFI_PS_ENABLED);

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		LOG_ERR("%s: wifi_nrf_fmac_set_power_save failed\n", __func__);
		goto out;
	}
#endif /* CONFIG_NRF_WIFI_LOW_POWER */
out:
	return status;
}
#endif /* !CONFIG_NRF700X_RADIO_TEST */


enum wifi_nrf_status wifi_nrf_fmac_dev_init_zep(struct wifi_nrf_ctx_zep *rpu_ctx_zep)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
#ifdef CONFIG_NRF_WIFI_LOW_POWER
	int sleep_type = -1;

#ifndef CONFIG_NRF700X_RADIO_TEST
	sleep_type = HW_SLEEP_ENABLE;
#else
	sleep_type = SLEEP_DISABLE;
#endif /* CONFIG_NRF700X_RADIO_TEST */
#endif /* CONFIG_NRF_WIFI_LOW_POWER */

	status = wifi_nrf_fmac_dev_init(rpu_ctx_zep->rpu_ctx,
#ifndef CONFIG_NRF700X_RADIO_TEST
					0,
					(unsigned char *)&rpu_ctx_zep->vif_ctx_zep[0].mac_addr,
					NULL,
#endif /* !CONFIG_NRF700X_RADIO_TEST */
#ifdef CONFIG_NRF_WIFI_LOW_POWER
					sleep_type,
#endif /* CONFIG_NRF_WIFI_LOW_POWER */
					NRF_WIFI_DEF_PHY_CALIB);

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		LOG_ERR("%s: wifi_nrf_fmac_dev_init failed\n", __func__);
		goto out;
	}
out:
	return status;
}


void wifi_nrf_fmac_dev_deinit_zep(struct wifi_nrf_ctx_zep *rpu_ctx_zep)
{
#ifndef CONFIG_NRF700X_RADIO_TEST
	struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx = NULL;
	struct wifi_nrf_vif_ctx_zep *vif_ctx_zep = NULL;
	unsigned char vif_idx = MAX_NUM_VIFS;
	unsigned char i = 0;

	fmac_dev_ctx = rpu_ctx_zep->rpu_ctx;

	/* Delete all other interfaces */
	for (i = 1; i < MAX_NUM_VIFS; i++) {
		if (fmac_dev_ctx->vif_ctx[i]) {
			vif_ctx_zep = fmac_dev_ctx->vif_ctx[i]->os_vif_ctx;
			vif_idx = vif_ctx_zep->vif_idx;
			wifi_nrf_fmac_del_vif(fmac_dev_ctx, vif_idx);
			fmac_dev_ctx->vif_ctx[i] = NULL;
		}
	}
#endif /* !CONFIG_NRF700X_RADIO_TEST */

	wifi_nrf_fmac_dev_deinit(rpu_ctx_zep->rpu_ctx);
}

static int wifi_nrf_drv_main_zep(const struct device *dev)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
#ifndef CONFIG_NRF700X_RADIO_TEST
	struct wifi_nrf_vif_ctx_zep *vif_ctx_zep = NULL;
	struct wifi_nrf_fmac_callbk_fns callbk_fns = { 0 };
	struct nrf_wifi_data_config_params data_config = { 0 };
	struct rx_buf_pool_params rx_buf_pools[MAX_NUM_OF_RX_QUEUES];

	vif_ctx_zep = dev->data;

#ifdef CONFIG_NRF700X_DATA_TX
	data_config.aggregation = aggregation;
	data_config.wmm = wmm;
	data_config.max_num_tx_agg_sessions = max_num_tx_agg_sessions;
	data_config.max_num_rx_agg_sessions = max_num_rx_agg_sessions;
	data_config.max_tx_aggregation = max_tx_aggregation;
	data_config.reorder_buf_size = reorder_buf_size;
	data_config.max_rxampdu_size = max_rxampdu_size;
	data_config.rate_protection_type = rate_protection_type;

	callbk_fns.if_carr_state_chg_callbk_fn = wifi_nrf_if_carr_state_chg;
	callbk_fns.rx_frm_callbk_fn = wifi_nrf_if_rx_frm;
#endif
	rx_buf_pools[0].num_bufs = rx1_num_bufs;
	rx_buf_pools[1].num_bufs = rx2_num_bufs;
	rx_buf_pools[2].num_bufs = rx3_num_bufs;
	rx_buf_pools[0].buf_sz = rx1_buf_sz;
	rx_buf_pools[1].buf_sz = rx2_buf_sz;
	rx_buf_pools[2].buf_sz = rx3_buf_sz;

	callbk_fns.scan_start_callbk_fn = wifi_nrf_event_proc_scan_start_zep;
	callbk_fns.scan_done_callbk_fn = wifi_nrf_event_proc_scan_done_zep;
	callbk_fns.disp_scan_res_callbk_fn = wifi_nrf_event_proc_disp_scan_res_zep;
	callbk_fns.twt_config_callbk_fn = wifi_nrf_event_proc_twt_setup_zep;
	callbk_fns.twt_teardown_callbk_fn = wifi_nrf_event_proc_twt_teardown_zep;
	callbk_fns.twt_sleep_callbk_fn = wifi_nrf_event_proc_twt_sleep_zep;
#ifdef CONFIG_WPA_SUPP
	callbk_fns.scan_res_callbk_fn = wifi_nrf_wpa_supp_event_proc_scan_res;
	callbk_fns.auth_resp_callbk_fn = wifi_nrf_wpa_supp_event_proc_auth_resp;
	callbk_fns.assoc_resp_callbk_fn = wifi_nrf_wpa_supp_event_proc_assoc_resp;
	callbk_fns.deauth_callbk_fn = wifi_nrf_wpa_supp_event_proc_deauth;
	callbk_fns.disassoc_callbk_fn = wifi_nrf_wpa_supp_event_proc_disassoc;
	callbk_fns.get_station_callbk_fn = wifi_nrf_wpa_supp_event_proc_get_sta;
	callbk_fns.get_interface_callbk_fn = wifi_nrf_wpa_supp_event_proc_get_if;
	callbk_fns.mgmt_tx_status = wifi_nrf_wpa_supp_event_mgmt_tx_status;
	callbk_fns.unprot_mlme_mgmt_rx_callbk_fn = wifi_nrf_wpa_supp_event_proc_unprot_mgmt;
	callbk_fns.event_get_wiphy = wifi_nrf_wpa_supp_event_get_wiphy;
	callbk_fns.mgmt_rx_callbk_fn = wifi_nrf_wpa_supp_event_mgmt_rx_callbk_fn;
#endif /* CONFIG_WPA_SUPP */

	rpu_drv_priv_zep.fmac_priv = wifi_nrf_fmac_init(&data_config,
							rx_buf_pools,
							&callbk_fns);
#else /* !CONFIG_NRF700X_RADIO_TEST */
	rpu_drv_priv_zep.fmac_priv = wifi_nrf_fmac_init();
#endif /* CONFIG_NRF700X_RADIO_TEST */

	if (rpu_drv_priv_zep.fmac_priv == NULL) {
		LOG_ERR("%s: wifi_nrf_fmac_init failed\n",
			__func__);
		goto err;
	}

	status = wifi_nrf_fmac_dev_add_zep(&rpu_drv_priv_zep);

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		LOG_ERR("%s: wifi_nrf_fmac_dev_add_zep failed\n",
			__func__);
		goto fmac_deinit;
	}

#ifndef CONFIG_NRF700X_RADIO_TEST
	status = wifi_nrf_fmac_vif_add_zep(&rpu_drv_priv_zep.rpu_ctx_zep,
					   0,
					   dev);

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		LOG_ERR("%s: wifi_nrf_fmac_vif_add_zep failed\n",
			__func__);
		goto rpu_rem;
	}
#endif /* !CONFIG_NRF700X_RADIO_TEST */

	status = wifi_nrf_fmac_dev_init_zep(&rpu_drv_priv_zep.rpu_ctx_zep);

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		LOG_ERR("%s: wifi_nrf_fmac_dev_init_zep failed\n",
			__func__);
#ifdef CONFIG_NRF700X_RADIO_TEST
		goto rpu_rem;
#else
		goto vif_rem;
#endif /* !CONFIG_NRF700X_RADIO_TEST */
	}

#ifndef CONFIG_NRF700X_RADIO_TEST
	/* Bring the default interface up in the firmware */
	status = wifi_nrf_fmac_vif_state_chg(vif_ctx_zep,
					     WIFI_NRF_FMAC_IF_OP_STATE_UP);

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		LOG_ERR("%s: wifi_nrf_fmac_vif_state_chg failed\n",
			__func__);
		goto dev_deinit;
	} else {
		vif_ctx_zep->if_op_state = WIFI_NRF_FMAC_IF_OP_STATE_UP;
	}
#endif /* !CONFIG_NRF700X_RADIO_TEST */

	return 0;
#ifndef CONFIG_NRF700X_RADIO_TEST
dev_deinit:
	wifi_nrf_fmac_dev_deinit_zep(&rpu_drv_priv_zep.rpu_ctx_zep);
vif_rem:
	wifi_nrf_fmac_vif_rem_zep(vif_ctx_zep);
#endif /* !CONFIG_NRF700X_RADIO_TEST */
rpu_rem:
	wifi_nrf_fmac_dev_rem_zep(&rpu_drv_priv_zep.rpu_ctx_zep);
fmac_deinit:
	wifi_nrf_fmac_deinit(rpu_drv_priv_zep.fmac_priv);
err:
	return -1;
}

#ifndef CONFIG_NRF700X_RADIO_TEST
static const struct net_wifi_mgmt_offload wifi_offload_ops = {
	.wifi_iface.iface_api.init = wifi_nrf_if_init,
	.wifi_iface.get_capabilities = wifi_nrf_if_caps_get,
	.wifi_iface.send = wifi_nrf_if_send,
	.scan = wifi_nrf_disp_scan_zep,
#ifdef CONFIG_NET_STATISTICS_WIFI
	.get_stats = wifi_nrf_stats_get,
#endif /* CONFIG_NET_STATISTICS_WIFI */
	.set_power_save = wifi_nrf_set_power_save,
	.set_twt = wifi_nrf_set_twt,
	.set_power_save_mode = wifi_nrf_set_power_save_mode,
};

#ifdef CONFIG_WPA_SUPP
static const struct zep_wpa_supp_dev_ops wpa_supp_ops = {
	.init = wifi_nrf_wpa_supp_dev_init,
	.deinit = wifi_nrf_wpa_supp_dev_deinit,
	.scan2 = wifi_nrf_wpa_supp_scan2,
	.scan_abort = wifi_nrf_wpa_supp_scan_abort,
	.get_scan_results2 = wifi_nrf_wpa_supp_scan_results_get,
	.deauthenticate = wifi_nrf_wpa_supp_deauthenticate,
	.authenticate = wifi_nrf_wpa_supp_authenticate,
	.associate = wifi_nrf_wpa_supp_associate,
	.set_supp_port = wifi_nrf_wpa_set_supp_port,
	.set_key = wifi_nrf_wpa_supp_set_key,
	.signal_poll = wifi_nrf_wpa_supp_signal_poll,
	.send_mlme = wifi_nrf_nl80211_send_mlme,
	.get_wiphy = wifi_nrf_supp_get_wiphy,
	.register_frame = wifi_nrf_supp_register_frame,
	.get_capa = wifi_nrf_supp_get_capa,
};
#endif /* CONFIG_WPA_SUPP */
#endif /* !CONFIG_NRF700X_RADIO_TEST */


#ifdef CONFIG_NETWORKING
ETH_NET_DEVICE_INIT(wlan0, /* name - token */
		    "wlan0", /* driver name - dev->name */
		    wifi_nrf_drv_main_zep, /* init_fn */
		    NULL, /* pm_action_cb */
		    &rpu_drv_priv_zep.rpu_ctx_zep.vif_ctx_zep[0], /* data */
#ifdef CONFIG_WPA_SUPP
		    &wpa_supp_ops, /* cfg */
#else /* CONFIG_WPA_SUPP */
		    NULL, /* cfg */
#endif /* !CONFIG_WPA_SUPP */
		    CONFIG_WIFI_INIT_PRIORITY, /* prio */
		    &wifi_offload_ops, /* api */
		    1500); /*mtu */
#else
DEVICE_DEFINE(wlan0, /* name - token */
	      "wlan0", /* driver name - dev->name */
	      wifi_nrf_drv_main_zep, /* init_fn */
	      NULL, /* pm_action_cb */
#ifndef CONFIG_NRF700X_RADIO_TEST
	      &rpu_drv_priv_zep.rpu_ctx_zep.vif_ctx_zep[0], /* data */
#else /* !CONFIG_NRF700X_RADIO_TEST */
	      NULL,
#endif /* CONFIG_NRF700X_RADIO_TEST */
	      NULL, /* cfg */
	      POST_KERNEL,
	      CONFIG_WIFI_INIT_PRIORITY, /* prio */
	      NULL); /* api */
#endif /* CONFIG_WPA_SUPP */
