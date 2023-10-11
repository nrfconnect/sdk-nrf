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
#include <net/l2_wifi_connect.h>


#include <util.h>
#include <fmac_api.h>
#include "fmac_util.h"
#include <zephyr_fmac_main.h>

#ifndef CONFIG_NRF700X_RADIO_TEST
#ifdef CONFIG_NRF700X_STA_MODE
#include <zephyr_wifi_mgmt_scan.h>
#include <zephyr_wifi_mgmt.h>
#include <zephyr_wpa_supp_if.h>
#else
#include <zephyr_wifi_mgmt_scan.h>
#endif /* CONFIG_WPA_SPP */
#include <zephyr/net/conn_mgr_connectivity.h>

#endif /* !CONFIG_NRF700X_RADIO_TEST */

LOG_MODULE_DECLARE(wifi_nrf, CONFIG_WIFI_LOG_LEVEL);

struct nrf_wifi_drv_priv_zep rpu_drv_priv_zep;

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

struct nrf_wifi_drv_priv_zep rpu_drv_priv_zep;

const char *nrf_wifi_get_drv_version(void)
{
	return NRF700X_DRIVER_VERSION;
}

/* If the interface is not Wi-Fi then errors are expected, so, fail silently */
struct nrf_wifi_vif_ctx_zep *nrf_wifi_get_vif_ctx(struct net_if *iface)
{
	struct nrf_wifi_vif_ctx_zep *vif_ctx_zep = NULL;
	struct nrf_wifi_ctx_zep *rpu_ctx = &rpu_drv_priv_zep.rpu_ctx_zep;

	if (!iface || !rpu_ctx) {
		return NULL;
	}

	for (int i = 0; i < ARRAY_SIZE(rpu_ctx->vif_ctx_zep); i++) {
		if (rpu_ctx->vif_ctx_zep[i].zep_net_if_ctx == iface) {
			vif_ctx_zep = &rpu_ctx->vif_ctx_zep[i];
			break;
		}
	}

	return vif_ctx_zep;
}

void nrf_wifi_event_proc_scan_start_zep(void *if_priv,
					struct nrf_wifi_umac_event_trigger_scan *scan_start_event,
					unsigned int event_len)
{
	struct nrf_wifi_vif_ctx_zep *vif_ctx_zep = NULL;

	vif_ctx_zep = if_priv;

	if (vif_ctx_zep->scan_type == SCAN_DISPLAY) {
		return;
	}

#ifdef CONFIG_NRF700X_STA_MODE
	nrf_wifi_wpa_supp_event_proc_scan_start(if_priv);
#endif /* CONFIG_NRF700X_STA_MODE */
}


void nrf_wifi_event_proc_scan_done_zep(void *vif_ctx,
				       struct nrf_wifi_umac_event_trigger_scan *scan_done_event,
				       unsigned int event_len)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_vif_ctx_zep *vif_ctx_zep = NULL;

	vif_ctx_zep = vif_ctx;

	if (!vif_ctx_zep) {
		LOG_ERR("%s: vif_ctx_zep is NULL\n", __func__);
		return;
	}

	if (vif_ctx_zep->scan_type == SCAN_DISPLAY) {
		status = nrf_wifi_disp_scan_res_get_zep(vif_ctx_zep);

		if (status != NRF_WIFI_STATUS_SUCCESS) {
			LOG_ERR("%s: nrf_wifi_disp_scan_res_get_zep failed\n", __func__);
			return;
		}
		vif_ctx_zep->scan_in_progress = false;
#ifdef CONFIG_NRF700X_STA_MODE
	} else if (vif_ctx_zep->scan_type == SCAN_CONNECT) {
		nrf_wifi_wpa_supp_event_proc_scan_done(vif_ctx_zep,
						       scan_done_event,
						       event_len,
						       0);
#endif /* CONFIG_NRF700X_STA_MODE */
	} else {
		LOG_ERR("%s: Scan type = %d not supported yet\n", __func__, vif_ctx_zep->scan_type);
		return;
	}

	status = NRF_WIFI_STATUS_SUCCESS;
}

void nrf_wifi_scan_timeout_work(struct k_work *work)
{
	struct nrf_wifi_vif_ctx_zep *vif_ctx_zep = NULL;
	struct nrf_wifi_ctx_zep *rpu_ctx_zep = NULL;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;
	struct wifi_scan_result res;
	scan_result_cb_t disp_scan_cb = NULL;

	vif_ctx_zep = CONTAINER_OF(work, struct nrf_wifi_vif_ctx_zep, scan_timeout_work);

	disp_scan_cb = (scan_result_cb_t)vif_ctx_zep->disp_scan_cb;

	if (!vif_ctx_zep->scan_in_progress) {
		LOG_INF("%s: Scan not in progress\n", __func__);
		return;
	}

	rpu_ctx_zep = vif_ctx_zep->rpu_ctx_zep;
	fmac_dev_ctx = rpu_ctx_zep->rpu_ctx;

	if (disp_scan_cb) {
		memset(&res, 0x0, sizeof(res));

		disp_scan_cb(vif_ctx_zep->zep_net_if_ctx, -ETIMEDOUT, &res);
		vif_ctx_zep->disp_scan_cb = NULL;
	} else {
#ifdef CONFIG_NRF700X_STA_MODE
		/* WPA supplicant scan */
		union wpa_event_data event;
		struct scan_info *info = NULL;

		memset(&event, 0, sizeof(event));

		info = &event.scan_info;

		info->aborted = 0;
		info->external_scan = 0;
		info->nl_scan_event = 1;

		if (vif_ctx_zep->supp_drv_if_ctx &&
			vif_ctx_zep->supp_callbk_fns.scan_done) {
			vif_ctx_zep->supp_callbk_fns.scan_done(vif_ctx_zep->supp_drv_if_ctx,
				&event);
		}
#endif /* CONFIG_NRF700X_STA_MODE */
	}

	vif_ctx_zep->scan_in_progress = false;
}

#ifdef CONFIG_NRF700X_STA_MODE
static void nrf_wifi_process_rssi_from_rx(void *vif_ctx,
				   signed short signal)
{
	struct nrf_wifi_vif_ctx_zep *vif_ctx_zep = vif_ctx;
	struct nrf_wifi_ctx_zep *rpu_ctx_zep = NULL;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;

	vif_ctx_zep = vif_ctx;

	if (!vif_ctx_zep) {
		LOG_ERR("%s: vif_ctx_zep is NULL\n", __func__);
		return;
	}

	rpu_ctx_zep = vif_ctx_zep->rpu_ctx_zep;

	if (!rpu_ctx_zep) {
		LOG_ERR("%s: rpu_ctx_zep is NULL\n", __func__);
		return;
	}

	fmac_dev_ctx = rpu_ctx_zep->rpu_ctx;

	vif_ctx_zep->rssi = MBM_TO_DBM(signal);
	vif_ctx_zep->rssi_record_timestamp_us =
		nrf_wifi_osal_time_get_curr_us(fmac_dev_ctx->fpriv->opriv);
}
#endif /* CONFIG_NRF700X_STA_MODE */


void nrf_wifi_event_get_reg_zep(void *vif_ctx,
				struct nrf_wifi_reg *get_reg_event,
				unsigned int event_len)
{
	struct nrf_wifi_vif_ctx_zep *vif_ctx_zep = NULL;
	struct nrf_wifi_ctx_zep *rpu_ctx_zep = NULL;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;

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

int nrf_wifi_reg_domain(const struct device *dev, struct wifi_reg_domain *reg_domain)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_ctx_zep *rpu_ctx_zep = NULL;
	struct nrf_wifi_vif_ctx_zep *vif_ctx_zep = NULL;
	struct nrf_wifi_fmac_reg_info reg_domain_info = {0};
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

		status = nrf_wifi_fmac_set_reg(rpu_ctx_zep->rpu_ctx, &reg_domain_info);
		if (status != NRF_WIFI_STATUS_SUCCESS) {
			LOG_ERR("%s: Failed to set regulatory domain\n", __func__);
			goto err;
		}
	} else if (reg_domain->oper == WIFI_MGMT_GET) {
		status = nrf_wifi_fmac_get_reg(rpu_ctx_zep->rpu_ctx, &reg_domain_info);
		if (status != NRF_WIFI_STATUS_SUCCESS) {
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
#ifdef CONFIG_NRF700X_STA_MODE
void nrf_wifi_event_proc_cookie_rsp(void *vif_ctx,
				    struct nrf_wifi_umac_event_cookie_rsp *cookie_rsp_event,
				    unsigned int event_len)
{
	struct nrf_wifi_vif_ctx_zep *vif_ctx_zep = NULL;
	struct nrf_wifi_ctx_zep *rpu_ctx_zep = NULL;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;

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
#endif /* CONFIG_NRF700X_STA_MODE */
#endif /* !CONFIG_NRF700X_RADIO_TEST */


void set_tx_pwr_ceil_default(struct nrf_wifi_tx_pwr_ceil_params *pwr_ceil_params)
{
	memset(pwr_ceil_params, 0, sizeof(*pwr_ceil_params));
#if defined(CONFIG_BOARD_NRF7002DK_NRF7001_NRF5340_CPUAPP) || \
defined(CONFIG_BOARD_NRF7002DK_NRF5340_CPUAPP) || \
defined(CONFIG_BOARD_NRF5340DK_NRF5340_CPUAPP)
	pwr_ceil_params->max_pwr_2g_dsss = 0x54;
	pwr_ceil_params->max_pwr_2g_mcs7 = 0x40;
	pwr_ceil_params->max_pwr_2g_mcs0 = 0x40;
#endif
#if defined(CONFIG_BOARD_NRF7002DK_NRF5340_CPUAPP) || defined(CONFIG_BOARD_NRF5340DK_NRF5340_CPUAPP)
	pwr_ceil_params->max_pwr_5g_low_mcs7 = 0x38;
	pwr_ceil_params->max_pwr_5g_mid_mcs7 = 0x38;
	pwr_ceil_params->max_pwr_5g_high_mcs7 = 0x38;
	pwr_ceil_params->max_pwr_5g_low_mcs0 = 0x38;
	pwr_ceil_params->max_pwr_5g_mid_mcs0 = 0x38;
	pwr_ceil_params->max_pwr_5g_high_mcs0 = 0x38;
#endif
}

void configure_tx_pwr_settings(struct nrf_wifi_tx_pwr_ctrl_params *tx_pwr_ctrl_params,
				struct nrf_wifi_tx_pwr_ceil_params *tx_pwr_ceil_params)
{
	tx_pwr_ctrl_params->ant_gain_2g = CONFIG_NRF700X_ANT_GAIN_2G;
	tx_pwr_ctrl_params->ant_gain_5g_band1 = CONFIG_NRF700X_ANT_GAIN_5G_BAND1;
	tx_pwr_ctrl_params->ant_gain_5g_band2 = CONFIG_NRF700X_ANT_GAIN_5G_BAND2;
	tx_pwr_ctrl_params->ant_gain_5g_band3 = CONFIG_NRF700X_ANT_GAIN_5G_BAND3;
	tx_pwr_ctrl_params->band_edge_2g_lo = CONFIG_NRF700X_BAND_2G_LOWER_EDGE_BACKOFF;
	tx_pwr_ctrl_params->band_edge_2g_hi = CONFIG_NRF700X_BAND_2G_UPPER_EDGE_BACKOFF;
	tx_pwr_ctrl_params->band_edge_5g_unii_1_lo = CONFIG_NRF700X_BAND_UNII_1_LOWER_EDGE_BACKOFF;
	tx_pwr_ctrl_params->band_edge_5g_unii_1_hi = CONFIG_NRF700X_BAND_UNII_1_UPPER_EDGE_BACKOFF;
	tx_pwr_ctrl_params->band_edge_5g_unii_2a_lo =
					CONFIG_NRF700X_BAND_UNII_2A_LOWER_EDGE_BACKOFF;
	tx_pwr_ctrl_params->band_edge_5g_unii_2a_hi =
					CONFIG_NRF700X_BAND_UNII_2A_UPPER_EDGE_BACKOFF;
	tx_pwr_ctrl_params->band_edge_5g_unii_2c_lo =
					CONFIG_NRF700X_BAND_UNII_2C_LOWER_EDGE_BACKOFF;
	tx_pwr_ctrl_params->band_edge_5g_unii_2c_hi =
					CONFIG_NRF700X_BAND_UNII_2C_UPPER_EDGE_BACKOFF;
	tx_pwr_ctrl_params->band_edge_5g_unii_3_lo = CONFIG_NRF700X_BAND_UNII_3_LOWER_EDGE_BACKOFF;
	tx_pwr_ctrl_params->band_edge_5g_unii_3_hi = CONFIG_NRF700X_BAND_UNII_3_UPPER_EDGE_BACKOFF;
	tx_pwr_ctrl_params->band_edge_5g_unii_4_lo = CONFIG_NRF700X_BAND_UNII_4_LOWER_EDGE_BACKOFF;
	tx_pwr_ctrl_params->band_edge_5g_unii_4_hi = CONFIG_NRF700X_BAND_UNII_4_UPPER_EDGE_BACKOFF;

#if defined(CONFIG_BOARD_NRF7002DK_NRF7001_NRF5340_CPUAPP) || \
defined(CONFIG_BOARD_NRF7002DK_NRF5340_CPUAPP) || \
defined(CONFIG_BOARD_NRF5340DK_NRF5340_CPUAPP)
	set_tx_pwr_ceil_default(tx_pwr_ceil_params);
	#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf70_tx_power_ceiling), max_pwr_2g_dsss)
		tx_pwr_ceil_params->max_pwr_2g_dsss =
			DT_PROP(DT_NODELABEL(nrf70_tx_power_ceiling), max_pwr_2g_dsss);
	#endif

	#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf70_tx_power_ceiling), max_pwr_2g_mcs7)
		tx_pwr_ceil_params->max_pwr_2g_mcs7 =
			DT_PROP(DT_NODELABEL(nrf70_tx_power_ceiling), max_pwr_2g_mcs7);
	#endif

	#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf70_tx_power_ceiling), max_pwr_2g_mcs0)
		tx_pwr_ceil_params->max_pwr_2g_mcs0 =
			DT_PROP(DT_NODELABEL(nrf70_tx_power_ceiling), max_pwr_2g_mcs0);
	#endif

	#if DT_NODE_EXISTS(DT_NODELABEL(nrf70_tx_power_ceiling))
		tx_pwr_ceil_params->rf_tx_pwr_ceil_params_override = 1;
	#else
		tx_pwr_ceil_params->rf_tx_pwr_ceil_params_override = 0;
	#endif
#endif

#if defined(CONFIG_BOARD_NRF7002DK_NRF5340_CPUAPP) || defined(CONFIG_BOARD_NRF5340DK_NRF5340_CPUAPP)

	#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf70_tx_power_ceiling), max_pwr_5g_low_mcs7)
		tx_pwr_ceil_params->max_pwr_5g_low_mcs7 =
			DT_PROP(DT_NODELABEL(nrf70_tx_power_ceiling), max_pwr_5g_low_mcs7);
	#endif

	#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf70_tx_power_ceiling), max_pwr_5g_mid_mcs7)
		tx_pwr_ceil_params->max_pwr_5g_mid_mcs7 =
			DT_PROP(DT_NODELABEL(nrf70_tx_power_ceiling), max_pwr_5g_mid_mcs7);
	#endif

	#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf70_tx_power_ceiling), max_pwr_5g_high_mcs7)
		tx_pwr_ceil_params->max_pwr_5g_high_mcs7 =
			DT_PROP(DT_NODELABEL(nrf70_tx_power_ceiling), max_pwr_5g_high_mcs7);
	#endif

	#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf70_tx_power_ceiling), max_pwr_5g_low_mcs0)
		tx_pwr_ceil_params->max_pwr_5g_low_mcs0 =
			DT_PROP(DT_NODELABEL(nrf70_tx_power_ceiling), max_pwr_5g_low_mcs0);
	#endif

	#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf70_tx_power_ceiling), max_pwr_5g_mid_mcs0)
		tx_pwr_ceil_params->max_pwr_5g_mid_mcs0 =
			DT_PROP(DT_NODELABEL(nrf70_tx_power_ceiling), max_pwr_5g_mid_mcs0);
	#endif

	#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf70_tx_power_ceiling), max_pwr_5g_high_mcs0)
		tx_pwr_ceil_params->max_pwr_5g_high_mcs0 =
			DT_PROP(DT_NODELABEL(nrf70_tx_power_ceiling), max_pwr_5g_high_mcs0);
	#endif
#endif
}

enum nrf_wifi_status nrf_wifi_fmac_dev_add_zep(struct nrf_wifi_drv_priv_zep *drv_priv_zep)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_ctx_zep *rpu_ctx_zep = NULL;
	void *rpu_ctx = NULL;
#if defined(CONFIG_BOARD_NRF7001)
	enum op_band op_band = BAND_24G;
#else /* CONFIG_BOARD_NRF7001 */
	enum op_band op_band = BAND_ALL;
#endif /* CONFIG_BOARD_NRF7001 */
#ifdef CONFIG_NRF_WIFI_LOW_POWER
	int sleep_type = -1;

#ifndef CONFIG_NRF700X_RADIO_TEST
	sleep_type = HW_SLEEP_ENABLE;
#else
	sleep_type = SLEEP_DISABLE;
#endif /* CONFIG_NRF700X_RADIO_TEST */
#endif /* CONFIG_NRF_WIFI_LOW_POWER */
	struct nrf_wifi_tx_pwr_ctrl_params tx_pwr_ctrl_params;
	struct nrf_wifi_tx_pwr_ceil_params tx_pwr_ceil_params;

	unsigned int fw_ver = 0;

	rpu_ctx_zep = &drv_priv_zep->rpu_ctx_zep;

	rpu_ctx_zep->drv_priv_zep = drv_priv_zep;

	rpu_ctx = nrf_wifi_fmac_dev_add(drv_priv_zep->fmac_priv, rpu_ctx_zep);

	if (!rpu_ctx) {
		LOG_ERR("%s: nrf_wifi_fmac_dev_add failed\n", __func__);
		rpu_ctx_zep = NULL;
		goto out;
	}

	rpu_ctx_zep->rpu_ctx = rpu_ctx;

	status = nrf_wifi_fw_load(rpu_ctx);
	if (status != NRF_WIFI_STATUS_SUCCESS) {
		LOG_ERR("%s: nrf_wifi_fw_load failed\n", __func__);
		goto out;
	}

	status = nrf_wifi_fmac_ver_get(rpu_ctx,
				       &fw_ver);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		LOG_ERR("%s: FW version read failed\n", __func__);
		goto out;
	}

	LOG_DBG("Firmware (v%d.%d.%d.%d) booted successfully\n",
		NRF_WIFI_UMAC_VER(fw_ver),
		NRF_WIFI_UMAC_VER_MAJ(fw_ver),
		NRF_WIFI_UMAC_VER_MIN(fw_ver),
		NRF_WIFI_UMAC_VER_EXTRA(fw_ver));

	configure_tx_pwr_settings(&tx_pwr_ctrl_params,
				  &tx_pwr_ceil_params);


#ifdef CONFIG_NRF700X_RADIO_TEST
	status = nrf_wifi_fmac_dev_init_rt(rpu_ctx_zep->rpu_ctx,
#ifdef CONFIG_NRF_WIFI_LOW_POWER
					sleep_type,
#endif /* CONFIG_NRF_WIFI_LOW_POWER */
					NRF_WIFI_DEF_PHY_CALIB,
					op_band,
					IS_ENABLED(CONFIG_NRF_WIFI_BEAMFORMING),
					&tx_pwr_ctrl_params);
#else
	status = nrf_wifi_fmac_dev_init(rpu_ctx_zep->rpu_ctx,
					NULL,
#ifdef CONFIG_NRF_WIFI_LOW_POWER
					sleep_type,
#endif /* CONFIG_NRF_WIFI_LOW_POWER */
					NRF_WIFI_DEF_PHY_CALIB,
					op_band,
					IS_ENABLED(CONFIG_NRF_WIFI_BEAMFORMING),
					&tx_pwr_ctrl_params,
					&tx_pwr_ceil_params);
#endif /* CONFIG_NRF700X_RADIO_TEST */


	if (status != NRF_WIFI_STATUS_SUCCESS) {
		LOG_ERR("%s: nrf_wifi_fmac_dev_init failed\n", __func__);
		goto out;
	}
out:
	return status;
}

enum nrf_wifi_status nrf_wifi_fmac_dev_rem_zep(struct nrf_wifi_drv_priv_zep *drv_priv_zep)
{
	struct nrf_wifi_ctx_zep *rpu_ctx_zep = NULL;

	rpu_ctx_zep = &drv_priv_zep->rpu_ctx_zep;
#ifdef CONFIG_NRF700X_RADIO_TEST
	nrf_wifi_fmac_dev_deinit_rt(rpu_ctx_zep->rpu_ctx);
	nrf_wifi_fmac_dev_rem_rt(rpu_ctx_zep->rpu_ctx);
#else
	nrf_wifi_fmac_dev_deinit(rpu_ctx_zep->rpu_ctx);
	nrf_wifi_fmac_dev_rem(rpu_ctx_zep->rpu_ctx);
#endif /* CONFIG_NRF700X_RADIO_TEST */

	rpu_ctx_zep->rpu_ctx = NULL;
	LOG_DBG("%s: FMAC device removed\n", __func__);

	return NRF_WIFI_STATUS_SUCCESS;
}

static int nrf_wifi_drv_main_zep(const struct device *dev)
{
#ifndef CONFIG_NRF700X_RADIO_TEST
	struct nrf_wifi_fmac_callbk_fns callbk_fns = { 0 };
	struct nrf_wifi_data_config_params data_config = { 0 };
	struct rx_buf_pool_params rx_buf_pools[MAX_NUM_OF_RX_QUEUES];
	struct nrf_wifi_vif_ctx_zep *vif_ctx_zep = dev->data;

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
	callbk_fns.if_carr_state_chg_callbk_fn = nrf_wifi_if_carr_state_chg;
	callbk_fns.rx_frm_callbk_fn = nrf_wifi_if_rx_frm;
#endif
	rx_buf_pools[0].num_bufs = rx1_num_bufs;
	rx_buf_pools[1].num_bufs = rx2_num_bufs;
	rx_buf_pools[2].num_bufs = rx3_num_bufs;
	rx_buf_pools[0].buf_sz = rx1_buf_sz;
	rx_buf_pools[1].buf_sz = rx2_buf_sz;
	rx_buf_pools[2].buf_sz = rx3_buf_sz;

	callbk_fns.scan_start_callbk_fn = nrf_wifi_event_proc_scan_start_zep;
	callbk_fns.scan_done_callbk_fn = nrf_wifi_event_proc_scan_done_zep;
	callbk_fns.disp_scan_res_callbk_fn = nrf_wifi_event_proc_disp_scan_res_zep;
#ifdef CONFIG_WIFI_MGMT_RAW_SCAN_RESULTS
	callbk_fns.rx_bcn_prb_resp_callbk_fn = nrf_wifi_rx_bcn_prb_resp_frm;
#endif /* CONFIG_WIFI_MGMT_RAW_SCAN_RESULTS */
#ifdef CONFIG_NRF700X_STA_MODE
	callbk_fns.twt_config_callbk_fn = nrf_wifi_event_proc_twt_setup_zep;
	callbk_fns.twt_teardown_callbk_fn = nrf_wifi_event_proc_twt_teardown_zep;
	callbk_fns.twt_sleep_callbk_fn = nrf_wifi_event_proc_twt_sleep_zep;
	callbk_fns.event_get_reg = nrf_wifi_event_get_reg_zep;
	callbk_fns.event_get_ps_info = nrf_wifi_event_proc_get_power_save_info;
	callbk_fns.cookie_rsp_callbk_fn = nrf_wifi_event_proc_cookie_rsp;
	callbk_fns.process_rssi_from_rx = nrf_wifi_process_rssi_from_rx;
	callbk_fns.scan_res_callbk_fn = nrf_wifi_wpa_supp_event_proc_scan_res;
	callbk_fns.auth_resp_callbk_fn = nrf_wifi_wpa_supp_event_proc_auth_resp;
	callbk_fns.assoc_resp_callbk_fn = nrf_wifi_wpa_supp_event_proc_assoc_resp;
	callbk_fns.deauth_callbk_fn = nrf_wifi_wpa_supp_event_proc_deauth;
	callbk_fns.disassoc_callbk_fn = nrf_wifi_wpa_supp_event_proc_disassoc;
	callbk_fns.get_station_callbk_fn = nrf_wifi_wpa_supp_event_proc_get_sta;
	callbk_fns.get_interface_callbk_fn = nrf_wifi_wpa_supp_event_proc_get_if;
	callbk_fns.mgmt_tx_status = nrf_wifi_wpa_supp_event_mgmt_tx_status;
	callbk_fns.unprot_mlme_mgmt_rx_callbk_fn = nrf_wifi_wpa_supp_event_proc_unprot_mgmt;
	callbk_fns.event_get_wiphy = nrf_wifi_wpa_supp_event_get_wiphy;
	callbk_fns.mgmt_rx_callbk_fn = nrf_wifi_wpa_supp_event_mgmt_rx_callbk_fn;
	callbk_fns.get_conn_info_callbk_fn = nrf_wifi_supp_event_proc_get_conn_info;
#endif /* CONFIG_NRF700X_STA_MODE */

	rpu_drv_priv_zep.fmac_priv = nrf_wifi_fmac_init(&data_config,
							rx_buf_pools,
							&callbk_fns);
#else /* !CONFIG_NRF700X_RADIO_TEST */
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;

	rpu_drv_priv_zep.fmac_priv = nrf_wifi_fmac_init_rt();
#endif /* CONFIG_NRF700X_RADIO_TEST */

	if (rpu_drv_priv_zep.fmac_priv == NULL) {
		LOG_ERR("%s: nrf_wifi_fmac_init failed\n",
			__func__);
		goto err;
	}

#ifdef CONFIG_NRF700X_DATA_TX
	struct nrf_wifi_fmac_priv_def *def_priv = NULL;

	def_priv = wifi_fmac_priv(rpu_drv_priv_zep.fmac_priv);
	def_priv->max_ampdu_len_per_token =
		(RPU_PKTRAM_SIZE - (CONFIG_NRF700X_RX_NUM_BUFS * CONFIG_NRF700X_RX_MAX_DATA_SIZE)) /
		CONFIG_NRF700X_MAX_TX_TOKENS;
	/* Align to 4-byte */
	def_priv->max_ampdu_len_per_token &= ~0x3;

	/* Alignment overhead for size based coalesce */
	def_priv->avail_ampdu_len_per_token =
	def_priv->max_ampdu_len_per_token -
		(MAX_PKT_RAM_TX_ALIGN_OVERHEAD * max_tx_aggregation);
#endif /* CONFIG_NRF700X_DATA_TX */

#ifdef CONFIG_NRF700X_RADIO_TEST
	status = nrf_wifi_fmac_dev_add_zep(&rpu_drv_priv_zep);
	if (status != NRF_WIFI_STATUS_SUCCESS) {
		LOG_ERR("%s: nrf_wifi_fmac_dev_add_zep failed\n", __func__);
		goto fmac_deinit;
	}
#else
	k_work_init_delayable(&vif_ctx_zep->scan_timeout_work,
			      nrf_wifi_scan_timeout_work);
#endif /* CONFIG_NRF700X_RADIO_TEST */

	return 0;
#ifdef CONFIG_NRF700X_RADIO_TEST
fmac_deinit:
	nrf_wifi_fmac_deinit_rt(rpu_drv_priv_zep.fmac_priv);
#endif /* CONFIG_NRF700X_RADIO_TEST */
err:
	return -1;
}

#ifndef CONFIG_NRF700X_RADIO_TEST

static struct wifi_mgmt_ops nrf_wifi_mgmt_ops = {
	.scan = nrf_wifi_disp_scan_zep,
#ifdef CONFIG_NET_STATISTICS_WIFI
	.get_stats = nrf_wifi_stats_get,
#endif /* CONFIG_NET_STATISTICS_WIFI */
#ifdef CONFIG_NRF700X_STA_MODE
	.set_power_save = nrf_wifi_set_power_save,
	.set_twt = nrf_wifi_set_twt,
	.reg_domain = nrf_wifi_reg_domain,
	.get_power_save_config = nrf_wifi_get_power_save_config,
#endif /* CONFIG_NRF700X_STA_MODE */
};


static const struct net_wifi_mgmt_offload wifi_offload_ops = {
	.wifi_iface.iface_api.init = nrf_wifi_if_init_zep,
	.wifi_iface.start = nrf_wifi_if_start_zep,
	.wifi_iface.stop = nrf_wifi_if_stop_zep,
	.wifi_iface.set_config = nrf_wifi_if_set_config_zep,
	.wifi_iface.get_capabilities = nrf_wifi_if_caps_get,
	.wifi_iface.send = nrf_wifi_if_send,
#ifdef CONFIG_NET_STATISTICS_ETHERNET
	.wifi_iface.get_stats = nrf_wifi_eth_stats_get,
#endif /* CONFIG_NET_STATISTICS_ETHERNET */
	.wifi_mgmt_api = &nrf_wifi_mgmt_ops,
};

#ifdef CONFIG_NRF700X_STA_MODE
static const struct zep_wpa_supp_dev_ops wpa_supp_ops = {
	.init = nrf_wifi_wpa_supp_dev_init,
	.deinit = nrf_wifi_wpa_supp_dev_deinit,
	.scan2 = nrf_wifi_wpa_supp_scan2,
	.scan_abort = nrf_wifi_wpa_supp_scan_abort,
	.get_scan_results2 = nrf_wifi_wpa_supp_scan_results_get,
	.deauthenticate = nrf_wifi_wpa_supp_deauthenticate,
	.authenticate = nrf_wifi_wpa_supp_authenticate,
	.associate = nrf_wifi_wpa_supp_associate,
	.set_supp_port = nrf_wifi_wpa_set_supp_port,
	.set_key = nrf_wifi_wpa_supp_set_key,
	.signal_poll = nrf_wifi_wpa_supp_signal_poll,
	.send_mlme = nrf_wifi_nl80211_send_mlme,
	.get_wiphy = nrf_wifi_supp_get_wiphy,
	.register_frame = nrf_wifi_supp_register_frame,
	.get_capa = nrf_wifi_supp_get_capa,
	.get_conn_info = nrf_wifi_supp_get_conn_info,
};
#endif /* CONFIG_NRF700X_STA_MODE */
#endif /* !CONFIG_NRF700X_RADIO_TEST */


#ifdef CONFIG_NET_L2_ETHERNET
ETH_NET_DEVICE_INIT(wlan0, /* name - token */
		    "wlan0", /* driver name - dev->name */
		    nrf_wifi_drv_main_zep, /* init_fn */
		    NULL, /* pm_action_cb */
		    &rpu_drv_priv_zep.rpu_ctx_zep.vif_ctx_zep[0], /* data */
#ifdef CONFIG_NRF700X_STA_MODE
		    &wpa_supp_ops, /* cfg */
#else /* CONFIG_NRF700X_STA_MODE */
		    NULL, /* cfg */
#endif /* !CONFIG_NRF700X_STA_MODE */
		    CONFIG_WIFI_INIT_PRIORITY, /* prio */
		    &wifi_offload_ops, /* api */
		    1500); /*mtu */
#else
DEVICE_DEFINE(wlan0, /* name - token */
	      "wlan0", /* driver name - dev->name */
	      nrf_wifi_drv_main_zep, /* init_fn */
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
#endif /* CONFIG_NRF700X_STA_MODE */

#ifdef CONFIG_L2_WIFI_CONNECTIVITY
CONN_MGR_BIND_CONN(wlan0, L2_CONN_WLAN0);
#endif /* CONFIG_L2_WIFI_CONNECTIVITY */
