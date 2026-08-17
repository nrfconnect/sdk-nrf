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
#include <common/mem_mgmt.h>
#include <offload_raw_tx/hal_api.h>
#include <offload_raw_tx/fmac_api.h>
#include <offload_raw_tx/fmac_cmd.h>
#include <offload_raw_tx/fmac_event.h>
#include <offload_raw_tx/fmac_structs.h>
#include <common/util.h>
#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(wifi_nrf, CONFIG_WIFI_NRF71_LOG_LEVEL);

static enum nrf_wifi_status nrf_wifi_fmac_off_raw_tx_fw_init(
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx, unsigned int *rf_params_addr,
	unsigned int vtf_buffer_start_address,
#ifdef NRF_WIFI_LOW_POWER
	int sleep_type,
#endif /* NRF_WIFI_LOW_POWER */
	unsigned int phy_calib, unsigned char op_band, bool beamforming,
	struct nrf_wifi_tx_pwr_ctrl_params *tx_pwr_ctrl, struct nrf_wifi_board_params *board_params,
	unsigned char *country_code)
{
	unsigned long start_time_us = 0;
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;

	if (!fmac_dev_ctx) {
		LOG_ERR("%s: Invalid device context",
				      __func__);
		goto out;
	}

	status = umac_cmd_off_raw_tx_init(fmac_dev_ctx, rf_params_addr, vtf_buffer_start_address,
#ifdef NRF_WIFI_LOW_POWER
					  sleep_type,
#endif /* NRF_WIFI_LOW_POWER */
					  phy_calib, op_band, beamforming, tx_pwr_ctrl,
					  board_params, country_code);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		LOG_ERR("%s: UMAC init failed",
				      __func__);
		goto out;
	}

	start_time_us = k_ticks_to_us_floor64(k_uptime_ticks());
	while (!fmac_dev_ctx->fw_init_done) {
		k_msleep(1);
#define MAX_INIT_WAIT (5 * 1000 * 1000)
		if ((k_ticks_to_us_floor64(k_uptime_ticks()) - start_time_us) >= MAX_INIT_WAIT) {
			break;
		}
	}

	if (!fmac_dev_ctx->fw_init_done) {
		LOG_ERR("%s: UMAC init timed out",
				      __func__);
		status = NRF_WIFI_STATUS_FAIL;
		goto out;
	}

	status = NRF_WIFI_STATUS_SUCCESS;
out:
	return status;
}


static void nrf_wifi_off_raw_tx_fmac_fw_deinit(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx)
{
	if (!fmac_dev_ctx) {
		LOG_ERR("%s: Invalid device context",
				      __func__);
		return;
	}

	if (fmac_dev_ctx->op_mode != NRF_WIFI_OP_MODE_OFF_RAW_TX) {
		LOG_ERR("%s: Invalid op mode",
				      __func__);
		return;
	}
}


struct nrf_wifi_fmac_priv *nrf_wifi_off_raw_tx_fmac_init(void)
{
	struct nrf_wifi_fmac_priv *fpriv = NULL;
	struct nrf_wifi_hal_cfg_params hal_cfg_params;

	fpriv = nrf_wifi_mem_zalloc(NRF_WIFI_MEM_POOL_TYPE_CTRL, sizeof(*fpriv));
	if (!fpriv) {
		LOG_ERR("%s: Unable to allocate fpriv",
				      __func__);
		goto out;
	}

	nrf_wifi_mem_set(&hal_cfg_params,
			      0,
			      sizeof(hal_cfg_params));

	fpriv->hpriv = nrf_wifi_hal_init(&hal_cfg_params,
					 &nrf_wifi_off_raw_tx_fmac_event_callback,
					 NULL);
	if (!fpriv->hpriv) {
		LOG_ERR("%s: Unable to do HAL init",
				      __func__);
		nrf_wifi_mem_free(NRF_WIFI_MEM_POOL_TYPE_CTRL, fpriv);
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
		LOG_ERR("%s: Invalid op mode",
				      __func__);
		goto out;
	}

	fmac_dev_ctx = nrf_wifi_mem_zalloc(NRF_WIFI_MEM_POOL_TYPE_CTRL, sizeof(*fmac_dev_ctx) +
						sizeof(*off_raw_tx_fmac_dev_ctx));

	if (!fmac_dev_ctx) {
		LOG_ERR("%s: Unable to allocate fmac_dev_ctx",
				      __func__);
		goto out;
	}

	fmac_dev_ctx->fpriv = fpriv;
	fmac_dev_ctx->os_dev_ctx = os_dev_ctx;

	fmac_dev_ctx->hal_dev_ctx = nrf_wifi_off_raw_tx_hal_dev_add(fpriv->hpriv,
								    fmac_dev_ctx);

	if (!fmac_dev_ctx->hal_dev_ctx) {
		LOG_ERR("%s: nrf_wifi_off_raw_tx_hal_dev_add failed",
				      __func__);

		nrf_wifi_mem_free(NRF_WIFI_MEM_POOL_TYPE_CTRL, fmac_dev_ctx);
		fmac_dev_ctx = NULL;
		goto out;
	}

	fmac_dev_ctx->op_mode = NRF_WIFI_OP_MODE_OFF_RAW_TX;
out:
	return fmac_dev_ctx;
}

enum nrf_wifi_status
nrf_wifi_off_raw_tx_fmac_dev_init(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
#ifdef NRF_WIFI_LOW_POWER
				  int sleep_type,
#endif /* NRF_WIFI_LOW_POWER */
				  unsigned int phy_calib,
				  unsigned char op_band,
				  bool beamforming,
				  struct nrf_wifi_tx_pwr_ctrl_params *tx_pwr_ctrl_params,
				  struct nrf_wifi_tx_pwr_ceil_params *tx_pwr_ceil_params,
				  struct nrf_wifi_board_params *board_params,
				  unsigned char *country_code,
				  unsigned int *rf_params_addr,
				  unsigned int vtf_buffer_start_address)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;

	if (!fmac_dev_ctx) {
		LOG_ERR("%s: Invalid device context",
				      __func__);
		goto out;
	}

	if (fmac_dev_ctx->op_mode != NRF_WIFI_OP_MODE_OFF_RAW_TX) {
		LOG_ERR("%s: Invalid op mode",
				      __func__);
		goto out;
	}

	status = nrf_wifi_hal_dev_init(fmac_dev_ctx->hal_dev_ctx);
	if (status != NRF_WIFI_STATUS_SUCCESS) {
		LOG_ERR("%s: nrf_wifi_hal_dev_init failed",
				      __func__);
		goto out;
	}

	fmac_dev_ctx->tx_pwr_ceil_params =
		nrf_wifi_mem_alloc(NRF_WIFI_MEM_POOL_TYPE_CTRL,
				   sizeof(*tx_pwr_ceil_params));
	nrf_wifi_mem_cpy(fmac_dev_ctx->tx_pwr_ceil_params,
			      tx_pwr_ceil_params,
			      sizeof(*tx_pwr_ceil_params));

	status = nrf_wifi_fmac_off_raw_tx_fw_init(
		fmac_dev_ctx, rf_params_addr, vtf_buffer_start_address,
#ifdef NRF_WIFI_LOW_POWER
		sleep_type,
#endif /* NRF_WIFI_LOW_POWER */
		phy_calib, op_band, beamforming, tx_pwr_ctrl_params, board_params, country_code);

	if (status == NRF_WIFI_STATUS_FAIL) {
		LOG_ERR("%s: nrf_wifi_fmac_off_raw_tx_fw_init failed",
				      __func__);
		goto out;
	}
out:
	return status;
}


void nrf_wifi_off_raw_tx_fmac_dev_deinit(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx)
{
	if (fmac_dev_ctx->op_mode != NRF_WIFI_OP_MODE_OFF_RAW_TX) {
		LOG_ERR("%s: Invalid op mode",
				      __func__);
		return;
	}

	nrf_wifi_mem_free(NRF_WIFI_MEM_POOL_TYPE_CTRL, fmac_dev_ctx->tx_pwr_ceil_params);
	nrf_wifi_off_raw_tx_fmac_fw_deinit(fmac_dev_ctx);
}


enum nrf_wifi_status nrf_wifi_off_raw_tx_fmac_conf(
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
	struct nrf_wifi_offload_ctrl_params *off_ctrl_params,
	struct nrf_wifi_offload_tx_ctrl *off_tx_params)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_off_raw_tx_fmac_dev_ctx *dev_ctx_off_raw_tx;
	struct nrf_wifi_fmac_reg_info reg_domain_info = {0};
	unsigned char count = 0;

	if (!fmac_dev_ctx) {
		LOG_ERR("%s: Invalid device context",
				      __func__);
		goto out;
	}

	if (fmac_dev_ctx->op_mode != NRF_WIFI_OP_MODE_OFF_RAW_TX) {
		LOG_ERR("%s: Invalid op mode",
				      __func__);
		goto out;
	}

	dev_ctx_off_raw_tx = wifi_dev_priv(fmac_dev_ctx);
	dev_ctx_off_raw_tx->off_raw_tx_cmd_done = true;

	if (!off_ctrl_params || !off_tx_params) {
		LOG_ERR("%s: Invalid offloaded raw tx params",
				      __func__);
		goto out;
	}

	status = umac_cmd_off_raw_tx_conf(fmac_dev_ctx,
					  off_ctrl_params,
					  off_tx_params);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		LOG_ERR("%s: umac_cmd_offload_raw_tx_conf failed", __func__);
		goto out;
	}

	do {
		k_msleep(1);
		count++;
	} while ((dev_ctx_off_raw_tx->off_raw_tx_cmd_done == true) &&
		 (count < NRF_WIFI_FMAC_PARAMS_RECV_TIMEOUT));

	if (count == NRF_WIFI_FMAC_PARAMS_RECV_TIMEOUT) {
		LOG_ERR("%s: Timed out",
				      __func__);
		goto out;
	}

	if (dev_ctx_off_raw_tx->off_raw_tx_cmd_status != NRF_WIFI_UMAC_CMD_SUCCESS) {
		status = nrf_wifi_fmac_get_reg(fmac_dev_ctx, &reg_domain_info);
		if (status != NRF_WIFI_STATUS_SUCCESS) {
			LOG_ERR("%s: Failed to get regulatory domain",
					      __func__);
			goto out;
		}

		LOG_ERR("%s: Failed to set config, check against %.2s reg domain",
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
		LOG_ERR("%s: Invalid device context",
				      __func__);
		goto out;
	}

	if (fmac_dev_ctx->op_mode != NRF_WIFI_OP_MODE_OFF_RAW_TX) {
		LOG_ERR("%s: Invalid op mode",
				      __func__);
		goto out;
	}

	status = umac_cmd_off_raw_tx_ctrl(fmac_dev_ctx, 1);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		LOG_ERR("%s: umac_cmd_off_raw_tx_ctrl failed", __func__);
		goto out;
	}
out:
	return status;
}

enum nrf_wifi_status nrf_wifi_off_raw_tx_fmac_stop(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;

	if (!fmac_dev_ctx) {
		LOG_ERR("%s: Invalid device context",
				      __func__);
		goto out;
	}

	if (fmac_dev_ctx->op_mode != NRF_WIFI_OP_MODE_OFF_RAW_TX) {
		LOG_ERR("%s: Invalid op mode",
				      __func__);
		goto out;
	}

	status = umac_cmd_off_raw_tx_ctrl(fmac_dev_ctx, 0);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		LOG_ERR("%s: umac_cmd_offload_raw_tx_ctrl failed", __func__);
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
		LOG_ERR("%s: Invalid op mode",
				      __func__);
		goto out;
	}

	if (fmac_dev_ctx->stats_req == true) {
		LOG_ERR("%s: Stats request already pending",
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
		k_msleep(1);
		count++;
	} while ((fmac_dev_ctx->stats_req == true) &&
		 (count < NRF_WIFI_FMAC_STATS_RECV_TIMEOUT));

	if (count == NRF_WIFI_FMAC_STATS_RECV_TIMEOUT) {
		LOG_ERR("%s: Timed out",
				      __func__);
		goto out;
	}

	status = NRF_WIFI_STATUS_SUCCESS;
out:
	return status;
}
