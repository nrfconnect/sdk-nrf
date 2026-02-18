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
#include "system/fmac_api.h"
#include "system/hal_api.h"
#include "system/fmac_structs.h"
#include "common/fmac_util.h"
#include "system/fmac_peer.h"
#include "system/fmac_vif.h"
#include "system/fmac_tx.h"
#include "system/fmac_rx.h"
#include "system/fmac_cmd.h"
#include "system/fmac_event.h"
#include "util.h"


static unsigned char nrf_wifi_fmac_vif_idx_get(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx)
{
	unsigned char i = 0;
	struct nrf_wifi_sys_fmac_dev_ctx *sys_dev_ctx = NULL;

	sys_dev_ctx = wifi_dev_priv(fmac_dev_ctx);

	for (i = 0; i < MAX_NUM_VIFS; i++) {
		if (sys_dev_ctx->vif_ctx[i] == NULL) {
			break;
		}
	}

	return i;
}


#ifdef NRF71_DATA_TX
static enum nrf_wifi_status nrf_wifi_sys_fmac_init_tx(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx)
{
	struct nrf_wifi_fmac_priv *fpriv = NULL;
	struct nrf_wifi_sys_fmac_priv *sys_fpriv = NULL;
	struct nrf_wifi_sys_fmac_dev_ctx *sys_dev_ctx = NULL;
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	unsigned int size = 0;

	fpriv = fmac_dev_ctx->fpriv;

	sys_fpriv = wifi_fmac_priv(fpriv);
	sys_dev_ctx = wifi_dev_priv(fmac_dev_ctx);

	size = (sys_fpriv->num_tx_tokens *
		sys_fpriv->data_config.max_tx_aggregation *
		sizeof(struct nrf_wifi_fmac_buf_map_info));

	sys_dev_ctx->tx_buf_info = nrf_wifi_osal_data_mem_zalloc(size);

	if (!sys_dev_ctx->tx_buf_info) {
		nrf_wifi_osal_log_err("%s: No space for TX buf info",
				      __func__);
		goto out;
	}

	status = tx_init(fmac_dev_ctx);

out:
	return status;
}


static void nrf_wifi_sys_fmac_deinit_tx(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx)
{
	struct nrf_wifi_fmac_priv *fpriv = NULL;
	struct nrf_wifi_sys_fmac_dev_ctx *sys_dev_ctx = NULL;

	sys_dev_ctx = wifi_dev_priv(fmac_dev_ctx);

	fpriv = fmac_dev_ctx->fpriv;

	tx_deinit(fmac_dev_ctx);

	nrf_wifi_osal_data_mem_free(sys_dev_ctx->tx_buf_info);
}

#endif /* NRF71_DATA_TX */

static enum nrf_wifi_status nrf_wifi_sys_fmac_init_rx(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx)
{
	struct nrf_wifi_fmac_priv *fpriv = NULL;
	struct nrf_wifi_sys_fmac_priv *sys_fpriv = NULL;
	struct nrf_wifi_sys_fmac_dev_ctx *sys_dev_ctx = NULL;
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	unsigned int size = 0;
	unsigned int desc_id = 0;

	fpriv = fmac_dev_ctx->fpriv;
	sys_fpriv = wifi_fmac_priv(fpriv);
	sys_dev_ctx = wifi_dev_priv(fmac_dev_ctx);

	size = (sys_fpriv->num_rx_bufs * sizeof(struct nrf_wifi_fmac_buf_map_info));

	sys_dev_ctx->rx_buf_info = nrf_wifi_osal_data_mem_zalloc(size);

	if (!sys_dev_ctx->rx_buf_info) {
		nrf_wifi_osal_log_err("%s: No space for RX buf info",
				      __func__);
		goto out;
	}

	for (desc_id = 0; desc_id < sys_fpriv->num_rx_bufs; desc_id++) {
		status = nrf_wifi_fmac_rx_cmd_send(fmac_dev_ctx,
						   NRF_WIFI_FMAC_RX_CMD_TYPE_INIT,
						   desc_id);

		if (status != NRF_WIFI_STATUS_SUCCESS) {
			nrf_wifi_osal_log_err("%s: RX init failed for desc_id = %d",
					      __func__,
					      desc_id);
			goto out;
		}
	}
#ifdef NRF71_RX_WQ_ENABLED
	sys_dev_ctx->rx_tasklet = nrf_wifi_osal_tasklet_alloc(NRF_WIFI_TASKLET_TYPE_RX);
	if (!sys_dev_ctx->rx_tasklet) {
		nrf_wifi_osal_log_err("%s: No space for RX tasklet",
				      __func__);
		status = NRF_WIFI_STATUS_FAIL;
		goto out;
	}

	sys_dev_ctx->rx_tasklet_event_q = nrf_wifi_utils_q_alloc();
	if (!sys_dev_ctx->rx_tasklet_event_q) {
		nrf_wifi_osal_log_err("%s: No space for RX tasklet event queue",
				      __func__);
		status = NRF_WIFI_STATUS_FAIL;
		goto out;
	}

	nrf_wifi_osal_tasklet_init(sys_dev_ctx->rx_tasklet,
				   nrf_wifi_fmac_rx_tasklet,
				   (unsigned long)fmac_dev_ctx);
#endif /* NRF71_RX_WQ_ENABLED */
out:
	return status;
}


static enum nrf_wifi_status nrf_wifi_sys_fmac_deinit_rx(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx)
{
	struct nrf_wifi_fmac_priv *fpriv = NULL;
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_sys_fmac_dev_ctx *sys_dev_ctx = NULL;
	struct nrf_wifi_sys_fmac_priv *sys_fpriv = NULL;
	unsigned int desc_id = 0;

	fpriv = fmac_dev_ctx->fpriv;
	sys_fpriv = wifi_fmac_priv(fpriv);
	sys_dev_ctx = wifi_dev_priv(fmac_dev_ctx);

#ifdef NRF71_RX_WQ_ENABLED
	nrf_wifi_osal_tasklet_free(sys_dev_ctx->rx_tasklet);
	nrf_wifi_utils_q_free(sys_dev_ctx->rx_tasklet_event_q);
#endif /* NRF71_RX_WQ_ENABLED */

	for (desc_id = 0; desc_id < sys_fpriv->num_rx_bufs; desc_id++) {
		status = nrf_wifi_fmac_rx_cmd_send(fmac_dev_ctx,
						   NRF_WIFI_FMAC_RX_CMD_TYPE_DEINIT,
						   desc_id);

		if (status != NRF_WIFI_STATUS_SUCCESS) {
			nrf_wifi_osal_log_err("%s: RX deinit failed for desc_id = %d",
					      __func__,
					      desc_id);
			goto out;
		}
	}

	nrf_wifi_osal_data_mem_free(sys_dev_ctx->rx_buf_info);

	sys_dev_ctx->rx_buf_info = NULL;
out:
	return status;
}

static enum nrf_wifi_status
nrf_wifi_sys_fmac_fw_init(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx, unsigned int *rf_params_addr,
			  unsigned int vtf_buffer_start_address,
#ifdef NRF_WIFI_LOW_POWER
			  int sleep_type,
#endif /* NRF_WIFI_LOW_POWER */
			  unsigned int phy_calib, unsigned char op_band, bool beamforming,
			  struct nrf_wifi_tx_pwr_ctrl_params *tx_pwr_ctrl,
			  struct nrf_wifi_board_params *board_params, unsigned char *country_code)
{
	unsigned long start_time_us = 0;
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_sys_fmac_priv *sys_fpriv = NULL;
#ifdef NRF_WIFI_RX_BUFF_PROG_UMAC
	struct nrf_wifi_rx_buf *rx_buf_ipc = NULL, *rx_buf_info_iter = NULL;
	unsigned int desc_id = 0;
	unsigned int buf_addr = 0;
#endif /*NRF_WIFI_RX_BUFF_PROG_UMAC */

	sys_fpriv = wifi_fmac_priv(fmac_dev_ctx->fpriv);

#ifdef NRF71_DATA_TX
	status = nrf_wifi_sys_fmac_init_tx(fmac_dev_ctx);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		nrf_wifi_osal_log_err("%s: Init TX failed",
				      __func__);
		goto out;
	}
#endif /* NRF71_DATA_TX */

	status = nrf_wifi_sys_fmac_init_rx(fmac_dev_ctx);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		nrf_wifi_osal_log_err("%s: Init RX failed",
				      __func__);
#ifdef NRF71_DATA_TX
		nrf_wifi_sys_fmac_deinit_tx(fmac_dev_ctx);
#endif
		goto out;
	}

	status = umac_cmd_sys_init(
		fmac_dev_ctx, rf_params_addr, vtf_buffer_start_address, &sys_fpriv->data_config,
#ifdef NRF_WIFI_LOW_POWER
		sleep_type,
#endif /* NRF_WIFI_LOW_POWER */
		phy_calib, op_band, beamforming, tx_pwr_ctrl, board_params, country_code);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		nrf_wifi_osal_log_err("%s: UMAC init failed",
				      __func__);
		nrf_wifi_sys_fmac_deinit_rx(fmac_dev_ctx);
#ifdef NRF71_DATA_TX
		nrf_wifi_sys_fmac_deinit_tx(fmac_dev_ctx);
#endif /* NRF71_DATA_TX */
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
		nrf_wifi_sys_fmac_deinit_rx(fmac_dev_ctx);
#ifdef NRF71_DATA_TX
		nrf_wifi_sys_fmac_deinit_tx(fmac_dev_ctx);
#endif /* NRF71_DATA_TX */
		status = NRF_WIFI_STATUS_FAIL;
		goto out;
	}

#ifdef NRF_WIFI_RX_BUFF_PROG_UMAC
	rx_buf_ipc = nrf_wifi_osal_mem_zalloc(sys_fpriv->num_rx_bufs *
					      sizeof(struct nrf_wifi_rx_buf));

	rx_buf_info_iter = rx_buf_ipc;

	for (desc_id = 0; desc_id < sys_fpriv->num_rx_bufs; desc_id++) {
		buf_addr = (unsigned int)nrf_wifi_fmac_get_rx_buf_map_addr(fmac_dev_ctx,
									  desc_id);
		if (buf_addr) {
			rx_buf_info_iter->skb_pointer = buf_addr;
			nrf_wifi_osal_log_dbg("%s: RX buf mapped desc_id=%d buf_addr=%p",
					      __func__, desc_id, (void *)buf_addr);
			rx_buf_info_iter->skb_desc_no = desc_id;
			rx_buf_info_iter++;
		} else {
			nrf_wifi_osal_log_err("%s: UMAC rx buff not mapped for desc_id=%d",
					      __func__, desc_id);
			status = NRF_WIFI_STATUS_FAIL;
			goto out;
		}
	}
	status = nrf_wifi_fmac_prog_rx_buf_info(fmac_dev_ctx,
						rx_buf_ipc,
						sys_fpriv->num_rx_bufs);
	if (status != NRF_WIFI_STATUS_SUCCESS) {
		nrf_wifi_osal_log_err("%s: UMAC rx buff programming failed",
				      __func__);
		status = NRF_WIFI_STATUS_FAIL;
		goto out;
	} else {
		nrf_wifi_osal_log_dbg("%s: UMAC rx buff programmed num_buffs=%d",
				      __func__, sys_fpriv->num_rx_bufs);
		nrf_wifi_osal_mem_free(rx_buf_ipc);
	}
#endif /*NRF_WIFI_RX_BUFF_PROG_UMAC */
	status = NRF_WIFI_STATUS_SUCCESS;

out:
	return status;
}

static void nrf_wifi_sys_fmac_fw_deinit(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx)
{
	/* TODO: To be activated once UMAC supports deinit */
#ifdef NOTYET
	unsigned long start_time_us = 0;
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;

	status = umac_cmd_deinit(fmac_dev_ctx);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		nrf_wifi_osal_log_err("%s: UMAC deinit failed",
				      __func__);
		goto out;
	}

	start_time_us = nrf_wifi_osal_time_get_curr_us();

	while (!fmac_dev_ctx->fw_deinit_done) {
#define MAX_DEINIT_WAIT (5 * 1000 * 1000)
		nrf_wifi_osal_sleep_ms(1);
		if (nrf_wifi_osal_time_elapsed_us(start_time_us) >= MAX_DEINIT_WAIT) {
			break;
		}
	}

	if (!fmac_dev_ctx->fw_deinit_done) {
		nrf_wifi_osal_log_err("%s: UMAC deinit timed out",
				      __func__);
		status = NRF_WIFI_STATUS_FAIL;
		goto out;
	}
out:
#endif /* NOTYET */
	nrf_wifi_sys_fmac_deinit_rx(fmac_dev_ctx);
#ifdef NRF71_DATA_TX
	nrf_wifi_sys_fmac_deinit_tx(fmac_dev_ctx);
#endif /* NRF71_DATA_TX */

}

struct nrf_wifi_fmac_dev_ctx *nrf_wifi_sys_fmac_dev_add(struct nrf_wifi_fmac_priv *fpriv,
							void *os_dev_ctx)
{
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;
	struct nrf_wifi_sys_fmac_dev_ctx *sys_fmac_dev_ctx = NULL;
#ifdef NRF71_DATA_TX
	struct nrf_wifi_sys_fmac_priv *sys_fpriv = NULL;
#endif /* NRF71_DATA_TX */

	if (!fpriv || !os_dev_ctx) {
		return NULL;
	}

	if (fpriv->op_mode != NRF_WIFI_OP_MODE_SYS) {
		nrf_wifi_osal_log_err("%s: Invalid op mode",
				      __func__);
		goto out;
	}

	fmac_dev_ctx = nrf_wifi_osal_mem_zalloc(sizeof(*fmac_dev_ctx) + sizeof(*sys_fmac_dev_ctx));

	if (!fmac_dev_ctx) {
		nrf_wifi_osal_log_err("%s: Unable to allocate fmac_dev_ctx",
				      __func__);
		goto out;
	}

	fmac_dev_ctx->fpriv = fpriv;
	fmac_dev_ctx->os_dev_ctx = os_dev_ctx;

	fmac_dev_ctx->hal_dev_ctx = nrf_wifi_sys_hal_dev_add(fpriv->hpriv,
							     fmac_dev_ctx);

	if (!fmac_dev_ctx->hal_dev_ctx) {
		nrf_wifi_osal_log_err("%s: nrf_wifi_sys_hal_dev_add failed",
				      __func__);

		nrf_wifi_osal_mem_free(fmac_dev_ctx);
		fmac_dev_ctx = NULL;
		goto out;
	}
#ifdef NRF71_DATA_TX

	sys_fpriv = wifi_fmac_priv(fpriv);
	fpriv->hpriv->cfg_params.max_ampdu_len_per_token = sys_fpriv->max_ampdu_len_per_token;
#endif /* NRF71_DATA_TX */

	fmac_dev_ctx->op_mode = NRF_WIFI_OP_MODE_SYS;
out:
	return fmac_dev_ctx;
}

enum nrf_wifi_status
nrf_wifi_sys_fmac_dev_init(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
#ifdef NRF_WIFI_LOW_POWER
			   int sleep_type,
#endif /* NRF_WIFI_LOW_POWER */
			   unsigned int phy_calib, unsigned char op_band, bool beamforming,
			   struct nrf_wifi_tx_pwr_ctrl_params *tx_pwr_ctrl_params,
			   struct nrf_wifi_tx_pwr_ceil_params *tx_pwr_ceil_params,
			   struct nrf_wifi_board_params *board_params, unsigned char *country_code,
			   unsigned int *rf_params_addr, unsigned int vtf_buffer_start_address)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;

	if (!fmac_dev_ctx) {
		nrf_wifi_osal_log_err("%s: Invalid device context",
				      __func__);
		goto out;
	}

	if (fmac_dev_ctx->op_mode != NRF_WIFI_OP_MODE_SYS) {
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

	status = nrf_wifi_sys_fmac_fw_init(fmac_dev_ctx, rf_params_addr, vtf_buffer_start_address,
#ifdef NRF_WIFI_LOW_POWER
					   sleep_type,
#endif /* NRF_WIFI_LOW_POWER */
					   phy_calib, op_band, beamforming, tx_pwr_ctrl_params,
					   board_params, country_code);

	if (status == NRF_WIFI_STATUS_FAIL) {
		nrf_wifi_osal_log_err("%s: nrf_wifi_sys_fmac_fw_init failed",
				      __func__);
		goto out;
	}
out:
	return status;
}

#ifdef NRF71_SR_COEX_SLEEP_CTRL_GPIO_CTRL
enum nrf_wifi_status nrf_wifi_coex_config_sleep_ctrl_gpio_ctrl(
		struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
		unsigned int alt_swctrl1_function_bt_coex_status1,
		unsigned int invert_bt_coex_grant_output)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;

	status = nrf_wifi_hal_coex_config_sleep_ctrl_gpio_ctrl(fmac_dev_ctx->hal_dev_ctx,
				   alt_swctrl1_function_bt_coex_status1,
				   invert_bt_coex_grant_output);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		nrf_wifi_osal_log_err("%s: Failed to configure sleep control GPIO control register",
					  __func__);
	}
	return status;
}
#endif /* NRF71_SR_COEX_SLEEP_CTRL_GPIO_CTRL */

void nrf_wifi_sys_fmac_dev_deinit(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx)
{
	if (fmac_dev_ctx->op_mode != NRF_WIFI_OP_MODE_SYS) {
		nrf_wifi_osal_log_err("%s: Invalid op mode",
				      __func__);
		return;
	}

	nrf_wifi_hal_dev_deinit(fmac_dev_ctx->hal_dev_ctx);
	nrf_wifi_sys_fmac_fw_deinit(fmac_dev_ctx);
	nrf_wifi_osal_mem_free(fmac_dev_ctx->tx_pwr_ceil_params);
}

#ifdef NRF_WIFI_RPU_RECOVERY
enum nrf_wifi_status nrf_wifi_sys_fmac_rpu_recovery_callback(void *mac_dev_ctx,
							     void *event_data,
							     unsigned int len)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;
	struct nrf_wifi_sys_fmac_dev_ctx *sys_dev_ctx = NULL;
	struct nrf_wifi_fmac_priv *fpriv = NULL;
	struct nrf_wifi_sys_fmac_priv *sys_fpriv = NULL;

	fmac_dev_ctx = mac_dev_ctx;
	if (!fmac_dev_ctx) {
		goto out;
	}

	if (fmac_dev_ctx->op_mode != NRF_WIFI_OP_MODE_SYS) {
		nrf_wifi_osal_log_err("%s: Invalid op mode",
				      __func__);
		goto out;
	}

	fpriv = fmac_dev_ctx->fpriv;
	sys_fpriv = wifi_fmac_priv(fpriv);

	sys_dev_ctx = wifi_dev_priv(fmac_dev_ctx);
	if (!sys_dev_ctx) {
		nrf_wifi_osal_log_err("%s: Invalid device context",
				      __func__);
		goto out;
	}

	if (!sys_fpriv->callbk_fns.rpu_recovery_callbk_fn) {
		nrf_wifi_osal_log_err("%s: No RPU recovery callback function",
				      __func__);
		goto out;
	}

	/* Here we only care about FMAC, so, just use VIF0 */
	sys_fpriv->callbk_fns.rpu_recovery_callbk_fn(sys_dev_ctx->vif_ctx[0],
			event_data, len);

	status = NRF_WIFI_STATUS_SUCCESS;
out:
	return status;
}
#endif /* NRF_WIFI_RPU_RECOVERY */

struct nrf_wifi_fmac_priv *nrf_wifi_sys_fmac_init(struct nrf_wifi_data_config_params *data_config,
						  struct rx_buf_pool_params *rx_buf_pools,
						  struct nrf_wifi_fmac_callbk_fns *callbk_fns)
{
	struct nrf_wifi_fmac_priv *fpriv = NULL;
	struct nrf_wifi_sys_fmac_priv *sys_fpriv = NULL;
	struct nrf_wifi_hal_cfg_params hal_cfg_params;
	unsigned int pool_idx = 0;
	unsigned int desc = 0;

	fpriv = nrf_wifi_osal_mem_zalloc(sizeof(*fpriv) + sizeof(*sys_fpriv));

	if (!fpriv) {
		nrf_wifi_osal_log_err("%s: Unable to allocate fpriv",
				      __func__);
		goto out;
	}

	sys_fpriv = wifi_fmac_priv(fpriv);

	nrf_wifi_osal_mem_set(&hal_cfg_params,
			      0,
			      sizeof(hal_cfg_params));

	nrf_wifi_osal_mem_cpy(&sys_fpriv->callbk_fns,
			      callbk_fns,
			      sizeof(sys_fpriv->callbk_fns));

	nrf_wifi_osal_mem_cpy(&sys_fpriv->data_config,
			      data_config,
			      sizeof(sys_fpriv->data_config));

#ifdef NRF71_DATA_TX
	sys_fpriv->num_tx_tokens = NRF71_MAX_TX_TOKENS;
	sys_fpriv->num_tx_tokens_per_ac = (sys_fpriv->num_tx_tokens / NRF_WIFI_FMAC_AC_MAX);
	sys_fpriv->num_tx_tokens_spare = (sys_fpriv->num_tx_tokens % NRF_WIFI_FMAC_AC_MAX);
#endif /* NRF71_DATA_TX */
	nrf_wifi_osal_mem_cpy(sys_fpriv->rx_buf_pools,
			      rx_buf_pools,
			      sizeof(sys_fpriv->rx_buf_pools));

	for (pool_idx = 0; pool_idx < MAX_NUM_OF_RX_QUEUES; pool_idx++) {
		sys_fpriv->rx_desc[pool_idx] = desc;

		desc += sys_fpriv->rx_buf_pools[pool_idx].num_bufs;
	}

	sys_fpriv->num_rx_bufs = desc;

	hal_cfg_params.rx_buf_headroom_sz = RX_BUF_HEADROOM;
	hal_cfg_params.tx_buf_headroom_sz = TX_BUF_HEADROOM;
#ifdef NRF71_DATA_TX
	hal_cfg_params.max_tx_frms = (sys_fpriv->num_tx_tokens *
				      sys_fpriv->data_config.max_tx_aggregation);
#endif /* NRF71_DATA_TX */

	for (pool_idx = 0; pool_idx < MAX_NUM_OF_RX_QUEUES; pool_idx++) {
		hal_cfg_params.rx_buf_pool[pool_idx].num_bufs =
			sys_fpriv->rx_buf_pools[pool_idx].num_bufs;
		hal_cfg_params.rx_buf_pool[pool_idx].buf_sz =
			sys_fpriv->rx_buf_pools[pool_idx].buf_sz + RX_BUF_HEADROOM;
	}

	hal_cfg_params.max_tx_frm_sz = NRF_WIFI_IFACE_MTU + NRF_WIFI_FMAC_ETH_HDR_LEN +
					TX_BUF_HEADROOM;

	hal_cfg_params.max_cmd_size = MAX_NRF_WIFI_UMAC_CMD_SIZE;


	fpriv->hpriv = nrf_wifi_hal_init(&hal_cfg_params,
					 &nrf_wifi_sys_fmac_event_callback,
#ifdef NRF_WIFI_RPU_RECOVERY
					 &nrf_wifi_sys_fmac_rpu_recovery_callback);
#else
					 NULL);
#endif

	if (!fpriv->hpriv) {
		nrf_wifi_osal_log_err("%s: Unable to do HAL init",
				      __func__);
		nrf_wifi_osal_mem_free(fpriv);
		fpriv = NULL;
		sys_fpriv = NULL;
		goto out;
	}

	fpriv->op_mode = NRF_WIFI_OP_MODE_SYS;
out:
	return fpriv;
}


enum nrf_wifi_status nrf_wifi_sys_fmac_scan(void *dev_ctx,
					    unsigned char if_idx,
					    struct nrf_wifi_umac_scan_info *scan_info)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_umac_cmd_scan *scan_cmd = NULL;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;
	struct nrf_wifi_sys_fmac_dev_ctx *sys_dev_ctx = NULL;
	int channel_info_len = (sizeof(struct nrf_wifi_channel) *
				scan_info->scan_params.num_scan_channels);

	fmac_dev_ctx = dev_ctx;

	if (fmac_dev_ctx->op_mode != NRF_WIFI_OP_MODE_SYS) {
		nrf_wifi_osal_log_err("%s: Invalid op mode",
				      __func__);
		goto out;
	}

	sys_dev_ctx = wifi_dev_priv(fmac_dev_ctx);
	if (!sys_dev_ctx) {
		nrf_wifi_osal_log_err("%s: Invalid device context",
				      __func__);
		goto out;
	}

	scan_cmd = nrf_wifi_osal_mem_zalloc((sizeof(*scan_cmd) + channel_info_len));

	if (!scan_cmd) {
		nrf_wifi_osal_log_err("%s: Unable to allocate memory",
				      __func__);
		goto out;
	}

	if (scan_info->scan_reason == SCAN_DISPLAY) {
		nrf_wifi_osal_log_dbg("%s: scan_db_len = %d",
				      __func__,
				      scan_info->scan_db_len);
		scan_info->scan_db_addr =
			(unsigned int)nrf_wifi_osal_mem_zalloc(scan_info->scan_db_len);
		if (!scan_info->scan_db_addr) {
			nrf_wifi_osal_log_err("%s: Unable to allocate memory",
					      __func__);
			goto out;
		}
	}

	scan_cmd->umac_hdr.cmd_evnt = NRF_WIFI_UMAC_CMD_TRIGGER_SCAN;
	scan_cmd->umac_hdr.ids.wdev_id = if_idx;
	scan_cmd->umac_hdr.ids.valid_fields |= NRF_WIFI_INDEX_IDS_WDEV_ID_VALID;

	nrf_wifi_osal_mem_cpy(&scan_cmd->info,
			      scan_info,
			      (sizeof(scan_cmd->info) + channel_info_len));

	status = umac_cmd_cfg(fmac_dev_ctx,
			      scan_cmd,
			      sizeof(*scan_cmd) + channel_info_len);
out:
	if (scan_cmd) {
		nrf_wifi_osal_mem_free(scan_cmd);
	}

	return status;
}

enum nrf_wifi_status nrf_wifi_sys_fmac_abort_scan(void *dev_ctx,
						  unsigned char if_idx)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_umac_cmd_abort_scan *scan_abort_cmd = NULL;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;
	struct nrf_wifi_sys_fmac_dev_ctx *sys_dev_ctx = NULL;

	fmac_dev_ctx = dev_ctx;

	if (fmac_dev_ctx->op_mode != NRF_WIFI_OP_MODE_SYS) {
		nrf_wifi_osal_log_err("%s: Invalid op mode",
				      __func__);
		goto out;
	}

	sys_dev_ctx = wifi_dev_priv(fmac_dev_ctx);

	scan_abort_cmd = nrf_wifi_osal_mem_zalloc(sizeof(*scan_abort_cmd));

	if (!scan_abort_cmd) {
		nrf_wifi_osal_log_err("%s: Unable to allocate memory",
				      __func__);
		goto out;
	}

	scan_abort_cmd->umac_hdr.cmd_evnt = NRF_WIFI_UMAC_CMD_ABORT_SCAN;
	scan_abort_cmd->umac_hdr.ids.wdev_id = if_idx;
	scan_abort_cmd->umac_hdr.ids.valid_fields |= NRF_WIFI_INDEX_IDS_WDEV_ID_VALID;

	status = umac_cmd_cfg(fmac_dev_ctx,
			      scan_abort_cmd,
			      sizeof(*scan_abort_cmd));
out:
	if (scan_abort_cmd) {
		nrf_wifi_osal_mem_free(scan_abort_cmd);
	}

	return status;
}

enum nrf_wifi_status nrf_wifi_sys_fmac_scan_res_get(void *dev_ctx,
						unsigned char vif_idx,
						int scan_type)

{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_umac_cmd_get_scan_results *scan_res_cmd = NULL;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;
	struct nrf_wifi_sys_fmac_dev_ctx *sys_dev_ctx = NULL;

	fmac_dev_ctx = dev_ctx;

	if (fmac_dev_ctx->op_mode != NRF_WIFI_OP_MODE_SYS) {
		nrf_wifi_osal_log_err("%s: Invalid op mode",
				      __func__);
		goto out;
	}

	sys_dev_ctx = wifi_dev_priv(fmac_dev_ctx);

	scan_res_cmd = nrf_wifi_osal_mem_zalloc(sizeof(*scan_res_cmd));

	if (!scan_res_cmd) {
		nrf_wifi_osal_log_err("%s: Unable to allocate memory",
				      __func__);
		goto out;
	}

	scan_res_cmd->umac_hdr.cmd_evnt = NRF_WIFI_UMAC_CMD_GET_SCAN_RESULTS;
	scan_res_cmd->umac_hdr.ids.wdev_id = vif_idx;
	scan_res_cmd->umac_hdr.ids.valid_fields |= NRF_WIFI_INDEX_IDS_WDEV_ID_VALID;
	scan_res_cmd->scan_reason = scan_type;

	status = umac_cmd_cfg(fmac_dev_ctx,
			      scan_res_cmd,
			      sizeof(*scan_res_cmd));
out:
	if (scan_res_cmd) {
		nrf_wifi_osal_mem_free(scan_res_cmd);
	}

	return status;
}

#ifdef NRF71_STA_MODE
enum nrf_wifi_status nrf_wifi_sys_fmac_auth(void *dev_ctx,
					    unsigned char if_idx,
					    struct nrf_wifi_umac_auth_info *auth_info)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_umac_cmd_auth *auth_cmd = NULL;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;
	struct nrf_wifi_fmac_vif_ctx *vif_ctx = NULL;
	struct nrf_wifi_sys_fmac_dev_ctx *sys_dev_ctx = NULL;

	fmac_dev_ctx = dev_ctx;

	if (fmac_dev_ctx->op_mode != NRF_WIFI_OP_MODE_SYS) {
		nrf_wifi_osal_log_err("%s: Invalid op mode",
				      __func__);
		goto out;
	}

	sys_dev_ctx = wifi_dev_priv(fmac_dev_ctx);
	vif_ctx = sys_dev_ctx->vif_ctx[if_idx];

	auth_cmd = nrf_wifi_osal_mem_zalloc(sizeof(*auth_cmd));

	if (!auth_cmd) {
		nrf_wifi_osal_log_err("%s: Unable to allocate memory",
				      __func__);
		goto out;
	}

	auth_cmd->umac_hdr.cmd_evnt = NRF_WIFI_UMAC_CMD_AUTHENTICATE;
	auth_cmd->umac_hdr.ids.wdev_id = if_idx;
	auth_cmd->umac_hdr.ids.valid_fields |= NRF_WIFI_INDEX_IDS_WDEV_ID_VALID;

	nrf_wifi_osal_mem_cpy(&auth_cmd->info,
			      auth_info,
			      sizeof(auth_cmd->info));

	if (auth_info->frequency != 0) {
		auth_cmd->valid_fields |= NRF_WIFI_CMD_AUTHENTICATE_FREQ_VALID;
	}

	if (auth_info->ssid.nrf_wifi_ssid_len > 0) {
		auth_cmd->valid_fields |= NRF_WIFI_CMD_AUTHENTICATE_SSID_VALID;
	}

	if (auth_info->key_info.key.nrf_wifi_key_len > 0) {
		auth_cmd->info.key_info.valid_fields |= NRF_WIFI_KEY_IDX_VALID;
		auth_cmd->info.key_info.valid_fields |= NRF_WIFI_KEY_VALID;
		auth_cmd->info.key_info.valid_fields |= NRF_WIFI_CIPHER_SUITE_VALID;

		auth_cmd->valid_fields |= NRF_WIFI_CMD_AUTHENTICATE_KEY_INFO_VALID;
	}

	if (auth_info->sae.sae_data_len > 0) {
		auth_cmd->valid_fields |= NRF_WIFI_CMD_AUTHENTICATE_SAE_VALID;
	}

	status = umac_cmd_cfg(fmac_dev_ctx,
			      auth_cmd,
			      sizeof(*auth_cmd));
out:
	if (auth_cmd) {
		nrf_wifi_osal_mem_free(auth_cmd);
	}

	return status;
}


enum nrf_wifi_status nrf_wifi_sys_fmac_deauth(void *dev_ctx,
					      unsigned char if_idx,
					      struct nrf_wifi_umac_disconn_info *deauth_info)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_umac_cmd_disconn *deauth_cmd = NULL;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;

	fmac_dev_ctx = dev_ctx;

	if (fmac_dev_ctx->op_mode != NRF_WIFI_OP_MODE_SYS) {
		nrf_wifi_osal_log_err("%s: Invalid op mode",
				      __func__);
		goto out;
	}

	deauth_cmd = nrf_wifi_osal_mem_zalloc(sizeof(*deauth_cmd));

	if (!deauth_cmd) {
		nrf_wifi_osal_log_err("%s: Unable to allocate memory", __func__);
		goto out;
	}

	deauth_cmd->umac_hdr.cmd_evnt = NRF_WIFI_UMAC_CMD_DEAUTHENTICATE;
	deauth_cmd->umac_hdr.ids.wdev_id = if_idx;
	deauth_cmd->umac_hdr.ids.valid_fields |= NRF_WIFI_INDEX_IDS_WDEV_ID_VALID;

	nrf_wifi_osal_mem_cpy(&deauth_cmd->info,
			      deauth_info,
			      sizeof(deauth_cmd->info));

	if (!nrf_wifi_util_is_arr_zero(deauth_info->mac_addr,
				       sizeof(deauth_info->mac_addr))) {
		deauth_cmd->valid_fields |= NRF_WIFI_CMD_MLME_MAC_ADDR_VALID;
	}

	status = umac_cmd_cfg(fmac_dev_ctx,
			      deauth_cmd,
			      sizeof(*deauth_cmd));

out:
	if (deauth_cmd) {
		nrf_wifi_osal_mem_free(deauth_cmd);
	}

	return status;
}


enum nrf_wifi_status nrf_wifi_sys_fmac_assoc(void *dev_ctx,
					     unsigned char if_idx,
					     struct nrf_wifi_umac_assoc_info *assoc_info)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_umac_cmd_assoc *assoc_cmd = NULL;
	struct nrf_wifi_connect_common_info *connect_common_info = NULL;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;
	struct nrf_wifi_sys_fmac_dev_ctx *sys_dev_ctx = NULL;
	struct nrf_wifi_fmac_vif_ctx *vif_ctx = NULL;

	fmac_dev_ctx = dev_ctx;

	if (fmac_dev_ctx->op_mode != NRF_WIFI_OP_MODE_SYS) {
		nrf_wifi_osal_log_err("%s: Invalid op mode",
				      __func__);
		goto out;
	}

	sys_dev_ctx = wifi_dev_priv(fmac_dev_ctx);

	vif_ctx = sys_dev_ctx->vif_ctx[if_idx];

	nrf_wifi_osal_mem_cpy(vif_ctx->bssid,
			      assoc_info->nrf_wifi_bssid,
			      NRF_WIFI_ETH_ADDR_LEN);

	assoc_cmd = nrf_wifi_osal_mem_zalloc(sizeof(*assoc_cmd));

	if (!assoc_cmd) {
		nrf_wifi_osal_log_err("%s: Unable to allocate memory",
				      __func__);
		goto out;
	}

	assoc_cmd->umac_hdr.cmd_evnt = NRF_WIFI_UMAC_CMD_ASSOCIATE;
	assoc_cmd->umac_hdr.ids.wdev_id = if_idx;
	assoc_cmd->umac_hdr.ids.valid_fields |= NRF_WIFI_INDEX_IDS_WDEV_ID_VALID;

	connect_common_info = &assoc_cmd->connect_common_info;

	nrf_wifi_osal_mem_cpy(connect_common_info->mac_addr,
			      assoc_info->nrf_wifi_bssid,
			      NRF_WIFI_ETH_ADDR_LEN);

	if (!nrf_wifi_util_is_arr_zero(connect_common_info->mac_addr,
				       sizeof(connect_common_info->mac_addr))) {
		connect_common_info->valid_fields |=
			NRF_WIFI_CONNECT_COMMON_INFO_MAC_ADDR_VALID;
	}

	if (assoc_info->ssid.nrf_wifi_ssid_len > 0) {
		nrf_wifi_osal_mem_cpy(&connect_common_info->ssid,
				      &assoc_info->ssid,
				      sizeof(connect_common_info->ssid));

		connect_common_info->valid_fields |=
			NRF_WIFI_CONNECT_COMMON_INFO_SSID_VALID;
	}

	connect_common_info->frequency = assoc_info->center_frequency;
	connect_common_info->valid_fields |= NRF_WIFI_CONNECT_COMMON_INFO_FREQ_VALID;

	if (assoc_info->wpa_ie.ie_len > 0) {
		nrf_wifi_osal_mem_cpy(&connect_common_info->wpa_ie,
				      &assoc_info->wpa_ie,
				      sizeof(connect_common_info->wpa_ie));

		connect_common_info->valid_fields |=
			NRF_WIFI_CONNECT_COMMON_INFO_WPA_IE_VALID;
	}

	connect_common_info->use_mfp = assoc_info->use_mfp;
	connect_common_info->valid_fields |=
		NRF_WIFI_CONNECT_COMMON_INFO_USE_MFP_VALID;

	connect_common_info->nrf_wifi_flags |=
		NRF_WIFI_CMD_CONNECT_COMMON_INFO_USE_RRM;

	connect_common_info->control_port =
		assoc_info->control_port;

	if (assoc_info->prev_bssid_flag) {
		nrf_wifi_osal_mem_cpy(connect_common_info->prev_bssid,
				      assoc_info->prev_bssid,
				      NRF_WIFI_ETH_ADDR_LEN);
		connect_common_info->nrf_wifi_flags |= NRF_WIFI_CONNECT_COMMON_INFO_PREV_BSSID;
	}

	if (assoc_info->bss_max_idle_time) {
		connect_common_info->maxidle_insec = assoc_info->bss_max_idle_time;
	}

	if (assoc_info->conn_type == NRF_WIFI_CONN_TYPE_SECURE) {
		connect_common_info->nrf_wifi_flags |=
			NRF_WIFI_CONNECT_COMMON_INFO_SECURITY;
	}

	status = umac_cmd_cfg(fmac_dev_ctx,
			      assoc_cmd,
			      sizeof(*assoc_cmd));
out:
	if (assoc_cmd) {
		nrf_wifi_osal_mem_free(assoc_cmd);
	}

	return status;
}


enum nrf_wifi_status nrf_wifi_sys_fmac_disassoc(void *dev_ctx,
						unsigned char if_idx,
						struct nrf_wifi_umac_disconn_info *disassoc_info)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_umac_cmd_disconn *disassoc_cmd = NULL;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;

	fmac_dev_ctx = dev_ctx;

	if (fmac_dev_ctx->op_mode != NRF_WIFI_OP_MODE_SYS) {
		nrf_wifi_osal_log_err("%s: Invalid op mode",
				      __func__);
		goto out;
	}

	disassoc_cmd = nrf_wifi_osal_mem_zalloc(sizeof(*disassoc_cmd));

	if (!disassoc_cmd) {
		nrf_wifi_osal_log_err("%s: Unable to allocate memory",
				      __func__);
		goto out;
	}

	disassoc_cmd->umac_hdr.cmd_evnt = NRF_WIFI_UMAC_CMD_DEAUTHENTICATE;
	disassoc_cmd->umac_hdr.ids.wdev_id = if_idx;
	disassoc_cmd->umac_hdr.ids.valid_fields |= NRF_WIFI_INDEX_IDS_WDEV_ID_VALID;

	nrf_wifi_osal_mem_cpy(&disassoc_cmd->info,
			      disassoc_info,
			      sizeof(disassoc_cmd->info));

	if (!nrf_wifi_util_is_arr_zero(disassoc_info->mac_addr,
				       sizeof(disassoc_info->mac_addr))) {
		disassoc_cmd->valid_fields |= NRF_WIFI_CMD_MLME_MAC_ADDR_VALID;
	}

	status = umac_cmd_cfg(fmac_dev_ctx,
			      disassoc_cmd,
			      sizeof(*disassoc_cmd));

out:
	if (disassoc_cmd) {
		nrf_wifi_osal_mem_free(disassoc_cmd);
	}

	return status;
}


enum nrf_wifi_status nrf_wifi_sys_fmac_add_key(void *dev_ctx,
					       unsigned char if_idx,
					       struct nrf_wifi_umac_key_info *key_info,
					       const char *mac_addr)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_umac_cmd_key *key_cmd = NULL;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;
	struct nrf_wifi_sys_fmac_dev_ctx *sys_dev_ctx = NULL;
	struct nrf_wifi_fmac_vif_ctx *vif_ctx = NULL;
	int peer_id = -1;

	fmac_dev_ctx = dev_ctx;

	if (fmac_dev_ctx->op_mode != NRF_WIFI_OP_MODE_SYS) {
		nrf_wifi_osal_log_err("%s: Invalid op mode",
				      __func__);
		goto out;
	}

	sys_dev_ctx = wifi_dev_priv(fmac_dev_ctx);
	vif_ctx = sys_dev_ctx->vif_ctx[if_idx];

	key_cmd = nrf_wifi_osal_mem_zalloc(sizeof(*key_cmd));

	if (!key_cmd) {
		nrf_wifi_osal_log_err("%s: Unable to allocate memory", __func__);
		goto out;
	}

	key_cmd->umac_hdr.cmd_evnt = NRF_WIFI_UMAC_CMD_NEW_KEY;
	key_cmd->umac_hdr.ids.wdev_id = if_idx;
	key_cmd->umac_hdr.ids.valid_fields |= NRF_WIFI_INDEX_IDS_WDEV_ID_VALID;

	nrf_wifi_osal_mem_cpy(&key_cmd->key_info,
			      key_info,
			      sizeof(key_cmd->key_info));

	if (mac_addr) {
		nrf_wifi_osal_mem_cpy(key_cmd->mac_addr,
				      mac_addr,
				      NRF_WIFI_ETH_ADDR_LEN);

		key_cmd->valid_fields |= NRF_WIFI_CMD_KEY_MAC_ADDR_VALID;
	}

	if (key_info->key_type == NRF_WIFI_KEYTYPE_GROUP) {
		vif_ctx->groupwise_cipher = key_info->cipher_suite;
	} else if (key_info->key_type == NRF_WIFI_KEYTYPE_PAIRWISE) {
		peer_id = nrf_wifi_fmac_peer_get_id(fmac_dev_ctx,
						    mac_addr);

		if (peer_id == -1) {
			nrf_wifi_osal_log_err("%s: Invalid peer",
					      __func__);
			goto out;
		}

		sys_dev_ctx->tx_config.peers[peer_id].pairwise_cipher = key_info->cipher_suite;
	} else {
		nrf_wifi_osal_log_err("%s: Invalid key type %d",
				      __func__,
				      key_info->key_type);
		goto out;
	}

	if (key_info->key.nrf_wifi_key_len > 0) {
		key_cmd->key_info.valid_fields |= NRF_WIFI_KEY_VALID;
		key_cmd->key_info.valid_fields |= NRF_WIFI_KEY_IDX_VALID;
	}

	if (key_info->seq.nrf_wifi_seq_len > 0) {
		key_cmd->key_info.valid_fields |= NRF_WIFI_SEQ_VALID;
	}

	key_cmd->key_info.valid_fields |= NRF_WIFI_CIPHER_SUITE_VALID;
	key_cmd->key_info.valid_fields |= NRF_WIFI_KEY_TYPE_VALID;

	status = umac_cmd_cfg(fmac_dev_ctx,
			      key_cmd,
			      sizeof(*key_cmd));

out:
	if (key_cmd) {
		nrf_wifi_osal_mem_free(key_cmd);
	}

	return status;
}

enum nrf_wifi_status nrf_wifi_sys_fmac_del_key(void *dev_ctx,
					       unsigned char if_idx,
					       struct nrf_wifi_umac_key_info *key_info,
					       const char *mac_addr)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_umac_cmd_key *key_cmd = NULL;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;
	struct nrf_wifi_fmac_vif_ctx *vif_ctx = NULL;
	struct nrf_wifi_sys_fmac_dev_ctx *sys_dev_ctx = NULL;

	fmac_dev_ctx = dev_ctx;

	if (fmac_dev_ctx->op_mode != NRF_WIFI_OP_MODE_SYS) {
		nrf_wifi_osal_log_err("%s: Invalid op mode",
				      __func__);
		goto out;
	}

	sys_dev_ctx = wifi_dev_priv(fmac_dev_ctx);

	key_cmd = nrf_wifi_osal_mem_zalloc(sizeof(*key_cmd));

	if (!key_cmd) {
		nrf_wifi_osal_log_err("%s: Unable to allocate memory",
				      __func__);
		goto out;
	}

	key_cmd->umac_hdr.cmd_evnt = NRF_WIFI_UMAC_CMD_DEL_KEY;
	key_cmd->umac_hdr.ids.wdev_id = if_idx;
	key_cmd->umac_hdr.ids.valid_fields |= NRF_WIFI_INDEX_IDS_WDEV_ID_VALID;

	nrf_wifi_osal_mem_cpy(&key_cmd->key_info,
			      key_info,
			      sizeof(key_cmd->key_info));

	if (mac_addr) {
		nrf_wifi_osal_mem_cpy(key_cmd->mac_addr,
				      mac_addr,
				      NRF_WIFI_ETH_ADDR_LEN);

		key_cmd->valid_fields |= NRF_WIFI_CMD_KEY_MAC_ADDR_VALID;
	}

	key_cmd->key_info.valid_fields |= NRF_WIFI_KEY_IDX_VALID;
	key_cmd->key_info.valid_fields |= NRF_WIFI_KEY_TYPE_VALID;

	vif_ctx = sys_dev_ctx->vif_ctx[if_idx];

	if (key_info->key_type == NRF_WIFI_KEYTYPE_GROUP) {
		vif_ctx->groupwise_cipher = 0;
	}

	status = umac_cmd_cfg(fmac_dev_ctx,
			      key_cmd,
			      sizeof(*key_cmd));

out:
	if (key_cmd) {
		nrf_wifi_osal_mem_free(key_cmd);
	}

	return status;
}


enum nrf_wifi_status nrf_wifi_sys_fmac_set_key(void *dev_ctx,
					       unsigned char if_idx,
					       struct nrf_wifi_umac_key_info *key_info)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_umac_cmd_set_key *set_key_cmd = NULL;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;

	fmac_dev_ctx = dev_ctx;

	if (fmac_dev_ctx->op_mode != NRF_WIFI_OP_MODE_SYS) {
		nrf_wifi_osal_log_err("%s: Invalid op mode",
				      __func__);
		goto out;
	}

	set_key_cmd = nrf_wifi_osal_mem_zalloc(sizeof(*set_key_cmd));

	if (!set_key_cmd) {
		nrf_wifi_osal_log_err("%s: Unable to allocate memory",
				      __func__);
		goto out;
	}

	set_key_cmd->umac_hdr.cmd_evnt = NRF_WIFI_UMAC_CMD_SET_KEY;
	set_key_cmd->umac_hdr.ids.wdev_id = if_idx;
	set_key_cmd->umac_hdr.ids.valid_fields |= NRF_WIFI_INDEX_IDS_WDEV_ID_VALID;

	nrf_wifi_osal_mem_cpy(&set_key_cmd->key_info,
			      key_info,
			      sizeof(set_key_cmd->key_info));

	set_key_cmd->key_info.valid_fields |= NRF_WIFI_KEY_IDX_VALID;

	status = umac_cmd_cfg(fmac_dev_ctx,
			      set_key_cmd,
			      sizeof(*set_key_cmd));

out:
	if (set_key_cmd) {
		nrf_wifi_osal_mem_free(set_key_cmd);
	}

	return status;
}

enum nrf_wifi_status nrf_wifi_sys_fmac_chg_sta(void *dev_ctx,
					       unsigned char if_idx,
					       struct nrf_wifi_umac_chg_sta_info *chg_sta_info)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_umac_cmd_chg_sta *chg_sta_cmd = NULL;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;

	fmac_dev_ctx = dev_ctx;

	if (fmac_dev_ctx->op_mode != NRF_WIFI_OP_MODE_SYS) {
		nrf_wifi_osal_log_err("%s: Invalid op mode",
				      __func__);
		goto out;
	}

	chg_sta_cmd = nrf_wifi_osal_mem_zalloc(sizeof(*chg_sta_cmd));

	if (!chg_sta_cmd) {
		nrf_wifi_osal_log_err("%s: Unable to allocate memory",
				      __func__);
		goto out;
	}

	chg_sta_cmd->umac_hdr.cmd_evnt = NRF_WIFI_UMAC_CMD_SET_STATION;
	chg_sta_cmd->umac_hdr.ids.wdev_id = if_idx;
	chg_sta_cmd->umac_hdr.ids.valid_fields |= NRF_WIFI_INDEX_IDS_WDEV_ID_VALID;

	nrf_wifi_osal_mem_cpy(&chg_sta_cmd->info,
			      chg_sta_info,
			      sizeof(chg_sta_cmd->info));

	if (chg_sta_info->aid) {
		chg_sta_cmd->valid_fields |=
			NRF_WIFI_CMD_SET_STATION_AID_VALID;
	}


	if (chg_sta_info->supported_channels.supported_channels_len > 0) {
		chg_sta_cmd->valid_fields |=
			NRF_WIFI_CMD_SET_STATION_SUPPORTED_CHANNELS_VALID;
	}

	if (chg_sta_info->supported_oper_classes.supported_oper_classes_len > 0) {
		chg_sta_cmd->valid_fields |=
			NRF_WIFI_CMD_SET_STATION_SUPPORTED_OPER_CLASSES_VALID;
	}

	chg_sta_cmd->valid_fields |= NRF_WIFI_CMD_SET_STATION_STA_FLAGS2_VALID;

	if (chg_sta_info->opmode_notif) {
		chg_sta_cmd->valid_fields |=
			NRF_WIFI_CMD_SET_STATION_OPMODE_NOTIF_VALID;
	}

	if (chg_sta_info->wme_max_sp) {
		chg_sta_cmd->valid_fields |=
			NRF_WIFI_CMD_SET_STATION_STA_WME_MAX_SP_VALID;
	}

	status = umac_cmd_cfg(fmac_dev_ctx,
			      chg_sta_cmd,
			      sizeof(*chg_sta_cmd));

out:
	if (chg_sta_cmd) {
		nrf_wifi_osal_mem_free(chg_sta_cmd);
	}

	return status;
}

#ifdef NRF71_AP_MODE
enum nrf_wifi_status nrf_wifi_sys_fmac_set_bss(void *dev_ctx,
					       unsigned char if_idx,
					       struct nrf_wifi_umac_bss_info *bss_info)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_umac_cmd_set_bss *set_bss_cmd = NULL;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;

	fmac_dev_ctx = dev_ctx;

	if (fmac_dev_ctx->op_mode != NRF_WIFI_OP_MODE_SYS) {
		nrf_wifi_osal_log_err("%s: Invalid op mode",
				      __func__);
		goto out;
	}

	set_bss_cmd = nrf_wifi_osal_mem_zalloc(sizeof(*set_bss_cmd));

	if (!set_bss_cmd) {
		nrf_wifi_osal_log_err("%s: Unable to allocate memory",
				      __func__);
		goto out;
	}

	set_bss_cmd->umac_hdr.cmd_evnt = NRF_WIFI_UMAC_CMD_SET_BSS;
	set_bss_cmd->umac_hdr.ids.wdev_id = if_idx;
	set_bss_cmd->umac_hdr.ids.valid_fields |= NRF_WIFI_INDEX_IDS_WDEV_ID_VALID;

	nrf_wifi_osal_mem_cpy(&set_bss_cmd->bss_info,
			      bss_info,
			      sizeof(set_bss_cmd->bss_info));

	set_bss_cmd->valid_fields = NRF_WIFI_CMD_SET_BSS_CTS_VALID |
		NRF_WIFI_CMD_SET_BSS_PREAMBLE_VALID |
		NRF_WIFI_CMD_SET_BSS_SLOT_VALID |
		NRF_WIFI_CMD_SET_BSS_HT_OPMODE_VALID |
		NRF_WIFI_CMD_SET_BSS_AP_ISOLATE_VALID;

	if ((bss_info->p2p_go_ctwindow > 0) &&
	    (bss_info->p2p_go_ctwindow < 127)) {
		set_bss_cmd->valid_fields |=
			(NRF_WIFI_CMD_SET_BSS_P2P_CTWINDOW_VALID |
			 NRF_WIFI_CMD_SET_BSS_P2P_OPPPS_VALID);
	}

	status = umac_cmd_cfg(fmac_dev_ctx,
			      set_bss_cmd,
			      sizeof(*set_bss_cmd));

out:
	if (set_bss_cmd) {
		nrf_wifi_osal_mem_free(set_bss_cmd);
	}

	return status;
}

enum nrf_wifi_status nrf_wifi_sys_fmac_chg_bcn(void *dev_ctx,
					       unsigned char if_idx,
					       struct nrf_wifi_umac_set_beacon_info *data)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_umac_cmd_set_beacon *set_bcn_cmd = NULL;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;

	fmac_dev_ctx = dev_ctx;

	if (fmac_dev_ctx->op_mode != NRF_WIFI_OP_MODE_SYS) {
		nrf_wifi_osal_log_err("%s: Invalid op mode",
				      __func__);
		goto out;
	}

	set_bcn_cmd = nrf_wifi_osal_mem_zalloc(sizeof(*set_bcn_cmd));

	if (!set_bcn_cmd) {
		nrf_wifi_osal_log_err("%s: Unable to allocate memory", __func__);
		goto out;
	}

	set_bcn_cmd->umac_hdr.cmd_evnt = NRF_WIFI_UMAC_CMD_SET_BEACON;
	set_bcn_cmd->umac_hdr.ids.wdev_id = if_idx;
	set_bcn_cmd->umac_hdr.ids.valid_fields |= NRF_WIFI_INDEX_IDS_WDEV_ID_VALID;

	nrf_wifi_osal_mem_cpy(&set_bcn_cmd->info,
			      data,
			      sizeof(set_bcn_cmd->info));

	nrf_wifi_osal_log_dbg("%s: Sending command to rpu",
			      __func__);

	status = umac_cmd_cfg(fmac_dev_ctx,
			      set_bcn_cmd,
			      sizeof(*set_bcn_cmd));

out:
	if (set_bcn_cmd) {
		nrf_wifi_osal_mem_free(set_bcn_cmd);
	}

	return status;
}


enum nrf_wifi_status nrf_wifi_sys_fmac_start_ap(void *dev_ctx,
						unsigned char if_idx,
						struct nrf_wifi_umac_start_ap_info *ap_info)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_umac_cmd_start_ap *start_ap_cmd = NULL;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;
	struct nrf_wifi_umac_set_wiphy_info *wiphy_info = NULL;

	fmac_dev_ctx = dev_ctx;

	if (fmac_dev_ctx->op_mode != NRF_WIFI_OP_MODE_SYS) {
		nrf_wifi_osal_log_err("%s: Invalid op mode",
				      __func__);
		goto out;
	}

	start_ap_cmd = nrf_wifi_osal_mem_zalloc(sizeof(*start_ap_cmd));

	if (!start_ap_cmd) {
		nrf_wifi_osal_log_err("%s: Unable to allocate memory",
				      __func__);
		goto out;
	}

	start_ap_cmd->umac_hdr.cmd_evnt = NRF_WIFI_UMAC_CMD_START_AP;
	start_ap_cmd->umac_hdr.ids.wdev_id = if_idx;
	start_ap_cmd->umac_hdr.ids.valid_fields |= NRF_WIFI_INDEX_IDS_WDEV_ID_VALID;

	nrf_wifi_osal_mem_cpy(&start_ap_cmd->info,
			      ap_info,
			      sizeof(start_ap_cmd->info));

	start_ap_cmd->valid_fields |=
		NRF_WIFI_CMD_BEACON_INFO_BEACON_INTERVAL_VALID |
		NRF_WIFI_CMD_BEACON_INFO_VERSIONS_VALID |
		NRF_WIFI_CMD_BEACON_INFO_CIPHER_SUITE_GROUP_VALID;

	start_ap_cmd->info.freq_params.valid_fields |=
		NRF_WIFI_SET_FREQ_PARAMS_FREQ_VALID |
		NRF_WIFI_SET_FREQ_PARAMS_CHANNEL_WIDTH_VALID |
		NRF_WIFI_SET_FREQ_PARAMS_CENTER_FREQ1_VALID |
		NRF_WIFI_SET_FREQ_PARAMS_CENTER_FREQ2_VALID |
		NRF_WIFI_SET_FREQ_PARAMS_CHANNEL_TYPE_VALID;

	start_ap_cmd->info.connect_common_info.valid_fields |=
		NRF_WIFI_CONNECT_COMMON_INFO_WPA_VERSIONS_VALID;

	if (ap_info->connect_common_info.num_cipher_suites_pairwise > 0) {
		start_ap_cmd->info.connect_common_info.valid_fields |=
			NRF_WIFI_CONNECT_COMMON_INFO_CIPHER_SUITES_PAIRWISE_VALID;
	}

	if (ap_info->connect_common_info.num_akm_suites > 0) {
		start_ap_cmd->info.connect_common_info.valid_fields |=
			NRF_WIFI_CONNECT_COMMON_INFO_AKM_SUITES_VALID;
	}

	if (ap_info->connect_common_info.control_port_ether_type) {
		start_ap_cmd->info.connect_common_info.valid_fields |=
			NRF_WIFI_CONNECT_COMMON_INFO_CONTROL_PORT_ETHER_TYPE;
	}

	if (ap_info->connect_common_info.control_port_no_encrypt) {
		start_ap_cmd->info.connect_common_info.valid_fields |=
			NRF_WIFI_CONNECT_COMMON_INFO_CONTROL_PORT_NO_ENCRYPT;
	}

	if ((ap_info->p2p_go_ctwindow > 0) &&
	    (ap_info->p2p_go_ctwindow < 127)) {
		start_ap_cmd->valid_fields |=
			NRF_WIFI_CMD_BEACON_INFO_P2P_CTWINDOW_VALID;
		start_ap_cmd->valid_fields |=
			NRF_WIFI_CMD_BEACON_INFO_P2P_OPPPS_VALID;
	}

	wiphy_info = nrf_wifi_osal_mem_zalloc(sizeof(*wiphy_info));

	if (!wiphy_info) {
		nrf_wifi_osal_log_err("%s: Unable to allocate memory",
				      __func__);
		goto out;
	}

	wiphy_info->freq_params.frequency = ap_info->freq_params.frequency;

	wiphy_info->freq_params.channel_width = ap_info->freq_params.channel_width;

	wiphy_info->freq_params.center_frequency1 = ap_info->freq_params.center_frequency1;
	wiphy_info->freq_params.center_frequency2 = ap_info->freq_params.center_frequency2;

	wiphy_info->freq_params.channel_type = ap_info->freq_params.channel_type;

	status = nrf_wifi_sys_fmac_set_wiphy_params(fmac_dev_ctx,
						if_idx,
						wiphy_info);

	if (status == NRF_WIFI_STATUS_FAIL) {
		nrf_wifi_osal_log_err("%s: nrf_wifi_sys_fmac_set_wiphy_params failes",
				      __func__);
		goto out;
	}

	nrf_wifi_fmac_peers_flush(fmac_dev_ctx, if_idx);

	status = umac_cmd_cfg(fmac_dev_ctx,
			      start_ap_cmd,
			      sizeof(*start_ap_cmd));

out:
	if (wiphy_info) {
		nrf_wifi_osal_mem_free(wiphy_info);
	}

	if (start_ap_cmd) {
		nrf_wifi_osal_mem_free(start_ap_cmd);
	}

	return status;
}


enum nrf_wifi_status nrf_wifi_sys_fmac_stop_ap(void *dev_ctx,
					       unsigned char if_idx)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_umac_cmd_stop_ap *stop_ap_cmd = NULL;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;

	fmac_dev_ctx = dev_ctx;

	if (fmac_dev_ctx->op_mode != NRF_WIFI_OP_MODE_SYS) {
		nrf_wifi_osal_log_err("%s: Invalid op mode",
				      __func__);
		goto out;
	}

	stop_ap_cmd = nrf_wifi_osal_mem_zalloc(sizeof(*stop_ap_cmd));

	if (!stop_ap_cmd) {
		nrf_wifi_osal_log_err("%s: Unable to allocate memory",
				      __func__);
		goto out;
	}

	stop_ap_cmd->umac_hdr.cmd_evnt = NRF_WIFI_UMAC_CMD_STOP_AP;
	stop_ap_cmd->umac_hdr.ids.wdev_id = if_idx;
	stop_ap_cmd->umac_hdr.ids.valid_fields |= NRF_WIFI_INDEX_IDS_WDEV_ID_VALID;

	nrf_wifi_fmac_peers_flush(fmac_dev_ctx, if_idx);

	status = umac_cmd_cfg(fmac_dev_ctx,
			      stop_ap_cmd,
			      sizeof(*stop_ap_cmd));

out:
	if (stop_ap_cmd) {
		nrf_wifi_osal_mem_free(stop_ap_cmd);
	}

	return status;
}

enum nrf_wifi_status nrf_wifi_sys_fmac_del_sta(void *dev_ctx,
					       unsigned char if_idx,
					       struct nrf_wifi_umac_del_sta_info *del_sta_info)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_umac_cmd_del_sta *del_sta_cmd = NULL;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;

	fmac_dev_ctx = dev_ctx;

	if (fmac_dev_ctx->op_mode != NRF_WIFI_OP_MODE_SYS) {
		nrf_wifi_osal_log_err("%s: Invalid op mode",
				      __func__);
		goto out;
	}

	del_sta_cmd = nrf_wifi_osal_mem_zalloc(sizeof(*del_sta_cmd));

	if (!del_sta_cmd) {
		nrf_wifi_osal_log_err("%s: Unable to allocate memory",
				      __func__);
		goto out;
	}

	del_sta_cmd->umac_hdr.cmd_evnt = NRF_WIFI_UMAC_CMD_DEL_STATION;
	del_sta_cmd->umac_hdr.ids.wdev_id = if_idx;
	del_sta_cmd->umac_hdr.ids.valid_fields |= NRF_WIFI_INDEX_IDS_WDEV_ID_VALID;

	nrf_wifi_osal_mem_cpy(&del_sta_cmd->info,
			      del_sta_info,
			      sizeof(del_sta_cmd->info));

	if (!nrf_wifi_util_is_arr_zero(del_sta_info->mac_addr,
				       sizeof(del_sta_info->mac_addr))) {
		del_sta_cmd->valid_fields |= NRF_WIFI_CMD_DEL_STATION_MAC_ADDR_VALID;
	}

	if (del_sta_info->mgmt_subtype) {
		del_sta_cmd->valid_fields |=
			NRF_WIFI_CMD_DEL_STATION_MGMT_SUBTYPE_VALID;
	}

	if (del_sta_info->reason_code) {
		del_sta_cmd->valid_fields |=
			NRF_WIFI_CMD_DEL_STATION_REASON_CODE_VALID;
	}

	status = umac_cmd_cfg(fmac_dev_ctx,
			      del_sta_cmd,
			      sizeof(*del_sta_cmd));

out:
	if (del_sta_cmd) {
		nrf_wifi_osal_mem_free(del_sta_cmd);
	}

	return status;
}


enum nrf_wifi_status nrf_wifi_sys_fmac_add_sta(void *dev_ctx,
					       unsigned char if_idx,
					       struct nrf_wifi_umac_add_sta_info *add_sta_info)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_umac_cmd_add_sta *add_sta_cmd = NULL;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;

	fmac_dev_ctx = dev_ctx;

	if (fmac_dev_ctx->op_mode != NRF_WIFI_OP_MODE_SYS) {
		nrf_wifi_osal_log_err("%s: Invalid op mode",
				      __func__);
		goto out;
	}

	add_sta_cmd = nrf_wifi_osal_mem_zalloc(sizeof(*add_sta_cmd));

	if (!add_sta_cmd) {
		nrf_wifi_osal_log_err("%s: Unable to allocate memory",
				      __func__);
		goto out;
	}

	add_sta_cmd->umac_hdr.cmd_evnt = NRF_WIFI_UMAC_CMD_NEW_STATION;
	add_sta_cmd->umac_hdr.ids.wdev_id = if_idx;
	add_sta_cmd->umac_hdr.ids.valid_fields |= NRF_WIFI_INDEX_IDS_WDEV_ID_VALID;

	nrf_wifi_osal_mem_cpy(&add_sta_cmd->info,
			      add_sta_info,
			      sizeof(add_sta_cmd->info));

	if (add_sta_info->aid) {
		add_sta_cmd->valid_fields |= NRF_WIFI_CMD_NEW_STATION_AID_VALID;
	}

	if (add_sta_info->sta_capability) {
		add_sta_cmd->valid_fields |=
			NRF_WIFI_CMD_NEW_STATION_STA_CAPABILITY_VALID;
	}

	add_sta_cmd->valid_fields |= NRF_WIFI_CMD_NEW_STATION_LISTEN_INTERVAL_VALID;

	if (add_sta_info->supp_rates.nrf_wifi_num_rates > 0) {
		add_sta_cmd->valid_fields |=
			NRF_WIFI_CMD_NEW_STATION_SUPP_RATES_VALID;
	}

	if (add_sta_info->ext_capability.ext_capability_len > 0) {
		add_sta_cmd->valid_fields |=
			NRF_WIFI_CMD_NEW_STATION_EXT_CAPABILITY_VALID;
	}

	if (add_sta_info->supported_channels.supported_channels_len > 0) {
		add_sta_cmd->valid_fields |=
			NRF_WIFI_CMD_NEW_STATION_SUPPORTED_CHANNELS_VALID;
	}

	if (add_sta_info->supported_oper_classes.supported_oper_classes_len > 0) {
		add_sta_cmd->valid_fields |=
			NRF_WIFI_CMD_NEW_STATION_SUPPORTED_OPER_CLASSES_VALID;
	}

	add_sta_cmd->valid_fields |= NRF_WIFI_CMD_NEW_STATION_STA_FLAGS2_VALID;

	add_sta_cmd->valid_fields |=
		NRF_WIFI_CMD_NEW_STATION_HT_CAPABILITY_VALID;

	if (add_sta_info->opmode_notif) {
		add_sta_cmd->valid_fields |=
			NRF_WIFI_CMD_NEW_STATION_OPMODE_NOTIF_VALID;
	}

	if (add_sta_info->wme_uapsd_queues) {
		add_sta_cmd->valid_fields |=
			NRF_WIFI_CMD_NEW_STATION_STA_WME_UAPSD_QUEUES_VALID;
	}

	if (add_sta_info->wme_max_sp) {
		add_sta_cmd->valid_fields |=
			NRF_WIFI_CMD_NEW_STATION_STA_WME_MAX_SP_VALID;
	}

	status = umac_cmd_cfg(fmac_dev_ctx,
			      add_sta_cmd,
			      sizeof(*add_sta_cmd));

out:
	if (add_sta_cmd) {
		nrf_wifi_osal_mem_free(add_sta_cmd);
	}

	return status;
}

enum nrf_wifi_status nrf_wifi_sys_fmac_mgmt_frame_reg(
	void *dev_ctx,
	unsigned char if_idx,
	struct nrf_wifi_umac_mgmt_frame_info *frame_info)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_umac_cmd_mgmt_frame_reg *frame_reg_cmd = NULL;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;

	fmac_dev_ctx = dev_ctx;

	if (fmac_dev_ctx->op_mode != NRF_WIFI_OP_MODE_SYS) {
		nrf_wifi_osal_log_err("%s: Invalid op mode",
				      __func__);
		goto out;
	}

	frame_reg_cmd = nrf_wifi_osal_mem_zalloc(sizeof(*frame_reg_cmd));

	if (!frame_reg_cmd) {
		nrf_wifi_osal_log_err("%s: Unable to allocate memory",
				      __func__);
		goto out;
	}

	frame_reg_cmd->umac_hdr.cmd_evnt = NRF_WIFI_UMAC_CMD_REGISTER_FRAME;
	frame_reg_cmd->umac_hdr.ids.wdev_id = if_idx;
	frame_reg_cmd->umac_hdr.ids.valid_fields |= NRF_WIFI_INDEX_IDS_WDEV_ID_VALID;

	nrf_wifi_osal_mem_cpy(&frame_reg_cmd->info,
			      frame_info,
			      sizeof(frame_reg_cmd->info));

	status = umac_cmd_cfg(fmac_dev_ctx,
			      frame_reg_cmd,
			      sizeof(*frame_reg_cmd));

out:
	if (frame_reg_cmd) {
		nrf_wifi_osal_mem_free(frame_reg_cmd);
	}

	return status;
}

#endif /* NRF71_AP_MODE */

#ifdef NRF71_P2P_MODE
enum nrf_wifi_status nrf_wifi_sys_fmac_p2p_dev_start(void *dev_ctx,
						     unsigned char if_idx)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_cmd_start_p2p *start_p2p_dev_cmd = NULL;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;

	fmac_dev_ctx = dev_ctx;

	if (fmac_dev_ctx->op_mode != NRF_WIFI_OP_MODE_SYS) {
		nrf_wifi_osal_log_err("%s: Invalid op mode",
				      __func__);
		goto out;
	}

	start_p2p_dev_cmd = nrf_wifi_osal_mem_zalloc(sizeof(*start_p2p_dev_cmd));

	if (!start_p2p_dev_cmd) {
		nrf_wifi_osal_log_err("%s: Unable to allocate memory", __func__);
		goto out;
	}

	start_p2p_dev_cmd->umac_hdr.cmd_evnt = NRF_WIFI_UMAC_CMD_START_P2P_DEVICE;
	start_p2p_dev_cmd->umac_hdr.ids.wdev_id = if_idx;
	start_p2p_dev_cmd->umac_hdr.ids.valid_fields |= NRF_WIFI_INDEX_IDS_WDEV_ID_VALID;

	status = umac_cmd_cfg(fmac_dev_ctx,
			      start_p2p_dev_cmd,
			      sizeof(*start_p2p_dev_cmd));

out:
	if (start_p2p_dev_cmd) {
		nrf_wifi_osal_mem_free(start_p2p_dev_cmd);
	}

	return status;
}


enum nrf_wifi_status nrf_wifi_sys_fmac_p2p_dev_stop(void *dev_ctx,
						    unsigned char if_idx)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_umac_cmd_stop_p2p_dev *stop_p2p_dev_cmd = NULL;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;

	fmac_dev_ctx = dev_ctx;

	if (fmac_dev_ctx->op_mode != NRF_WIFI_OP_MODE_SYS) {
		nrf_wifi_osal_log_err("%s: Invalid op mode",
				      __func__);
		goto out;
	}

	stop_p2p_dev_cmd = nrf_wifi_osal_mem_zalloc(sizeof(*stop_p2p_dev_cmd));

	if (!stop_p2p_dev_cmd) {
		nrf_wifi_osal_log_err("%s: Unable to allocate memory",
				      __func__);
		goto out;
	}

	stop_p2p_dev_cmd->umac_hdr.cmd_evnt = NRF_WIFI_UMAC_CMD_STOP_P2P_DEVICE;
	stop_p2p_dev_cmd->umac_hdr.ids.wdev_id = if_idx;
	stop_p2p_dev_cmd->umac_hdr.ids.valid_fields |= NRF_WIFI_INDEX_IDS_WDEV_ID_VALID;

	status = umac_cmd_cfg(fmac_dev_ctx,
			      stop_p2p_dev_cmd,
			      sizeof(*stop_p2p_dev_cmd));
out:
	if (stop_p2p_dev_cmd) {
		nrf_wifi_osal_mem_free(stop_p2p_dev_cmd);
	}

	return status;
}


enum nrf_wifi_status nrf_wifi_sys_fmac_p2p_roc_start(void *dev_ctx,
						     unsigned char if_idx,
						     struct remain_on_channel_info *roc_info)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_umac_cmd_remain_on_channel *roc_cmd = NULL;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;

	fmac_dev_ctx = dev_ctx;

	if (fmac_dev_ctx->op_mode != NRF_WIFI_OP_MODE_SYS) {
		nrf_wifi_osal_log_err("%s: Invalid op mode",
				      __func__);
		goto out;
	}

	roc_cmd = nrf_wifi_osal_mem_zalloc(sizeof(struct nrf_wifi_umac_cmd_remain_on_channel));

	if (!roc_cmd) {
		nrf_wifi_osal_log_err("%s: Unable to allocate memory",
				      __func__);
		goto out;
	}

	roc_cmd->umac_hdr.cmd_evnt = NRF_WIFI_UMAC_CMD_REMAIN_ON_CHANNEL;
	roc_cmd->umac_hdr.ids.wdev_id = if_idx;
	roc_cmd->umac_hdr.ids.valid_fields |= NRF_WIFI_INDEX_IDS_WDEV_ID_VALID;

	nrf_wifi_osal_mem_cpy(&roc_cmd->info,
			      roc_info,
			      sizeof(roc_cmd->info));

	if (roc_info->dur != 0) {
		roc_cmd->valid_fields |= NRF_WIFI_CMD_ROC_DURATION_VALID;
	}

	roc_cmd->info.nrf_wifi_freq_params.valid_fields = NRF_WIFI_SET_FREQ_PARAMS_FREQ_VALID;
	roc_cmd->valid_fields |= NRF_WIFI_CMD_ROC_FREQ_PARAMS_VALID;

	status = umac_cmd_cfg(fmac_dev_ctx,
			      roc_cmd,
			      sizeof(*roc_cmd));

out:
	if (roc_cmd) {
		nrf_wifi_osal_mem_free(roc_cmd);
	}

	return status;
}


enum nrf_wifi_status nrf_wifi_sys_fmac_p2p_roc_stop(void *dev_ctx,
						    unsigned char if_idx,
						    unsigned long long cookie)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_umac_cmd_cancel_remain_on_channel *cancel_roc_cmd = NULL;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;

	fmac_dev_ctx = dev_ctx;

	if (fmac_dev_ctx->op_mode != NRF_WIFI_OP_MODE_SYS) {
		nrf_wifi_osal_log_err("%s: Invalid op mode",
				      __func__);
		goto out;
	}

	cancel_roc_cmd = nrf_wifi_osal_mem_zalloc(sizeof(*cancel_roc_cmd));

	if (!cancel_roc_cmd) {
		nrf_wifi_osal_log_err("%s: Unable to allocate memory",
				      __func__);
		goto out;
	}

	cancel_roc_cmd->umac_hdr.cmd_evnt = NRF_WIFI_UMAC_CMD_CANCEL_REMAIN_ON_CHANNEL;
	cancel_roc_cmd->umac_hdr.ids.wdev_id = if_idx;
	cancel_roc_cmd->umac_hdr.ids.valid_fields |= NRF_WIFI_INDEX_IDS_WDEV_ID_VALID;
	cancel_roc_cmd->cookie = cookie;
	cancel_roc_cmd->valid_fields |= NRF_WIFI_CMD_CANCEL_ROC_COOKIE_VALID;

	status = umac_cmd_cfg(fmac_dev_ctx,
			      cancel_roc_cmd,
			      sizeof(*cancel_roc_cmd));
out:
	if (cancel_roc_cmd) {
		nrf_wifi_osal_mem_free(cancel_roc_cmd);
	}

	return status;
}

#endif /* NRF71_P2P_MODE */
#endif /* NRF71_STA_MODE */

enum nrf_wifi_status nrf_wifi_sys_fmac_mgmt_tx(void *dev_ctx,
					       unsigned char if_idx,
					       struct nrf_wifi_umac_mgmt_tx_info *mgmt_tx_info)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_umac_cmd_mgmt_tx *mgmt_tx_cmd = NULL;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;

	fmac_dev_ctx = dev_ctx;

	if (fmac_dev_ctx->op_mode != NRF_WIFI_OP_MODE_SYS) {
		nrf_wifi_osal_log_err("%s: Invalid op mode",
				      __func__);
		goto out;
	}

	mgmt_tx_cmd = nrf_wifi_osal_mem_zalloc(sizeof(*mgmt_tx_cmd));

	if (!mgmt_tx_cmd) {
		nrf_wifi_osal_log_err("%s: Unable to allocate memory",
				      __func__);
		goto out;
	}

	mgmt_tx_cmd->umac_hdr.cmd_evnt = NRF_WIFI_UMAC_CMD_FRAME;
	mgmt_tx_cmd->umac_hdr.ids.wdev_id = if_idx;
	mgmt_tx_cmd->umac_hdr.ids.valid_fields |= NRF_WIFI_INDEX_IDS_WDEV_ID_VALID;

	nrf_wifi_osal_mem_cpy(&mgmt_tx_cmd->info,
			      mgmt_tx_info,
			      sizeof(mgmt_tx_cmd->info));

	mgmt_tx_cmd->valid_fields |= (NRF_WIFI_CMD_FRAME_FREQ_VALID |
				      NRF_WIFI_CMD_FRAME_DURATION_VALID |
				      NRF_WIFI_CMD_SET_FRAME_FREQ_PARAMS_VALID);

	mgmt_tx_cmd->info.freq_params.valid_fields |=
		(NRF_WIFI_SET_FREQ_PARAMS_FREQ_VALID |
		 NRF_WIFI_SET_FREQ_PARAMS_CHANNEL_WIDTH_VALID |
		 NRF_WIFI_SET_FREQ_PARAMS_CENTER_FREQ1_VALID |
		 NRF_WIFI_SET_FREQ_PARAMS_CENTER_FREQ2_VALID |
		 NRF_WIFI_SET_FREQ_PARAMS_CHANNEL_TYPE_VALID);

	status = umac_cmd_cfg(fmac_dev_ctx,
			      mgmt_tx_cmd,
			      sizeof(*mgmt_tx_cmd));

out:
	if (mgmt_tx_cmd) {
		nrf_wifi_osal_mem_free(mgmt_tx_cmd);
	}

	return status;
}

enum nrf_wifi_status nrf_wifi_sys_fmac_mac_addr(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
						unsigned char *addr)
{
	unsigned char vif_idx = 0;
	struct nrf_wifi_sys_fmac_dev_ctx *sys_dev_ctx = NULL;

	if (fmac_dev_ctx->op_mode != NRF_WIFI_OP_MODE_SYS) {
		nrf_wifi_osal_log_err("%s: Invalid op mode",
				      __func__);
		return NRF_WIFI_STATUS_FAIL;
	}

	vif_idx = nrf_wifi_fmac_vif_idx_get(fmac_dev_ctx);
	sys_dev_ctx = wifi_dev_priv(fmac_dev_ctx);

	if (vif_idx == MAX_NUM_VIFS) {
		return NRF_WIFI_STATUS_FAIL;
	}

	nrf_wifi_osal_mem_cpy(addr,
			      sys_dev_ctx->vif_ctx[vif_idx]->mac_addr,
			      NRF_WIFI_ETH_ADDR_LEN);

	if (((unsigned short)addr[5] + vif_idx) > 0xff) {
		nrf_wifi_osal_log_err("%s: MAC Address rollover!!",
				      __func__);
	}

	addr[5] += vif_idx;

	return NRF_WIFI_STATUS_SUCCESS;
}


unsigned char nrf_wifi_sys_fmac_add_vif(void *dev_ctx,
					void *os_vif_ctx,
					struct nrf_wifi_umac_add_vif_info *vif_info)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_umac_cmd_add_vif *add_vif_cmd = NULL;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;
	struct nrf_wifi_fmac_vif_ctx *vif_ctx = NULL;
	unsigned char vif_idx = 0;
	struct nrf_wifi_sys_fmac_dev_ctx *sys_dev_ctx = NULL;

	fmac_dev_ctx = dev_ctx;

	if (fmac_dev_ctx->op_mode != NRF_WIFI_OP_MODE_SYS) {
		nrf_wifi_osal_log_err("%s: Invalid op mode",
				      __func__);
		goto out;
	}

	sys_dev_ctx = wifi_dev_priv(fmac_dev_ctx);

	switch (vif_info->iftype) {
	case NRF_WIFI_IFTYPE_STATION:
	case NRF_WIFI_IFTYPE_P2P_CLIENT:
	case NRF_WIFI_IFTYPE_AP:
	case NRF_WIFI_IFTYPE_P2P_GO:
		break;
	default:
		nrf_wifi_osal_log_err("%s: VIF type not supported",
				      __func__);
		goto err;
	}

	if (nrf_wifi_fmac_vif_check_if_limit(fmac_dev_ctx,
					     vif_info->iftype)) {
		goto err;
	}

	vif_ctx = nrf_wifi_osal_mem_zalloc(sizeof(*vif_ctx));

	if (!vif_ctx) {
		nrf_wifi_osal_log_err("%s: Unable to allocate memory for VIF ctx",
				      __func__);
		goto err;
	}

	vif_ctx->fmac_dev_ctx = fmac_dev_ctx;
	vif_ctx->os_vif_ctx = os_vif_ctx;
	vif_ctx->if_type = vif_info->iftype;
	vif_ctx->mode = NRF_WIFI_STA_MODE;

	/**
	 * Set initial packet filter setting to filter all.
	 * subsequent calls to set packet filter will set
	 * packet_filter settings to appropriate value as
	 * desired by application.
	 */
#if defined(NRF71_RAW_DATA_RX) || defined(NRF71_PROMISC_DATA_RX)
	vif_ctx->packet_filter = 1;
#endif

	nrf_wifi_osal_mem_cpy(vif_ctx->mac_addr,
			      vif_info->mac_addr,
			      sizeof(vif_ctx->mac_addr));

	vif_idx = nrf_wifi_fmac_vif_idx_get(fmac_dev_ctx);

	if (vif_idx == MAX_NUM_VIFS) {
		nrf_wifi_osal_log_err("%s: Unable to add additional VIF",
				      __func__);
		goto err;
	}

	/* We don't need to send a command to the RPU for the default interface,
	 * since the FW is adding that interface by default. We just need to
	 * send commands for non-default interfaces
	 */
	if (vif_idx != 0) {
		add_vif_cmd = nrf_wifi_osal_mem_zalloc(sizeof(*add_vif_cmd));

		if (!add_vif_cmd) {
			nrf_wifi_osal_log_err("%s: Unable to allocate memory for cmd",
					      __func__);
			goto err;
		}

		add_vif_cmd->umac_hdr.cmd_evnt = NRF_WIFI_UMAC_CMD_NEW_INTERFACE;
		add_vif_cmd->umac_hdr.ids.wdev_id = vif_idx;
		add_vif_cmd->umac_hdr.ids.valid_fields |= NRF_WIFI_INDEX_IDS_WDEV_ID_VALID;

		nrf_wifi_osal_mem_cpy(&add_vif_cmd->info,
				      vif_info,
				      sizeof(add_vif_cmd->info));

		add_vif_cmd->valid_fields |= NRF_WIFI_CMD_NEW_INTERFACE_IFTYPE_VALID;
		add_vif_cmd->valid_fields |= NRF_WIFI_CMD_NEW_INTERFACE_IFNAME_VALID;
		add_vif_cmd->valid_fields |= NRF_WIFI_CMD_NEW_INTERFACE_MAC_ADDR_VALID;

		status = umac_cmd_cfg(fmac_dev_ctx,
				      add_vif_cmd,
				      sizeof(*add_vif_cmd));

		if (status == NRF_WIFI_STATUS_FAIL) {
			nrf_wifi_osal_log_err("%s: NRF_WIFI_UMAC_CMD_NEW_INTERFACE failed",
					      __func__);
			goto err;
		}

	}

	sys_dev_ctx->vif_ctx[vif_idx] = vif_ctx;

	nrf_wifi_fmac_vif_incr_if_type(fmac_dev_ctx,
				       vif_ctx->if_type);

	goto out;
err:
	if (vif_ctx) {
		nrf_wifi_osal_mem_free(vif_ctx);
	}

	vif_idx = MAX_NUM_VIFS;

out:
	if (add_vif_cmd) {
		nrf_wifi_osal_mem_free(add_vif_cmd);
	}

	return vif_idx;
}


enum nrf_wifi_status nrf_wifi_sys_fmac_del_vif(void *dev_ctx,
					       unsigned char if_idx)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;
	struct nrf_wifi_fmac_vif_ctx *vif_ctx = NULL;
	struct nrf_wifi_umac_cmd_del_vif *del_vif_cmd = NULL;
	struct nrf_wifi_sys_fmac_dev_ctx *sys_dev_ctx = NULL;

	fmac_dev_ctx = dev_ctx;

	if (fmac_dev_ctx->op_mode != NRF_WIFI_OP_MODE_SYS) {
		nrf_wifi_osal_log_err("%s: Invalid op mode",
				      __func__);
		goto out;
	}

	sys_dev_ctx = wifi_dev_priv(fmac_dev_ctx);

	switch (sys_dev_ctx->vif_ctx[if_idx]->if_type) {
	case NRF_WIFI_IFTYPE_STATION:
	case NRF_WIFI_IFTYPE_P2P_CLIENT:
	case NRF_WIFI_IFTYPE_AP:
	case NRF_WIFI_IFTYPE_P2P_GO:
		break;
	default:
		nrf_wifi_osal_log_err("%s: VIF type not supported",
				      __func__);
		goto out;
	}

	vif_ctx = sys_dev_ctx->vif_ctx[if_idx];

	if (!vif_ctx) {
		nrf_wifi_osal_log_err("%s: VIF ctx does not exist",
				      __func__);
		goto out;
	}

	/* We should not send a command to the RPU for the default interface,
	 * since the FW is adding that interface by default. We just need to
	 * send commands for non-default interfaces
	 */
	if (if_idx != 0) {
		del_vif_cmd = nrf_wifi_osal_mem_zalloc(sizeof(*del_vif_cmd));

		if (!del_vif_cmd) {
			nrf_wifi_osal_log_err("%s: Unable to allocate memory for cmd",
					      __func__);
			goto out;
		}

		del_vif_cmd->umac_hdr.cmd_evnt = NRF_WIFI_UMAC_CMD_DEL_INTERFACE;
		del_vif_cmd->umac_hdr.ids.wdev_id = if_idx;
		del_vif_cmd->umac_hdr.ids.valid_fields |= NRF_WIFI_INDEX_IDS_WDEV_ID_VALID;

		status = umac_cmd_cfg(fmac_dev_ctx,
				      del_vif_cmd,
				      sizeof(*del_vif_cmd));

		if (status != NRF_WIFI_STATUS_SUCCESS) {
			nrf_wifi_osal_log_err("%s: NRF_WIFI_UMAC_CMD_DEL_INTERFACE failed",
					      __func__);
			goto out;
		}
	} else {
		status = NRF_WIFI_STATUS_SUCCESS;
	}

	nrf_wifi_fmac_vif_decr_if_type(fmac_dev_ctx, vif_ctx->if_type);

out:
	if (del_vif_cmd) {
		nrf_wifi_osal_mem_free(del_vif_cmd);
	}

	if (vif_ctx) {
		nrf_wifi_osal_mem_free(vif_ctx);
	}

	return status;
}


enum nrf_wifi_status nrf_wifi_sys_fmac_chg_vif(void *dev_ctx,
					       unsigned char if_idx,
					       struct nrf_wifi_umac_chg_vif_attr_info *vif_info)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_umac_cmd_chg_vif_attr *chg_vif_cmd = NULL;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;

	fmac_dev_ctx = dev_ctx;

	if (fmac_dev_ctx->op_mode != NRF_WIFI_OP_MODE_SYS) {
		nrf_wifi_osal_log_err("%s: Invalid op mode",
				      __func__);
		goto out;
	}

	switch (vif_info->iftype) {
	case NRF_WIFI_IFTYPE_STATION:
	case NRF_WIFI_IFTYPE_P2P_CLIENT:
	case NRF_WIFI_IFTYPE_AP:
	case NRF_WIFI_IFTYPE_P2P_GO:
		break;
	default:
		nrf_wifi_osal_log_err("%s: VIF type not supported", __func__);
		goto out;
	}

	if (nrf_wifi_fmac_vif_check_if_limit(fmac_dev_ctx,
					     vif_info->iftype)) {
		goto out;
	}

	chg_vif_cmd = nrf_wifi_osal_mem_zalloc(sizeof(*chg_vif_cmd));

	if (!chg_vif_cmd) {
		nrf_wifi_osal_log_err("%s: Unable to allocate memory",
				      __func__);
		goto out;
	}

	chg_vif_cmd->umac_hdr.cmd_evnt = NRF_WIFI_UMAC_CMD_SET_INTERFACE;
	chg_vif_cmd->umac_hdr.ids.wdev_id = if_idx;
	chg_vif_cmd->umac_hdr.ids.valid_fields |= NRF_WIFI_INDEX_IDS_WDEV_ID_VALID;

	nrf_wifi_osal_mem_cpy(&chg_vif_cmd->info,
			      vif_info,
			      sizeof(chg_vif_cmd->info));

	chg_vif_cmd->valid_fields |= NRF_WIFI_SET_INTERFACE_IFTYPE_VALID;
	chg_vif_cmd->valid_fields |= NRF_WIFI_SET_INTERFACE_USE_4ADDR_VALID;

	status = umac_cmd_cfg(fmac_dev_ctx,
			      chg_vif_cmd,
			      sizeof(*chg_vif_cmd));
out:
	if (chg_vif_cmd) {
		nrf_wifi_osal_mem_free(chg_vif_cmd);
	}

	if (status == NRF_WIFI_STATUS_SUCCESS) {
		struct nrf_wifi_sys_fmac_dev_ctx *sys_dev_ctx = NULL;

		sys_dev_ctx = wifi_dev_priv(fmac_dev_ctx);

		nrf_wifi_fmac_vif_update_if_type(fmac_dev_ctx,
						 if_idx,
						 vif_info->iftype);

		sys_dev_ctx->vif_ctx[if_idx]->if_type = vif_info->iftype;
	}

	return status;
}


#define RPU_CMD_TIMEOUT_MS 10000
enum nrf_wifi_status nrf_wifi_sys_fmac_chg_vif_state(
	void *dev_ctx,
	unsigned char if_idx,
	struct nrf_wifi_umac_chg_vif_state_info *vif_info)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_umac_cmd_chg_vif_state *chg_vif_state_cmd = NULL;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;
	struct nrf_wifi_fmac_vif_ctx *vif_ctx = NULL;
	unsigned int count = RPU_CMD_TIMEOUT_MS;
	struct nrf_wifi_sys_fmac_dev_ctx *sys_dev_ctx = NULL;

	fmac_dev_ctx = dev_ctx;

	if (fmac_dev_ctx->op_mode != NRF_WIFI_OP_MODE_SYS) {
		nrf_wifi_osal_log_err("%s: Invalid op mode",
				      __func__);
		goto out;
	}

	sys_dev_ctx = wifi_dev_priv(fmac_dev_ctx);

	chg_vif_state_cmd = nrf_wifi_osal_mem_zalloc(sizeof(*chg_vif_state_cmd));

	if (!chg_vif_state_cmd) {
		nrf_wifi_osal_log_err("%s: Unable to allocate memory",
				      __func__);
		goto out;
	}

	chg_vif_state_cmd->umac_hdr.cmd_evnt = NRF_WIFI_UMAC_CMD_SET_IFFLAGS;
	chg_vif_state_cmd->umac_hdr.ids.wdev_id = if_idx;
	chg_vif_state_cmd->umac_hdr.ids.valid_fields |=
		NRF_WIFI_INDEX_IDS_WDEV_ID_VALID;

	nrf_wifi_osal_mem_cpy(&chg_vif_state_cmd->info,
			      vif_info,
			      sizeof(chg_vif_state_cmd->info));

	vif_ctx = sys_dev_ctx->vif_ctx[if_idx];

	vif_ctx->ifflags = false;

	status = umac_cmd_cfg(fmac_dev_ctx,
			      chg_vif_state_cmd,
			      sizeof(*chg_vif_state_cmd));

	while (!vif_ctx->ifflags && (--count > 0)) {
		nrf_wifi_osal_sleep_ms(1);
	}

	if (count == 0) {
		status = NRF_WIFI_STATUS_FAIL;
		nrf_wifi_osal_log_err("%s: RPU is unresponsive for %d sec",
				      __func__, RPU_CMD_TIMEOUT_MS / 1000);
		goto out;
	}
#ifdef NRF71_AP_MODE
	if (vif_ctx->if_type == NRF_WIFI_IFTYPE_AP) {
		if (vif_info->state == 1) {
			sys_dev_ctx->tx_config.peers[MAX_PEERS].peer_id = MAX_PEERS;
			sys_dev_ctx->tx_config.peers[MAX_PEERS].if_idx = if_idx;
		} else if (vif_info->state == 0) {
			sys_dev_ctx->tx_config.peers[MAX_PEERS].peer_id = -1;
			sys_dev_ctx->tx_config.peers[MAX_PEERS].if_idx = if_idx;
		}
	}
#endif /* NRF71_AP_MODE */
out:
	if (chg_vif_state_cmd) {
		nrf_wifi_osal_mem_free(chg_vif_state_cmd);
	}

	return status;
}


enum nrf_wifi_status nrf_wifi_sys_fmac_set_vif_macaddr(void *dev_ctx,
						       unsigned char if_idx,
						       unsigned char *mac_addr)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;
	struct nrf_wifi_umac_cmd_change_macaddr *cmd = NULL;

	if (!dev_ctx) {
		goto out;
	}

	if (!mac_addr) {
		nrf_wifi_osal_log_err("%s: Invalid MAC address",
				      __func__);
		goto out;
	}

	fmac_dev_ctx = dev_ctx;

	if (fmac_dev_ctx->op_mode != NRF_WIFI_OP_MODE_SYS) {
		nrf_wifi_osal_log_err("%s: Invalid op mode",
				      __func__);
		goto out;
	}

	cmd = nrf_wifi_osal_mem_zalloc(sizeof(*cmd));

	if (!cmd) {
		nrf_wifi_osal_log_err("%s: Unable to allocate cmd",
				      __func__);
		goto out;
	}

	cmd->umac_hdr.cmd_evnt = NRF_WIFI_UMAC_CMD_CHANGE_MACADDR;
	cmd->umac_hdr.ids.wdev_id = if_idx;
	cmd->umac_hdr.ids.valid_fields |= NRF_WIFI_INDEX_IDS_WDEV_ID_VALID;

	nrf_wifi_osal_mem_cpy(cmd->macaddr_info.mac_addr,
			      mac_addr,
			      sizeof(cmd->macaddr_info.mac_addr));

	status = umac_cmd_cfg(fmac_dev_ctx,
			      cmd,
			      sizeof(*cmd));
out:
	if (cmd) {
		nrf_wifi_osal_mem_free(cmd);
	}

	return status;
}

enum nrf_wifi_status nrf_wifi_sys_fmac_set_wiphy_params(
	void *dev_ctx,
	unsigned char if_idx,
	struct nrf_wifi_umac_set_wiphy_info *wiphy_info)
{
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;
	struct nrf_wifi_umac_cmd_set_wiphy *set_wiphy_cmd = NULL;
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	int freq_params_valid = 0;

	if (!dev_ctx) {
		goto out;
	}

	fmac_dev_ctx = dev_ctx;

	if (fmac_dev_ctx->op_mode != NRF_WIFI_OP_MODE_SYS) {
		nrf_wifi_osal_log_err("%s: Invalid op mode",
				      __func__);
		goto out;
	}

	if (!wiphy_info) {
		nrf_wifi_osal_log_err("%s: wiphy_info: Invalid memory",
				       __func__);
		goto out;
	}

	set_wiphy_cmd = nrf_wifi_osal_mem_zalloc(sizeof(*set_wiphy_cmd));

	if (!set_wiphy_cmd) {
		nrf_wifi_osal_log_err("%s: Unable to allocate memory",
				      __func__);
		goto out;
	}

	set_wiphy_cmd->umac_hdr.cmd_evnt = NRF_WIFI_UMAC_CMD_SET_WIPHY;
	set_wiphy_cmd->umac_hdr.ids.wdev_id = if_idx;
	set_wiphy_cmd->umac_hdr.ids.valid_fields |= NRF_WIFI_INDEX_IDS_WDEV_ID_VALID;

	if (wiphy_info->freq_params.frequency) {
		freq_params_valid = 1;
		wiphy_info->freq_params.valid_fields |=
			NRF_WIFI_SET_FREQ_PARAMS_FREQ_VALID;
	}

	if (wiphy_info->freq_params.channel_width) {
		freq_params_valid = 1;
		wiphy_info->freq_params.valid_fields |=
			NRF_WIFI_SET_FREQ_PARAMS_CHANNEL_WIDTH_VALID;
	}

	if (wiphy_info->freq_params.center_frequency1) {
		freq_params_valid = 1;
		wiphy_info->freq_params.valid_fields |=
			NRF_WIFI_SET_FREQ_PARAMS_CENTER_FREQ1_VALID;
	}

	if (wiphy_info->freq_params.center_frequency2) {
		freq_params_valid = 1;
		wiphy_info->freq_params.valid_fields |=
			NRF_WIFI_SET_FREQ_PARAMS_CENTER_FREQ2_VALID;
	}

	if (wiphy_info->freq_params.channel_type) {
		freq_params_valid = 1;
		wiphy_info->freq_params.valid_fields |=
			NRF_WIFI_SET_FREQ_PARAMS_CHANNEL_TYPE_VALID;
	}

	if (freq_params_valid) {
		set_wiphy_cmd->valid_fields |=
			NRF_WIFI_CMD_SET_WIPHY_FREQ_PARAMS_VALID;
	}

	if (wiphy_info->rts_threshold) {
		set_wiphy_cmd->valid_fields |=
			NRF_WIFI_CMD_SET_WIPHY_RTS_THRESHOLD_VALID;
	}

	if (wiphy_info->frag_threshold) {
		set_wiphy_cmd->valid_fields |=
			NRF_WIFI_CMD_SET_WIPHY_FRAG_THRESHOLD_VALID;
	}

	if (wiphy_info->retry_long) {
		set_wiphy_cmd->valid_fields |=
			NRF_WIFI_CMD_SET_WIPHY_RETRY_LONG_VALID;
	}

	if (wiphy_info->retry_short) {
		set_wiphy_cmd->valid_fields |=
			NRF_WIFI_CMD_SET_WIPHY_RETRY_SHORT_VALID;
	}

	nrf_wifi_osal_mem_cpy(&set_wiphy_cmd->info,
			      wiphy_info,
			      sizeof(set_wiphy_cmd->info));

	status = umac_cmd_cfg(fmac_dev_ctx,
			      set_wiphy_cmd,
			      sizeof(*set_wiphy_cmd));
out:
	if (set_wiphy_cmd) {
		nrf_wifi_osal_mem_free(set_wiphy_cmd);
	}

	return status;
}

#ifdef NRF71_STA_MODE
enum nrf_wifi_status nrf_wifi_sys_fmac_get_tx_power(void *dev_ctx,
						    unsigned int if_idx)
{

	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_umac_cmd_get_tx_power *cmd = NULL;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;

	fmac_dev_ctx = dev_ctx;

	if (fmac_dev_ctx->op_mode != NRF_WIFI_OP_MODE_SYS) {
		nrf_wifi_osal_log_err("%s: Invalid op mode",
				      __func__);
		goto out;
	}

	cmd = nrf_wifi_osal_mem_zalloc(sizeof(*cmd));

	if (!cmd) {
		nrf_wifi_osal_log_err("%s: Unable to allocate memory",
				      __func__);
		goto out;
	}

	cmd->umac_hdr.cmd_evnt = NRF_WIFI_UMAC_CMD_GET_TX_POWER;
	cmd->umac_hdr.ids.wdev_id = if_idx;
	cmd->umac_hdr.ids.valid_fields |= NRF_WIFI_INDEX_IDS_WDEV_ID_VALID;

	status = umac_cmd_cfg(fmac_dev_ctx, cmd, sizeof(*cmd));

out:
	if (cmd) {
		nrf_wifi_osal_mem_free(cmd);
	}

	return status;
}


enum nrf_wifi_status nrf_wifi_sys_fmac_get_channel(void *dev_ctx,
						   unsigned int if_idx)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_umac_cmd_get_channel *cmd = NULL;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;

	fmac_dev_ctx = dev_ctx;

	if (fmac_dev_ctx->op_mode != NRF_WIFI_OP_MODE_SYS) {
		nrf_wifi_osal_log_err("%s: Invalid op mode",
				      __func__);
		goto out;
	}

	cmd = nrf_wifi_osal_mem_zalloc(sizeof(*cmd));

	if (!cmd) {
		nrf_wifi_osal_log_err("%s: Unable to allocate memory",
				      __func__);
		goto out;
	}

	cmd->umac_hdr.cmd_evnt = NRF_WIFI_UMAC_CMD_GET_CHANNEL;
	cmd->umac_hdr.ids.wdev_id = if_idx;
	cmd->umac_hdr.ids.valid_fields |= NRF_WIFI_INDEX_IDS_WDEV_ID_VALID;

	status = umac_cmd_cfg(fmac_dev_ctx,
			      cmd,
			      sizeof(*cmd));
out:
	if (cmd) {
		nrf_wifi_osal_mem_free(cmd);
	}

	return status;
}


enum nrf_wifi_status nrf_wifi_sys_fmac_get_station(void *dev_ctx,
						   unsigned int if_idx,
						   unsigned char *mac)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_umac_cmd_get_sta *cmd = NULL;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;

	fmac_dev_ctx = dev_ctx;

	if (fmac_dev_ctx->op_mode != NRF_WIFI_OP_MODE_SYS) {
		nrf_wifi_osal_log_err("%s: Invalid op mode",
				      __func__);
		goto out;
	}

	cmd = nrf_wifi_osal_mem_zalloc(sizeof(*cmd));

	if (!cmd) {
		nrf_wifi_osal_log_err("%s: Unable to allocate memory",
				      __func__);
		goto out;
	}

	cmd->umac_hdr.cmd_evnt = NRF_WIFI_UMAC_CMD_GET_STATION;
	cmd->umac_hdr.ids.wdev_id = if_idx;
	cmd->umac_hdr.ids.valid_fields |= NRF_WIFI_INDEX_IDS_WDEV_ID_VALID;

	nrf_wifi_osal_mem_cpy(cmd->info.mac_addr,
			      mac,
			      NRF_WIFI_ETH_ADDR_LEN);

	status = umac_cmd_cfg(fmac_dev_ctx,
			      cmd,
			      sizeof(*cmd));
out:
	if (cmd) {
		nrf_wifi_osal_mem_free(cmd);
	}

	return status;
}

enum nrf_wifi_status nrf_wifi_sys_fmac_get_interface(void *dev_ctx,
						     unsigned int if_idx)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_cmd_get_interface *cmd = NULL;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;

	if (!dev_ctx || if_idx > MAX_NUM_VIFS) {
		goto out;
	}
	fmac_dev_ctx = dev_ctx;

	if (fmac_dev_ctx->op_mode != NRF_WIFI_OP_MODE_SYS) {
		nrf_wifi_osal_log_err("%s: Invalid op mode",
				      __func__);
		goto out;
	}

	cmd = nrf_wifi_osal_mem_zalloc(sizeof(*cmd));

	if (!cmd) {
		nrf_wifi_osal_log_err("%s: Unable to allocate memory",
				      __func__);
		goto out;
	}

	cmd->umac_hdr.cmd_evnt = NRF_WIFI_UMAC_CMD_GET_INTERFACE;
	cmd->umac_hdr.ids.wdev_id = if_idx;
	cmd->umac_hdr.ids.valid_fields |= NRF_WIFI_INDEX_IDS_WDEV_ID_VALID;

	status = umac_cmd_cfg(fmac_dev_ctx,
			      cmd,
			      sizeof(*cmd));
out:
	if (cmd) {
		nrf_wifi_osal_mem_free(cmd);
	}

	return status;
}


enum nrf_wifi_status nrf_wifi_sys_fmac_set_qos_map(void *dev_ctx,
						   unsigned char if_idx,
						   struct nrf_wifi_umac_qos_map_info *qos_info)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_umac_cmd_set_qos_map *set_qos_cmd = NULL;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;

	fmac_dev_ctx = dev_ctx;

	if (fmac_dev_ctx->op_mode != NRF_WIFI_OP_MODE_SYS) {
		nrf_wifi_osal_log_err("%s: Invalid op mode",
				      __func__);
		goto out;
	}

	set_qos_cmd = nrf_wifi_osal_mem_zalloc(sizeof(*set_qos_cmd));

	if (!set_qos_cmd) {
		nrf_wifi_osal_log_err("%s: Unable to allocate memory",
				      __func__);
		goto out;
	}

	set_qos_cmd->umac_hdr.cmd_evnt = NRF_WIFI_UMAC_CMD_SET_QOS_MAP;
	set_qos_cmd->umac_hdr.ids.wdev_id = if_idx;
	set_qos_cmd->umac_hdr.ids.valid_fields |= NRF_WIFI_INDEX_IDS_WDEV_ID_VALID;

	if (qos_info->qos_map_info_len) {
		nrf_wifi_osal_mem_cpy(&set_qos_cmd->info.qos_map_info,
				      qos_info->qos_map_info,
				      qos_info->qos_map_info_len);

		set_qos_cmd->info.qos_map_info_len =
			qos_info->qos_map_info_len;
	}

	status = umac_cmd_cfg(fmac_dev_ctx,
			      set_qos_cmd,
			      sizeof(*set_qos_cmd));
out:
	if (set_qos_cmd) {
		nrf_wifi_osal_mem_free(set_qos_cmd);
	}

	return status;
}


enum nrf_wifi_status nrf_wifi_sys_fmac_set_power_save(void *dev_ctx,
						      unsigned char if_idx,
						      bool state)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_umac_cmd_set_power_save *set_ps_cmd = NULL;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;

	fmac_dev_ctx = dev_ctx;

	if (fmac_dev_ctx->op_mode != NRF_WIFI_OP_MODE_SYS) {
		nrf_wifi_osal_log_err("%s: Invalid op mode",
				      __func__);
		goto out;
	}

	set_ps_cmd = nrf_wifi_osal_mem_zalloc(sizeof(*set_ps_cmd));

	if (!set_ps_cmd) {
		nrf_wifi_osal_log_err("%s: Unable to allocate memory", __func__);
		goto out;
	}

	set_ps_cmd->umac_hdr.cmd_evnt = NRF_WIFI_UMAC_CMD_SET_POWER_SAVE;
	set_ps_cmd->umac_hdr.ids.wdev_id = if_idx;
	set_ps_cmd->umac_hdr.ids.valid_fields |=
		NRF_WIFI_INDEX_IDS_WDEV_ID_VALID;
	set_ps_cmd->info.ps_state = state;

	status = umac_cmd_cfg(fmac_dev_ctx,
			      set_ps_cmd,
			      sizeof(*set_ps_cmd));
out:
	if (set_ps_cmd) {
		nrf_wifi_osal_mem_free(set_ps_cmd);
	}

	return status;
}


enum nrf_wifi_status nrf_wifi_sys_fmac_set_uapsd_queue(void *dev_ctx,
						       unsigned char if_idx,
						       unsigned int uapsd_queue)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_umac_cmd_config_uapsd  *set_uapsdq_cmd = NULL;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;

	fmac_dev_ctx = dev_ctx;

	if (!dev_ctx) {
		goto out;
	}

	if (fmac_dev_ctx->op_mode != NRF_WIFI_OP_MODE_SYS) {
		nrf_wifi_osal_log_err("%s: Invalid op mode",
				      __func__);
		goto out;
	}

	set_uapsdq_cmd = nrf_wifi_osal_mem_zalloc(sizeof(*set_uapsdq_cmd));
	if (!set_uapsdq_cmd) {
		nrf_wifi_osal_log_err("%s: Unable to allocate memory",
				      __func__);
		goto out;
	}

	set_uapsdq_cmd->umac_hdr.cmd_evnt = NRF_WIFI_UMAC_CMD_CONFIG_UAPSD;
	set_uapsdq_cmd->umac_hdr.ids.wdev_id = if_idx;
	set_uapsdq_cmd->umac_hdr.ids.valid_fields |=
		NRF_WIFI_INDEX_IDS_WDEV_ID_VALID;
	set_uapsdq_cmd->info.uapsd_queue = uapsd_queue;

	status = umac_cmd_cfg(fmac_dev_ctx,
			      set_uapsdq_cmd,
			      sizeof(*set_uapsdq_cmd));
out:
	if (set_uapsdq_cmd) {
		nrf_wifi_osal_mem_free(set_uapsdq_cmd);
	}

	return status;
}


enum nrf_wifi_status nrf_wifi_sys_fmac_set_power_save_timeout(void *dev_ctx,
							      unsigned char if_idx,
							      int ps_timeout)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_umac_cmd_set_power_save_timeout *set_ps_timeout_cmd = NULL;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;

	fmac_dev_ctx = dev_ctx;

	if (fmac_dev_ctx->op_mode != NRF_WIFI_OP_MODE_SYS) {
		nrf_wifi_osal_log_err("%s: Invalid op mode",
				      __func__);
		goto out;
	}

	set_ps_timeout_cmd = nrf_wifi_osal_mem_zalloc(sizeof(*set_ps_timeout_cmd));

	if (!set_ps_timeout_cmd) {
		nrf_wifi_osal_log_err("%s: Unable to allocate memory", __func__);
		goto out;
	}

	set_ps_timeout_cmd->umac_hdr.cmd_evnt = NRF_WIFI_UMAC_CMD_SET_POWER_SAVE_TIMEOUT;
	set_ps_timeout_cmd->umac_hdr.ids.wdev_id = if_idx;
	set_ps_timeout_cmd->umac_hdr.ids.valid_fields |=
		NRF_WIFI_INDEX_IDS_WDEV_ID_VALID;
	set_ps_timeout_cmd->timeout = ps_timeout;

	status = umac_cmd_cfg(fmac_dev_ctx,
			      set_ps_timeout_cmd,
			      sizeof(*set_ps_timeout_cmd));
out:
	if (set_ps_timeout_cmd) {
		nrf_wifi_osal_mem_free(set_ps_timeout_cmd);
	}

	return status;
}


enum nrf_wifi_status nrf_wifi_sys_fmac_get_wiphy(void *dev_ctx, unsigned char if_idx)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_cmd_get_wiphy *get_wiphy = NULL;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;

	fmac_dev_ctx = dev_ctx;

	if (!dev_ctx || (if_idx >= MAX_NUM_VIFS)) {
		goto out;
	}

	if (fmac_dev_ctx->op_mode != NRF_WIFI_OP_MODE_SYS) {
		nrf_wifi_osal_log_err("%s: Invalid op mode",
				      __func__);
		goto out;
	}

	get_wiphy = nrf_wifi_osal_mem_zalloc(sizeof(*get_wiphy));

	if (!get_wiphy) {
		nrf_wifi_osal_log_err("%s: Unable to allocate memory",
				      __func__);
		goto out;
	}

	get_wiphy->umac_hdr.cmd_evnt = NRF_WIFI_UMAC_CMD_GET_WIPHY;
	get_wiphy->umac_hdr.ids.wdev_id = if_idx;
	get_wiphy->umac_hdr.ids.valid_fields |= NRF_WIFI_INDEX_IDS_WDEV_ID_VALID;

	status = umac_cmd_cfg(fmac_dev_ctx,
			      get_wiphy,
			      sizeof(*get_wiphy));
out:
	if (get_wiphy) {
		nrf_wifi_osal_mem_free(get_wiphy);
	}

	return status;
}

enum nrf_wifi_status nrf_wifi_sys_fmac_register_frame(
	void *dev_ctx,
	unsigned char if_idx,
	struct nrf_wifi_umac_mgmt_frame_info *frame_info)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_umac_cmd_mgmt_frame_reg *frame_reg_cmd = NULL;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;

	fmac_dev_ctx = dev_ctx;

	if (!dev_ctx || (if_idx >= MAX_NUM_VIFS) || !frame_info) {
		goto out;
	}

	if (fmac_dev_ctx->op_mode != NRF_WIFI_OP_MODE_SYS) {
		nrf_wifi_osal_log_err("%s: Invalid op mode",
				      __func__);
		goto out;
	}

	frame_reg_cmd =
		nrf_wifi_osal_mem_zalloc(sizeof(*frame_reg_cmd));

	if (!frame_reg_cmd) {
		nrf_wifi_osal_log_err("%s: Unable to allocate memory",
				      __func__);
		goto out;
	}

	frame_reg_cmd->umac_hdr.cmd_evnt = NRF_WIFI_UMAC_CMD_REGISTER_FRAME;
	frame_reg_cmd->umac_hdr.ids.wdev_id = if_idx;
	frame_reg_cmd->umac_hdr.ids.valid_fields |= NRF_WIFI_INDEX_IDS_WDEV_ID_VALID;

	nrf_wifi_osal_mem_cpy(&frame_reg_cmd->info, frame_info, sizeof(frame_reg_cmd->info));

	status = umac_cmd_cfg(fmac_dev_ctx, frame_reg_cmd, sizeof(*frame_reg_cmd));
out:
	if (frame_reg_cmd) {
		nrf_wifi_osal_mem_free(frame_reg_cmd);
	}

	return status;
}

enum nrf_wifi_status nrf_wifi_sys_fmac_twt_setup(void *dev_ctx,
						 unsigned char if_idx,
						 struct nrf_wifi_umac_config_twt_info *twt_params)
{

	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_umac_cmd_config_twt *twt_setup_cmd = NULL;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;

	if (!dev_ctx || !twt_params) {
		goto out;
	}

	fmac_dev_ctx = dev_ctx;

	if (fmac_dev_ctx->op_mode != NRF_WIFI_OP_MODE_SYS) {
		nrf_wifi_osal_log_err("%s: Invalid op mode",
				      __func__);
		goto out;
	}

	twt_setup_cmd = nrf_wifi_osal_mem_zalloc(sizeof(*twt_setup_cmd));

	if (!twt_setup_cmd) {
		nrf_wifi_osal_log_err("%s: Unable to allocate memory",
				      __func__);
		goto out;
	}

	nrf_wifi_osal_mem_cpy(&twt_setup_cmd->info,
			      twt_params,
			      sizeof(twt_setup_cmd->info));

	twt_setup_cmd->umac_hdr.cmd_evnt = NRF_WIFI_UMAC_CMD_CONFIG_TWT;
	twt_setup_cmd->umac_hdr.ids.wdev_id = if_idx;
	twt_setup_cmd->umac_hdr.ids.valid_fields |= NRF_WIFI_INDEX_IDS_WDEV_ID_VALID;

	status = umac_cmd_cfg(fmac_dev_ctx,
			      twt_setup_cmd,
			      sizeof(*twt_setup_cmd));
out:
	if (twt_setup_cmd) {
		nrf_wifi_osal_mem_free(twt_setup_cmd);
	}

	return status;
}


enum nrf_wifi_status nrf_wifi_sys_fmac_twt_teardown(
	void *dev_ctx,
	unsigned char if_idx,
	struct nrf_wifi_umac_config_twt_info *twt_params)
{

	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_umac_cmd_teardown_twt *twt_teardown_cmd = NULL;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;

	if (!dev_ctx || !twt_params) {
		goto out;
	}

	fmac_dev_ctx = dev_ctx;

	if (fmac_dev_ctx->op_mode != NRF_WIFI_OP_MODE_SYS) {
		nrf_wifi_osal_log_err("%s: Invalid op mode",
				      __func__);
		goto out;
	}

	twt_teardown_cmd = nrf_wifi_osal_mem_zalloc(sizeof(*twt_teardown_cmd));

	if (!twt_teardown_cmd) {
		nrf_wifi_osal_log_err("%s: Unable to allocate memory",
				      __func__);
		goto out;
	}

	nrf_wifi_osal_mem_cpy(&twt_teardown_cmd->info,
			      twt_params,
			      sizeof(twt_teardown_cmd->info));

	twt_teardown_cmd->umac_hdr.cmd_evnt = NRF_WIFI_UMAC_CMD_TEARDOWN_TWT;
	twt_teardown_cmd->umac_hdr.ids.wdev_id = if_idx;
	twt_teardown_cmd->umac_hdr.ids.valid_fields |= NRF_WIFI_INDEX_IDS_WDEV_ID_VALID;

	status = umac_cmd_cfg(fmac_dev_ctx,
			      twt_teardown_cmd,
			      sizeof(*twt_teardown_cmd));
out:
	if (twt_teardown_cmd) {
		nrf_wifi_osal_mem_free(twt_teardown_cmd);
	}

	return status;
}

enum nrf_wifi_status nrf_wifi_sys_fmac_set_mcast_addr(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
						      unsigned char if_idx,
						      struct nrf_wifi_umac_mcast_cfg *mcast_info)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_umac_cmd_mcast_filter *set_mcast_cmd = NULL;

	if (fmac_dev_ctx->op_mode != NRF_WIFI_OP_MODE_SYS) {
		nrf_wifi_osal_log_err("%s: Invalid op mode",
				      __func__);
		goto out;
	}

	set_mcast_cmd = nrf_wifi_osal_mem_zalloc(sizeof(*set_mcast_cmd));

	if (!set_mcast_cmd) {
		nrf_wifi_osal_log_err("%s: Unable to allocate memory",
				      __func__);
		goto out;
	}

	set_mcast_cmd->umac_hdr.cmd_evnt = NRF_WIFI_UMAC_CMD_MCAST_FILTER;
	set_mcast_cmd->umac_hdr.ids.wdev_id = if_idx;
	set_mcast_cmd->umac_hdr.ids.valid_fields |= NRF_WIFI_INDEX_IDS_WDEV_ID_VALID;

	nrf_wifi_osal_mem_cpy(&set_mcast_cmd->info,
			      mcast_info,
			      sizeof(*mcast_info));

	status = umac_cmd_cfg(fmac_dev_ctx,
			      set_mcast_cmd,
			      sizeof(*set_mcast_cmd));
out:

	if (set_mcast_cmd) {
		nrf_wifi_osal_mem_free(set_mcast_cmd);
	}
	return status;
}


enum nrf_wifi_status nrf_wifi_sys_fmac_get_conn_info(void *dev_ctx,
						     unsigned char if_idx)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_umac_cmd_conn_info *cmd = NULL;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;

	fmac_dev_ctx = dev_ctx;

	if (fmac_dev_ctx->op_mode != NRF_WIFI_OP_MODE_SYS) {
		nrf_wifi_osal_log_err("%s: Invalid op mode",
				      __func__);
		goto out;
	}

	cmd = nrf_wifi_osal_mem_zalloc(sizeof(*cmd));

	if (!cmd) {
		nrf_wifi_osal_log_err("%s: Unable to allocate memory",
				      __func__);
		goto out;
	}
	cmd->umac_hdr.cmd_evnt = NRF_WIFI_UMAC_CMD_GET_CONNECTION_INFO;
	cmd->umac_hdr.ids.wdev_id = if_idx;
	cmd->umac_hdr.ids.valid_fields |= NRF_WIFI_INDEX_IDS_WDEV_ID_VALID;

	status = umac_cmd_cfg(fmac_dev_ctx,
			      cmd,
			      sizeof(*cmd));
out:
	if (cmd) {
		nrf_wifi_osal_mem_free(cmd);
	}

	return status;
}

enum nrf_wifi_status nrf_wifi_sys_fmac_get_power_save_info(void *dev_ctx,
							   unsigned char if_idx)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_umac_cmd_get_power_save_info *get_ps_info_cmd = NULL;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;

	fmac_dev_ctx = dev_ctx;

	if (fmac_dev_ctx->op_mode != NRF_WIFI_OP_MODE_SYS) {
		nrf_wifi_osal_log_err("%s: Invalid op mode",
				      __func__);
		goto out;
	}

	get_ps_info_cmd = nrf_wifi_osal_mem_zalloc(sizeof(*get_ps_info_cmd));

	if (!get_ps_info_cmd) {
		nrf_wifi_osal_log_err("%s: Unable to allocate memory", __func__);
		goto out;
	}

	get_ps_info_cmd->umac_hdr.cmd_evnt = NRF_WIFI_UMAC_CMD_GET_POWER_SAVE_INFO;
	get_ps_info_cmd->umac_hdr.ids.wdev_id = if_idx;
	get_ps_info_cmd->umac_hdr.ids.valid_fields |=
			NRF_WIFI_INDEX_IDS_WDEV_ID_VALID;

	status = umac_cmd_cfg(fmac_dev_ctx,
			      get_ps_info_cmd,
			      sizeof(*get_ps_info_cmd));
out:
	if (get_ps_info_cmd) {
		nrf_wifi_osal_mem_free(get_ps_info_cmd);
	}

	return status;
}

enum nrf_wifi_status nrf_wifi_sys_fmac_set_listen_interval(void *dev_ctx,
							   unsigned char if_idx,
							   unsigned short listen_interval)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_umac_cmd_set_listen_interval *set_listen_interval_cmd = NULL;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;

	fmac_dev_ctx = dev_ctx;

	if (fmac_dev_ctx->op_mode != NRF_WIFI_OP_MODE_SYS) {
		nrf_wifi_osal_log_err("%s: Invalid op mode",
				      __func__);
		goto out;
	}

	set_listen_interval_cmd = nrf_wifi_osal_mem_zalloc(sizeof(*set_listen_interval_cmd));

	if (!set_listen_interval_cmd) {
		nrf_wifi_osal_log_err("%s: Unable to allocate memory", __func__);
		goto out;
	}

	set_listen_interval_cmd->umac_hdr.cmd_evnt = NRF_WIFI_UMAC_CMD_SET_LISTEN_INTERVAL;
	set_listen_interval_cmd->umac_hdr.ids.wdev_id = if_idx;
	set_listen_interval_cmd->umac_hdr.ids.valid_fields |=
		NRF_WIFI_INDEX_IDS_WDEV_ID_VALID;
	set_listen_interval_cmd->listen_interval = listen_interval;

	status = umac_cmd_cfg(fmac_dev_ctx,
			      set_listen_interval_cmd,
			      sizeof(*set_listen_interval_cmd));
out:
	if (set_listen_interval_cmd) {
		nrf_wifi_osal_mem_free(set_listen_interval_cmd);
	}

	return status;
}


enum nrf_wifi_status nrf_wifi_sys_fmac_set_ps_wakeup_mode(void *dev_ctx,
							  unsigned char if_idx,
							  bool ps_wakeup_mode)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_umac_cmd_config_extended_ps *set_ps_wakeup_mode_cmd = NULL;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;

	fmac_dev_ctx = dev_ctx;

	if (fmac_dev_ctx->op_mode != NRF_WIFI_OP_MODE_SYS) {
		nrf_wifi_osal_log_err("%s: Invalid op mode",
				      __func__);
		goto out;
	}

	set_ps_wakeup_mode_cmd = nrf_wifi_osal_mem_zalloc(sizeof(*set_ps_wakeup_mode_cmd));

	if (!set_ps_wakeup_mode_cmd) {
		nrf_wifi_osal_log_err("%s: Unable to allocate memory", __func__);
		goto out;
	}

	set_ps_wakeup_mode_cmd->umac_hdr.cmd_evnt = NRF_WIFI_UMAC_CMD_CONFIG_EXTENDED_PS;
	set_ps_wakeup_mode_cmd->umac_hdr.ids.wdev_id = if_idx;
	set_ps_wakeup_mode_cmd->umac_hdr.ids.valid_fields |=
		NRF_WIFI_INDEX_IDS_WDEV_ID_VALID;
	set_ps_wakeup_mode_cmd->enable_extended_ps = ps_wakeup_mode;

	status = umac_cmd_cfg(fmac_dev_ctx,
			      set_ps_wakeup_mode_cmd,
			      sizeof(*set_ps_wakeup_mode_cmd));
out:
	if (set_ps_wakeup_mode_cmd) {
		nrf_wifi_osal_mem_free(set_ps_wakeup_mode_cmd);
	}

	return status;
}

enum nrf_wifi_status nrf_wifi_sys_fmac_set_ps_exit_strategy(void *dev_ctx,
							    unsigned char if_idx,
							    unsigned int ps_exit_strategy)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_cmd_ps_exit_strategy *set_ps_exit_strategy_cmd = NULL;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;

	(void)if_idx;

	fmac_dev_ctx = dev_ctx;

	if (fmac_dev_ctx->op_mode != NRF_WIFI_OP_MODE_SYS) {
		nrf_wifi_osal_log_err("%s: Invalid op mode",
				      __func__);
		goto out;
	}

	set_ps_exit_strategy_cmd = nrf_wifi_osal_mem_zalloc(sizeof(*set_ps_exit_strategy_cmd));

	if (!set_ps_exit_strategy_cmd) {
		nrf_wifi_osal_log_err("%s: Unable to allocate memory", __func__);
		goto out;
	}

	set_ps_exit_strategy_cmd->umac_hdr.cmd_evnt = NRF_WIFI_UMAC_CMD_PS_EXIT_STRATEGY;
	set_ps_exit_strategy_cmd->umac_hdr.ids.wdev_id = if_idx;
	set_ps_exit_strategy_cmd->umac_hdr.ids.valid_fields |=
		NRF_WIFI_INDEX_IDS_WDEV_ID_VALID;
	set_ps_exit_strategy_cmd->ps_exit_strategy = ps_exit_strategy;

	status = umac_cmd_cfg(fmac_dev_ctx,
			      set_ps_exit_strategy_cmd,
			      sizeof(*set_ps_exit_strategy_cmd));
out:
	if (set_ps_exit_strategy_cmd) {
		nrf_wifi_osal_mem_free(set_ps_exit_strategy_cmd);
	}

	return status;
}
#endif /* NRF71_STA_MODE */

enum nrf_wifi_status nrf_wifi_sys_fmac_stats_get(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
						 enum rpu_stats_type stats_type,
						 struct rpu_sys_op_stats *stats)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	unsigned char count = 0;

	if (fmac_dev_ctx->op_mode != NRF_WIFI_OP_MODE_SYS) {
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

	status = umac_cmd_sys_prog_stats_get(fmac_dev_ctx, stats_type);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		goto out;
	}

	do {
		nrf_wifi_osal_sleep_ms(1);
		count++;
	} while ((fmac_dev_ctx->stats_req == true) &&
		 (count < NRF_WIFI_FMAC_STATS_RECV_TIMEOUT));

	if (count == NRF_WIFI_FMAC_STATS_RECV_TIMEOUT) {
		status = NRF_WIFI_STATUS_FAIL;
		fmac_dev_ctx->stats_req = false;
		nrf_wifi_osal_log_err("%s: Timed out (%d ms)", __func__,
				      NRF_WIFI_FMAC_STATS_RECV_TIMEOUT);
		goto out;
	}

	status = NRF_WIFI_STATUS_SUCCESS;
out:
	return status;
}


#ifdef NRF71_SYSTEM_WITH_RAW_MODES
enum nrf_wifi_status nrf_wifi_sys_fmac_set_mode(void *dev_ctx,
						unsigned char if_idx,
						unsigned char mode)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_cmd_raw_config_mode *umac_cmd_data = NULL;
	struct host_rpu_msg *umac_cmd = NULL;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = dev_ctx;
	int len = 0;

	if (!fmac_dev_ctx) {
		nrf_wifi_osal_log_err("%s: Invalid parameters",
				      __func__);
		goto out;
	}

	if (fmac_dev_ctx->op_mode != NRF_WIFI_OP_MODE_SYS) {
		nrf_wifi_osal_log_err("%s: Invalid op mode",
				      __func__);
		goto out;
	}

	len = sizeof(*umac_cmd_data);
	umac_cmd = umac_cmd_alloc(fmac_dev_ctx,
				  NRF_WIFI_HOST_RPU_MSG_TYPE_SYSTEM,
				  len);

	if (!umac_cmd) {
		nrf_wifi_osal_log_err("%s: umac_cmd_alloc failed",
				      __func__);
		goto out;
	}

	umac_cmd_data = (struct nrf_wifi_cmd_raw_config_mode *)(umac_cmd->msg);
	umac_cmd_data->sys_head.cmd_event = NRF_WIFI_CMD_RAW_CONFIG_MODE;
	umac_cmd_data->sys_head.len = len;
	umac_cmd_data->if_index = if_idx;
	umac_cmd_data->op_mode = mode;

	status = nrf_wifi_hal_ctrl_cmd_send(fmac_dev_ctx->hal_dev_ctx,
					    umac_cmd,
					    (sizeof(*umac_cmd) + len));
out:
	return status;
}
#endif

#if defined(NRF71_RAW_DATA_TX) || defined(NRF71_RAW_DATA_RX)
enum nrf_wifi_status nrf_wifi_sys_fmac_set_channel(void *dev_ctx,
						   unsigned char if_idx,
						   unsigned int channel)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_cmd_set_channel *umac_cmd_data = NULL;
	struct host_rpu_msg *umac_cmd = NULL;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = dev_ctx;
	int len = 0;

	if (!fmac_dev_ctx) {
		nrf_wifi_osal_log_err("%s: Invalid parameters",
				      __func__);
		goto out;
	}

	if (fmac_dev_ctx->op_mode != NRF_WIFI_OP_MODE_SYS) {
		nrf_wifi_osal_log_err("%s: Invalid op mode",
				      __func__);
		goto out;
	}

	len = sizeof(*umac_cmd_data);
	umac_cmd = umac_cmd_alloc(fmac_dev_ctx,
				  NRF_WIFI_HOST_RPU_MSG_TYPE_SYSTEM,
				  len);

	if (!umac_cmd) {
		nrf_wifi_osal_log_err("%s: umac_cmd_alloc failed",
				      __func__);
		goto out;
	}

	umac_cmd_data = (struct nrf_wifi_cmd_set_channel *)(umac_cmd->msg);
	umac_cmd_data->sys_head.cmd_event = NRF_WIFI_CMD_CHANNEL;
	umac_cmd_data->sys_head.len = len;
	umac_cmd_data->if_index = if_idx;
	umac_cmd_data->chan.primary_num = channel;

	status = nrf_wifi_hal_ctrl_cmd_send(fmac_dev_ctx->hal_dev_ctx,
					    umac_cmd,
					    (sizeof(*umac_cmd) + len));
out:
	return status;
}
#endif /* NRF71_RAW_DATA_TX || NRF71_RAW_DATA_RX */

#if defined(NRF71_RAW_DATA_RX) || defined(NRF71_PROMISC_DATA_RX)
enum nrf_wifi_status nrf_wifi_sys_fmac_set_packet_filter(void *dev_ctx, unsigned char filter,
							 unsigned char if_idx,
							 unsigned short buffer_size)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_cmd_raw_config_filter *umac_cmd_data = NULL;
	struct host_rpu_msg *umac_cmd = NULL;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = dev_ctx;
	int len = 0;

	if (!fmac_dev_ctx) {
		nrf_wifi_osal_log_err("%s: Invalid parameters",
				      __func__);
		goto out;
	}

	if (fmac_dev_ctx->op_mode != NRF_WIFI_OP_MODE_SYS) {
		nrf_wifi_osal_log_err("%s: Invalid op mode",
				      __func__);
		goto out;
	}

	len = sizeof(*umac_cmd_data);
	umac_cmd = umac_cmd_alloc(fmac_dev_ctx,
				  NRF_WIFI_HOST_RPU_MSG_TYPE_SYSTEM,
				  len);
	if (!umac_cmd) {
		nrf_wifi_osal_log_err("%s: umac_cmd_alloc failed",
				      __func__);
		goto out;
	}

	umac_cmd_data = (struct nrf_wifi_cmd_raw_config_filter *)(umac_cmd->msg);
	umac_cmd_data->sys_head.cmd_event = NRF_WIFI_CMD_RAW_CONFIG_FILTER;
	umac_cmd_data->sys_head.len = len;
	umac_cmd_data->if_index = if_idx;
	umac_cmd_data->filter = filter;
	umac_cmd_data->capture_len = buffer_size;

	status = nrf_wifi_hal_ctrl_cmd_send(fmac_dev_ctx->hal_dev_ctx,
					    umac_cmd,
					    (sizeof(*umac_cmd) + len));
out:
	return status;
}
#endif /* NRF71_RAW_DATA_RX || NRF71_PROMISC_DATA_RX */


#ifdef NRF71_UTIL
enum nrf_wifi_status nrf_wifi_sys_fmac_set_tx_rate(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
						   unsigned char rate_flag,
						   int data_rate)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct host_rpu_msg *umac_cmd = NULL;
	struct nrf_wifi_cmd_fix_tx_rate *umac_cmd_data = NULL;
	int len = 0;

	if (fmac_dev_ctx->op_mode != NRF_WIFI_OP_MODE_SYS) {
		nrf_wifi_osal_log_err("%s: Invalid op mode",
				      __func__);
		goto out;
	}

	len = sizeof(*umac_cmd_data);

	umac_cmd = umac_cmd_alloc(fmac_dev_ctx,
				  NRF_WIFI_HOST_RPU_MSG_TYPE_SYSTEM,
				  len);

	if (!umac_cmd) {
		nrf_wifi_osal_log_err("%s: umac_cmd_alloc failed",
				      __func__);
		goto out;
	}

	umac_cmd_data = (struct nrf_wifi_cmd_fix_tx_rate *)(umac_cmd->msg);

	umac_cmd_data->sys_head.cmd_event = NRF_WIFI_CMD_TX_FIX_DATA_RATE;
	umac_cmd_data->sys_head.len = len;

	umac_cmd_data->rate_flags = rate_flag;
	umac_cmd_data->fixed_rate = data_rate;

	status = nrf_wifi_hal_ctrl_cmd_send(fmac_dev_ctx->hal_dev_ctx,
					    umac_cmd,
					    (sizeof(*umac_cmd) + len));
out:
	return status;
}

enum nrf_wifi_status nrf_wifi_sys_fmac_conf_ltf_gi(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
						   unsigned char he_ltf,
						   unsigned char he_gi,
						   unsigned char enabled)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;

	if (fmac_dev_ctx->op_mode != NRF_WIFI_OP_MODE_SYS) {
		nrf_wifi_osal_log_err("%s: Invalid op mode",
				      __func__);
		goto out;
	}

	status = umac_cmd_sys_he_ltf_gi(fmac_dev_ctx, he_ltf, he_gi, enabled);

out:
	return status;
}

#ifdef NRF_WIFI_LOW_POWER
enum nrf_wifi_status nrf_wifi_sys_fmac_get_host_rpu_ps_ctrl_state(void *dev_ctx,
								  int *rpu_ps_ctrl_state)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;

	fmac_dev_ctx = dev_ctx;

	if (!fmac_dev_ctx || !rpu_ps_ctrl_state) {
		nrf_wifi_osal_log_err("%s: Invalid parameters",
				      __func__);
		goto out;
	}

	if (fmac_dev_ctx->op_mode != NRF_WIFI_OP_MODE_SYS) {
		nrf_wifi_osal_log_err("%s: Invalid op mode",
				      __func__);
		goto out;
	}

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		nrf_wifi_osal_log_err("%s: Fetching of RPU PS state failed",
				      __func__);
		goto out;
	}
out:
	return status;
}
#endif /* NRF_WIFI_LOW_POWER */

enum nrf_wifi_status nrf_wifi_sys_fmac_debug_stats_get(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
						       enum rpu_stats_type stats_type,
						       struct nrf_wifi_rpu_debug_stats *stats)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	unsigned char count = 0;

	if (!fmac_dev_ctx || !stats) {
		nrf_wifi_osal_log_err("%s: Invalid parameters", __func__);
		goto out;
	}

	if (fmac_dev_ctx->op_mode != NRF_WIFI_OP_MODE_SYS) {
		nrf_wifi_osal_log_err("%s: Invalid op mode", __func__);
		goto out;
	}

	if (fmac_dev_ctx->debug_stats_req) {
		nrf_wifi_osal_log_err("%s: Debug stats request already pending",
				      __func__);
		goto out;
	}


	fmac_dev_ctx->debug_stats_req = true;
	fmac_dev_ctx->debug_stats = stats;

	status = umac_cmd_sys_debug_stats_get(fmac_dev_ctx, stats_type);
	if (status != NRF_WIFI_STATUS_SUCCESS) {
		fmac_dev_ctx->debug_stats_req = false;
		goto out;
	}

	do {
		nrf_wifi_osal_sleep_ms(1);
		count++;
	} while ((fmac_dev_ctx->debug_stats_req == true) &&
		 (count < NRF_WIFI_FMAC_STATS_RECV_TIMEOUT));

	if (count == NRF_WIFI_FMAC_STATS_RECV_TIMEOUT) {
		status = NRF_WIFI_STATUS_FAIL;
		fmac_dev_ctx->debug_stats_req = false;
		nrf_wifi_osal_log_err("%s: Timed out (%d ms)", __func__,
				      NRF_WIFI_FMAC_STATS_RECV_TIMEOUT);
		goto out;
	}

	status = NRF_WIFI_STATUS_SUCCESS;
out:
	return status;
}

enum nrf_wifi_status nrf_wifi_sys_fmac_umac_int_stats_get(
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
	struct umac_int_stats *stats)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	unsigned char count = 0;

	if (!fmac_dev_ctx || !stats) {
		nrf_wifi_osal_log_err("%s: Invalid parameters", __func__);
		goto out;
	}

	if (fmac_dev_ctx->op_mode != NRF_WIFI_OP_MODE_SYS) {
		nrf_wifi_osal_log_err("%s: Invalid op mode", __func__);
		goto out;
	}

	if (fmac_dev_ctx->umac_int_stats_req) {
		nrf_wifi_osal_log_err("%s: UMAC int stats request already pending",
				      __func__);
		goto out;
	}

	fmac_dev_ctx->umac_int_stats_req = true;
	fmac_dev_ctx->umac_int_stats = stats;

	status = umac_cmd_sys_umac_int_stats_get(fmac_dev_ctx);
	if (status != NRF_WIFI_STATUS_SUCCESS) {
		fmac_dev_ctx->umac_int_stats_req = false;
		goto out;
	}

	do {
		nrf_wifi_osal_sleep_ms(1);
		count++;
	} while ((fmac_dev_ctx->umac_int_stats_req == true) &&
		 (count < NRF_WIFI_FMAC_STATS_RECV_TIMEOUT));

	if (count == NRF_WIFI_FMAC_STATS_RECV_TIMEOUT) {
		status = NRF_WIFI_STATUS_FAIL;
		fmac_dev_ctx->umac_int_stats_req = false;
		nrf_wifi_osal_log_err("%s: Timed out (%d ms)", __func__,
				      NRF_WIFI_FMAC_STATS_RECV_TIMEOUT);
		goto out;
	}

	status = NRF_WIFI_STATUS_SUCCESS;
out:
	return status;
}

#endif /* NRF71_UTIL */


#ifdef NRF_WIFI_RX_BUFF_PROG_UMAC
#define MAX_BUFS_PER_CMD 32
enum nrf_wifi_status nrf_wifi_fmac_prog_rx_buf_info(void *dev_ctx,
						struct nrf_wifi_rx_buf *rx_buf,
						unsigned int rx_buf_nums)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_cmd_rx_buf_info *rx_buf_cmd = NULL;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;
	struct nrf_wifi_fmac_dev_ctx_def *def_dev_ctx = NULL;
	struct nrf_wifi_rx_buf *rx_buf_iter = NULL;
	int i = 0, remained_buf_cnt = 0, counter = 0,  rx_buff_prog_cnt = 0;

	fmac_dev_ctx = dev_ctx;
	def_dev_ctx = wifi_dev_priv(fmac_dev_ctx);

	if (rx_buf_nums > MAX_BUFS_PER_CMD) {
		rx_buff_prog_cnt = MAX_BUFS_PER_CMD;
		remained_buf_cnt = rx_buf_nums % MAX_BUFS_PER_CMD;
		counter = rx_buf_nums / MAX_BUFS_PER_CMD;
	} else {
		rx_buff_prog_cnt = rx_buf_nums;
		counter = 1;
	}

	rx_buf_iter = rx_buf;

	for (i = 0; i < counter; i++) {
		rx_buf_cmd = nrf_wifi_osal_mem_zalloc(sizeof(*rx_buf_cmd) +
					rx_buff_prog_cnt * sizeof(struct nrf_wifi_rx_buf));
		if (!rx_buf_cmd) {
			nrf_wifi_osal_log_err("%s: Unable to allocate memory\n", __func__);
			goto out;
		}

		rx_buf_cmd->umac_hdr.cmd_evnt = NRF_WIFI_UMAC_CMD_CONFIG_RX_BUF;
		nrf_wifi_osal_mem_cpy(&rx_buf_cmd->info,
				      rx_buf,
				      rx_buff_prog_cnt * sizeof(struct nrf_wifi_rx_buf));

		rx_buf_cmd->rx_buf_num = rx_buff_prog_cnt;

		status = umac_cmd_cfg(fmac_dev_ctx,
				      rx_buf_cmd,
				      sizeof(*rx_buf_cmd) +
				      rx_buff_prog_cnt * sizeof(struct nrf_wifi_rx_buf));
		if (rx_buf_cmd) {
			nrf_wifi_osal_mem_free(rx_buf_cmd);
		}
	}
	if (remained_buf_cnt > 0) {
		rx_buf_cmd = nrf_wifi_osal_mem_zalloc(sizeof(*rx_buf_cmd) +
					remained_buf_cnt * sizeof(struct nrf_wifi_rx_buf));
		if (!rx_buf_cmd) {
			nrf_wifi_osal_log_err("%s: Unable to allocate memory\n", __func__);
			goto out;
		}

		rx_buf_cmd->umac_hdr.cmd_evnt = NRF_WIFI_UMAC_CMD_CONFIG_RX_BUF;
		nrf_wifi_osal_mem_cpy(&rx_buf_cmd->info,
				      rx_buf + (MAX_BUFS_PER_CMD),
				      remained_buf_cnt *
				      sizeof(struct nrf_wifi_rx_buf));

		rx_buf_cmd->rx_buf_num = remained_buf_cnt;

		status = umac_cmd_cfg(fmac_dev_ctx,
				      rx_buf_cmd,
				      sizeof(*rx_buf_cmd) +
				      remained_buf_cnt * sizeof(struct nrf_wifi_rx_buf));
		if (rx_buf_cmd) {
			nrf_wifi_osal_mem_free(rx_buf_cmd);
		}
	}
out:
	return status;
}
#endif /*NRF_WIFI_RX_BUFF_PROG_UMAC */
