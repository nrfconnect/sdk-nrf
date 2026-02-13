/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @brief File containing command specific definitions for the
 * FMAC IF Layer of the Wi-Fi driver.
 */

#include "common/hal_api_common.h"

#include <nrf71_wifi_ctrl.h>
#include "common/fmac_structs_common.h"
#include "common/fmac_util.h"

struct host_rpu_msg *umac_cmd_alloc(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
				    int type,
				    int len)
{
	struct host_rpu_msg *umac_cmd = NULL;

	umac_cmd = nrf_wifi_osal_mem_zalloc(sizeof(*umac_cmd) + len);

	if (!umac_cmd) {
		nrf_wifi_osal_log_err("%s: Failed to allocate UMAC cmd",
				      __func__);
		goto out;
	}

	umac_cmd->type = type;
	umac_cmd->hdr.len = sizeof(*umac_cmd) + len;

out:
	return umac_cmd;
}


enum nrf_wifi_status umac_cmd_cfg(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
				  void *params,
				  int len)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct host_rpu_msg *umac_cmd = NULL;

	if (!fmac_dev_ctx->fw_init_done) {
		struct nrf_wifi_umac_hdr *umac_hdr = NULL;

		umac_hdr = (struct nrf_wifi_umac_hdr *)params;
		nrf_wifi_osal_log_err("%s: UMAC buff config not yet done(%d)",
				      __func__,
				      umac_hdr->cmd_evnt);
		goto out;
	}

	umac_cmd = umac_cmd_alloc(fmac_dev_ctx,
				  NRF_WIFI_HOST_RPU_MSG_TYPE_UMAC,
				  len);

	if (!umac_cmd) {
		nrf_wifi_osal_log_err("%s: umac_cmd_alloc failed",
				      __func__);
		goto out;
	}

	nrf_wifi_osal_mem_cpy(umac_cmd->msg,
			      params,
			      len);

	status = nrf_wifi_hal_ctrl_cmd_send(fmac_dev_ctx->hal_dev_ctx,
					    umac_cmd,
					    (sizeof(*umac_cmd) + len));

	nrf_wifi_osal_log_dbg("%s: Command %d sent to RPU",
			      __func__,
			      ((struct nrf_wifi_umac_hdr *)params)->cmd_evnt);

out:
	return status;
}


enum nrf_wifi_status umac_cmd_deinit(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct host_rpu_msg *umac_cmd = NULL;
	struct nrf_wifi_cmd_sys_deinit *umac_cmd_data = NULL;
	unsigned int len = 0;

	len = sizeof(*umac_cmd_data);
	umac_cmd = umac_cmd_alloc(fmac_dev_ctx,
				  NRF_WIFI_HOST_RPU_MSG_TYPE_SYSTEM,
				  len);
	if (!umac_cmd) {
		nrf_wifi_osal_log_err("%s: umac_cmd_alloc failed",
				      __func__);
		goto out;
	}
	umac_cmd_data = (struct nrf_wifi_cmd_sys_deinit *)(umac_cmd->msg);
	umac_cmd_data->sys_head.cmd_event = NRF_WIFI_CMD_DEINIT;
	umac_cmd_data->sys_head.len = len;
	status = nrf_wifi_hal_ctrl_cmd_send(fmac_dev_ctx->hal_dev_ctx,
					    umac_cmd,
					    (sizeof(*umac_cmd) + len));
out:
	return status;
}

enum nrf_wifi_status umac_cmd_srcoex(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
				     void *cmd,
				     unsigned int cmd_len)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct host_rpu_msg *umac_cmd = NULL;
	struct nrf_wifi_cmd_coex_config *umac_cmd_data = NULL;
	int len = 0;

	len = sizeof(*umac_cmd_data)+cmd_len;

	umac_cmd = umac_cmd_alloc(fmac_dev_ctx,
				  NRF_WIFI_HOST_RPU_MSG_TYPE_SYSTEM,
				  len);

	if (!umac_cmd) {
		nrf_wifi_osal_log_err("%s: umac_cmd_alloc failed",
				      __func__);
		goto out;
	}

	umac_cmd_data = (struct nrf_wifi_cmd_coex_config *)(umac_cmd->msg);

	umac_cmd_data->sys_head.cmd_event = NRF_WIFI_CMD_SRCOEX;
	umac_cmd_data->sys_head.len = len;
	umac_cmd_data->coex_config_info.len = cmd_len;

	nrf_wifi_osal_mem_cpy(umac_cmd_data->coex_config_info.coex_cmd,
			      cmd,
			      cmd_len);

	status = nrf_wifi_hal_ctrl_cmd_send(fmac_dev_ctx->hal_dev_ctx,
					    umac_cmd,
					    (sizeof(*umac_cmd) + len));
out:
	return status;


}


enum nrf_wifi_status umac_cmd_prog_stats_reset(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct host_rpu_msg *umac_cmd = NULL;
	struct nrf_wifi_cmd_reset_stats *umac_cmd_data = NULL;
	int len = 0;

	len = sizeof(*umac_cmd_data);

	umac_cmd = umac_cmd_alloc(fmac_dev_ctx,
				  NRF_WIFI_HOST_RPU_MSG_TYPE_SYSTEM,
				  len);
	if (!umac_cmd) {
		nrf_wifi_osal_log_err("%s: umac_cmd_alloc failed",
				      __func__);
		goto out;
	}

	umac_cmd_data = (struct nrf_wifi_cmd_reset_stats *)(umac_cmd->msg);

	umac_cmd_data->sys_head.cmd_event = NRF_WIFI_CMD_RESET_STATISTICS;
	umac_cmd_data->sys_head.len = len;

	status = nrf_wifi_hal_ctrl_cmd_send(fmac_dev_ctx->hal_dev_ctx,
					    umac_cmd,
					    (sizeof(*umac_cmd) + len));

out:
	return status;
}
