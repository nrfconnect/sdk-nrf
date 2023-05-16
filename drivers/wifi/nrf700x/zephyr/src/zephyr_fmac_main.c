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
#ifdef CONFIG_NET_L2_ETHERNET
#include <zephyr/net/ethernet.h>
#endif
#include <zephyr/logging/log.h>
#include <zephyr/net/wifi_mgmt.h>

#include <rpu_fw_patches.h>
#include <util.h>
#include <fmac_api.h>
#include <zephyr_fmac_main.h>
#ifdef CONFIG_WPA_SUPP
#include <zephyr_wpa_supp_if.h>
#endif /* CONFIG_WPA_SUPP */
#ifndef CONFIG_NRF700X_RADIO_TEST
#include <zephyr_wifi_mgmt.h>

#endif /* !CONFIG_NRF700X_RADIO_TEST */

LOG_MODULE_DECLARE(wifi_nrf, CONFIG_WIFI_LOG_LEVEL);

struct wifi_nrf_drv_priv_zep rpu_drv_priv_zep;

/* 3 bytes for addreess, 3 bytes for length */
#define MAX_PKT_RAM_TX_ALIGN_OVERHEAD 6
#ifndef CONFIG_NRF700X_RADIO_TEST
#ifdef CONFIG_NRF700X_DATA_TX

#define MAX_RX_QUEUES 3

#define MAX_TX_FRAME_SIZE \
	(CONFIG_NRF700X_TX_MAX_DATA_SIZE + TX_BUF_HEADROOM)
#define TOTAL_TX_SIZE \
	(CONFIG_NRF700X_MAX_TX_TOKENS * CONFIG_NRF700X_TX_MAX_DATA_SIZE)
#define TOTAL_RX_SIZE \
	(CONFIG_NRF700X_RX_NUM_BUFS * CONFIG_NRF700X_RX_MAX_DATA_SIZE)

BUILD_ASSERT(CONFIG_NRF700X_MAX_TX_TOKENS >= 1,
	"At least one TX token is required");
BUILD_ASSERT(CONFIG_NRF700X_MAX_TX_AGGREGATION <= 15,
	"Max TX aggregation is 15");
BUILD_ASSERT(CONFIG_NRF700X_RX_NUM_BUFS >= 1,
	"At least one RX buffer is required");
BUILD_ASSERT(RPU_PKTRAM_SIZE - TOTAL_RX_SIZE >= TOTAL_TX_SIZE,
	"Packet RAM overflow: not enough memory for TX");

static const unsigned char aggregation = 1;
static const unsigned char wmm = 1;
static const unsigned char max_num_tx_agg_sessions = 4;
static const unsigned char max_num_rx_agg_sessions = 8;
static const unsigned char reorder_buf_size = 16;
static const unsigned char max_rxampdu_size = MAX_RX_AMPDU_SIZE_64KB;

static const unsigned char max_tx_aggregation = CONFIG_NRF700X_MAX_TX_AGGREGATION;

static const unsigned int rx1_num_bufs = CONFIG_NRF700X_RX_NUM_BUFS / MAX_RX_QUEUES;
static const unsigned int rx2_num_bufs = CONFIG_NRF700X_RX_NUM_BUFS / MAX_RX_QUEUES;
static const unsigned int rx3_num_bufs = CONFIG_NRF700X_RX_NUM_BUFS / MAX_RX_QUEUES;

static const unsigned int rx1_buf_sz = CONFIG_NRF700X_RX_MAX_DATA_SIZE;
static const unsigned int rx2_buf_sz = CONFIG_NRF700X_RX_MAX_DATA_SIZE;
static const unsigned int rx3_buf_sz = CONFIG_NRF700X_RX_MAX_DATA_SIZE;

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

void wifi_nrf_event_get_reg_zep(void *vif_ctx,
				struct nrf_wifi_reg *get_reg_event,
				unsigned int event_len)
{
	struct wifi_nrf_vif_ctx_zep *vif_ctx_zep = NULL;
	struct wifi_nrf_ctx_zep *rpu_ctx_zep = NULL;
	struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx = NULL;

	LOG_DBG("%s: alpha2 = %c%c\n", __func__,
		   get_reg_event->nrf_wifi_alpha2[0],
		   get_reg_event->nrf_wifi_alpha2[1]);
	vif_ctx_zep = vif_ctx;

	if (!vif_ctx_zep) {
		LOG_ERR("%s: vif_ctx_zep is NULL\n", __func__);
		return;
	}

	rpu_ctx_zep = vif_ctx_zep->rpu_ctx_zep;
	fmac_dev_ctx = rpu_ctx_zep->rpu_ctx;

	memcpy(&fmac_dev_ctx->alpha2,
		   &get_reg_event->nrf_wifi_alpha2,
		   sizeof(get_reg_event->nrf_wifi_alpha2));
	fmac_dev_ctx->alpha2_valid = true;
}

void wifi_nrf_event_proc_cookie_rsp(void *vif_ctx,
				    struct nrf_wifi_umac_event_cookie_rsp *cookie_rsp_event,
				    unsigned int event_len)
{
	struct wifi_nrf_vif_ctx_zep *vif_ctx_zep = NULL;
	struct wifi_nrf_ctx_zep *rpu_ctx_zep = NULL;
	struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx = NULL;

	vif_ctx_zep = vif_ctx;

	if (!vif_ctx_zep) {
		LOG_ERR("%s: vif_ctx_zep is NULL\n", __func__);
		return;
	}

	rpu_ctx_zep = vif_ctx_zep->rpu_ctx_zep;
	fmac_dev_ctx = rpu_ctx_zep->rpu_ctx;

	LOG_DBG("%s: cookie_rsp_event->cookie = %llx\n", __func__, cookie_rsp_event->cookie);
	LOG_DBG("%s: host_cookie = %llx\n", __func__, cookie_rsp_event->host_cookie);
	LOG_DBG("%s: mac_addr = %x:%x:%x:%x:%x:%x\n", __func__,
		cookie_rsp_event->mac_addr[0],
		cookie_rsp_event->mac_addr[1],
		cookie_rsp_event->mac_addr[2],
		cookie_rsp_event->mac_addr[3],
		cookie_rsp_event->mac_addr[4],
		cookie_rsp_event->mac_addr[5]);

	vif_ctx_zep->cookie_resp_received = true;
	/* TODO: When supp_callbk_fns.mgmt_tx_status is implemented, add logic
	 * here to use the cookie and host_cookie to map requests to responses.
	 */
}

int wifi_nrf_reg_domain(const struct device *dev, struct wifi_reg_domain *reg_domain)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct wifi_nrf_ctx_zep *rpu_ctx_zep = NULL;
	struct wifi_nrf_vif_ctx_zep *vif_ctx_zep = NULL;
	struct wifi_nrf_fmac_reg_info reg_domain_info = {0};
	int ret = -1;

	if (!dev || !reg_domain) {
		goto err;
	}

	vif_ctx_zep = dev->data;

	if (!vif_ctx_zep) {
		LOG_ERR("%s: vif_ctx_zep is NULL\n", __func__);
		goto err;
	}

	rpu_ctx_zep = vif_ctx_zep->rpu_ctx_zep;

	if (!rpu_ctx_zep) {
		LOG_ERR("%s: rpu_ctx_zep is NULL\n", __func__);
		goto err;
	}

	if (reg_domain->oper == WIFI_MGMT_SET) {
		memcpy(reg_domain_info.alpha2, reg_domain->country_code, WIFI_COUNTRY_CODE_LEN);

		reg_domain_info.force = reg_domain->force;

		status = wifi_nrf_fmac_set_reg(rpu_ctx_zep->rpu_ctx, &reg_domain_info);
		if (status != WIFI_NRF_STATUS_SUCCESS) {
			LOG_ERR("%s: Failed to set regulatory domain\n", __func__);
			goto err;
		}
	} else if (reg_domain->oper == WIFI_MGMT_GET) {
		status = wifi_nrf_fmac_get_reg(rpu_ctx_zep->rpu_ctx, &reg_domain_info);
		if (status != WIFI_NRF_STATUS_SUCCESS) {
			LOG_ERR("%s: Failed to get regulatory domain\n", __func__);
			goto err;
		}
		memcpy(reg_domain->country_code, reg_domain_info.alpha2, WIFI_COUNTRY_CODE_LEN);
	} else {
		LOG_ERR("%s: Invalid operation: %d\n", __func__, reg_domain->oper);
		goto err;
	}

	ret = 0;
err:
	return ret;
}

#endif /* !CONFIG_NRF700X_RADIO_TEST */

enum wifi_nrf_status wifi_nrf_fmac_dev_add_zep(struct wifi_nrf_drv_priv_zep *drv_priv_zep)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct wifi_nrf_ctx_zep *rpu_ctx_zep = NULL;
	struct wifi_nrf_fmac_fw_info fw_info;
	void *rpu_ctx = NULL;
#if defined(CONFIG_BOARD_NRF7000) || defined(CONFIG_BOARD_NRF7001)
	enum op_band op_band = BAND_24G;
#else /* CONFIG_BOARD_NRF7000 || CONFIG_BOARD_NRF7001 */
	enum op_band op_band = BAND_ALL;
#endif /* CONFIG_BOARD_NRF7000 || CONFIG_BOARD_NRF7001 */
#ifdef CONFIG_NRF_WIFI_LOW_POWER
	int sleep_type = -1;

#ifndef CONFIG_NRF700X_RADIO_TEST
	sleep_type = HW_SLEEP_ENABLE;
#else
	sleep_type = SLEEP_DISABLE;
#endif /* CONFIG_NRF700X_RADIO_TEST */
#endif /* CONFIG_NRF_WIFI_LOW_POWER */
	unsigned int umac_ver = 0;
	unsigned int lmac_ver = 0;
	struct nrf_wifi_tx_pwr_ctrl_params tx_pwr_ctrl_params;

	rpu_ctx_zep = &drv_priv_zep->rpu_ctx_zep;

	rpu_ctx_zep->drv_priv_zep = drv_priv_zep;

	rpu_ctx = wifi_nrf_fmac_dev_add(drv_priv_zep->fmac_priv, rpu_ctx_zep);

	if (!rpu_ctx) {
		LOG_ERR("%s: wifi_nrf_fmac_dev_add failed\n", __func__);
		rpu_ctx_zep = NULL;
		goto out;
	}

	rpu_ctx_zep->rpu_ctx = rpu_ctx;

	memset(&fw_info,
	       0,
	       sizeof(fw_info));

	fw_info.lmac_patch_pri.data = wifi_nrf_lmac_patch_pri_bimg;
	fw_info.lmac_patch_pri.size = sizeof(wifi_nrf_lmac_patch_pri_bimg);
	fw_info.lmac_patch_sec.data = wifi_nrf_lmac_patch_sec_bin;
	fw_info.lmac_patch_sec.size = sizeof(wifi_nrf_lmac_patch_sec_bin);
	fw_info.umac_patch_pri.data = wifi_nrf_umac_patch_pri_bimg;
	fw_info.umac_patch_pri.size = sizeof(wifi_nrf_umac_patch_pri_bimg);
	fw_info.umac_patch_sec.data = wifi_nrf_umac_patch_sec_bin;
	fw_info.umac_patch_sec.size = sizeof(wifi_nrf_umac_patch_sec_bin);

	/* Load the FW patches to the RPU */
	status = wifi_nrf_fmac_fw_load(rpu_ctx,
				       &fw_info);

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		LOG_ERR("%s: wifi_nrf_fmac_fw_load failed\n", __func__);
		goto out;
	}

	status = wifi_nrf_fmac_ver_get(rpu_ctx,
				       &umac_ver,
				       &lmac_ver);

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		LOG_ERR("%s: FW version read failed\n", __func__);
		goto out;
	}

	LOG_INF("Firmware (v%d.%d.%d.%d) booted successfully\n",
		NRF_WIFI_UMAC_VER(umac_ver),
		NRF_WIFI_UMAC_VER_MAJ(umac_ver),
		NRF_WIFI_UMAC_VER_MIN(umac_ver),
		NRF_WIFI_UMAC_VER_EXTRA(umac_ver));

	tx_pwr_ctrl_params.ant_gain_2g = CONFIG_NRF700X_ANT_GAIN_2G;
	tx_pwr_ctrl_params.ant_gain_5g_band1 = CONFIG_NRF700X_ANT_GAIN_5G_BAND1;
	tx_pwr_ctrl_params.ant_gain_5g_band2 = CONFIG_NRF700X_ANT_GAIN_5G_BAND2;
	tx_pwr_ctrl_params.ant_gain_5g_band3 = CONFIG_NRF700X_ANT_GAIN_5G_BAND3;
	tx_pwr_ctrl_params.band_edge_2g_lo = CONFIG_NRF700X_BAND_2G_LOWER_EDGE_BACKOFF;
	tx_pwr_ctrl_params.band_edge_2g_hi = CONFIG_NRF700X_BAND_2G_UPPER_EDGE_BACKOFF;
	tx_pwr_ctrl_params.band_edge_5g_unii_1_lo = CONFIG_NRF700X_BAND_UNII_1_LOWER_EDGE_BACKOFF;
	tx_pwr_ctrl_params.band_edge_5g_unii_1_hi = CONFIG_NRF700X_BAND_UNII_1_UPPER_EDGE_BACKOFF;
	tx_pwr_ctrl_params.band_edge_5g_unii_2a_lo = CONFIG_NRF700X_BAND_UNII_2A_LOWER_EDGE_BACKOFF;
	tx_pwr_ctrl_params.band_edge_5g_unii_2a_hi = CONFIG_NRF700X_BAND_UNII_2A_UPPER_EDGE_BACKOFF;
	tx_pwr_ctrl_params.band_edge_5g_unii_2c_lo = CONFIG_NRF700X_BAND_UNII_2C_LOWER_EDGE_BACKOFF;
	tx_pwr_ctrl_params.band_edge_5g_unii_2c_hi = CONFIG_NRF700X_BAND_UNII_2C_UPPER_EDGE_BACKOFF;
	tx_pwr_ctrl_params.band_edge_5g_unii_3_lo = CONFIG_NRF700X_BAND_UNII_3_LOWER_EDGE_BACKOFF;
	tx_pwr_ctrl_params.band_edge_5g_unii_3_hi = CONFIG_NRF700X_BAND_UNII_3_UPPER_EDGE_BACKOFF;
	tx_pwr_ctrl_params.band_edge_5g_unii_4_lo = CONFIG_NRF700X_BAND_UNII_4_LOWER_EDGE_BACKOFF;
	tx_pwr_ctrl_params.band_edge_5g_unii_4_hi = CONFIG_NRF700X_BAND_UNII_4_UPPER_EDGE_BACKOFF;

	status = wifi_nrf_fmac_dev_init(rpu_ctx_zep->rpu_ctx,
#ifndef CONFIG_NRF700X_RADIO_TEST
					NULL,
#endif /* !CONFIG_NRF700X_RADIO_TEST */
#ifdef CONFIG_NRF_WIFI_LOW_POWER
					sleep_type,
#endif /* CONFIG_NRF_WIFI_LOW_POWER */
					NRF_WIFI_DEF_PHY_CALIB,
					op_band,
					&tx_pwr_ctrl_params);

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		LOG_ERR("%s: wifi_nrf_fmac_dev_init failed\n", __func__);
		goto out;
	}
out:
	return status;
}


static int wifi_nrf_drv_main_zep(const struct device *dev)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
#ifndef CONFIG_NRF700X_RADIO_TEST
	struct wifi_nrf_fmac_callbk_fns callbk_fns = { 0 };
	struct nrf_wifi_data_config_params data_config = { 0 };
	struct rx_buf_pool_params rx_buf_pools[MAX_NUM_OF_RX_QUEUES];
	struct wifi_nrf_vif_ctx_zep *vif_ctx_zep = dev->data;

	vif_ctx_zep->rpu_ctx_zep = &rpu_drv_priv_zep.rpu_ctx_zep;

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
	callbk_fns.event_get_reg = wifi_nrf_event_get_reg_zep;
	callbk_fns.event_get_ps_info = wifi_nrf_event_proc_get_power_save_info;
	callbk_fns.cookie_rsp_callbk_fn = wifi_nrf_event_proc_cookie_rsp;
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
	callbk_fns.get_conn_info_callbk_fn = wifi_nrf_supp_event_proc_get_conn_info;
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

#ifdef CONFIG_NRF700X_DATA_TX
	rpu_drv_priv_zep.fmac_priv->max_ampdu_len_per_token =
		(RPU_PKTRAM_SIZE - (CONFIG_NRF700X_RX_NUM_BUFS * CONFIG_NRF700X_RX_MAX_DATA_SIZE)) /
		CONFIG_NRF700X_MAX_TX_TOKENS;
	/* Align to 4-byte */
	rpu_drv_priv_zep.fmac_priv->max_ampdu_len_per_token &= ~0x3;

	/* Alignment overhead for size based coalesce */
	rpu_drv_priv_zep.fmac_priv->avail_ampdu_len_per_token =
	rpu_drv_priv_zep.fmac_priv->max_ampdu_len_per_token -
		(MAX_PKT_RAM_TX_ALIGN_OVERHEAD * max_tx_aggregation);
#endif /* CONFIG_NRF700X_DATA_TX */

	status = wifi_nrf_fmac_dev_add_zep(&rpu_drv_priv_zep);

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		LOG_ERR("%s: wifi_nrf_fmac_dev_add_zep failed\n",
			__func__);
		goto fmac_deinit;
	}
#ifndef CONFIG_NRF700X_RADIO_TEST
	k_work_init_delayable(&vif_ctx_zep->scan_timeout_work,
			      wifi_nrf_scan_timeout_work);
#endif /* !CONFIG_NRF700X_RADIO_TEST */

	return 0;
fmac_deinit:
	wifi_nrf_fmac_deinit(rpu_drv_priv_zep.fmac_priv);
err:
	return -1;
}

#ifndef CONFIG_NRF700X_RADIO_TEST
static const struct net_wifi_mgmt_offload wifi_offload_ops = {
	.wifi_iface.iface_api.init = wifi_nrf_if_init_zep,
	.wifi_iface.start = wifi_nrf_if_start_zep,
	.wifi_iface.stop = wifi_nrf_if_stop_zep,
	.wifi_iface.set_config = wifi_nrf_if_set_config_zep,
	.wifi_iface.get_capabilities = wifi_nrf_if_caps_get,
	.wifi_iface.send = wifi_nrf_if_send,
#ifdef CONFIG_NET_STATISTICS_ETHERNET
	.wifi_iface.get_stats = wifi_nrf_eth_stats_get,
#endif /* CONFIG_NET_STATISTICS_ETHERNET */
	.scan = wifi_nrf_disp_scan_zep,
#ifdef CONFIG_NET_STATISTICS_WIFI
	.get_stats = wifi_nrf_stats_get,
#endif /* CONFIG_NET_STATISTICS_WIFI */
	.set_power_save = wifi_nrf_set_power_save,
	.set_twt = wifi_nrf_set_twt,
	.set_power_save_mode = wifi_nrf_set_power_save_mode,
	.reg_domain = wifi_nrf_reg_domain,
	.get_power_save_config = wifi_nrf_get_power_save_config,
	.set_power_save_timeout = wifi_nrf_set_power_save_timeout,
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
	.get_conn_info = wifi_nrf_supp_get_conn_info,
};
#endif /* CONFIG_WPA_SUPP */
#endif /* !CONFIG_NRF700X_RADIO_TEST */


#ifdef CONFIG_NET_L2_ETHERNET
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
	      &rpu_drv_priv_zep, /* data */
#else /* !CONFIG_NRF700X_RADIO_TEST */
	      NULL,
#endif /* CONFIG_NRF700X_RADIO_TEST */
	      NULL, /* cfg */
	      POST_KERNEL,
	      CONFIG_WIFI_INIT_PRIORITY, /* prio */
	      NULL); /* api */
#endif /* CONFIG_WPA_SUPP */
