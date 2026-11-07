/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Host driver debug statistics for the nRF71 Wi-Fi driver.
 */

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/util.h>

#include <common/fmac_structs_common.h>
#include <system/core.h>
#include <system/drv_dbg_stats.h>

static const char * const err_names[] = {
	[NRF_WIFI_DRV_DBG_ERR_CMD_ALLOC] = "cmd_alloc_fail",
	[NRF_WIFI_DRV_DBG_ERR_CMD_NOT_READY] = "cmd_before_fw_init",
	[NRF_WIFI_DRV_DBG_ERR_CMD_SEND] = "cmd_send_fail",
	[NRF_WIFI_DRV_DBG_ERR_EVT_INVALID_VIF] = "evt_invalid_vif",
	[NRF_WIFI_DRV_DBG_ERR_EVT_NO_VIF] = "evt_no_host_vif",
	[NRF_WIFI_DRV_DBG_ERR_EVT_UNKNOWN] = "evt_unknown",
	[NRF_WIFI_DRV_DBG_ERR_EVT_DATA] = "evt_data_fail",
	[NRF_WIFI_DRV_DBG_ERR_FW_UMAC] = "fw_umac_error",
	[NRF_WIFI_DRV_DBG_ERR_FW_LMAC] = "fw_lmac_error",
};

BUILD_ASSERT(ARRAY_SIZE(err_names) == NRF_WIFI_DRV_DBG_ERR_MAX);

static const char * const tmo_names[] = {
	[NRF_WIFI_DRV_DBG_TMO_FW_INIT] = "fw_init",
	[NRF_WIFI_DRV_DBG_TMO_FW_STATS] = "fw_stats",
	[NRF_WIFI_DRV_DBG_TMO_IPC_TX] = "ipc_tx",
	[NRF_WIFI_DRV_DBG_TMO_SCAN] = "scan",
	[NRF_WIFI_DRV_DBG_TMO_SCAN_ABORT] = "scan_abort",
	[NRF_WIFI_DRV_DBG_TMO_GET_STATION] = "get_station",
	[NRF_WIFI_DRV_DBG_TMO_GET_INTERFACE] = "get_interface",
	[NRF_WIFI_DRV_DBG_TMO_GET_CONN_INFO] = "get_conn_info",
	[NRF_WIFI_DRV_DBG_TMO_MGMT_TX] = "mgmt_tx",
	[NRF_WIFI_DRV_DBG_TMO_IF_STATE] = "if_state",
	[NRF_WIFI_DRV_DBG_TMO_IF_TYPE] = "if_type",
	[NRF_WIFI_DRV_DBG_TMO_CARRIER] = "carrier",
	[NRF_WIFI_DRV_DBG_TMO_PS_INFO] = "ps_info",
	[NRF_WIFI_DRV_DBG_TMO_SET_CHANNEL] = "set_channel",
};

BUILD_ASSERT(ARRAY_SIZE(tmo_names) == NRF_WIFI_DRV_DBG_TMO_MAX);

static struct nrf_wifi_ctx_zep *fmac_to_ctx(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx)
{
	/* In system mode the FMAC device is added with the Zephyr RPU context
	 * as its OS device context.
	 */
	return fmac_dev_ctx ? fmac_dev_ctx->os_dev_ctx : NULL;
}

void nrf_wifi_drv_dbg_err(struct nrf_wifi_ctx_zep *ctx, enum nrf_wifi_drv_dbg_err err)
{
	if (ctx && err < NRF_WIFI_DRV_DBG_ERR_MAX) {
		atomic_inc(&ctx->drv_dbg_stats.err[err]);
	}
}

void nrf_wifi_drv_dbg_fmac_err(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
			       enum nrf_wifi_drv_dbg_err err)
{
	nrf_wifi_drv_dbg_err(fmac_to_ctx(fmac_dev_ctx), err);
}

void nrf_wifi_drv_dbg_tmo(struct nrf_wifi_ctx_zep *ctx, enum nrf_wifi_drv_dbg_tmo tmo)
{
	if (ctx && tmo < NRF_WIFI_DRV_DBG_TMO_MAX) {
		atomic_inc(&ctx->drv_dbg_stats.tmo[tmo]);
	}
}

void nrf_wifi_drv_dbg_fmac_tmo(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
			       enum nrf_wifi_drv_dbg_tmo tmo)
{
	nrf_wifi_drv_dbg_tmo(fmac_to_ctx(fmac_dev_ctx), tmo);
}

static void show_table(const struct shell *sh, const char *title, const char * const *names,
		       atomic_t *vals, size_t num)
{
	shell_print(sh, "%s", title);
	shell_print(sh, "======================");
	for (size_t i = 0; i < num; i++) {
		shell_print(sh, "%s: %ld", names[i], (long)atomic_get(&vals[i]));
	}
	shell_print(sh, "");
}

void nrf_wifi_drv_dbg_stats_show(const struct shell *sh, struct nrf_wifi_ctx_zep *ctx)
{
	struct nrf_wifi_drv_dbg_stats *stats = &ctx->drv_dbg_stats;

	show_table(sh, "Driver control path errors", err_names, stats->err,
		   ARRAY_SIZE(stats->err));
	show_table(sh, "Driver RPU response timeouts", tmo_names, stats->tmo,
		   ARRAY_SIZE(stats->tmo));
}

void nrf_wifi_drv_dbg_stats_clear(struct nrf_wifi_ctx_zep *ctx)
{
	struct nrf_wifi_drv_dbg_stats *stats = &ctx->drv_dbg_stats;

	for (size_t i = 0; i < ARRAY_SIZE(stats->err); i++) {
		atomic_clear(&stats->err[i]);
	}

	for (size_t i = 0; i < ARRAY_SIZE(stats->tmo); i++) {
		atomic_clear(&stats->tmo[i]);
	}
}
