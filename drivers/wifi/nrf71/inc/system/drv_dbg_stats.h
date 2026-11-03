/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Host driver debug statistics for the nRF71 Wi-Fi driver.
 *
 * The counters live in the Zephyr OS layer context, so they survive an RPU
 * recovery, which re-creates the FMAC device context.
 */

#ifndef __SYSTEM_DRV_DBG_STATS_H__
#define __SYSTEM_DRV_DBG_STATS_H__

#include <zephyr/sys/atomic.h>

struct nrf_wifi_ctx_zep;
struct nrf_wifi_fmac_dev_ctx;
struct shell;

/**
 * @brief Host driver control path errors.
 */
enum nrf_wifi_drv_dbg_err {
	/** Allocating a command buffer failed. */
	NRF_WIFI_DRV_DBG_ERR_CMD_ALLOC,
	/** UMAC command issued before the firmware init done event. */
	NRF_WIFI_DRV_DBG_ERR_CMD_NOT_READY,
	/** Handing a command to the IPC transport failed. */
	NRF_WIFI_DRV_DBG_ERR_CMD_SEND,
	/** UMAC event carried an out of range interface index. */
	NRF_WIFI_DRV_DBG_ERR_EVT_INVALID_VIF,
	/** UMAC event for an interface with no host VIF context. */
	NRF_WIFI_DRV_DBG_ERR_EVT_NO_VIF,
	/** Unknown system event. */
	NRF_WIFI_DRV_DBG_ERR_EVT_UNKNOWN,
	/** Processing a data path (RX, TX done, carrier) event failed. */
	NRF_WIFI_DRV_DBG_ERR_EVT_DATA,
	/** Error stats event reported by the UMAC. */
	NRF_WIFI_DRV_DBG_ERR_FW_UMAC,
	/** Error stats event reported by the LMAC. */
	NRF_WIFI_DRV_DBG_ERR_FW_LMAC,
	NRF_WIFI_DRV_DBG_ERR_MAX,
};

/**
 * @brief Host driver waits on the RPU that timed out.
 */
enum nrf_wifi_drv_dbg_tmo {
	/** Firmware init done event. */
	NRF_WIFI_DRV_DBG_TMO_FW_INIT,
	/** Firmware statistics events (stats, debug stats, UMAC internal stats). */
	NRF_WIFI_DRV_DBG_TMO_FW_STATS,
	/** IPC command queue stayed busy. */
	NRF_WIFI_DRV_DBG_TMO_IPC_TX,
	/** Scan done event. */
	NRF_WIFI_DRV_DBG_TMO_SCAN,
	/** Scan abort response. */
	NRF_WIFI_DRV_DBG_TMO_SCAN_ABORT,
	/** Get station response. */
	NRF_WIFI_DRV_DBG_TMO_GET_STATION,
	/** Get interface response. */
	NRF_WIFI_DRV_DBG_TMO_GET_INTERFACE,
	/** Get connection info response. */
	NRF_WIFI_DRV_DBG_TMO_GET_CONN_INFO,
	/** Management frame TX cookie response. */
	NRF_WIFI_DRV_DBG_TMO_MGMT_TX,
	/** Interface state change event. */
	NRF_WIFI_DRV_DBG_TMO_IF_STATE,
	/** Interface type change event. */
	NRF_WIFI_DRV_DBG_TMO_IF_TYPE,
	/** Carrier state event. */
	NRF_WIFI_DRV_DBG_TMO_CARRIER,
	/** Power save info event. */
	NRF_WIFI_DRV_DBG_TMO_PS_INFO,
	/** Set channel done event. */
	NRF_WIFI_DRV_DBG_TMO_SET_CHANNEL,
	NRF_WIFI_DRV_DBG_TMO_MAX,
};

/**
 * @brief Host driver debug statistics.
 */
struct nrf_wifi_drv_dbg_stats {
	/** Control path errors, indexed by @ref nrf_wifi_drv_dbg_err. */
	atomic_t err[NRF_WIFI_DRV_DBG_ERR_MAX];
	/** RPU response timeouts, indexed by @ref nrf_wifi_drv_dbg_tmo. */
	atomic_t tmo[NRF_WIFI_DRV_DBG_TMO_MAX];
};

#ifdef CONFIG_NRF71_UTIL
/**
 * @brief Count a control path error.
 *
 * @param ctx Zephyr OS layer RPU context.
 * @param err Error to count.
 */
void nrf_wifi_drv_dbg_err(struct nrf_wifi_ctx_zep *ctx, enum nrf_wifi_drv_dbg_err err);

/**
 * @brief Count a control path error from the FMAC layer.
 *
 * @param fmac_dev_ctx FMAC device context.
 * @param err Error to count.
 */
void nrf_wifi_drv_dbg_fmac_err(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
			       enum nrf_wifi_drv_dbg_err err);

/**
 * @brief Count an RPU response timeout.
 *
 * @param ctx Zephyr OS layer RPU context.
 * @param tmo Timeout to count.
 */
void nrf_wifi_drv_dbg_tmo(struct nrf_wifi_ctx_zep *ctx, enum nrf_wifi_drv_dbg_tmo tmo);

/**
 * @brief Count an RPU response timeout from the FMAC layer.
 *
 * @param fmac_dev_ctx FMAC device context.
 * @param tmo Timeout to count.
 */
void nrf_wifi_drv_dbg_fmac_tmo(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
			       enum nrf_wifi_drv_dbg_tmo tmo);

/**
 * @brief Print the debug statistics on a shell.
 *
 * @param sh Shell instance.
 * @param ctx Zephyr OS layer RPU context.
 */
void nrf_wifi_drv_dbg_stats_show(const struct shell *sh, struct nrf_wifi_ctx_zep *ctx);

/**
 * @brief Reset all the debug statistics.
 *
 * @param ctx Zephyr OS layer RPU context.
 */
void nrf_wifi_drv_dbg_stats_clear(struct nrf_wifi_ctx_zep *ctx);
#else
static inline void nrf_wifi_drv_dbg_err(struct nrf_wifi_ctx_zep *ctx,
					enum nrf_wifi_drv_dbg_err err)
{
}

static inline void nrf_wifi_drv_dbg_fmac_err(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
					     enum nrf_wifi_drv_dbg_err err)
{
}

static inline void nrf_wifi_drv_dbg_tmo(struct nrf_wifi_ctx_zep *ctx,
					enum nrf_wifi_drv_dbg_tmo tmo)
{
}

static inline void nrf_wifi_drv_dbg_fmac_tmo(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
					     enum nrf_wifi_drv_dbg_tmo tmo)
{
}
#endif /* CONFIG_NRF71_UTIL */

#endif /* __SYSTEM_DRV_DBG_STATS_H__ */
