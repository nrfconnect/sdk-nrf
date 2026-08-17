/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief File containing API definitions for the Offloaded raw TX feature.
 */

#include <common/mem_mgmt.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/wifi/nrf_wifi/off_raw_tx/off_raw_tx_api.h>
#include <offload_raw_tx/fmac_api.h>
#include <nrf71_wifi_ctrl.h>
#include <nrf71_wifi_rf.h>
#include <util.h>
#include <offload_raw_tx/api.h>

#define DT_DRV_COMPAT nordic_wlan
LOG_MODULE_DECLARE(wifi_nrf, CONFIG_WIFI_NRF71_LOG_LEVEL);

struct nrf_wifi_off_raw_tx_drv_priv off_raw_tx_drv_priv;

static const int valid_data_rates[] = { 1, 2, 55, 11, 6, 9, 12, 18, 24, 36, 48, 54,
				  0, 1, 2, 3, 4, 5, 6, 7, -1 };

static void configure_tx_pwr_settings(struct nrf_wifi_tx_pwr_ctrl_params *ctrl_params)
{
	ctrl_params->ant_gain_2g = NRF71_ANT_GAIN_2G;
	ctrl_params->ant_gain_5g_band1 = NRF71_ANT_GAIN_5G_BAND1;
	ctrl_params->ant_gain_5g_band2 = NRF71_ANT_GAIN_5G_BAND2;
	ctrl_params->ant_gain_5g_band3 = NRF71_ANT_GAIN_5G_BAND3;
}

static void configure_board_dep_params(struct nrf_wifi_board_params *board_params)
{
	board_params->pcb_loss_2g = NRF71_PCB_LOSS_2G;
#ifndef CONFIG_NRF_WIFI_2G_BAND
	board_params->pcb_loss_5g_band1 = NRF71_PCB_LOSS_5G_BAND1;
	board_params->pcb_loss_5g_band2 = NRF71_PCB_LOSS_5G_BAND2;
	board_params->pcb_loss_5g_band3 = NRF71_PCB_LOSS_5G_BAND3;
#endif /* CONFIG_NRF_WIFI_2G_BAND */
}

#ifdef CONFIG_WIFI_FIXED_MAC_ADDRESS_ENABLED
static int bytes_from_str(uint8_t *buf, int buf_len, const char *src)
{
	size_t i;
	size_t src_len = strlen(src);
	char *endptr;

	for (i = 0U; i < src_len; i++) {
		if (!isxdigit((unsigned char)src[i]) &&
		    src[i] != ':') {
			return -EINVAL;
		}
	}

	(void)memset(buf, 0, buf_len);

	for (i = 0U; i < (size_t)buf_len; i++) {
		buf[i] = (uint8_t)strtol(src, &endptr, 16);
		src = ++endptr;
	}

	return 0;
}
#endif /* CONFIG_WIFI_FIXED_MAC_ADDRESS_ENABLED */


int nrf_wifi_off_raw_tx_init(uint8_t *mac_addr, unsigned char *country_code)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_off_raw_tx_drv_ctx *drv_ctx = NULL;
	void *rpu_ctx = NULL;
	k_spinlock_key_t key;
	unsigned char op_band = nrf_wifi_utils_get_op_band();
	struct nrf_wifi_tx_pwr_ctrl_params tx_pwr_ctrl_params;
	struct nrf_wifi_tx_pwr_ceil_params tx_pwr_ceil_params;
	struct nrf_wifi_board_params board_params;
	unsigned int fw_ver = 0;

	key = k_spin_lock(&off_raw_tx_drv_priv.lock);

	off_raw_tx_drv_priv.fmac_priv = nrf_wifi_off_raw_tx_fmac_init();

	if (off_raw_tx_drv_priv.fmac_priv == NULL) {
		LOG_ERR("%s: Failed to initialize nRF71 driver",
			__func__);
		goto err;
	}

	drv_ctx = &off_raw_tx_drv_priv.drv_ctx;

	drv_ctx->drv_priv = &off_raw_tx_drv_priv;

	rpu_ctx = nrf_wifi_off_raw_tx_fmac_dev_add(off_raw_tx_drv_priv.fmac_priv,
						   drv_ctx);
	if (!rpu_ctx) {
		LOG_ERR("%s: Failed to add device", __func__);
		drv_ctx = NULL;
		goto err;
	}

	drv_ctx->rpu_ctx = rpu_ctx;

	status = nrf_wifi_fmac_ver_get(rpu_ctx,
				       &fw_ver);
	if (status != NRF_WIFI_STATUS_SUCCESS) {
		LOG_ERR("%s: Failed to read the firmware version", __func__);
		goto err;
	}

	LOG_DBG("Firmware (v%d.%d.%d.%d) booted successfully",
		NRF_WIFI_UMAC_VER(fw_ver),
		NRF_WIFI_UMAC_VER_MAJ(fw_ver),
		NRF_WIFI_UMAC_VER_MIN(fw_ver),
		NRF_WIFI_UMAC_VER_EXTRA(fw_ver));

	memset(drv_ctx->phy_rf_params_addr, 0, sizeof(drv_ctx->phy_rf_params_addr));
	status = nrf_wifi_fmac_config_rf_params(drv_ctx->rpu_ctx, drv_ctx->phy_rf_params_addr);
	if (status != NRF_WIFI_STATUS_SUCCESS) {
		LOG_ERR("%s: Failed to configure RF params", __func__);
		goto err;
	}

	/* TODO: Remove hardcodes once we hook in sensor readings */
	status = nrf_wifi_fmac_config_vtf_params(drv_ctx->rpu_ctx,
						 243,
						 25,
						 0,
						 &drv_ctx->vtf_buffer_start_address);
	if (status != NRF_WIFI_STATUS_SUCCESS) {
		LOG_ERR("%s: Failed to configure VTF params", __func__);
		goto err;
	}

	memset(&tx_pwr_ctrl_params, 0, sizeof(tx_pwr_ctrl_params));
	memset(&tx_pwr_ceil_params, 0, sizeof(tx_pwr_ceil_params));
	memset(&board_params, 0, sizeof(board_params));

	configure_tx_pwr_settings(&tx_pwr_ctrl_params);
	configure_board_dep_params(&board_params);

	status = nrf_wifi_off_raw_tx_fmac_dev_init(drv_ctx->rpu_ctx,
#ifdef CONFIG_NRF_WIFI_LOW_POWER
						   HW_SLEEP_ENABLE,
#endif /* CONFIG_NRF_WIFI_LOW_POWER */
						   NRF_WIFI_DEF_PHY_CALIB,
						   op_band,
						   IS_ENABLED(CONFIG_NRF_WIFI_BEAMFORMING),
						   &tx_pwr_ctrl_params,
						   &tx_pwr_ceil_params,
						   &board_params,
						   country_code,
						   drv_ctx->phy_rf_params_addr,
						   drv_ctx->vtf_buffer_start_address);
	if (status != NRF_WIFI_STATUS_SUCCESS) {
		LOG_ERR("%s: Firmware initialization failed", __func__);
		goto err;
	}

	if (mac_addr) {
		memcpy(drv_ctx->mac_addr, mac_addr, 6);
	} else {
#ifdef CONFIG_WIFI_FIXED_MAC_ADDRESS_ENABLED
		int ret = -1;

		ret = bytes_from_str(drv_ctx->mac_addr,
				     6,
				     CONFIG_WIFI_FIXED_MAC_ADDRESS);
		if (ret < 0) {
			LOG_ERR("%s: Failed to parse MAC address: %s",
				__func__,
				CONFIG_WIFI_FIXED_MAC_ADDRESS);
			goto err;
		}
#elif CONFIG_WIFI_OTP_MAC_ADDRESS
	/* Set dummy MAC address */
	drv_ctx->mac_addr[0] = 0x00;
	drv_ctx->mac_addr[1] = 0x00;
	drv_ctx->mac_addr[2] = 0x5E;
	drv_ctx->mac_addr[3] = 0x00;
	drv_ctx->mac_addr[4] = 0x10;
	drv_ctx->mac_addr[5] = 0x00;
#endif /* CONFIG_WIFI_FIXED_MAC_ADDRESS_ENABLED */

		if (!nrf_wifi_utils_is_mac_addr_valid(drv_ctx->mac_addr)) {
			LOG_ERR("%s: Invalid MAC address: %02X:%02X:%02X:%02X:%02X:%02X",
				__func__,
				drv_ctx->mac_addr[0],
				drv_ctx->mac_addr[1],
				drv_ctx->mac_addr[2],
				drv_ctx->mac_addr[3],
				drv_ctx->mac_addr[4],
				drv_ctx->mac_addr[5]);
			goto err;
		}
	}

	k_spin_unlock(&off_raw_tx_drv_priv.lock, key);

	return 0;
err:
	if (drv_ctx->rpu_ctx) {
		nrf_wifi_fmac_dev_rem(drv_ctx->rpu_ctx);
		drv_ctx->rpu_ctx = NULL;
	}

	k_spin_unlock(&off_raw_tx_drv_priv.lock, key);
	nrf_wifi_off_raw_tx_deinit();
	return -1;
}


void nrf_wifi_off_raw_tx_deinit(void)
{
	k_spinlock_key_t key;
	struct nrf_wifi_off_raw_tx_drv_ctx *drv_ctx = &off_raw_tx_drv_priv.drv_ctx;
	int i;

	key = k_spin_lock(&off_raw_tx_drv_priv.lock);

	if (!off_raw_tx_drv_priv.fmac_priv) {
		k_spin_unlock(&off_raw_tx_drv_priv.lock, key);
		return;
	}

	nrf_wifi_fmac_deinit(off_raw_tx_drv_priv.fmac_priv);

	for (i = 0; i < NUM_RF_PARAM_ADDRS; i++) {
		if (drv_ctx->phy_rf_params_addr[i]) {
			nrf_wifi_mem_free(NRF_WIFI_MEM_POOL_TYPE_CTRL,
					   (void *)drv_ctx->phy_rf_params_addr[i]);
			drv_ctx->phy_rf_params_addr[i] = 0;
		}
	}
	if (drv_ctx->vtf_buffer_start_address) {
		nrf_wifi_mem_free(NRF_WIFI_MEM_POOL_TYPE_CTRL,
				  (void *)drv_ctx->vtf_buffer_start_address);
		drv_ctx->vtf_buffer_start_address = 0;
	}

	k_spin_unlock(&off_raw_tx_drv_priv.lock, key);
}

static unsigned char off_raw_tx_op_band(const struct nrf_wifi_off_raw_tx_conf *conf)
{
	switch (conf->band) {
	case NRF_WIFI_OFF_RAW_TX_BAND_2GHZ:
		return NRF_WIFI_OP_BAND_2GHZ;
	case NRF_WIFI_OFF_RAW_TX_BAND_5GHZ:
		return NRF_WIFI_OP_BAND_5GHZ;
	case NRF_WIFI_OFF_RAW_TX_BAND_6GHZ:
		return NRF_WIFI_OP_BAND_6GHZ;
	case NRF_WIFI_OFF_RAW_TX_BAND_AUTO:
	default:
		if (conf->chan >= 1 && conf->chan <= 14) {
			return NRF_WIFI_OP_BAND_2GHZ;
		}
		return NRF_WIFI_OP_BAND_5GHZ;
	}
}

static bool validate_rate(enum nrf_wifi_off_raw_tx_tput_mode tput_mode,
			enum nrf_wifi_off_raw_tx_rate rate)
{
	if (tput_mode == TPUT_MODE_LEGACY) {
		if (rate > RATE_54M) {
			return false;
		}
	} else {
		if (rate <= RATE_54M) {
			return false;
		}
	}

	return true;
}

static bool validate_chan_band(enum nrf_wifi_off_raw_tx_band band, unsigned int chan)
{
	switch (band) {
	case NRF_WIFI_OFF_RAW_TX_BAND_2GHZ:
		if (chan < 1 || chan > 14) {
			LOG_ERR("%s: Channel %u is not a valid 2.4 GHz channel",
				__func__, chan);
			return false;
		}
		break;
	case NRF_WIFI_OFF_RAW_TX_BAND_5GHZ:
		if (chan < 36 || chan > 165) {
			LOG_ERR("%s: Channel %u is not a valid 5 GHz channel",
				__func__, chan);
			return false;
		}
		break;
	case NRF_WIFI_OFF_RAW_TX_BAND_6GHZ:
		/* 6 GHz channel numbers overlap with lower bands; any channel
		 * is valid once the band is explicitly selected.
		 */
		break;
	case NRF_WIFI_OFF_RAW_TX_BAND_AUTO:
	default:
		if ((chan >= 1 && chan <= 14) || (chan >= 36 && chan <= 165)) {
			break;
		}
		LOG_ERR("%s: Channel %u needs band (e.g. NRF_WIFI_OFF_RAW_TX_BAND_6GHZ)",
			__func__, chan);
		return false;
	}

	return true;
}

int nrf_wifi_off_raw_tx_conf_update(struct nrf_wifi_off_raw_tx_conf *conf)
{
	int ret = -1;
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_off_raw_tx_drv_ctx *drv_ctx = &off_raw_tx_drv_priv.drv_ctx;
	struct nrf_wifi_offload_ctrl_params *off_ctrl_params = NULL;
	struct nrf_wifi_offload_tx_ctrl *off_tx_params = NULL;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;
	k_spinlock_key_t key;
	bool locked = false;

	if (!conf) {
		LOG_ERR("%s: Config params is NULL", __func__);
		return -1;
	}

	off_ctrl_params = nrf_wifi_mem_zalloc(NRF_WIFI_MEM_POOL_TYPE_CTRL,
					      sizeof(*off_ctrl_params));
	if (!off_ctrl_params) {
		LOG_ERR("%s: Failed to allocate memory for off_ctrl_params", __func__);
		return -1;
	}

	key = k_spin_lock(&off_raw_tx_drv_priv.lock);
	locked = true;

	fmac_dev_ctx = drv_ctx->rpu_ctx;

	if (!fmac_dev_ctx) {
		LOG_ERR("%s: FMAC device context is NULL", __func__);
		goto out;
	}

	off_tx_params = nrf_wifi_mem_zalloc(NRF_WIFI_MEM_POOL_TYPE_CTRL, sizeof(*off_tx_params));
	if (!off_tx_params) {
		LOG_ERR("%s Failed to allocate memory for off_tx_params: ", __func__);
		goto out;
	}

	if (!validate_rate(conf->tput_mode, conf->rate)) {
		LOG_ERR("%s Invalid rate. Throughput mode: %d, rate: %d\n", __func__,
				      conf->tput_mode, conf->rate);
		goto out;
	}

	if (!validate_chan_band(conf->band, conf->chan)) {
		LOG_ERR("%s: Invalid channel or band", __func__);
		goto out;
	}

	off_ctrl_params->chan.primary_num = conf->chan;
	off_ctrl_params->chan.op_band = off_raw_tx_op_band(conf);
	off_ctrl_params->chan.bw = RPU_CH_BW_20;
	off_ctrl_params->chan.sec_20_offset = 0;
	off_ctrl_params->chan.sec_40_offset = 0;
	off_ctrl_params->period_in_us = conf->period_us;
	off_ctrl_params->tx_pwr = conf->tx_pwr;
	off_tx_params->he_gi_type = conf->he_gi;
	off_tx_params->he_ltf = conf->he_ltf;

	if (!conf->pkt || conf->pkt_len < NRF_WIFI_OFF_RAW_TX_FRAME_SIZE_MIN ||
	    conf->pkt_len > NRF_WIFI_OFF_RAW_TX_FRAME_SIZE_MAX) {
		LOG_ERR("%s: Invalid packet length %u", __func__, conf->pkt_len);
		goto out;
	}

	off_tx_params->pkt_ram_ptr = (unsigned int)(uintptr_t)conf->pkt;
	off_tx_params->pkt_length = conf->pkt_len;
	off_tx_params->rate_flags = conf->tput_mode;
	off_tx_params->rate = valid_data_rates[conf->rate];
	off_tx_params->rate_preamble_type = conf->short_preamble;
	off_tx_params->rate_retries = conf->num_retries;

	status = nrf_wifi_off_raw_tx_fmac_conf(fmac_dev_ctx,
					       off_ctrl_params,
					       off_tx_params);
	if (status != NRF_WIFI_STATUS_SUCCESS) {
		LOG_ERR("%s: nRF71 offloaded raw TX configuration failed",
				      __func__);
		goto out;
	}

	ret = 0;
out:
	nrf_wifi_mem_free(NRF_WIFI_MEM_POOL_TYPE_CTRL, off_ctrl_params);
	nrf_wifi_mem_free(NRF_WIFI_MEM_POOL_TYPE_CTRL, off_tx_params);
	if (locked) {
		k_spin_unlock(&off_raw_tx_drv_priv.lock, key);
	}
	return ret;
}


int nrf_wifi_off_raw_tx_start(struct nrf_wifi_off_raw_tx_conf *conf)
{
	int ret = -1;
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_off_raw_tx_drv_ctx *drv_ctx = &off_raw_tx_drv_priv.drv_ctx;
	k_spinlock_key_t key;

	ret = nrf_wifi_off_raw_tx_conf_update(conf);
	if (ret != 0) {
		LOG_ERR("%s: nRF71 offloaded raw TX configuration failed",
				      __func__);
		goto out;
	}

	key = k_spin_lock(&off_raw_tx_drv_priv.lock);
	if (!drv_ctx->rpu_ctx) {
		LOG_ERR("%s: FMAC device context is NULL", __func__);
		goto out;
	}

	status = nrf_wifi_off_raw_tx_fmac_start(drv_ctx->rpu_ctx);
	if (status != NRF_WIFI_STATUS_SUCCESS) {
		LOG_ERR("%s: nRF71 offloaded raw TX start failed",
				      __func__);
		goto out;
	}

	ret = 0;
out:
	k_spin_unlock(&off_raw_tx_drv_priv.lock, key);
	return ret;
}


int nrf_wifi_off_raw_tx_stop(void)
{
	int ret = -1;
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_off_raw_tx_drv_ctx *drv_ctx = &off_raw_tx_drv_priv.drv_ctx;
	k_spinlock_key_t key;

	key = k_spin_lock(&off_raw_tx_drv_priv.lock);

	if (!drv_ctx->rpu_ctx) {
		LOG_ERR("%s: FMAC device context is NULL", __func__);
		goto out;
	}

	status = nrf_wifi_off_raw_tx_fmac_stop(drv_ctx->rpu_ctx);
	if (status != NRF_WIFI_STATUS_SUCCESS) {
		LOG_ERR("%s: nRF71 offloaded raw TX stop failed",
				      __func__);
		goto out;
	}

	ret = 0;
out:
	k_spin_unlock(&off_raw_tx_drv_priv.lock, key);
	return ret;
}


int nrf_wifi_off_raw_tx_mac_addr_get(uint8_t *mac_addr)
{
	struct nrf_wifi_off_raw_tx_drv_ctx *drv_ctx = &off_raw_tx_drv_priv.drv_ctx;

	if (!mac_addr) {
		LOG_ERR("%s: Invalid param", __func__);
		return -EINVAL;
	}

	memcpy(mac_addr, drv_ctx->mac_addr, 6);
	return 0;
}

int nrf_wifi_off_raw_tx_stats(struct nrf_wifi_off_raw_tx_stats *off_raw_tx_stats)
{
	int ret = -1;
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_off_raw_tx_drv_ctx *drv_ctx = &off_raw_tx_drv_priv.drv_ctx;
	struct rpu_off_raw_tx_op_stats stats;
	k_spinlock_key_t key;

	memset(&stats, 0, sizeof(stats));

	key = k_spin_lock(&off_raw_tx_drv_priv.lock);

	if (!drv_ctx->rpu_ctx) {
		LOG_ERR("%s: FMAC device context is NULL", __func__);
		goto out;
	}

	status = nrf_wifi_off_raw_tx_fmac_stats_get(drv_ctx->rpu_ctx,
						    0,
						    &stats);
	if (status != NRF_WIFI_STATUS_SUCCESS) {
		LOG_ERR("%s: nRF71 offloaded raw TX stats failed",
				      __func__);
		goto out;
	}

	off_raw_tx_stats->off_raw_tx_pkt_sent = stats.fw.offload_raw_tx_cnt;

	ret = 0;
out:
	k_spin_unlock(&off_raw_tx_drv_priv.lock, key);
	return ret;
}
