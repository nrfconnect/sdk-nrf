/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @brief File containing API definitions for the
 * FMAC IF Layer of the Wi-Fi driver.
 */

#include <nrf71_wifi_ctrl.h>
#include "offload_raw_tx/phy_rf_params.h"
#include "offload_raw_tx/hal_api.h"
#include "offload_raw_tx/fmac_api.h"
#include "offload_raw_tx/fmac_cmd.h"
#include "offload_raw_tx/fmac_event.h"
#include "offload_raw_tx/fmac_structs.h"
#include "common/fmac_util.h"
#include <stdio.h>

static enum nrf_wifi_status nrf_wifi_fmac_off_raw_tx_fw_init(
		struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
		struct nrf_wifi_phy_rf_params *rf_params,
		bool rf_params_valid,
#ifdef NRF_WIFI_LOW_POWER
		int sleep_type,
#endif /* NRF_WIFI_LOW_POWER */
		unsigned int phy_calib,
		unsigned char op_band,
		bool beamforming,
		struct nrf_wifi_tx_pwr_ctrl_params *tx_pwr_ctrl,
		struct nrf_wifi_board_params *board_params,
		unsigned char *country_code)
{
	unsigned long start_time_us = 0;
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;

	if (!fmac_dev_ctx) {
		nrf_wifi_osal_log_err("%s: Invalid device context",
				      __func__);
		goto out;
	}

	status = umac_cmd_off_raw_tx_init(fmac_dev_ctx,
					  rf_params,
					  rf_params_valid,
#ifdef NRF_WIFI_LOW_POWER
					  sleep_type,
#endif /* NRF_WIFI_LOW_POWER */
					  phy_calib,
					  op_band,
					  beamforming,
					  tx_pwr_ctrl,
					  board_params,
					  country_code);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		nrf_wifi_osal_log_err("%s: UMAC init failed",
				      __func__);
		goto out;
	}

	start_time_us = nrf_wifi_osal_time_get_curr_us();
	while (!fmac_dev_ctx->fw_init_done) {
		nrf_wifi_osal_sleep_ms(1);
#define MAX_INIT_WAIT (5 * 1000 * 1000)
		if (nrf_wifi_osal_time_elapsed_us(start_time_us) >= MAX_INIT_WAIT) {
			break;
		}
	}

	if (!fmac_dev_ctx->fw_init_done) {
		nrf_wifi_osal_log_err("%s: UMAC init timed out",
				      __func__);
		status = NRF_WIFI_STATUS_FAIL;
		goto out;
	}

	status = NRF_WIFI_STATUS_SUCCESS;
out:
	return status;
}


struct nrf_wifi_fmac_priv *nrf_wifi_off_raw_tx_fmac_init(void)
{
	struct nrf_wifi_fmac_priv *fpriv = NULL;
	struct nrf_wifi_hal_cfg_params hal_cfg_params;

	fpriv = nrf_wifi_osal_mem_zalloc(sizeof(*fpriv));
	if (!fpriv) {
		nrf_wifi_osal_log_err("%s: Unable to allocate fpriv",
				      __func__);
		goto out;
	}

	nrf_wifi_osal_mem_set(&hal_cfg_params,
			      0,
			      sizeof(hal_cfg_params));

	hal_cfg_params.max_cmd_size = MAX_NRF_WIFI_UMAC_CMD_SIZE;
	hal_cfg_params.max_event_size = MAX_EVENT_POOL_LEN;

	fpriv->hpriv = nrf_wifi_hal_init(&hal_cfg_params,
					 &nrf_wifi_off_raw_tx_fmac_event_callback,
					 NULL);
	if (!fpriv->hpriv) {
		nrf_wifi_osal_log_err("%s: Unable to do HAL init",
				      __func__);
		nrf_wifi_osal_mem_free(fpriv);
		fpriv = NULL;
		goto out;
	}

	fpriv->op_mode = NRF_WIFI_OP_MODE_OFF_RAW_TX;
out:
	return fpriv;
}


struct nrf_wifi_fmac_dev_ctx *nrf_wifi_off_raw_tx_fmac_dev_add(struct nrf_wifi_fmac_priv *fpriv,
							       void *os_dev_ctx)
{
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;
	struct nrf_wifi_off_raw_tx_fmac_dev_ctx *off_raw_tx_fmac_dev_ctx;

	if (!fpriv || !os_dev_ctx) {
		return NULL;
	}

	if (fpriv->op_mode != NRF_WIFI_OP_MODE_OFF_RAW_TX) {
		nrf_wifi_osal_log_err("%s: Invalid op mode",
				      __func__);
		goto out;
	}

	fmac_dev_ctx = nrf_wifi_osal_mem_zalloc(sizeof(*fmac_dev_ctx) +
						sizeof(*off_raw_tx_fmac_dev_ctx));

	if (!fmac_dev_ctx) {
		nrf_wifi_osal_log_err("%s: Unable to allocate fmac_dev_ctx",
				      __func__);
		goto out;
	}

	fmac_dev_ctx->fpriv = fpriv;
	fmac_dev_ctx->os_dev_ctx = os_dev_ctx;

	fmac_dev_ctx->hal_dev_ctx = nrf_wifi_off_raw_tx_hal_dev_add(fpriv->hpriv,
								    fmac_dev_ctx);

	if (!fmac_dev_ctx->hal_dev_ctx) {
		nrf_wifi_osal_log_err("%s: nrf_wifi_off_raw_tx_hal_dev_add failed",
				      __func__);

		nrf_wifi_osal_mem_free(fmac_dev_ctx);
		fmac_dev_ctx = NULL;
		goto out;
	}

	fmac_dev_ctx->op_mode = NRF_WIFI_OP_MODE_OFF_RAW_TX;
out:
	return fmac_dev_ctx;
}


enum nrf_wifi_status nrf_wifi_off_raw_tx_fmac_dev_init(
		struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
#ifdef NRF_WIFI_LOW_POWER
		int sleep_type,
#endif /* NRF_WIFI_LOW_POWER */
		unsigned int phy_calib,
		unsigned char op_band,
		bool beamforming,
		struct nrf_wifi_tx_pwr_ctrl_params *tx_pwr_ctrl_params,
		struct nrf_wifi_tx_pwr_ceil_params *tx_pwr_ceil_params,
		struct nrf_wifi_board_params *board_params,
		unsigned char *country_code)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_fmac_otp_info otp_info;
	struct nrf_wifi_phy_rf_params phy_rf_params;

	if (!fmac_dev_ctx) {
		nrf_wifi_osal_log_err("%s: Invalid device context",
				      __func__);
		goto out;
	}

	if (fmac_dev_ctx->op_mode != NRF_WIFI_OP_MODE_OFF_RAW_TX) {
		nrf_wifi_osal_log_err("%s: Invalid op mode",
				      __func__);
		goto out;
	}

	status = nrf_wifi_hal_dev_init(fmac_dev_ctx->hal_dev_ctx);
	if (status != NRF_WIFI_STATUS_SUCCESS) {
		nrf_wifi_osal_log_err("%s: nrf_wifi_hal_dev_init failed",
				      __func__);
		goto out;
	}

	fmac_dev_ctx->tx_pwr_ceil_params = nrf_wifi_osal_mem_alloc(sizeof(*tx_pwr_ceil_params));
	nrf_wifi_osal_mem_cpy(fmac_dev_ctx->tx_pwr_ceil_params,
			      tx_pwr_ceil_params,
			      sizeof(*tx_pwr_ceil_params));

	nrf_wifi_osal_mem_set(&otp_info,
			      0xFF,
			      sizeof(otp_info));

	status = nrf_wifi_hal_otp_info_get(fmac_dev_ctx->hal_dev_ctx,
					   &otp_info.info,
					   &otp_info.flags);
	if (status != NRF_WIFI_STATUS_SUCCESS) {
		nrf_wifi_osal_log_err("%s: Fetching of RPU OTP information failed",
				      __func__);
		goto out;
	}

	status = nrf_wifi_off_raw_tx_fmac_rf_params_get(fmac_dev_ctx,
							&phy_rf_params);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		nrf_wifi_osal_log_err("%s: RF parameters get failed",
				     __func__);
		goto out;
	}

	status = nrf_wifi_fmac_off_raw_tx_fw_init(fmac_dev_ctx,
						  &phy_rf_params,
						  true,
#ifdef NRF_WIFI_LOW_POWER
						  sleep_type,
#endif /* NRF_WIFI_LOW_POWER */
						  phy_calib,
						  op_band,
						  beamforming,
						  tx_pwr_ctrl_params,
						  board_params,
						  country_code);

	if (status == NRF_WIFI_STATUS_FAIL) {
		nrf_wifi_osal_log_err("%s: nrf_wifi_fmac_off_raw_tx_fw_init failed",
				      __func__);
		goto out;
	}
out:
	return status;
}


enum nrf_wifi_status nrf_wifi_off_raw_tx_fmac_conf(
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
	struct nrf_wifi_offload_ctrl_params *off_ctrl_params,
	struct nrf_wifi_offload_tx_ctrl *tx_params)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_off_raw_tx_fmac_dev_ctx *dev_ctx_off_raw_tx;
	struct nrf_wifi_fmac_reg_info reg_domain_info = {0};
	unsigned char count = 0;

	if (!fmac_dev_ctx) {
		nrf_wifi_osal_log_err("%s: Invalid device context",
				      __func__);
		goto out;
	}

	if (fmac_dev_ctx->op_mode != NRF_WIFI_OP_MODE_OFF_RAW_TX) {
		nrf_wifi_osal_log_err("%s: Invalid op mode",
				      __func__);
		goto out;
	}

	dev_ctx_off_raw_tx = wifi_dev_priv(fmac_dev_ctx);
	dev_ctx_off_raw_tx->off_raw_tx_cmd_done = true;

	if (!off_ctrl_params || !off_tx_params) {
		nrf_wifi_osal_log_err("%s: Invalid offloaded raw tx params",
				      __func__);
		goto out;
	}

	status = umac_cmd_off_raw_tx_conf(fmac_dev_ctx,
					  off_ctrl_params,
					  off_tx_params);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		nrf_wifi_osal_log_err("%s: umac_cmd_offload_raw_tx_conf failed", __func__);
		goto out;
	}

	do {
		nrf_wifi_osal_sleep_ms(1);
		count++;
	} while ((dev_ctx_off_raw_tx->off_raw_tx_cmd_done == true) &&
		 (count < NRF_WIFI_FMAC_PARAMS_RECV_TIMEOUT));

	if (count == NRF_WIFI_FMAC_PARAMS_RECV_TIMEOUT) {
		nrf_wifi_osal_log_err("%s: Timed out",
				      __func__);
		goto out;
	}

	if (dev_ctx_off_raw_tx->off_raw_tx_cmd_status != NRF_WIFI_UMAC_CMD_SUCCESS) {
		status = nrf_wifi_fmac_get_reg(fmac_dev_ctx, &reg_domain_info);
		if (status != NRF_WIFI_STATUS_SUCCESS) {
			nrf_wifi_osal_log_err("%s: Failed to get regulatory domain",
					      __func__);
			goto out;
		}

		nrf_wifi_osal_log_err("%s: Failed to set config, check against %.2s reg domain",
				      __func__,
				      fmac_dev_ctx->alpha2);
		status = NRF_WIFI_STATUS_FAIL;
		goto out;
	}

	status = NRF_WIFI_STATUS_SUCCESS;
out:
	return status;
}

enum nrf_wifi_status nrf_wifi_off_raw_tx_fmac_start(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;

	if (!fmac_dev_ctx) {
		nrf_wifi_osal_log_err("%s: Invalid device context",
				      __func__);
		goto out;
	}

	if (fmac_dev_ctx->op_mode != NRF_WIFI_OP_MODE_OFF_RAW_TX) {
		nrf_wifi_osal_log_err("%s: Invalid op mode",
				      __func__);
		goto out;
	}

	status = umac_cmd_off_raw_tx_ctrl(fmac_dev_ctx, 1);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		nrf_wifi_osal_log_err("%s: umac_cmd_off_raw_tx_ctrl failed", __func__);
		goto out;
	}
out:
	return status;
}

enum nrf_wifi_status nrf_wifi_off_raw_tx_fmac_stop(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;

	if (!fmac_dev_ctx) {
		nrf_wifi_osal_log_err("%s: Invalid device context",
				      __func__);
		goto out;
	}

	if (fmac_dev_ctx->op_mode != NRF_WIFI_OP_MODE_OFF_RAW_TX) {
		nrf_wifi_osal_log_err("%s: Invalid op mode",
				      __func__);
		goto out;
	}

	status = umac_cmd_off_raw_tx_ctrl(fmac_dev_ctx, 0);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		nrf_wifi_osal_log_err("%s: umac_cmd_offload_raw_tx_ctrl failed", __func__);
		goto out;
	}
out:
	return status;
}


enum nrf_wifi_status nrf_wifi_off_raw_tx_fmac_stats_get(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
							enum rpu_op_mode op_mode,
							struct rpu_off_raw_tx_op_stats *stats)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	unsigned char count = 0;

	if (fmac_dev_ctx->op_mode != NRF_WIFI_OP_MODE_OFF_RAW_TX) {
		nrf_wifi_osal_log_err("%s: Invalid op mode",
				      __func__);
		goto out;
	}

	if (fmac_dev_ctx->stats_req == true) {
		nrf_wifi_osal_log_err("%s: Stats request already pending",
				      __func__);
		goto out;
	}

	fmac_dev_ctx->stats_req = true;
	fmac_dev_ctx->fw_stats = &stats->fw;

	status = umac_cmd_off_raw_tx_prog_stats_get(fmac_dev_ctx);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		goto out;
	}

	do {
		nrf_wifi_osal_sleep_ms(1);
		count++;
	} while ((fmac_dev_ctx->stats_req == true) &&
		 (count < NRF_WIFI_FMAC_STATS_RECV_TIMEOUT));

	if (count == NRF_WIFI_FMAC_STATS_RECV_TIMEOUT) {
		nrf_wifi_osal_log_err("%s: Timed out",
				      __func__);
		goto out;
	}

	status = NRF_WIFI_STATUS_SUCCESS;
out:
	return status;
}

static int nrf_wifi_off_raw_tx_fmac_phy_rf_params_init(struct nrf_wifi_phy_rf_params *prf,
						       unsigned int package_info,
						       unsigned char *str)
{
	int ret = -1;
	unsigned int rf_param_offset = BAND_2G_LW_ED_BKF_DSSS_OFST - NRF_WIFI_RF_PARAMS_CONF_SIZE;
	/* Initilaize reserved bytes */
	nrf_wifi_osal_mem_set(prf,
			      0x0,
			      sizeof(prf));
	/* Initialize PD adjust values for MCS7. Currently these 4 bytes are not being used */
	prf->pd_adjust_val.pd_adjt_lb_chan = PD_ADJUST_VAL;
	prf->pd_adjust_val.pd_adjt_hb_low_chan = PD_ADJUST_VAL;
	prf->pd_adjust_val.pd_adjt_hb_mid_chan = PD_ADJUST_VAL;
	prf->pd_adjust_val.pd_adjt_hb_high_chan = PD_ADJUST_VAL;

	/* RX Gain offsets */
	prf->rx_gain_offset.rx_gain_lb_chan = CTRL_PWR_OPTIMIZATIONS;
	prf->rx_gain_offset.rx_gain_hb_low_chan = RX_GAIN_OFFSET_HB_LOW_CHAN;
	prf->rx_gain_offset.rx_gain_hb_mid_chan = RX_GAIN_OFFSET_HB_MID_CHAN;
	prf->rx_gain_offset.rx_gain_hb_high_chan = RX_GAIN_OFFSET_HB_HIGH_CHAN;

	if (package_info == CSP_PACKAGE_INFO) {
		prf->xo_offset.xo_freq_offset = CSP_XO_VAL;

		/* Configure systematic offset value */
		prf->syst_tx_pwr_offset.syst_off_lb_chan = CSP_SYSTEM_OFFSET_LB;
		prf->syst_tx_pwr_offset.syst_off_hb_low_chan = CSP_SYSTEM_OFFSET_HB_CHAN_LOW;
		prf->syst_tx_pwr_offset.syst_off_hb_mid_chan = CSP_SYSTEM_OFFSET_HB_CHAN_MID;
		prf->syst_tx_pwr_offset.syst_off_hb_high_chan = CSP_SYSTEM_OFFSET_HB_CHAN_HIGH;

		/* TX power ceiling */
		prf->max_pwr_ceil.max_dsss_pwr = CSP_MAX_TX_PWR_DSSS;
		prf->max_pwr_ceil.max_lb_mcs7_pwr = CSP_MAX_TX_PWR_LB_MCS7;
		prf->max_pwr_ceil.max_lb_mcs0_pwr = CSP_MAX_TX_PWR_LB_MCS0;
		prf->max_pwr_ceil.max_hb_low_chan_mcs7_pwr = CSP_MAX_TX_PWR_HB_LOW_CHAN_MCS7;
		prf->max_pwr_ceil.max_hb_mid_chan_mcs7_pwr = CSP_MAX_TX_PWR_HB_MID_CHAN_MCS7;
		prf->max_pwr_ceil.max_hb_high_chan_mcs7_pwr = CSP_MAX_TX_PWR_HB_HIGH_CHAN_MCS7;
		prf->max_pwr_ceil.max_hb_low_chan_mcs0_pwr = CSP_MAX_TX_PWR_HB_LOW_CHAN_MCS0;
		prf->max_pwr_ceil.max_hb_mid_chan_mcs0_pwr = CSP_MAX_TX_PWR_HB_MID_CHAN_MCS0;
		prf->max_pwr_ceil.max_hb_high_chan_mcs0_pwr = CSP_MAX_TX_PWR_HB_HIGH_CHAN_MCS0;
	} else {
		/** If nothing is written to OTP field corresponding to package info byte
		 * or if the package info field is corrupted then the default package
		 * package is QFN.
		 */

		/* Initialize XO */
		prf->xo_offset.xo_freq_offset = QFN_XO_VAL;

		/* Configure systematic offset value */
		prf->syst_tx_pwr_offset.syst_off_lb_chan = QFN_SYSTEM_OFFSET_LB;
		prf->syst_tx_pwr_offset.syst_off_hb_low_chan = QFN_SYSTEM_OFFSET_HB_CHAN_LOW;
		prf->syst_tx_pwr_offset.syst_off_hb_mid_chan = QFN_SYSTEM_OFFSET_HB_CHAN_MID;
		prf->syst_tx_pwr_offset.syst_off_hb_high_chan = QFN_SYSTEM_OFFSET_HB_CHAN_HIGH;

		/* TX power ceiling */
		prf->max_pwr_ceil.max_dsss_pwr = QFN_MAX_TX_PWR_DSSS;
		prf->max_pwr_ceil.max_lb_mcs7_pwr = QFN_MAX_TX_PWR_LB_MCS7;
		prf->max_pwr_ceil.max_lb_mcs0_pwr = QFN_MAX_TX_PWR_LB_MCS0;
		prf->max_pwr_ceil.max_hb_low_chan_mcs7_pwr = QFN_MAX_TX_PWR_HB_LOW_CHAN_MCS7;
		prf->max_pwr_ceil.max_hb_mid_chan_mcs7_pwr = QFN_MAX_TX_PWR_HB_MID_CHAN_MCS7;
		prf->max_pwr_ceil.max_hb_high_chan_mcs7_pwr = QFN_MAX_TX_PWR_HB_HIGH_CHAN_MCS7;
		prf->max_pwr_ceil.max_hb_low_chan_mcs0_pwr = QFN_MAX_TX_PWR_HB_LOW_CHAN_MCS0;
		prf->max_pwr_ceil.max_hb_mid_chan_mcs0_pwr = QFN_MAX_TX_PWR_HB_MID_CHAN_MCS0;
		prf->max_pwr_ceil.max_hb_high_chan_mcs0_pwr = QFN_MAX_TX_PWR_HB_HIGH_CHAN_MCS0;
	}

	ret = nrf_wifi_utils_hex_str_to_val((unsigned char *)&prf->phy_params,
					    sizeof(prf->phy_params),
					    str);

	prf->phy_params[rf_param_offset]  = NRF71_BAND_2G_LOWER_EDGE_BACKOFF_DSSS;
	prf->phy_params[rf_param_offset + 1]  = NRF71_BAND_2G_LOWER_EDGE_BACKOFF_HT;
	prf->phy_params[rf_param_offset + 2]  = NRF71_BAND_2G_LOWER_EDGE_BACKOFF_HE;
	prf->phy_params[rf_param_offset + 3]  = NRF71_BAND_2G_UPPER_EDGE_BACKOFF_DSSS;
	prf->phy_params[rf_param_offset + 4]  = NRF71_BAND_2G_UPPER_EDGE_BACKOFF_HT;
	prf->phy_params[rf_param_offset + 5]  = NRF71_BAND_2G_UPPER_EDGE_BACKOFF_HE;
	prf->phy_params[rf_param_offset + 6]  = NRF71_BAND_UNII_1_LOWER_EDGE_BACKOFF_HT;
	prf->phy_params[rf_param_offset + 7]  = NRF71_BAND_UNII_1_LOWER_EDGE_BACKOFF_HE;
	prf->phy_params[rf_param_offset + 8]  = NRF71_BAND_UNII_1_UPPER_EDGE_BACKOFF_HT;
	prf->phy_params[rf_param_offset + 9]  = NRF71_BAND_UNII_1_UPPER_EDGE_BACKOFF_HE;
	prf->phy_params[rf_param_offset + 10]  = NRF71_BAND_UNII_2A_LOWER_EDGE_BACKOFF_HT;
	prf->phy_params[rf_param_offset + 11]  = NRF71_BAND_UNII_2A_LOWER_EDGE_BACKOFF_HE;
	prf->phy_params[rf_param_offset + 12]  = NRF71_BAND_UNII_2A_UPPER_EDGE_BACKOFF_HT;
	prf->phy_params[rf_param_offset + 13]  = NRF71_BAND_UNII_2A_UPPER_EDGE_BACKOFF_HE;
	prf->phy_params[rf_param_offset + 14]  = NRF71_BAND_UNII_2C_LOWER_EDGE_BACKOFF_HT;
	prf->phy_params[rf_param_offset + 15]  = NRF71_BAND_UNII_2C_LOWER_EDGE_BACKOFF_HE;
	prf->phy_params[rf_param_offset + 16]  = NRF71_BAND_UNII_2C_UPPER_EDGE_BACKOFF_HT;
	prf->phy_params[rf_param_offset + 17]  = NRF71_BAND_UNII_2C_UPPER_EDGE_BACKOFF_HE;
	prf->phy_params[rf_param_offset + 18]  = NRF71_BAND_UNII_3_LOWER_EDGE_BACKOFF_HT;
	prf->phy_params[rf_param_offset + 19]  = NRF71_BAND_UNII_3_LOWER_EDGE_BACKOFF_HE;
	prf->phy_params[rf_param_offset + 20]  = NRF71_BAND_UNII_3_UPPER_EDGE_BACKOFF_HT;
	prf->phy_params[rf_param_offset + 21]  = NRF71_BAND_UNII_3_UPPER_EDGE_BACKOFF_HE;
	prf->phy_params[rf_param_offset + 22]  = NRF71_BAND_UNII_4_LOWER_EDGE_BACKOFF_HT;
	prf->phy_params[rf_param_offset + 23]  = NRF71_BAND_UNII_4_LOWER_EDGE_BACKOFF_HE;
	prf->phy_params[rf_param_offset + 24]  = NRF71_BAND_UNII_4_UPPER_EDGE_BACKOFF_HT;
	prf->phy_params[rf_param_offset + 25]  = NRF71_BAND_UNII_4_UPPER_EDGE_BACKOFF_HE;
	prf->phy_params[rf_param_offset + 26]  = NRF71_ANT_GAIN_2G;
	prf->phy_params[rf_param_offset + 27]  = NRF71_ANT_GAIN_5G_BAND1;
	prf->phy_params[rf_param_offset + 28]  = NRF71_ANT_GAIN_5G_BAND2;
	prf->phy_params[rf_param_offset + 29]  = NRF71_ANT_GAIN_5G_BAND3;
	prf->phy_params[rf_param_offset + 30]  = NRF71_PCB_LOSS_2G;
	prf->phy_params[rf_param_offset + 31]  = NRF71_PCB_LOSS_5G_BAND1;
	prf->phy_params[rf_param_offset + 32]  = NRF71_PCB_LOSS_5G_BAND2;
	prf->phy_params[rf_param_offset + 33]  = NRF71_PCB_LOSS_5G_BAND3;

	return ret;
}


enum nrf_wifi_status nrf_wifi_off_raw_tx_fmac_rf_params_get(
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
	struct nrf_wifi_phy_rf_params *phy_rf_params)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_fmac_otp_info otp_info;
	unsigned int ft_prog_ver;
	int ret = -1;
	/* If package_info is not written to OTP then the default value will be 0xFF. */
	unsigned int package_info = 0xFFFFFFFF;
	struct nrf_wifi_tx_pwr_ceil_params *tx_pwr_ceil_params;
	unsigned char backoff_2g_dsss = 0, backoff_2g_ofdm = 0;
	unsigned char backoff_5g_lowband = 0, backoff_5g_midband = 0, backoff_5g_highband = 0;

	if (!fmac_dev_ctx || !phy_rf_params) {
		nrf_wifi_osal_log_err("%s: Invalid parameters",
				      __func__);
		goto out;
	}

	if (fmac_dev_ctx->op_mode != NRF_WIFI_OP_MODE_OFF_RAW_TX) {
		nrf_wifi_osal_log_err("%s: Invalid op mode",
				      __func__);
		goto out;
	}

	tx_pwr_ceil_params = fmac_dev_ctx->tx_pwr_ceil_params;

	nrf_wifi_osal_mem_set(&otp_info,
			      0xFF,
			      sizeof(otp_info));

	status = nrf_wifi_hal_otp_info_get(fmac_dev_ctx->hal_dev_ctx,
					   &otp_info.info,
					   &otp_info.flags);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		nrf_wifi_osal_log_err("%s: Fetching of RPU OTP information failed",
				      __func__);
		goto out;
	}

	status = nrf_wifi_hal_otp_ft_prog_ver_get(fmac_dev_ctx->hal_dev_ctx,
						  &ft_prog_ver);
	if (status != NRF_WIFI_STATUS_SUCCESS) {
		nrf_wifi_osal_log_err("%s: Fetching of FT program version failed",
				      __func__);
		goto out;
	}

	status = nrf_wifi_hal_otp_pack_info_get(fmac_dev_ctx->hal_dev_ctx,
						&package_info);
	if (status != NRF_WIFI_STATUS_SUCCESS) {
		nrf_wifi_osal_log_err("%s: Fetching of Package info failed",
				      __func__);
		goto out;
	}

	ret = nrf_wifi_off_raw_tx_fmac_phy_rf_params_init(phy_rf_params,
							  package_info,
							  NRF_WIFI_OFF_RAW_TX_DEF_RF_PARAMS);

	if (ret == -1) {
		nrf_wifi_osal_log_err("%s: Initialization of RF params with default values failed",
				      __func__);
		status = NRF_WIFI_STATUS_FAIL;
		goto out;
	}
	if (!(otp_info.flags & (~CALIB_XO_FLAG_MASK))) {
		nrf_wifi_osal_mem_cpy(&phy_rf_params->xo_offset.xo_freq_offset,
				      (char *)otp_info.info.calib + OTP_OFF_CALIB_XO,
				      OTP_SZ_CALIB_XO);

	}

	ft_prog_ver = (ft_prog_ver & FT_PROG_VER_MASK) >> 16;

	if (ft_prog_ver == FT_PROG_VER1) {
		backoff_2g_dsss = FT_PROG_VER1_2G_DSSS_TXCEIL_BKOFF;
		backoff_2g_ofdm = FT_PROG_VER1_2G_OFDM_TXCEIL_BKOFF;
		backoff_5g_lowband = FT_PROG_VER1_5G_LOW_OFDM_TXCEIL_BKOFF;
		backoff_5g_midband = FT_PROG_VER1_5G_MID_OFDM_TXCEIL_BKOFF;
		backoff_5g_highband = FT_PROG_VER1_5G_HIGH_OFDM_TXCEIL_BKOFF;
	} else if (ft_prog_ver == FT_PROG_VER2) {
		backoff_2g_dsss = FT_PROG_VER2_2G_DSSS_TXCEIL_BKOFF;
		backoff_2g_ofdm = FT_PROG_VER2_2G_OFDM_TXCEIL_BKOFF;
		backoff_5g_lowband = FT_PROG_VER2_5G_LOW_OFDM_TXCEIL_BKOFF;
		backoff_5g_midband = FT_PROG_VER2_5G_MID_OFDM_TXCEIL_BKOFF;
		backoff_5g_highband = FT_PROG_VER2_5G_HIGH_OFDM_TXCEIL_BKOFF;
	} else if (ft_prog_ver == FT_PROG_VER3) {
		backoff_2g_dsss = FT_PROG_VER3_2G_DSSS_TXCEIL_BKOFF;
		backoff_2g_ofdm = FT_PROG_VER3_2G_OFDM_TXCEIL_BKOFF;
		backoff_5g_lowband = FT_PROG_VER3_5G_LOW_OFDM_TXCEIL_BKOFF;
		backoff_5g_midband = FT_PROG_VER3_5G_MID_OFDM_TXCEIL_BKOFF;
		backoff_5g_highband = FT_PROG_VER3_5G_HIGH_OFDM_TXCEIL_BKOFF;
	}
	phy_rf_params->max_pwr_ceil.max_dsss_pwr =
	MIN(tx_pwr_ceil_params->max_pwr_2g_dsss, phy_rf_params->max_pwr_ceil.max_dsss_pwr)
	- backoff_2g_dsss;
	phy_rf_params->max_pwr_ceil.max_lb_mcs7_pwr =
	MIN(tx_pwr_ceil_params->max_pwr_2g_mcs7, phy_rf_params->max_pwr_ceil.max_lb_mcs7_pwr)
	- backoff_2g_ofdm;
	phy_rf_params->max_pwr_ceil.max_lb_mcs0_pwr =
	MIN(tx_pwr_ceil_params->max_pwr_2g_mcs0, phy_rf_params->max_pwr_ceil.max_lb_mcs0_pwr)
	- backoff_2g_ofdm;
	phy_rf_params->max_pwr_ceil.max_hb_low_chan_mcs7_pwr =
	MIN(tx_pwr_ceil_params->max_pwr_5g_low_mcs7,
		phy_rf_params->max_pwr_ceil.max_hb_low_chan_mcs7_pwr) - backoff_5g_lowband;
	phy_rf_params->max_pwr_ceil.max_hb_mid_chan_mcs7_pwr =
	MIN(tx_pwr_ceil_params->max_pwr_5g_mid_mcs7,
		phy_rf_params->max_pwr_ceil.max_hb_mid_chan_mcs7_pwr) - backoff_5g_midband;
	phy_rf_params->max_pwr_ceil.max_hb_high_chan_mcs7_pwr =
	MIN(tx_pwr_ceil_params->max_pwr_5g_high_mcs7,
		phy_rf_params->max_pwr_ceil.max_hb_high_chan_mcs7_pwr) - backoff_5g_highband;
	phy_rf_params->max_pwr_ceil.max_hb_low_chan_mcs0_pwr =
	MIN(tx_pwr_ceil_params->max_pwr_5g_low_mcs0,
		phy_rf_params->max_pwr_ceil.max_hb_low_chan_mcs0_pwr) - backoff_5g_lowband;
	phy_rf_params->max_pwr_ceil.max_hb_mid_chan_mcs0_pwr =
	MIN(tx_pwr_ceil_params->max_pwr_5g_mid_mcs0,
	    phy_rf_params->max_pwr_ceil.max_hb_mid_chan_mcs0_pwr) - backoff_5g_midband;
	phy_rf_params->max_pwr_ceil.max_hb_high_chan_mcs0_pwr =
	MIN(tx_pwr_ceil_params->max_pwr_5g_high_mcs0,
	    phy_rf_params->max_pwr_ceil.max_hb_high_chan_mcs0_pwr) - backoff_5g_highband;

	status = NRF_WIFI_STATUS_SUCCESS;
out:
	return status;
}
