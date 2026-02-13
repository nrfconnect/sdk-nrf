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
#include "common/fmac_api_common.h"
#include "common/fmac_util.h"
#include "common/fmac_cmd_common.h"
#include "util.h"


void nrf_wifi_fmac_deinit(struct nrf_wifi_fmac_priv *fpriv)
{
	nrf_wifi_hal_deinit(fpriv->hpriv);

	nrf_wifi_osal_mem_free(fpriv);
}


void nrf_wifi_fmac_dev_rem(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx)
{
	nrf_wifi_hal_dev_rem(fmac_dev_ctx->hal_dev_ctx);

	nrf_wifi_osal_mem_free(fmac_dev_ctx);
}



enum nrf_wifi_status nrf_wifi_fmac_ver_get(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
					  unsigned int *fw_ver)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	*fw_ver = 0x01020304;
	status = NRF_WIFI_STATUS_SUCCESS;
	return status;
}


enum nrf_wifi_status nrf_wifi_fmac_get_reg(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
					   struct nrf_wifi_fmac_reg_info *reg_info)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_umac_cmd_get_reg *get_reg_cmd = NULL;
	unsigned int count = 0;

	if (!fmac_dev_ctx || !reg_info) {
		nrf_wifi_osal_log_err("%s: Invalid parameters",
				      __func__);
		goto err;
	}

	nrf_wifi_osal_log_dbg("%s: Get regulatory information", __func__);

	get_reg_cmd = nrf_wifi_osal_mem_zalloc(sizeof(*get_reg_cmd));

	if (!get_reg_cmd) {
		nrf_wifi_osal_log_err("%s: Unable to allocate memory",
				      __func__);
		goto err;
	}

	get_reg_cmd->umac_hdr.cmd_evnt = NRF_WIFI_UMAC_CMD_GET_REG;
	get_reg_cmd->umac_hdr.ids.valid_fields = 0;

	fmac_dev_ctx->alpha2_valid = false;
	fmac_dev_ctx->reg_chan_info = reg_info->reg_chan_info;

	status = umac_cmd_cfg(fmac_dev_ctx,
			      get_reg_cmd,
			      sizeof(*get_reg_cmd));

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		nrf_wifi_osal_log_err("%s: Failed to get regulatory information",	__func__);
		goto err;
	}

	do {
		nrf_wifi_osal_sleep_ms(100);
	} while (count++ < 100 && !fmac_dev_ctx->alpha2_valid);

	if (!fmac_dev_ctx->alpha2_valid) {
		nrf_wifi_osal_log_err("%s: Failed to get regulatory information",
				      __func__);
		goto err;
	}

	nrf_wifi_osal_mem_cpy(reg_info->alpha2,
			      fmac_dev_ctx->alpha2,
			      sizeof(reg_info->alpha2));

	reg_info->reg_chan_count = fmac_dev_ctx->reg_chan_count;

	status = NRF_WIFI_STATUS_SUCCESS;
err:
	if (get_reg_cmd) {
		nrf_wifi_osal_mem_free(get_reg_cmd);
	}
	return status;
}

enum nrf_wifi_status nrf_wifi_fmac_stats_reset(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	unsigned char count = 0;

	status = umac_cmd_prog_stats_reset(fmac_dev_ctx);
	if (status != NRF_WIFI_STATUS_SUCCESS) {
		goto out;
	}

	do {
		nrf_wifi_osal_sleep_ms(1);
	} while ((fmac_dev_ctx->stats_req == true) &&
		 (count++ < NRF_WIFI_FMAC_STATS_RECV_TIMEOUT));

	if (count == NRF_WIFI_FMAC_STATS_RECV_TIMEOUT) {
		nrf_wifi_osal_log_err("%s: Timed out",
				      __func__);
		goto out;
	}

	status = NRF_WIFI_STATUS_SUCCESS;

out:
	return status;
}


enum nrf_wifi_status nrf_wifi_fmac_conf_srcoex(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
					       void *cmd, unsigned int cmd_len)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;

	status = umac_cmd_srcoex(fmac_dev_ctx, cmd, cmd_len);

	return status;
}

enum nrf_wifi_status nrf_wifi_fmac_set_reg(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
					   struct nrf_wifi_fmac_reg_info *reg_info)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_cmd_req_set_reg *set_reg_cmd = NULL;
	unsigned int count = 0, max_count = NRF_WIFI_FMAC_REG_SET_TIMEOUT_MS / 20;
	enum nrf_wifi_reg_initiator exp_initiator = NRF_WIFI_REGDOM_SET_BY_USER;
	enum nrf_wifi_reg_type exp_reg_type = NRF_WIFI_REGDOM_TYPE_COUNTRY;
	char exp_alpha2[NRF_WIFI_COUNTRY_CODE_LEN] = {0};
	struct nrf_wifi_fmac_reg_info cur_reg_info = {0};
	struct nrf_wifi_event_regulatory_change *reg_change = NULL;

	if (!fmac_dev_ctx || !reg_info) {
		nrf_wifi_osal_log_err("%s: Invalid parameters",
				      __func__);
		goto out;
	}

	nrf_wifi_osal_log_dbg("%s: Setting regulatory information: %c%c", __func__,
			      reg_info->alpha2[0],
			      reg_info->alpha2[1]);

	/* No change event from UMAC for same regd */
	status = nrf_wifi_fmac_get_reg(fmac_dev_ctx, &cur_reg_info);
	if (status != NRF_WIFI_STATUS_SUCCESS) {
		nrf_wifi_osal_log_err("%s: Failed to get current regulatory information",
				      __func__);
		goto out;
	}

	if (nrf_wifi_osal_mem_cmp(cur_reg_info.alpha2,
				  reg_info->alpha2,
				  NRF_WIFI_COUNTRY_CODE_LEN) == 0) {
		nrf_wifi_osal_log_dbg("%s: Regulatory domain already set to %c%c",
				      __func__,
				      reg_info->alpha2[0],
				      reg_info->alpha2[1]);
		status = NRF_WIFI_STATUS_SUCCESS;
		goto out;
	}

	set_reg_cmd = nrf_wifi_osal_mem_zalloc(sizeof(*set_reg_cmd));

	if (!set_reg_cmd) {
		nrf_wifi_osal_log_err("%s: Unable to allocate memory",
				      __func__);
		goto out;
	}

	set_reg_cmd->umac_hdr.cmd_evnt = NRF_WIFI_UMAC_CMD_REQ_SET_REG;
	set_reg_cmd->umac_hdr.ids.valid_fields = 0;

	nrf_wifi_osal_mem_cpy(set_reg_cmd->nrf_wifi_alpha2,
			      reg_info->alpha2,
			      NRF_WIFI_COUNTRY_CODE_LEN);

	exp_alpha2[0] = reg_info->alpha2[0];
	exp_alpha2[1] = reg_info->alpha2[1];

	if (reg_info->alpha2[0] == '0' && reg_info->alpha2[1] == '0') {
		exp_reg_type = NRF_WIFI_REGDOM_TYPE_WORLD;
	}

	set_reg_cmd->valid_fields = NRF_WIFI_CMD_REQ_SET_REG_ALPHA2_VALID;

	/* New feature in rev B patch */
	if (reg_info->force) {
		set_reg_cmd->valid_fields |= NRF_WIFI_CMD_REQ_SET_REG_USER_REG_FORCE;
	}

	fmac_dev_ctx->reg_set_status = false;
	fmac_dev_ctx->waiting_for_reg_event = true;

	status = umac_cmd_cfg(fmac_dev_ctx,
			      set_reg_cmd,
			      sizeof(*set_reg_cmd));
	if (status != NRF_WIFI_STATUS_SUCCESS) {
		nrf_wifi_osal_log_err("%s: Failed to set regulatory information",
				      __func__);
		goto out;
	}

	fmac_dev_ctx->reg_set_status = false;
	nrf_wifi_osal_log_dbg("%s: Waiting for regulatory domain change event", __func__);
	while (!fmac_dev_ctx->reg_set_status && count++ <= max_count) {
		nrf_wifi_osal_sleep_ms(100);
	}

	if (!fmac_dev_ctx->reg_set_status) {
		nrf_wifi_osal_log_err("%s: Failed to set regulatory information",
				      __func__);
		status = NRF_WIFI_STATUS_FAIL;
		goto out;
	}

	fmac_dev_ctx->waiting_for_reg_event = false;
	reg_change = fmac_dev_ctx->reg_change;

	if (reg_change->intr != exp_initiator) {
		nrf_wifi_osal_log_err("%s: Non-user initiated reg domain change: exp: %d, got: %d",
				      __func__,
				      exp_initiator,
				      reg_change->intr);
		status = NRF_WIFI_STATUS_FAIL;
		goto out;
	}

	if (reg_change->regulatory_type != exp_reg_type) {
		nrf_wifi_osal_log_err("%s: Unexpected reg domain change: exp: %d, got: %d",
				      __func__,
				      exp_reg_type,
				      reg_change->regulatory_type);
		status = NRF_WIFI_STATUS_FAIL;
		goto out;
	}

	if ((reg_change->regulatory_type == NRF_WIFI_REGDOM_TYPE_COUNTRY) &&
		 nrf_wifi_osal_mem_cmp(reg_change->nrf_wifi_alpha2,
				       exp_alpha2,
				       NRF_WIFI_COUNTRY_CODE_LEN) != 0) {
		nrf_wifi_osal_log_err("%s: Unexpected alpha2 reg domain change: "
				      "exp: %c%c, got: %c%c",
				      __func__,
				      exp_alpha2[0],
				      exp_alpha2[1],
				      reg_change->nrf_wifi_alpha2[0],
				      reg_change->nrf_wifi_alpha2[1]);
		status = NRF_WIFI_STATUS_FAIL;
		goto out;
	}

out:
	if (set_reg_cmd) {
		nrf_wifi_osal_mem_free(set_reg_cmd);
	}

	if (reg_change) {
		nrf_wifi_osal_mem_free(reg_change);
		fmac_dev_ctx->reg_change = NULL;
	}

	return status;
}
