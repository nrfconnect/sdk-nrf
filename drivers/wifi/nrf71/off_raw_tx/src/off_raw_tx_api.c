/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief File containing API definitions for the Offloaded raw TX feature.
 */

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/wifi/nrf_wifi/off_raw_tx/off_raw_tx_api.h>
#include <off_raw_tx.h>
#include <offload_raw_tx/fmac_api.h>
#include <nrf71_wifi_ctrl.h>
#include <nrf71_wifi_rf.h>
#include <util.h>

#define DT_DRV_COMPAT nordic_wlan
LOG_MODULE_DECLARE(wifi_nrf, CONFIG_WIFI_NRF71_LOG_LEVEL);

struct rf_hex_param_off {
	const char *hex_str;
	uint8_t *bytes;
	int bytes_len;
};

static struct rf_hex_param_off rf_params_off[NUM_WIFI_PARAMS] = {
	{NRF_WIFI_PARAMS1, NULL, 0},  {NRF_WIFI_PARAMS2, NULL, 0},  {NRF_WIFI_PARAMS3, NULL, 0},
	{NRF_WIFI_PARAMS4, NULL, 0},  {NRF_WIFI_PARAMS5, NULL, 0},  {NRF_WIFI_PARAMS6, NULL, 0},
	{NRF_WIFI_PARAMS7, NULL, 0},  {NRF_WIFI_PARAMS8, NULL, 0},  {NRF_WIFI_PARAMS9, NULL, 0},
	{NRF_WIFI_PARAMS10, NULL, 0}, {NRF_WIFI_PARAMS11, NULL, 0}, {NRF_WIFI_PARAMS12, NULL, 0},
	{NRF_WIFI_PARAMS13, NULL, 0}, {NRF_WIFI_PARAMS14, NULL, 0}, {NRF_WIFI_PARAMS15, NULL, 0},
	{NRF_WIFI_PARAMS16, NULL, 0},
};

static enum nrf_wifi_status off_raw_tx_config_rf_params(void *dev_ctx, unsigned int *rf_params_addr)
{
	int index;
	int cleanup_idx;
	size_t str_len;
	int ret;

	(void)dev_ctx;
	for (index = 0; index < NUM_WIFI_PARAMS; index++) {
		if (!rf_params_off[index].hex_str) {
			continue;
		}
		str_len = strlen(rf_params_off[index].hex_str);
		rf_params_off[index].bytes = k_malloc(str_len);
		if (!rf_params_off[index].bytes) {
			LOG_ERR("%s: Unable to allocate %zu bytes", __func__, str_len);
			goto cleanup;
		}
		ret = nrf_wifi_utils_hex_str_to_val(rf_params_off[index].bytes,
						    (unsigned int)str_len,
						    (unsigned char *)rf_params_off[index].hex_str);
		if (ret < 0) {
			LOG_ERR("%s: hex_str_to_val failed", __func__);
			k_free(rf_params_off[index].bytes);
			rf_params_off[index].bytes = NULL;
			goto cleanup;
		}
		rf_params_off[index].bytes_len = ret;
		rf_params_addr[index] = (unsigned int)rf_params_off[index].bytes;
	}
	return NRF_WIFI_STATUS_SUCCESS;
cleanup:
	for (cleanup_idx = 0; cleanup_idx < index; cleanup_idx++) {
		if (rf_params_off[cleanup_idx].bytes) {
			k_free(rf_params_off[cleanup_idx].bytes);
			rf_params_off[cleanup_idx].bytes = NULL;
		}
	}
	return NRF_WIFI_STATUS_FAIL;
}

struct vtf_params_host_off {
	unsigned int voltage;
	unsigned int temp;
	unsigned int x0_freq;
};

static enum nrf_wifi_status off_raw_tx_config_vtf_params(void *dev_ctx, unsigned int voltage,
							 unsigned int temp, unsigned int x0,
							 unsigned int *vtf_buffer_start_address)
{
	struct vtf_params_host_off *vtf_buf;

	(void)dev_ctx;
	if (!vtf_buffer_start_address) {
		return NRF_WIFI_STATUS_FAIL;
	}
	vtf_buf = k_malloc(sizeof(*vtf_buf));
	if (!vtf_buf) {
		LOG_ERR("%s: k_malloc failed for VTF params", __func__);
		return NRF_WIFI_STATUS_FAIL;
	}
	vtf_buf->voltage = voltage;
	vtf_buf->temp = temp;
	vtf_buf->x0_freq = x0;
	*vtf_buffer_start_address = (unsigned int)vtf_buf;
	return NRF_WIFI_STATUS_SUCCESS;
}

struct nrf_wifi_off_raw_tx_drv_priv off_raw_tx_drv_priv;
extern const struct nrf_wifi_osal_ops nrf_wifi_os_zep_ops;

static const int valid_data_rates[] = { 1, 2, 55, 11, 6, 9, 12, 18, 24, 36, 48, 54,
				  0, 1, 2, 3, 4, 5, 6, 7, -1 };

/* DTS uses 1dBm as the unit for TX power, while the RPU uses 0.25dBm */
#define MAX_TX_PWR(label) DT_PROP(DT_NODELABEL(nrf71), label) * 4
static void configure_tx_pwr_settings(struct nrf_wifi_tx_pwr_ctrl_params *ctrl_params,
				      struct nrf_wifi_tx_pwr_ceil_params *ceil_params)
{
	ctrl_params->ant_gain_2g = CONFIG_NRF71_ANT_GAIN_2G;
	ctrl_params->ant_gain_5g_band1 = CONFIG_NRF71_ANT_GAIN_5G_BAND1;
	ctrl_params->ant_gain_5g_band2 = CONFIG_NRF71_ANT_GAIN_5G_BAND2;
	ctrl_params->ant_gain_5g_band3 = CONFIG_NRF71_ANT_GAIN_5G_BAND3;
	ctrl_params->band_edge_2g_lo_dss = CONFIG_NRF71_BAND_2G_LOWER_EDGE_BACKOFF_DSSS;
	ctrl_params->band_edge_2g_lo_ht = CONFIG_NRF71_BAND_2G_LOWER_EDGE_BACKOFF_HT;
	ctrl_params->band_edge_2g_lo_he = CONFIG_NRF71_BAND_2G_LOWER_EDGE_BACKOFF_HE;
	ctrl_params->band_edge_2g_hi_dsss = CONFIG_NRF71_BAND_2G_UPPER_EDGE_BACKOFF_DSSS;
	ctrl_params->band_edge_2g_hi_ht = CONFIG_NRF71_BAND_2G_UPPER_EDGE_BACKOFF_HT;
	ctrl_params->band_edge_2g_hi_he = CONFIG_NRF71_BAND_2G_UPPER_EDGE_BACKOFF_HE;
	ctrl_params->band_edge_5g_unii_1_lo_ht =
		CONFIG_NRF71_BAND_UNII_1_LOWER_EDGE_BACKOFF_HT;
	ctrl_params->band_edge_5g_unii_1_lo_he =
		CONFIG_NRF71_BAND_UNII_1_LOWER_EDGE_BACKOFF_HE;
	ctrl_params->band_edge_5g_unii_1_hi_ht =
		CONFIG_NRF71_BAND_UNII_1_UPPER_EDGE_BACKOFF_HT;
	ctrl_params->band_edge_5g_unii_1_hi_he =
		CONFIG_NRF71_BAND_UNII_1_UPPER_EDGE_BACKOFF_HE;
	ctrl_params->band_edge_5g_unii_2a_lo_ht =
		CONFIG_NRF71_BAND_UNII_2A_LOWER_EDGE_BACKOFF_HT;
	ctrl_params->band_edge_5g_unii_2a_lo_he =
		CONFIG_NRF71_BAND_UNII_2A_LOWER_EDGE_BACKOFF_HE;
	ctrl_params->band_edge_5g_unii_2a_hi_ht =
		CONFIG_NRF71_BAND_UNII_2A_UPPER_EDGE_BACKOFF_HT;
	ctrl_params->band_edge_5g_unii_2a_hi_he =
		CONFIG_NRF71_BAND_UNII_2A_UPPER_EDGE_BACKOFF_HE;
	ctrl_params->band_edge_5g_unii_2c_lo_ht =
		CONFIG_NRF71_BAND_UNII_2C_LOWER_EDGE_BACKOFF_HT;
	ctrl_params->band_edge_5g_unii_2c_lo_he =
		CONFIG_NRF71_BAND_UNII_2C_LOWER_EDGE_BACKOFF_HE;
	ctrl_params->band_edge_5g_unii_2c_hi_ht =
		CONFIG_NRF71_BAND_UNII_2C_UPPER_EDGE_BACKOFF_HT;
	ctrl_params->band_edge_5g_unii_2c_hi_he =
		CONFIG_NRF71_BAND_UNII_2C_UPPER_EDGE_BACKOFF_HE;
	ctrl_params->band_edge_5g_unii_3_lo_ht =
		CONFIG_NRF71_BAND_UNII_3_LOWER_EDGE_BACKOFF_HT;
	ctrl_params->band_edge_5g_unii_3_lo_he =
		CONFIG_NRF71_BAND_UNII_3_LOWER_EDGE_BACKOFF_HE;
	ctrl_params->band_edge_5g_unii_3_hi_ht =
		CONFIG_NRF71_BAND_UNII_3_UPPER_EDGE_BACKOFF_HT;
	ctrl_params->band_edge_5g_unii_3_hi_he =
		CONFIG_NRF71_BAND_UNII_3_UPPER_EDGE_BACKOFF_HE;
	ctrl_params->band_edge_5g_unii_4_lo_ht =
		CONFIG_NRF71_BAND_UNII_4_LOWER_EDGE_BACKOFF_HT;
	ctrl_params->band_edge_5g_unii_4_lo_he =
		CONFIG_NRF71_BAND_UNII_4_LOWER_EDGE_BACKOFF_HE;
	ctrl_params->band_edge_5g_unii_4_hi_ht =
		CONFIG_NRF71_BAND_UNII_4_UPPER_EDGE_BACKOFF_HT;
	ctrl_params->band_edge_5g_unii_4_hi_he =
		CONFIG_NRF71_BAND_UNII_4_UPPER_EDGE_BACKOFF_HE;
	ceil_params->max_pwr_2g_dsss = MAX_TX_PWR(wifi_max_tx_pwr_2g_dsss);
	ceil_params->max_pwr_2g_mcs7 = MAX_TX_PWR(wifi_max_tx_pwr_2g_mcs7);
	ceil_params->max_pwr_2g_mcs0 = MAX_TX_PWR(wifi_max_tx_pwr_2g_mcs0);
	ceil_params->max_pwr_5g_low_mcs7 = MAX_TX_PWR(wifi_max_tx_pwr_5g_low_mcs7);
	ceil_params->max_pwr_5g_mid_mcs7 = MAX_TX_PWR(wifi_max_tx_pwr_5g_mid_mcs7);
	ceil_params->max_pwr_5g_high_mcs7 = MAX_TX_PWR(wifi_max_tx_pwr_5g_high_mcs7);
	ceil_params->max_pwr_5g_low_mcs0 = MAX_TX_PWR(wifi_max_tx_pwr_5g_low_mcs0);
	ceil_params->max_pwr_5g_mid_mcs0 = MAX_TX_PWR(wifi_max_tx_pwr_5g_mid_mcs0);
	ceil_params->max_pwr_5g_high_mcs0 = MAX_TX_PWR(wifi_max_tx_pwr_5g_high_mcs0);
}

static void configure_board_dep_params(struct nrf_wifi_board_params *board_params)
{
	board_params->pcb_loss_2g = CONFIG_NRF71_PCB_LOSS_2G;
	board_params->pcb_loss_5g_band1 = CONFIG_NRF71_PCB_LOSS_5G_BAND1;
	board_params->pcb_loss_5g_band2 = CONFIG_NRF71_PCB_LOSS_5G_BAND2;
	board_params->pcb_loss_5g_band3 = CONFIG_NRF71_PCB_LOSS_5G_BAND3;
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

static enum op_band get_nrf_wifi_op_band(void)
{
	if (IS_ENABLED(CONFIG_NRF_WIFI_2G_BAND)) {
		return BAND_24G;
	}
	if (IS_ENABLED(CONFIG_NRF_WIFI_5G_BAND)) {
		return BAND_5G;
	}
	if (IS_ENABLED(CONFIG_NRF_WIFI_6G_BAND)) {
		return BAND_6G;
	}
	if (IS_ENABLED(CONFIG_NRF_WIFI_DUAL_BAND)) {
		return BAND_DUAL;
	}
	return BAND_ALL;
}

int nrf71_off_raw_tx_init(uint8_t *mac_addr, unsigned char *country_code)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_ctx_zep *rpu_ctx_zep = NULL;
	void *rpu_ctx = NULL;
	struct nrf_wifi_tx_pwr_ctrl_params ctrl_params;
	struct nrf_wifi_tx_pwr_ceil_params ceil_params;
	struct nrf_wifi_board_params board_params;
	unsigned int fw_ver = 0;
	k_spinlock_key_t key;
	enum op_band op_band = get_nrf_wifi_op_band();

	/* The OSAL layer needs to be initialized before any other initialization
	 * so that other layers (like FW IF,HW IF etc) have access to OS ops
	 */
	nrf_wifi_osal_init(&nrf_wifi_os_zep_ops);

	key = k_spin_lock(&off_raw_tx_drv_priv.lock);

	off_raw_tx_drv_priv.fmac_priv = nrf_wifi_off_raw_tx_fmac_init();

	if (off_raw_tx_drv_priv.fmac_priv == NULL) {
		LOG_ERR("%s: Failed to initialize nRF70 driver",
			__func__);
		goto err;
	}

	rpu_ctx_zep = &off_raw_tx_drv_priv.rpu_ctx_zep;

	rpu_ctx_zep->drv_priv_zep = &off_raw_tx_drv_priv;

	rpu_ctx = nrf_wifi_off_raw_tx_fmac_dev_add(off_raw_tx_drv_priv.fmac_priv,
						   rpu_ctx_zep);
	if (!rpu_ctx) {
		LOG_ERR("%s: Failed to add nRF70 device", __func__);
		rpu_ctx_zep = NULL;
		goto err;
	}

	rpu_ctx_zep->rpu_ctx = rpu_ctx;

	status = nrf_wifi_fmac_ver_get(rpu_ctx,
				       &fw_ver);
	if (status != NRF_WIFI_STATUS_SUCCESS) {
		LOG_ERR("%s: Failed to read the nRF70 firmware version", __func__);
		goto err;
	}

	LOG_DBG("nRF70 firmware (v%d.%d.%d.%d) booted successfully",
		NRF_WIFI_UMAC_VER(fw_ver),
		NRF_WIFI_UMAC_VER_MAJ(fw_ver),
		NRF_WIFI_UMAC_VER_MIN(fw_ver),
		NRF_WIFI_UMAC_VER_EXTRA(fw_ver));

	memset(&ctrl_params, 0, sizeof(ctrl_params));
	memset(&ceil_params, 0, sizeof(ceil_params));

	configure_tx_pwr_settings(&ctrl_params,
				  &ceil_params);

	memset(&board_params, 0, sizeof(board_params));

	configure_board_dep_params(&board_params);

	memset(rpu_ctx_zep->phy_rf_params_addr, 0, sizeof(rpu_ctx_zep->phy_rf_params_addr));
	status = off_raw_tx_config_rf_params(rpu_ctx_zep->rpu_ctx, rpu_ctx_zep->phy_rf_params_addr);
	if (status != NRF_WIFI_STATUS_SUCCESS) {
		LOG_ERR("%s: Failed to configure RF params", __func__);
		goto err;
	}

	/* TODO: Remove hardcodes once we hook in sensor readings */
	status = off_raw_tx_config_vtf_params(rpu_ctx_zep->rpu_ctx, 243, 25, 0,
					      &rpu_ctx_zep->vtf_buffer_start_address);
	if (status != NRF_WIFI_STATUS_SUCCESS) {
		LOG_ERR("%s: Failed to configure VTF params", __func__);
		goto err;
	}

	status = nrf_wifi_off_raw_tx_fmac_dev_init(
		rpu_ctx_zep->rpu_ctx,
#ifdef CONFIG_NRF_WIFI_LOW_POWER
		HW_SLEEP_ENABLE,
#endif /* CONFIG_NRF_WIFI_LOW_POWER */
		NRF_WIFI_DEF_PHY_CALIB, op_band, IS_ENABLED(CONFIG_NRF_WIFI_BEAMFORMING),
		&ctrl_params, &ceil_params, &board_params, country_code,
		rpu_ctx_zep->phy_rf_params_addr, rpu_ctx_zep->vtf_buffer_start_address);
	if (status != NRF_WIFI_STATUS_SUCCESS) {
		LOG_ERR("%s: nRF70 firmware initialization failed", __func__);
		goto err;
	}

	if (mac_addr) {
		memcpy(rpu_ctx_zep->mac_addr, mac_addr, 6);
	} else {
#ifdef CONFIG_WIFI_FIXED_MAC_ADDRESS_ENABLED
		int ret = -1;

		ret = bytes_from_str(rpu_ctx_zep->mac_addr,
				     6,
				     CONFIG_WIFI_FIXED_MAC_ADDRESS);
		if (ret < 0) {
			LOG_ERR("%s: Failed to parse MAC address: %s",
				__func__,
				CONFIG_WIFI_FIXED_MAC_ADDRESS);
			goto err;
		}
#elif CONFIG_WIFI_OTP_MAC_ADDRESS
		status = nrf_wifi_fmac_otp_mac_addr_get(off_raw_tx_drv_priv.rpu_ctx_zep.rpu_ctx,
							0,
							rpu_ctx_zep->mac_addr);
		if (status != NRF_WIFI_STATUS_SUCCESS) {
			LOG_ERR("%s: Fetching of MAC address from OTP failed",
				__func__);
			goto err;
		}
#endif /* CONFIG_WIFI_FIXED_MAC_ADDRESS_ENABLED */

		if (!nrf_wifi_utils_is_mac_addr_valid(rpu_ctx_zep->mac_addr)) {
			LOG_ERR("%s: Invalid MAC address: %02X:%02X:%02X:%02X:%02X:%02X",
				__func__,
				rpu_ctx_zep->mac_addr[0],
				rpu_ctx_zep->mac_addr[1],
				rpu_ctx_zep->mac_addr[2],
				rpu_ctx_zep->mac_addr[3],
				rpu_ctx_zep->mac_addr[4],
				rpu_ctx_zep->mac_addr[5]);
			goto err;
		}
	}

	k_spin_unlock(&off_raw_tx_drv_priv.lock, key);

	return 0;
err:
	if (rpu_ctx) {
		nrf_wifi_fmac_dev_rem(rpu_ctx);
		rpu_ctx_zep->rpu_ctx = NULL;
	}

	k_spin_unlock(&off_raw_tx_drv_priv.lock, key);
	nrf71_off_raw_tx_deinit();
	return -1;
}


void nrf71_off_raw_tx_deinit(void)
{
	k_spinlock_key_t key;
	struct nrf_wifi_ctx_zep *rpu_ctx_zep = &off_raw_tx_drv_priv.rpu_ctx_zep;
	int i;

	key = k_spin_lock(&off_raw_tx_drv_priv.lock);

	if (!off_raw_tx_drv_priv.fmac_priv) {
		k_spin_unlock(&off_raw_tx_drv_priv.lock, key);
		return;
	}

	nrf_wifi_fmac_deinit(off_raw_tx_drv_priv.fmac_priv);

	for (i = 0; i < NUM_WIFI_PARAMS; i++) {
		if (rpu_ctx_zep->phy_rf_params_addr[i]) {
			k_free((void *)rpu_ctx_zep->phy_rf_params_addr[i]);
			rpu_ctx_zep->phy_rf_params_addr[i] = 0;
			rf_params_off[i].bytes = NULL;
		}
	}
	if (rpu_ctx_zep->vtf_buffer_start_address) {
		k_free((void *)rpu_ctx_zep->vtf_buffer_start_address);
		rpu_ctx_zep->vtf_buffer_start_address = 0;
	}

	k_spin_unlock(&off_raw_tx_drv_priv.lock, key);
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

int nrf71_off_raw_tx_conf_update(struct nrf_wifi_off_raw_tx_conf *conf)
{
	int ret = -1;
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_offload_ctrl_params *off_ctrl_params = NULL;
	struct nrf_wifi_offload_tx_ctrl *off_tx_params = NULL;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;
	k_spinlock_key_t key;

	if (!conf) {
		LOG_ERR("%s: Config params is NULL", __func__);
		goto out;
	}

	off_ctrl_params = nrf_wifi_osal_mem_zalloc(sizeof(*off_ctrl_params));
	if (!off_ctrl_params) {
		LOG_ERR("%s: Failed to allocate memory for off_ctrl_params", __func__);
		goto out;
	}

	key = k_spin_lock(&off_raw_tx_drv_priv.lock);

	fmac_dev_ctx = off_raw_tx_drv_priv.rpu_ctx_zep.rpu_ctx;

	if (!fmac_dev_ctx) {
		LOG_ERR("%s: FMAC device context is NULL", __func__);
		goto out;
	}

	off_tx_params = nrf_wifi_osal_mem_zalloc(sizeof(*off_tx_params));
	if (!off_tx_params) {
		LOG_ERR("%s Failed to allocate memory for off_tx_params: ", __func__);
		goto out;
	}

	if (!validate_rate(conf->tput_mode, conf->rate)) {
		LOG_ERR("%s Invalid rate. Throughput mode: %d, rate: %d\n", __func__,
				      conf->tput_mode, conf->rate);
		goto out;
	}

	off_ctrl_params->channel_no = conf->chan;
	off_ctrl_params->period_in_us = conf->period_us;
	off_ctrl_params->tx_pwr = conf->tx_pwr;
	off_tx_params->he_gi_type = conf->he_gi;
	off_tx_params->he_ltf = conf->he_ltf;
	off_tx_params->pkt_ram_ptr = RPU_MEM_PKT_BASE;
	off_tx_params->pkt_length = conf->pkt_len;
	off_tx_params->rate_flags = conf->tput_mode;
	off_tx_params->rate = valid_data_rates[conf->rate];
	off_tx_params->rate_preamble_type = conf->short_preamble;
	off_tx_params->rate_retries = conf->num_retries;

	status = hal_rpu_mem_write(fmac_dev_ctx->hal_dev_ctx,
				   RPU_MEM_PKT_BASE,
				   conf->pkt,
				   conf->pkt_len);
	if (status != NRF_WIFI_STATUS_SUCCESS) {
		LOG_ERR("%s: hal_rpu_mem_write failed", __func__);
		goto out;
	}

	status = nrf_wifi_off_raw_tx_fmac_conf(fmac_dev_ctx,
					       off_ctrl_params,
					       off_tx_params);
	if (status != NRF_WIFI_STATUS_SUCCESS) {
		LOG_ERR("%s: nRF70 offloaded raw TX configuration failed",
				      __func__);
		goto out;
	}

	ret = 0;
out:
	nrf_wifi_osal_mem_free(off_ctrl_params);
	nrf_wifi_osal_mem_free(off_tx_params);
	k_spin_unlock(&off_raw_tx_drv_priv.lock, key);
	return ret;
}


int nrf71_off_raw_tx_start(struct nrf_wifi_off_raw_tx_conf *conf)
{
	int ret = -1;
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	k_spinlock_key_t key;

	status = nrf71_off_raw_tx_conf_update(conf);
	if (status != NRF_WIFI_STATUS_SUCCESS) {
		LOG_ERR("%s: nRF70 offloaded raw TX configuration failed",
				      __func__);
		goto out;
	}

	key = k_spin_lock(&off_raw_tx_drv_priv.lock);
	if (!off_raw_tx_drv_priv.rpu_ctx_zep.rpu_ctx) {
		LOG_ERR("%s: FMAC device context is NULL", __func__);
		goto out;
	}

	status = nrf_wifi_off_raw_tx_fmac_start(off_raw_tx_drv_priv.rpu_ctx_zep.rpu_ctx);
	if (status != NRF_WIFI_STATUS_SUCCESS) {
		LOG_ERR("%s: nRF70 offloaded raw TX start failed",
				      __func__);
		goto out;
	}

	ret = 0;
out:
	k_spin_unlock(&off_raw_tx_drv_priv.lock, key);
	return ret;
}


int nrf71_off_raw_tx_stop(void)
{
	int ret = -1;
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	k_spinlock_key_t key;

	key = k_spin_lock(&off_raw_tx_drv_priv.lock);

	if (!off_raw_tx_drv_priv.rpu_ctx_zep.rpu_ctx) {
		LOG_ERR("%s: FMAC device context is NULL", __func__);
		goto out;
	}

	status = nrf_wifi_off_raw_tx_fmac_stop(off_raw_tx_drv_priv.rpu_ctx_zep.rpu_ctx);
	if (status != NRF_WIFI_STATUS_SUCCESS) {
		LOG_ERR("%s: nRF70 offloaded raw TX stop failed",
				      __func__);
		goto out;
	}

	ret = 0;
out:
	k_spin_unlock(&off_raw_tx_drv_priv.lock, key);
	return ret;
}


int nrf71_off_raw_tx_mac_addr_get(uint8_t *mac_addr)
{
	if (!mac_addr) {
		LOG_ERR("%s: Invalid param", __func__);
		return -EINVAL;
	}

	memcpy(mac_addr, off_raw_tx_drv_priv.rpu_ctx_zep.mac_addr, 6);
	return 0;
}

int nrf71_off_raw_tx_stats(struct nrf_wifi_off_raw_tx_stats *off_raw_tx_stats)
{
	int ret = -1;
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct rpu_off_raw_tx_op_stats stats;
	k_spinlock_key_t key;

	memset(&stats, 0, sizeof(stats));

	key = k_spin_lock(&off_raw_tx_drv_priv.lock);

	if (!off_raw_tx_drv_priv.rpu_ctx_zep.rpu_ctx) {
		LOG_ERR("%s: FMAC device context is NULL", __func__);
		goto out;
	}

	status = nrf_wifi_off_raw_tx_fmac_stats_get(off_raw_tx_drv_priv.rpu_ctx_zep.rpu_ctx,
						    0,
						    &stats);
	if (status != NRF_WIFI_STATUS_SUCCESS) {
		LOG_ERR("%s: nRF70 offloaded raw TX stats failed",
				      __func__);
		goto out;
	}

	off_raw_tx_stats->off_raw_tx_pkt_sent = stats.fw.offload_raw_tx_cnt;

	ret = 0;
out:
	k_spin_unlock(&off_raw_tx_drv_priv.lock, key);
	return ret;
}
