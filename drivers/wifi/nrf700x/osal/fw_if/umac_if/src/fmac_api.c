/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief File containing API definitions for the
 * FMAC IF Layer of the Wi-Fi driver.
 */

#include "host_rpu_umac_if.h"
#include "hal_api.h"
#include "fmac_structs.h"
#include "fmac_util.h"
#include "fmac_peer.h"
#include "fmac_vif.h"
#include "fmac_tx.h"
#include "fmac_rx.h"
#include "fmac_cmd.h"
#include "fmac_event.h"
#include "fmac_bb.h"

static unsigned char wifi_nrf_fmac_vif_idx_get(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx)
{
	unsigned char i = 0;

	for (i = 0; i < MAX_NUM_VIFS; i++) {
		if (fmac_dev_ctx->vif_ctx[i] == NULL) {
			break;
		}
	}

	return i;
}


static enum wifi_nrf_status wifi_nrf_fmac_init_tx(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx)
{
	struct wifi_nrf_fmac_priv *fpriv = NULL;
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	unsigned int size = 0;

	fpriv = fmac_dev_ctx->fpriv;

	size = (fpriv->num_tx_tokens *
		fpriv->data_config.max_tx_aggregation *
		sizeof(struct wifi_nrf_fmac_buf_map_info));

	fmac_dev_ctx->tx_buf_info = wifi_nrf_osal_mem_zalloc(fmac_dev_ctx->fpriv->opriv,
							     size);

	if (!fmac_dev_ctx->tx_buf_info) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: No space for TX buf info\n",
				      __func__);
		goto out;
	}

	status = tx_init(fmac_dev_ctx);

out:
	return status;
}


static void wifi_nrf_fmac_deinit_tx(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx)
{
	struct wifi_nrf_fmac_priv *fpriv = NULL;

	fpriv = fmac_dev_ctx->fpriv;

	tx_deinit(fmac_dev_ctx);

	wifi_nrf_osal_mem_free(fmac_dev_ctx->fpriv->opriv,
			       fmac_dev_ctx->tx_buf_info);
}


static enum wifi_nrf_status wifi_nrf_fmac_init_rx(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx)
{
	struct wifi_nrf_fmac_priv *fpriv = NULL;
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	unsigned int size = 0;
	unsigned int desc_id = 0;

	fpriv = fmac_dev_ctx->fpriv;

	size = (fpriv->num_rx_bufs * sizeof(struct wifi_nrf_fmac_buf_map_info));

	fmac_dev_ctx->rx_buf_info = wifi_nrf_osal_mem_zalloc(fmac_dev_ctx->fpriv->opriv,
							     size);

	if (!fmac_dev_ctx->rx_buf_info) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: No space for RX buf info\n",
				      __func__);
		goto out;
	}

	for (desc_id = 0; desc_id < fmac_dev_ctx->fpriv->num_rx_bufs; desc_id++) {
		status = wifi_nrf_fmac_rx_cmd_send(fmac_dev_ctx,
						   WIFI_NRF_FMAC_RX_CMD_TYPE_INIT,
						   desc_id);

		if (status != WIFI_NRF_STATUS_SUCCESS) {
			wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
					      "%s: wifi_nrf_fmac_rx_cmd_send(INIT) failed for desc_id = %d\n",
					      __func__,
					      desc_id);
			goto out;
		}
	}
out:
	return status;
}


static enum wifi_nrf_status wifi_nrf_fmac_deinit_rx(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx)
{
	struct wifi_nrf_fmac_priv *fpriv = NULL;
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	unsigned int desc_id = 0;

	fpriv = fmac_dev_ctx->fpriv;

	for (desc_id = 0; desc_id < fpriv->num_rx_bufs; desc_id++) {
		status = wifi_nrf_fmac_rx_cmd_send(fmac_dev_ctx,
						   WIFI_NRF_FMAC_RX_CMD_TYPE_DEINIT,
						   desc_id);

		if (status != WIFI_NRF_STATUS_SUCCESS) {
			wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
					      "%s: wifi_nrf_fmac_rx_cmd_send(DEINIT) failed for desc_id = %d\n",
					      __func__,
					      desc_id);
			goto out;
		}
	}

	wifi_nrf_osal_mem_free(fmac_dev_ctx->fpriv->opriv,
			       fmac_dev_ctx->rx_buf_info);

	fmac_dev_ctx->rx_buf_info = NULL;
out:
	return status;
}


static enum wifi_nrf_status wifi_nrf_fmac_fw_init(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
						  unsigned char *mac,
						  unsigned char if_idx,
						  unsigned char *rf_params,
						  bool rf_params_valid,
#ifdef CONFIG_NRF_WIFI_LOW_POWER
						  int sleep_type,
#endif /* CONFIG_NRF_WIFI_LOW_POWER */
						  unsigned int phy_calib)
{
	unsigned long start_time_us = 0;
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;

	wifi_nrf_osal_mem_cpy(fmac_dev_ctx->fpriv->opriv,
			      fmac_dev_ctx->base_mac_addr,
			      mac,
			      IMG_ETH_ALEN);


	status = wifi_nrf_fmac_init_tx(fmac_dev_ctx);

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Init TX failed\n",
				      __func__);
		goto out;
	}

	status = wifi_nrf_fmac_init_rx(fmac_dev_ctx);

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Init RX failed\n",
				      __func__);
		wifi_nrf_fmac_deinit_tx(fmac_dev_ctx);
		goto out;
	}


	status = umac_cmd_init(fmac_dev_ctx,
			       mac,
			       if_idx,
			       rf_params,
			       rf_params_valid,
#ifdef CONFIG_NRF_WIFI_LOW_POWER
			       sleep_type,
#endif /* CONFIG_NRF_WIFI_LOW_POWER */
			       fmac_dev_ctx->fpriv->data_config,
			       phy_calib);

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: UMAC init failed\n",
				      __func__);
		wifi_nrf_fmac_deinit_rx(fmac_dev_ctx);
		wifi_nrf_fmac_deinit_tx(fmac_dev_ctx);
		goto out;
	}

	start_time_us = wifi_nrf_osal_time_get_curr_us(fmac_dev_ctx->fpriv->opriv);

	while (!fmac_dev_ctx->init_done) {
		wifi_nrf_osal_sleep_ms(fmac_dev_ctx->fpriv->opriv, 100);
#define MAX_INIT_WAIT (5 * 1000 * 1000)
		if (wifi_nrf_osal_time_elapsed_us(fmac_dev_ctx->fpriv->opriv,
						  start_time_us) >= MAX_INIT_WAIT) {
			break;
		}
	}

	if (!fmac_dev_ctx->init_done) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: UMAC init timed out\n",
				      __func__);
		wifi_nrf_fmac_deinit_rx(fmac_dev_ctx);
		wifi_nrf_fmac_deinit_tx(fmac_dev_ctx);
		status = WIFI_NRF_STATUS_FAIL;
		goto out;
	}

	status = WIFI_NRF_STATUS_SUCCESS;

out:
	return status;
}

static void wifi_nrf_fmac_fw_deinit(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx)
{
	unsigned long start_time_us = 0;
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;


	/* TODO: To be activated once UMAC supports deinit */
	status = umac_cmd_deinit(fmac_dev_ctx);

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: UMAC deinit failed\n",
				      __func__);
		goto out;
	}

	start_time_us = wifi_nrf_osal_time_get_curr_us(fmac_dev_ctx->fpriv->opriv);

	while (!fmac_dev_ctx->deinit_done) {
#define MAX_DEINIT_WAIT (5 * 1000 * 1000)
		if (wifi_nrf_osal_time_elapsed_us(fmac_dev_ctx->fpriv->opriv,
						  start_time_us) >= MAX_DEINIT_WAIT) {
			break;
		}
	}

	if (!fmac_dev_ctx->deinit_done) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: UMAC deinit timed out\n",
				      __func__);
		status = WIFI_NRF_STATUS_FAIL;
		goto out;
	}
out:

	wifi_nrf_fmac_deinit_rx(fmac_dev_ctx);
	wifi_nrf_fmac_deinit_tx(fmac_dev_ctx);
}


struct wifi_nrf_fmac_dev_ctx *wifi_nrf_fmac_dev_add(struct wifi_nrf_fmac_priv *fpriv,
						    void *os_dev_ctx)
{
	struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx = NULL;

	if (!fpriv || !os_dev_ctx) {
		return NULL;
	}

	fmac_dev_ctx = wifi_nrf_osal_mem_zalloc(fpriv->opriv,
						sizeof(*fmac_dev_ctx));

	if (!fmac_dev_ctx) {
		wifi_nrf_osal_log_err(fpriv->opriv,
				      "%s: Unable to allocate fmac_dev_ctx\n",
				      __func__);
		goto out;
	}

	fmac_dev_ctx->fpriv = fpriv;
	fmac_dev_ctx->os_dev_ctx = os_dev_ctx;

	fmac_dev_ctx->hal_dev_ctx = wifi_nrf_hal_dev_add(fpriv->hpriv,
							 fmac_dev_ctx);

	if (!fmac_dev_ctx->hal_dev_ctx) {
		wifi_nrf_osal_log_err(fpriv->opriv,
				      "%s: wifi_nrf_hal_dev_add failed\n",
				      __func__);

		wifi_nrf_osal_mem_free(fpriv->opriv,
				       fmac_dev_ctx);
		fmac_dev_ctx = NULL;
		goto out;
	}
out:
	return fmac_dev_ctx;
}


void wifi_nrf_fmac_dev_rem(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx)
{
	wifi_nrf_hal_dev_rem(fmac_dev_ctx->hal_dev_ctx);

	wifi_nrf_osal_mem_free(fmac_dev_ctx->fpriv->opriv,
			       fmac_dev_ctx);
}


struct host_rpu_umac_info *wifi_nrf_fmac_umac_info(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx)
{
	return wifi_nrf_hal_umac_info(fmac_dev_ctx->hal_dev_ctx);
}


enum wifi_nrf_status wifi_nrf_fmac_dev_init(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
					    struct wifi_nrf_fmac_init_dev_params *params)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;

	status = wifi_nrf_hal_dev_init(fmac_dev_ctx->hal_dev_ctx);

	if (status == WIFI_NRF_STATUS_FAIL) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: wifi_nrf_hal_dev_init failed\n",
				      __func__);
		goto out;
	}

	status = wifi_nrf_fmac_fw_init(fmac_dev_ctx,
				       params->base_mac_addr,
				       params->def_vif_idx,
				       params->rf_params,
				       params->rf_params_valid,
#ifdef CONFIG_NRF_WIFI_LOW_POWER
				       params->sleep_type,
#endif /* CONFIG_NRF_WIFI_LOW_POWER */
				       params->phy_calib);

	if (status == WIFI_NRF_STATUS_FAIL) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: wifi_nrf_fmac_fw_init failed\n",
				      __func__);
		goto out;
	}

out:
	return status;
}


void wifi_nrf_fmac_dev_deinit(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx)
{
	wifi_nrf_fmac_fw_deinit(fmac_dev_ctx);
}


struct wifi_nrf_fmac_priv *wifi_nrf_fmac_init(struct img_data_config_params *data_config,
					      struct rx_buf_pool_params *rx_buf_pools,
					      struct wifi_nrf_fmac_callbk_fns *callbk_fns)
{
	struct wifi_nrf_osal_priv *opriv = NULL;
	struct wifi_nrf_fmac_priv *fpriv = NULL;
	struct wifi_nrf_hal_cfg_params hal_cfg_params;
	unsigned int pool_idx = 0;
	unsigned int desc = 0;

	opriv = wifi_nrf_osal_init();

	if (!opriv) {
		goto out;
	}

	fpriv = wifi_nrf_osal_mem_zalloc(opriv,
					 sizeof(*fpriv));

	if (!fpriv) {
		wifi_nrf_osal_log_err(opriv,
				      "%s: Unable to allocate fpriv\n",
				      __func__);
		goto out;
	}

	fpriv->opriv = opriv;

	wifi_nrf_osal_mem_cpy(opriv,
			      &fpriv->callbk_fns,
			      callbk_fns,
			      sizeof(fpriv->callbk_fns));

	wifi_nrf_osal_mem_cpy(opriv,
			      &fpriv->data_config,
			      data_config,
			      sizeof(fpriv->data_config));

	fpriv->num_tx_tokens = MAX_TX_TOKENS;
	fpriv->num_tx_tokens_per_ac = (fpriv->num_tx_tokens / WIFI_NRF_FMAC_AC_MAX);
	fpriv->num_tx_tokens_spare = (fpriv->num_tx_tokens % WIFI_NRF_FMAC_AC_MAX);

	wifi_nrf_osal_mem_cpy(opriv,
			      fpriv->rx_buf_pools,
			      rx_buf_pools,
			      sizeof(fpriv->rx_buf_pools));

	for (pool_idx = 0; pool_idx < MAX_NUM_OF_RX_QUEUES; pool_idx++) {
		fpriv->rx_desc[pool_idx] = desc;

		desc += fpriv->rx_buf_pools[pool_idx].num_bufs;
	}

	fpriv->num_rx_bufs = desc;

	hal_cfg_params.max_cmd_size = MAX_IMG_UMAC_CMD_SIZE;
	hal_cfg_params.max_event_size = MAX_EVENT_POOL_LEN;

	hal_cfg_params.rx_buf_headroom_sz = RX_BUF_HEADROOM;
	hal_cfg_params.tx_buf_headroom_sz = TX_BUF_HEADROOM;

	hal_cfg_params.max_tx_frms = (fpriv->num_tx_tokens *
				      fpriv->data_config.max_tx_aggregation);

	for (pool_idx = 0; pool_idx < MAX_NUM_OF_RX_QUEUES; pool_idx++) {
		hal_cfg_params.rx_buf_pool[pool_idx].num_bufs =
			fpriv->rx_buf_pools[pool_idx].num_bufs;
		hal_cfg_params.rx_buf_pool[pool_idx].buf_sz =
			fpriv->rx_buf_pools[pool_idx].buf_sz + RX_BUF_HEADROOM;
	}

	hal_cfg_params.max_tx_frm_sz = TX_MAX_DATA_SIZE + TX_BUF_HEADROOM;

	fpriv->hpriv = wifi_nrf_hal_init(opriv,
					 &hal_cfg_params,
					 &wifi_nrf_fmac_event_callback);

	if (!fpriv->hpriv) {
		wifi_nrf_osal_log_err(opriv,
				      "%s: Unable to do HAL init\n",
				      __func__);
		wifi_nrf_osal_mem_free(opriv,
				       fpriv);
		fpriv = NULL;
		wifi_nrf_osal_deinit(opriv);
		opriv = NULL;
		goto out;
	}
out:
	return fpriv;
}


void wifi_nrf_fmac_deinit(struct wifi_nrf_fmac_priv *fpriv)
{
	struct wifi_nrf_osal_priv *opriv = NULL;

	opriv = fpriv->opriv;

	wifi_nrf_hal_deinit(fpriv->hpriv);

	wifi_nrf_osal_mem_free(opriv,
			       fpriv);

	wifi_nrf_osal_deinit(opriv);
}


enum wifi_nrf_status wifi_nrf_fmac_fw_load(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
					   struct wifi_nrf_fmac_fw_info *fmac_fw)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;

	/* Load the LMAC patches if available */
	if ((fmac_fw->lmac_patch_pri.data) && (fmac_fw->lmac_patch_pri.size)) {
		status = wifi_nrf_hal_proc_reset(fmac_dev_ctx->hal_dev_ctx,
						 RPU_PROC_TYPE_MCU_LMAC);

		if (status != WIFI_NRF_STATUS_SUCCESS) {
			wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
					      "%s: LMAC processor reset failed\n",
					      __func__);

			goto out;
		}

		/* Load the LMAC patches */
		status = wifi_nrf_hal_fw_patch_load(fmac_dev_ctx->hal_dev_ctx,
						    RPU_PROC_TYPE_MCU_LMAC,
						    fmac_fw->lmac_patch_pri.data,
						    fmac_fw->lmac_patch_pri.size,
						    fmac_fw->lmac_patch_sec.data,
						    fmac_fw->lmac_patch_sec.size);

		if (status != WIFI_NRF_STATUS_SUCCESS) {
			wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
					      "%s: LMAC patch load failed\n",
					      __func__);
			goto out;
		} else {
			wifi_nrf_osal_log_info(fmac_dev_ctx->fpriv->opriv,
					       "%s: LMAC patches loaded\n",
					       __func__);
		}

		status = wifi_nrf_hal_fw_patch_boot(fmac_dev_ctx->hal_dev_ctx,
						    RPU_PROC_TYPE_MCU_LMAC,
						    true);

		if (status != WIFI_NRF_STATUS_SUCCESS) {
			wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
					      "%s: Failed to boot LMAC with patch\n",
					      __func__);
			goto out;
		}

		status = wifi_nrf_hal_fw_chk_boot(fmac_dev_ctx->hal_dev_ctx,
						  RPU_PROC_TYPE_MCU_LMAC);

		if (status != WIFI_NRF_STATUS_SUCCESS) {
			wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
					      "%s: LMAC ROM boot check failed\n",
					      __func__);
			goto out;
		} else {
			wifi_nrf_osal_log_info(fmac_dev_ctx->fpriv->opriv,
					       "%s: LMAC boot check passed\n",
					       __func__);
		}
	} else {
		status = wifi_nrf_hal_fw_patch_boot(fmac_dev_ctx->hal_dev_ctx,
						    RPU_PROC_TYPE_MCU_LMAC,
						    false);

		if (status != WIFI_NRF_STATUS_SUCCESS) {
			wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
					      "%s: LMAC ROM boot failed\n",
					      __func__);
			goto out;
		}

		status = wifi_nrf_hal_fw_chk_boot(fmac_dev_ctx->hal_dev_ctx,
						  RPU_PROC_TYPE_MCU_LMAC);

		if (status != WIFI_NRF_STATUS_SUCCESS) {
			wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
					      "%s: LMAC ROM boot check failed\n",
					      __func__);
			goto out;
		} else {
			wifi_nrf_osal_log_info(fmac_dev_ctx->fpriv->opriv,
					       "%s: LMAC boot check passed\n",
					       __func__);
		}
	}

	/* Load the UMAC patches if available */
	if ((fmac_fw->umac_patch_pri.data) && (fmac_fw->umac_patch_pri.size)) {
		status = wifi_nrf_hal_proc_reset(fmac_dev_ctx->hal_dev_ctx,
						 RPU_PROC_TYPE_MCU_UMAC);

		if (status != WIFI_NRF_STATUS_SUCCESS) {
			wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
					      "%s: UMAC processor reset failed\n",
					      __func__);

			goto out;
		}

		/* Load the UMAC patches */
		status = wifi_nrf_hal_fw_patch_load(fmac_dev_ctx->hal_dev_ctx,
						    RPU_PROC_TYPE_MCU_UMAC,
						    fmac_fw->umac_patch_pri.data,
						    fmac_fw->umac_patch_pri.size,
						    fmac_fw->umac_patch_sec.data,
						    fmac_fw->umac_patch_sec.size);

		if (status != WIFI_NRF_STATUS_SUCCESS) {
			wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
					      "%s: UMAC patch load failed\n",
					      __func__);
			goto out;
		} else {
			wifi_nrf_osal_log_info(fmac_dev_ctx->fpriv->opriv,
					       "%s: UMAC patches loaded\n",
					       __func__);
		}

		status = wifi_nrf_hal_fw_patch_boot(fmac_dev_ctx->hal_dev_ctx,
						    RPU_PROC_TYPE_MCU_UMAC,
						    true);

		if (status != WIFI_NRF_STATUS_SUCCESS) {
			wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
					      "%s: Failed to boot UMAC with patch\n",
					      __func__);
			goto out;
		}

		status = wifi_nrf_hal_fw_chk_boot(fmac_dev_ctx->hal_dev_ctx,
						  RPU_PROC_TYPE_MCU_UMAC);

		if (status != WIFI_NRF_STATUS_SUCCESS) {
			wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
					      "%s: UMAC ROM boot check failed\n",
					      __func__);
			goto out;
		} else {
			wifi_nrf_osal_log_info(fmac_dev_ctx->fpriv->opriv,
					       "%s: UMAC boot check passed\n",
					       __func__);
		}
	} else {
		status = wifi_nrf_hal_fw_patch_boot(fmac_dev_ctx->hal_dev_ctx,
						    RPU_PROC_TYPE_MCU_UMAC,
						    false);

		if (status != WIFI_NRF_STATUS_SUCCESS) {
			wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
					      "%s: UMAC ROM boot failed\n",
					      __func__);
			goto out;
		}

		status = wifi_nrf_hal_fw_chk_boot(fmac_dev_ctx->hal_dev_ctx,
						  RPU_PROC_TYPE_MCU_UMAC);

		if (status != WIFI_NRF_STATUS_SUCCESS) {
			wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
					      "%s: UMAC ROM boot check failed\n",
					      __func__);
			goto out;
		} else {
			wifi_nrf_osal_log_info(fmac_dev_ctx->fpriv->opriv,
					       "%s: UMAC boot check passed\n",
					       __func__);
		}
	}
out:
	return status;
}


enum wifi_nrf_status wifi_nrf_fmac_stats_get(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
					     struct rpu_op_stats *stats)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	unsigned char count = 0;
	int stats_type = RPU_STATS_TYPE_ALL;


	if ((stats_type == RPU_STATS_TYPE_ALL) ||
	    (stats_type == RPU_STATS_TYPE_UMAC) ||
	    (stats_type == RPU_STATS_TYPE_LMAC) ||
	    (stats_type == RPU_STATS_TYPE_PHY)) {
		if (fmac_dev_ctx->stats_req == true) {
			wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
					      "%s: Stats request already pending\n",
					      __func__);
			goto out;
		}

		fmac_dev_ctx->stats_req = true;
		fmac_dev_ctx->fw_stats = &stats->fw;

		status = umac_cmd_prog_stats_get(fmac_dev_ctx,
						 stats_type);

		if (status != WIFI_NRF_STATUS_SUCCESS) {
			goto out;
		}

		do {
			wifi_nrf_osal_sleep_ms(fmac_dev_ctx->fpriv->opriv,
					       1);
			count++;
		} while ((fmac_dev_ctx->stats_req == true) &&
			 (count < WIFI_NRF_FMAC_STATS_RECV_TIMEOUT));

		if (count == WIFI_NRF_FMAC_STATS_RECV_TIMEOUT) {
			wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
					      "%s: Timed out\n",
					      __func__);
			goto out;
		}
	}

	if ((stats_type == RPU_STATS_TYPE_ALL) ||
	    (stats_type == RPU_STATS_TYPE_HOST)) {
		wifi_nrf_osal_mem_cpy(fmac_dev_ctx->fpriv->opriv,
				      &stats->host,
				      &fmac_dev_ctx->host_stats,
				      sizeof(fmac_dev_ctx->host_stats));
	}

	status = WIFI_NRF_STATUS_SUCCESS;
out:
	return status;
}


enum wifi_nrf_status wifi_nrf_fmac_ver_get(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	unsigned int umac_ver_addr = 0;
	unsigned int lmac_ver_addr = 0;

	umac_ver_addr = RPU_MEM_UMAC_VER;
	lmac_ver_addr = RPU_MEM_LMAC_VER;

	status = hal_rpu_mem_read(fmac_dev_ctx->hal_dev_ctx,
				  &fmac_dev_ctx->umac_ver,
				  umac_ver_addr,
				  sizeof(fmac_dev_ctx->umac_ver));

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Unable to read UMAC ver\n",
				      __func__);
		goto out;
	}

	status = hal_rpu_mem_read(fmac_dev_ctx->hal_dev_ctx,
				  &fmac_dev_ctx->lmac_ver,
				  lmac_ver_addr,
				  sizeof(fmac_dev_ctx->lmac_ver));

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Unable to read LMAC ver\n",
				      __func__);
		goto out;
	}

out:
	return status;
}




enum wifi_nrf_status wifi_nrf_fmac_scan(void *dev_ctx,
					unsigned char if_idx,
					struct img_umac_scan_info *scan_info)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct img_umac_cmd_scan *scan_cmd = NULL;
	struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx = NULL;
	int channel_info_len = (sizeof(struct img_channel) *
				scan_info->scan_params.num_scan_channels);

	fmac_dev_ctx = dev_ctx;

	if (fmac_dev_ctx->vif_ctx[if_idx]->if_type == IMG_IFTYPE_AP) {
		wifi_nrf_osal_log_info(fmac_dev_ctx->fpriv->opriv,
				       "%s: Scan operation not supported in AP mode\n",
				       __func__);
		goto out;
	}

	if (scan_info->scan_params.num_scan_channels > MAX_NUM_CHANNELS) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Num of channels in scan list more than supported\n",
				      __func__);
		goto out;
	}

	scan_cmd = wifi_nrf_osal_mem_zalloc(fmac_dev_ctx->fpriv->opriv,
					    (sizeof(*scan_cmd) + channel_info_len));

	if (!scan_cmd) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Unable to allocate memory\n",
				      __func__);
		goto out;
	}

	scan_cmd->umac_hdr.cmd_evnt = IMG_UMAC_CMD_TRIGGER_SCAN;
	scan_cmd->umac_hdr.ids.wdev_id = if_idx;
	scan_cmd->umac_hdr.ids.valid_fields |= IMG_INDEX_IDS_WDEV_ID_VALID;

	wifi_nrf_osal_mem_cpy(fmac_dev_ctx->fpriv->opriv,
			      &scan_cmd->info,
			      scan_info,
			      (sizeof(scan_cmd->info) + channel_info_len));

	status = umac_cmd_cfg(fmac_dev_ctx,
			      scan_cmd,
			      sizeof(*scan_cmd) + channel_info_len);
out:
	if (scan_cmd) {
		wifi_nrf_osal_mem_free(fmac_dev_ctx->fpriv->opriv,
				       scan_cmd);
	}

	return status;
}

enum wifi_nrf_status wifi_nrf_fmac_abort_scan(void *dev_ctx,
					unsigned char if_idx)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct img_umac_cmd_abort_scan *scan_abort_cmd = NULL;
	struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx = NULL;

	fmac_dev_ctx = dev_ctx;

	if (fmac_dev_ctx->vif_ctx[if_idx]->if_type == IMG_IFTYPE_AP) {
		wifi_nrf_osal_log_info(fmac_dev_ctx->fpriv->opriv,
				       "%s: Scan operation not supported in AP mode\n",
				       __func__);
		goto out;
	}

	scan_abort_cmd = wifi_nrf_osal_mem_zalloc(fmac_dev_ctx->fpriv->opriv,
					    sizeof(*scan_abort_cmd));

	if (!scan_abort_cmd) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Unable to allocate memory\n",
				      __func__);
		goto out;
	}

	scan_abort_cmd->umac_hdr.cmd_evnt = IMG_UMAC_CMD_ABORT_SCAN;
	scan_abort_cmd->umac_hdr.ids.wdev_id = if_idx;
	scan_abort_cmd->umac_hdr.ids.valid_fields |= IMG_INDEX_IDS_WDEV_ID_VALID;

	status = umac_cmd_cfg(fmac_dev_ctx,
			      scan_abort_cmd,
			      sizeof(*scan_abort_cmd));
out:
	if (scan_abort_cmd) {
		wifi_nrf_osal_mem_free(fmac_dev_ctx->fpriv->opriv,
				       scan_abort_cmd);
	}

	return status;
}

enum wifi_nrf_status wifi_nrf_fmac_scan_res_get(void *dev_ctx,
						unsigned char vif_idx,
						int scan_type)

{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct img_umac_cmd_get_scan_results *scan_res_cmd = NULL;
	struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx = NULL;

	fmac_dev_ctx = dev_ctx;

	if (fmac_dev_ctx->vif_ctx[vif_idx]->if_type == IMG_IFTYPE_AP) {
		wifi_nrf_osal_log_info(fmac_dev_ctx->fpriv->opriv,
				       "%s: Scan operation not supported in AP mode\n",
				       __func__);
		goto out;
	}

	scan_res_cmd = wifi_nrf_osal_mem_zalloc(fmac_dev_ctx->fpriv->opriv,
						sizeof(*scan_res_cmd));

	if (!scan_res_cmd) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Unable to allocate memory\n",
				      __func__);
		goto out;
	}

	scan_res_cmd->umac_hdr.cmd_evnt = IMG_UMAC_CMD_GET_SCAN_RESULTS;
	scan_res_cmd->umac_hdr.ids.wdev_id = vif_idx;
	scan_res_cmd->umac_hdr.ids.valid_fields |= IMG_INDEX_IDS_WDEV_ID_VALID;
	scan_res_cmd->scan_reason = scan_type;

	status = umac_cmd_cfg(fmac_dev_ctx,
			      scan_res_cmd,
			      sizeof(*scan_res_cmd));
out:
	if (scan_res_cmd) {
		wifi_nrf_osal_mem_free(fmac_dev_ctx->fpriv->opriv,
				       scan_res_cmd);
	}

	return status;
}


enum wifi_nrf_status wifi_nrf_fmac_auth(void *dev_ctx,
					unsigned char if_idx,
					struct img_umac_auth_info *auth_info)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct img_umac_cmd_auth *auth_cmd = NULL;
	struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx = NULL;
	struct wifi_nrf_fmac_vif_ctx *vif_ctx = NULL;

	fmac_dev_ctx = dev_ctx;
	vif_ctx = fmac_dev_ctx->vif_ctx[if_idx];

	auth_cmd = wifi_nrf_osal_mem_zalloc(fmac_dev_ctx->fpriv->opriv,
					    sizeof(*auth_cmd));

	if (!auth_cmd) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Unable to allocate memory\n",
				      __func__);
		goto out;
	}

	auth_cmd->umac_hdr.cmd_evnt = IMG_UMAC_CMD_AUTHENTICATE;
	auth_cmd->umac_hdr.ids.wdev_id = if_idx;
	auth_cmd->umac_hdr.ids.valid_fields |= IMG_INDEX_IDS_WDEV_ID_VALID;

	wifi_nrf_osal_mem_cpy(fmac_dev_ctx->fpriv->opriv,
			      &auth_cmd->info,
			      auth_info,
			      sizeof(auth_cmd->info));

	if (auth_info->frequency != 0) {
		auth_cmd->valid_fields |= IMG_CMD_AUTHENTICATE_FREQ_VALID;
	}

	if (auth_info->ssid.img_ssid_len > 0) {
		auth_cmd->valid_fields |= IMG_CMD_AUTHENTICATE_SSID_VALID;
	}

	if (auth_info->key_info.key.img_key_len > 0) {
		auth_cmd->info.key_info.valid_fields |= IMG_KEY_IDX_VALID;
		auth_cmd->info.key_info.valid_fields |= IMG_KEY_VALID;
		auth_cmd->info.key_info.valid_fields |= IMG_CIPHER_SUITE_VALID;

		auth_cmd->valid_fields |= IMG_CMD_AUTHENTICATE_KEY_INFO_VALID;
	}

	if (auth_info->sae.sae_data_len > 0) {
		auth_cmd->valid_fields |= IMG_CMD_AUTHENTICATE_SAE_VALID;
	}

	status = umac_cmd_cfg(fmac_dev_ctx,
			      auth_cmd,
			      sizeof(*auth_cmd));
out:
	if (auth_cmd) {
		wifi_nrf_osal_mem_free(fmac_dev_ctx->fpriv->opriv,
				       auth_cmd);
	}

	return status;
}


enum wifi_nrf_status wifi_nrf_fmac_deauth(void *dev_ctx,
					  unsigned char if_idx,
					  struct img_umac_disconn_info *deauth_info)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct img_umac_cmd_disconn *deauth_cmd = NULL;
	struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx = NULL;

	fmac_dev_ctx = dev_ctx;

	deauth_cmd = wifi_nrf_osal_mem_zalloc(fmac_dev_ctx->fpriv->opriv,
					      sizeof(*deauth_cmd));

	if (!deauth_cmd) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Unable to allocate memory\n", __func__);
		goto out;
	}

	deauth_cmd->umac_hdr.cmd_evnt = IMG_UMAC_CMD_DEAUTHENTICATE;
	deauth_cmd->umac_hdr.ids.wdev_id = if_idx;
	deauth_cmd->umac_hdr.ids.valid_fields |= IMG_INDEX_IDS_WDEV_ID_VALID;

	wifi_nrf_osal_mem_cpy(fmac_dev_ctx->fpriv->opriv,
			      &deauth_cmd->info,
			      deauth_info,
			      sizeof(deauth_cmd->info));

	if (!wifi_nrf_util_is_arr_zero(deauth_info->mac_addr,
				       sizeof(deauth_info->mac_addr))) {
		deauth_cmd->valid_fields |= IMG_CMD_MLME_MAC_ADDR_VALID;
	}

	status = umac_cmd_cfg(fmac_dev_ctx,
			      deauth_cmd,
			      sizeof(*deauth_cmd));

out:
	if (deauth_cmd) {
		wifi_nrf_osal_mem_free(fmac_dev_ctx->fpriv->opriv,
				       deauth_cmd);
	}

	return status;
}


enum wifi_nrf_status wifi_nrf_fmac_assoc(void *dev_ctx,
					 unsigned char if_idx,
					 struct img_umac_assoc_info *assoc_info)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct img_umac_cmd_assoc *assoc_cmd = NULL;
	struct img_connect_common_info *connect_common_info = NULL;
	struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx = NULL;
	struct wifi_nrf_fmac_vif_ctx *vif_ctx = NULL;

	fmac_dev_ctx = dev_ctx;

	vif_ctx = fmac_dev_ctx->vif_ctx[if_idx];

	wifi_nrf_osal_mem_cpy(fmac_dev_ctx->fpriv->opriv,
			      vif_ctx->bssid,
			      assoc_info->img_bssid,
			      IMG_ETH_ALEN);

	assoc_cmd = wifi_nrf_osal_mem_zalloc(fmac_dev_ctx->fpriv->opriv,
					     sizeof(*assoc_cmd));

	if (!assoc_cmd) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Unable to allocate memory\n",
				      __func__);
		goto out;
	}

	assoc_cmd->umac_hdr.cmd_evnt = IMG_UMAC_CMD_ASSOCIATE;
	assoc_cmd->umac_hdr.ids.wdev_id = if_idx;
	assoc_cmd->umac_hdr.ids.valid_fields |= IMG_INDEX_IDS_WDEV_ID_VALID;

	connect_common_info = &assoc_cmd->connect_common_info;

	wifi_nrf_osal_mem_cpy(fmac_dev_ctx->fpriv->opriv,
			      connect_common_info->mac_addr,
			      assoc_info->img_bssid,
			      IMG_ETH_ALEN);

	if (!wifi_nrf_util_is_arr_zero(connect_common_info->mac_addr,
				       sizeof(connect_common_info->mac_addr))) {
		connect_common_info->valid_fields |=
			IMG_CONNECT_COMMON_INFO_MAC_ADDR_VALID;
	}

	if (assoc_info->ssid.img_ssid_len > 0) {
		wifi_nrf_osal_mem_cpy(fmac_dev_ctx->fpriv->opriv,
				      &connect_common_info->ssid,
				      &assoc_info->ssid,
				      sizeof(connect_common_info->ssid));

		connect_common_info->valid_fields |=
			IMG_CONNECT_COMMON_INFO_SSID_VALID;
	}

	connect_common_info->frequency = assoc_info->center_frequency;
	connect_common_info->valid_fields |= IMG_CONNECT_COMMON_INFO_FREQ_VALID;

	if (assoc_info->wpa_ie.ie_len > 0) {
		wifi_nrf_osal_mem_cpy(fmac_dev_ctx->fpriv->opriv,
				      &connect_common_info->wpa_ie,
				      &assoc_info->wpa_ie,
				      sizeof(connect_common_info->wpa_ie));

		connect_common_info->valid_fields |=
			IMG_CONNECT_COMMON_INFO_WPA_IE_VALID;
	}

	connect_common_info->use_mfp = assoc_info->use_mfp;
	connect_common_info->valid_fields |=
		IMG_CONNECT_COMMON_INFO_USE_MFP_VALID;

	connect_common_info->control_port =
		assoc_info->control_port;

	status = umac_cmd_cfg(fmac_dev_ctx,
			      assoc_cmd,
			      sizeof(*assoc_cmd));
out:
	if (assoc_cmd) {
		wifi_nrf_osal_mem_free(fmac_dev_ctx->fpriv->opriv,
				       assoc_cmd);
	}

	return status;
}


enum wifi_nrf_status wifi_nrf_fmac_disassoc(void *dev_ctx,
					    unsigned char if_idx,
					    struct img_umac_disconn_info *disassoc_info)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct img_umac_cmd_disconn *disassoc_cmd = NULL;
	struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx = NULL;

	fmac_dev_ctx = dev_ctx;

	disassoc_cmd = wifi_nrf_osal_mem_zalloc(fmac_dev_ctx->fpriv->opriv,
						sizeof(*disassoc_cmd));

	if (!disassoc_cmd) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Unable to allocate memory\n",
				      __func__);
		goto out;
	}

	disassoc_cmd->umac_hdr.cmd_evnt = IMG_UMAC_CMD_DEAUTHENTICATE;
	disassoc_cmd->umac_hdr.ids.wdev_id = if_idx;
	disassoc_cmd->umac_hdr.ids.valid_fields |= IMG_INDEX_IDS_WDEV_ID_VALID;

	wifi_nrf_osal_mem_cpy(fmac_dev_ctx->fpriv->opriv,
			      &disassoc_cmd->info,
			      disassoc_info,
			      sizeof(disassoc_cmd->info));

	if (!wifi_nrf_util_is_arr_zero(disassoc_info->mac_addr,
				       sizeof(disassoc_info->mac_addr))) {
		disassoc_cmd->valid_fields |= IMG_CMD_MLME_MAC_ADDR_VALID;
	}

	status = umac_cmd_cfg(fmac_dev_ctx,
			      disassoc_cmd,
			      sizeof(*disassoc_cmd));

out:
	if (disassoc_cmd) {
		wifi_nrf_osal_mem_free(fmac_dev_ctx->fpriv->opriv,
				       disassoc_cmd);
	}

	return status;
}


enum wifi_nrf_status wifi_nrf_fmac_add_key(void *dev_ctx,
					   unsigned char if_idx,
					   struct img_umac_key_info *key_info,
					   const char *mac_addr)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct img_umac_cmd_key *key_cmd = NULL;
	struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx = NULL;
	struct wifi_nrf_fmac_vif_ctx *vif_ctx = NULL;
	int peer_id = -1;

	fmac_dev_ctx = dev_ctx;
	vif_ctx = fmac_dev_ctx->vif_ctx[if_idx];

	key_cmd = wifi_nrf_osal_mem_zalloc(fmac_dev_ctx->fpriv->opriv,
					   sizeof(*key_cmd));

	if (!key_cmd) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Unable to allocate memory\n", __func__);
		goto out;
	}

	key_cmd->umac_hdr.cmd_evnt = IMG_UMAC_CMD_NEW_KEY;
	key_cmd->umac_hdr.ids.wdev_id = if_idx;
	key_cmd->umac_hdr.ids.valid_fields |= IMG_INDEX_IDS_WDEV_ID_VALID;

	wifi_nrf_osal_mem_cpy(fmac_dev_ctx->fpriv->opriv,
			      &key_cmd->key_info,
			      key_info,
			      sizeof(key_cmd->key_info));

	if (mac_addr) {
		wifi_nrf_osal_mem_cpy(fmac_dev_ctx->fpriv->opriv,
				      key_cmd->mac_addr,
				      mac_addr,
				      IMG_ETH_ALEN);

		key_cmd->valid_fields |= IMG_CMD_KEY_MAC_ADDR_VALID;
	}

	if (key_info->key_type == IMG_KEYTYPE_GROUP) {
		vif_ctx->groupwise_cipher = key_info->cipher_suite;
	} else if (key_info->key_type == IMG_KEYTYPE_PAIRWISE) {
		peer_id = wifi_nrf_fmac_peer_get_id(fmac_dev_ctx,
						    mac_addr);

		if (peer_id == -1) {
			wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
					      "%s: Invalid peer\n",
					      __func__);
			goto out;
		}

		fmac_dev_ctx->tx_config.peers[peer_id].pairwise_cipher = key_info->cipher_suite;
	} else {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Invalid key type %d\n",
				      __func__,
				      key_info->key_type);
		goto out;
	}

	if (key_info->key.img_key_len > 0) {
		key_cmd->key_info.valid_fields |= IMG_KEY_VALID;
		key_cmd->key_info.valid_fields |= IMG_KEY_IDX_VALID;
	}

	if (key_info->seq.img_seq_len > 0) {
		key_cmd->key_info.valid_fields |= IMG_SEQ_VALID;
	}

	key_cmd->key_info.valid_fields |= IMG_CIPHER_SUITE_VALID;
	key_cmd->key_info.valid_fields |= IMG_KEY_TYPE_VALID;

	status = umac_cmd_cfg(fmac_dev_ctx,
			      key_cmd,
			      sizeof(*key_cmd));

out:
	if (key_cmd) {
		wifi_nrf_osal_mem_free(fmac_dev_ctx->fpriv->opriv,
				       key_cmd);
	}

	return status;
}


enum wifi_nrf_status wifi_nrf_fmac_del_key(void *dev_ctx,
					   unsigned char if_idx,
					   struct img_umac_key_info *key_info,
					   const char *mac_addr)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct img_umac_cmd_key *key_cmd = NULL;
	struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx = NULL;
	struct wifi_nrf_fmac_vif_ctx *vif_ctx = NULL;

	fmac_dev_ctx = dev_ctx;

	key_cmd = wifi_nrf_osal_mem_zalloc(fmac_dev_ctx->fpriv->opriv,
					   sizeof(*key_cmd));

	if (!key_cmd) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Unable to allocate memory\n",
				      __func__);
		goto out;
	}

	key_cmd->umac_hdr.cmd_evnt = IMG_UMAC_CMD_DEL_KEY;
	key_cmd->umac_hdr.ids.wdev_id = if_idx;
	key_cmd->umac_hdr.ids.valid_fields |= IMG_INDEX_IDS_WDEV_ID_VALID;

	wifi_nrf_osal_mem_cpy(fmac_dev_ctx->fpriv->opriv,
			      &key_cmd->key_info,
			      key_info,
			      sizeof(key_cmd->key_info));

	if (mac_addr) {
		wifi_nrf_osal_mem_cpy(fmac_dev_ctx->fpriv->opriv,
				      key_cmd->mac_addr,
				      mac_addr,
				      IMG_ETH_ALEN);

		key_cmd->valid_fields |= IMG_CMD_KEY_MAC_ADDR_VALID;
	}

	key_cmd->key_info.valid_fields |= IMG_KEY_IDX_VALID;
	key_cmd->key_info.valid_fields |= IMG_KEY_TYPE_VALID;

	vif_ctx = fmac_dev_ctx->vif_ctx[if_idx];

	if (key_info->key_type == IMG_KEYTYPE_GROUP) {
		vif_ctx->groupwise_cipher = 0;
	}

	status = umac_cmd_cfg(fmac_dev_ctx,
			      key_cmd,
			      sizeof(*key_cmd));

out:
	if (key_cmd) {
		wifi_nrf_osal_mem_free(fmac_dev_ctx->fpriv->opriv,
				       key_cmd);
	}

	return status;
}


enum wifi_nrf_status wifi_nrf_fmac_set_key(void *dev_ctx,
					   unsigned char if_idx,
					   struct img_umac_key_info *key_info)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct img_umac_cmd_set_key *set_key_cmd = NULL;
	struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx = NULL;

	fmac_dev_ctx = dev_ctx;

	set_key_cmd = wifi_nrf_osal_mem_zalloc(fmac_dev_ctx->fpriv->opriv,
					       sizeof(*set_key_cmd));

	if (!set_key_cmd) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Unable to allocate memory\n",
				      __func__);
		goto out;
	}

	set_key_cmd->umac_hdr.cmd_evnt = IMG_UMAC_CMD_SET_KEY;
	set_key_cmd->umac_hdr.ids.wdev_id = if_idx;
	set_key_cmd->umac_hdr.ids.valid_fields |= IMG_INDEX_IDS_WDEV_ID_VALID;

	wifi_nrf_osal_mem_cpy(fmac_dev_ctx->fpriv->opriv,
			      &set_key_cmd->key_info,
			      key_info,
			      sizeof(set_key_cmd->key_info));

	set_key_cmd->key_info.valid_fields |= IMG_KEY_IDX_VALID;

	status = umac_cmd_cfg(fmac_dev_ctx,
			      set_key_cmd,
			      sizeof(*set_key_cmd));

out:
	if (set_key_cmd) {
		wifi_nrf_osal_mem_free(fmac_dev_ctx->fpriv->opriv,
				       set_key_cmd);
	}

	return status;
}


enum wifi_nrf_status wifi_nrf_fmac_set_bss(void *dev_ctx,
					   unsigned char if_idx,
					   struct img_umac_bss_info *bss_info)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct img_umac_cmd_set_bss *set_bss_cmd = NULL;
	struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx = NULL;

	fmac_dev_ctx = dev_ctx;

	set_bss_cmd = wifi_nrf_osal_mem_zalloc(fmac_dev_ctx->fpriv->opriv,
					       sizeof(*set_bss_cmd));

	if (!set_bss_cmd) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Unable to allocate memory\n",
				      __func__);
		goto out;
	}

	set_bss_cmd->umac_hdr.cmd_evnt = IMG_UMAC_CMD_SET_BSS;
	set_bss_cmd->umac_hdr.ids.wdev_id = if_idx;
	set_bss_cmd->umac_hdr.ids.valid_fields |= IMG_INDEX_IDS_WDEV_ID_VALID;

	wifi_nrf_osal_mem_cpy(fmac_dev_ctx->fpriv->opriv,
			      &set_bss_cmd->bss_info,
			      bss_info,
			      sizeof(set_bss_cmd->bss_info));

	set_bss_cmd->valid_fields = IMG_CMD_SET_BSS_CTS_VALID |
		IMG_CMD_SET_BSS_PREAMBLE_VALID |
		IMG_CMD_SET_BSS_SLOT_VALID |
		IMG_CMD_SET_BSS_HT_OPMODE_VALID |
		IMG_CMD_SET_BSS_AP_ISOLATE_VALID;

	if ((bss_info->p2p_go_ctwindow > 0) &&
	    (bss_info->p2p_go_ctwindow < 127)) {
		set_bss_cmd->valid_fields |=
			(IMG_CMD_SET_BSS_P2P_CTWINDOW_VALID |
			 IMG_CMD_SET_BSS_P2P_OPPPS_VALID);
	}

	status = umac_cmd_cfg(fmac_dev_ctx,
			      set_bss_cmd,
			      sizeof(*set_bss_cmd));

out:
	if (set_bss_cmd) {
		wifi_nrf_osal_mem_free(fmac_dev_ctx->fpriv->opriv,
				       set_bss_cmd);
	}

	return status;
}

enum wifi_nrf_status wifi_nrf_fmac_chg_bcn(void *dev_ctx,
					   unsigned char if_idx,
					   struct img_umac_set_beacon_info *data)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct img_umac_cmd_set_beacon *set_bcn_cmd = NULL;
	struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx = NULL;

	fmac_dev_ctx = dev_ctx;

	set_bcn_cmd = wifi_nrf_osal_mem_zalloc(fmac_dev_ctx->fpriv->opriv,
					       sizeof(*set_bcn_cmd));

	if (!set_bcn_cmd) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Unable to allocate memory\n", __func__);
		goto out;
	}

	set_bcn_cmd->umac_hdr.cmd_evnt = IMG_UMAC_CMD_SET_BEACON;
	set_bcn_cmd->umac_hdr.ids.wdev_id = if_idx;
	set_bcn_cmd->umac_hdr.ids.valid_fields |= IMG_INDEX_IDS_WDEV_ID_VALID;

	wifi_nrf_osal_mem_cpy(fmac_dev_ctx->fpriv->opriv,
			      &set_bcn_cmd->info,
			      data,
			      sizeof(set_bcn_cmd->info));

	wifi_nrf_osal_log_dbg(fmac_dev_ctx->fpriv->opriv,
			      "%s: Sending command to rpu\n",
			      __func__);

	status = umac_cmd_cfg(fmac_dev_ctx,
			      set_bcn_cmd,
			      sizeof(*set_bcn_cmd));

out:
	if (set_bcn_cmd) {
		wifi_nrf_osal_mem_free(fmac_dev_ctx->fpriv->opriv,
				       set_bcn_cmd);
	}

	return status;
}

enum wifi_nrf_status wifi_nrf_fmac_set_wiphy_params(void *dev_ctx,
						    unsigned char if_idx,
						    struct img_umac_set_wiphy_info *wiphy_info)
{
	struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx = NULL;
	struct img_umac_cmd_set_wiphy *set_wiphy_cmd = NULL;
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	int freq_params_valid = 0;

	fmac_dev_ctx = dev_ctx;

	set_wiphy_cmd = wifi_nrf_osal_mem_zalloc(fmac_dev_ctx->fpriv->opriv,
						 sizeof(*set_wiphy_cmd));

	if (!set_wiphy_cmd) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Unable to allocate memory\n",
				      __func__);
		goto out;
	}

	set_wiphy_cmd->umac_hdr.cmd_evnt = IMG_UMAC_CMD_SET_WIPHY;
	set_wiphy_cmd->umac_hdr.ids.wdev_id = if_idx;
	set_wiphy_cmd->umac_hdr.ids.valid_fields |= IMG_INDEX_IDS_WDEV_ID_VALID;

	if (wiphy_info->freq_params.frequency) {
		freq_params_valid = 1;
		wiphy_info->freq_params.valid_fields |=
			IMG_SET_FREQ_PARAMS_FREQ_VALID;
	}

	if (wiphy_info->freq_params.channel_width) {
		freq_params_valid = 1;
		wiphy_info->freq_params.valid_fields |=
			IMG_SET_FREQ_PARAMS_CHANNEL_WIDTH_VALID;
	}

	if (wiphy_info->freq_params.center_frequency1) {
		freq_params_valid = 1;
		wiphy_info->freq_params.valid_fields |=
			IMG_SET_FREQ_PARAMS_CENTER_FREQ1_VALID;
	}

	if (wiphy_info->freq_params.center_frequency2) {
		freq_params_valid = 1;
		wiphy_info->freq_params.valid_fields |=
			IMG_SET_FREQ_PARAMS_CENTER_FREQ2_VALID;
	}

	if (wiphy_info->freq_params.channel_type) {
		freq_params_valid = 1;
		wiphy_info->freq_params.valid_fields |=
			IMG_SET_FREQ_PARAMS_CHANNEL_TYPE_VALID;
	}

	if (freq_params_valid) {
		set_wiphy_cmd->valid_fields |=
			IMG_CMD_SET_WIPHY_FREQ_PARAMS_VALID;
	}

	if (wiphy_info->rts_threshold) {
		set_wiphy_cmd->valid_fields |=
			IMG_CMD_SET_WIPHY_RTS_THRESHOLD_VALID;
	}

	if (wiphy_info->frag_threshold) {
		set_wiphy_cmd->valid_fields |=
			IMG_CMD_SET_WIPHY_FRAG_THRESHOLD_VALID;
	}

	if (wiphy_info->retry_long) {
		set_wiphy_cmd->valid_fields |=
			IMG_CMD_SET_WIPHY_RETRY_LONG_VALID;
	}

	if (wiphy_info->retry_short) {
		set_wiphy_cmd->valid_fields |=
			IMG_CMD_SET_WIPHY_RETRY_SHORT_VALID;
	}

	wifi_nrf_osal_mem_cpy(fmac_dev_ctx->fpriv->opriv,
			      &set_wiphy_cmd->info,
			      wiphy_info,
			      sizeof(set_wiphy_cmd->info));

	status = umac_cmd_cfg(fmac_dev_ctx,
			      set_wiphy_cmd,
			      sizeof(*set_wiphy_cmd));
out:
	if (set_wiphy_cmd) {
		wifi_nrf_osal_mem_free(fmac_dev_ctx->fpriv->opriv,
				       set_wiphy_cmd);
	}

	return status;
}


enum wifi_nrf_status wifi_nrf_fmac_start_ap(void *dev_ctx,
					    unsigned char if_idx,
					    struct img_umac_start_ap_info *ap_info)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct img_umac_cmd_start_ap *start_ap_cmd = NULL;
	struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx = NULL;
	struct img_umac_set_wiphy_info *wiphy_info = NULL;

	fmac_dev_ctx = dev_ctx;

	start_ap_cmd = wifi_nrf_osal_mem_zalloc(fmac_dev_ctx->fpriv->opriv,
						sizeof(*start_ap_cmd));

	if (!start_ap_cmd) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Unable to allocate memory\n",
				      __func__);
		goto out;
	}

	start_ap_cmd->umac_hdr.cmd_evnt = IMG_UMAC_CMD_START_AP;
	start_ap_cmd->umac_hdr.ids.wdev_id = if_idx;
	start_ap_cmd->umac_hdr.ids.valid_fields |= IMG_INDEX_IDS_WDEV_ID_VALID;

	wifi_nrf_osal_mem_cpy(fmac_dev_ctx->fpriv->opriv,
			      &start_ap_cmd->info,
			      ap_info,
			      sizeof(start_ap_cmd->info));

	start_ap_cmd->valid_fields |=
		IMG_CMD_BEACON_INFO_BEACON_INTERVAL_VALID |
		IMG_CMD_BEACON_INFO_VERSIONS_VALID |
		IMG_CMD_BEACON_INFO_CIPHER_SUITE_GROUP_VALID;

	start_ap_cmd->info.freq_params.valid_fields |=
		IMG_SET_FREQ_PARAMS_FREQ_VALID |
		IMG_SET_FREQ_PARAMS_CHANNEL_WIDTH_VALID |
		IMG_SET_FREQ_PARAMS_CENTER_FREQ1_VALID |
		IMG_SET_FREQ_PARAMS_CENTER_FREQ2_VALID |
		IMG_SET_FREQ_PARAMS_CHANNEL_TYPE_VALID;

	start_ap_cmd->info.connect_common_info.valid_fields |=
		IMG_CONNECT_COMMON_INFO_WPA_VERSIONS_VALID;

	if (ap_info->connect_common_info.num_cipher_suites_pairwise > 0) {
		start_ap_cmd->info.connect_common_info.valid_fields |=
			IMG_CONNECT_COMMON_INFO_CIPHER_SUITES_PAIRWISE_VALID;
	}

	if (ap_info->connect_common_info.num_akm_suites > 0) {
		start_ap_cmd->info.connect_common_info.valid_fields |=
			IMG_CONNECT_COMMON_INFO_AKM_SUITES_VALID;
	}

	if (ap_info->connect_common_info.control_port_ether_type) {
		start_ap_cmd->info.connect_common_info.valid_fields |=
			IMG_CONNECT_COMMON_INFO_CONTROL_PORT_ETHER_TYPE;
	}

	if (ap_info->connect_common_info.control_port_no_encrypt) {
		start_ap_cmd->info.connect_common_info.valid_fields |=
			IMG_CONNECT_COMMON_INFO_CONTROL_PORT_NO_ENCRYPT;
	}

	if ((ap_info->p2p_go_ctwindow > 0) &&
	    (ap_info->p2p_go_ctwindow < 127)) {
		start_ap_cmd->valid_fields |=
			IMG_CMD_BEACON_INFO_P2P_CTWINDOW_VALID;
		start_ap_cmd->valid_fields |=
			IMG_CMD_BEACON_INFO_P2P_OPPPS_VALID;
	}

	wiphy_info = wifi_nrf_osal_mem_zalloc(fmac_dev_ctx->fpriv->opriv,
					      sizeof(*wiphy_info));

	if (!wiphy_info) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Unable to allocate memory\n",
				      __func__);
		goto out;
	}

	wiphy_info->freq_params.frequency = ap_info->freq_params.frequency;

	wiphy_info->freq_params.channel_width = ap_info->freq_params.channel_width;

	wiphy_info->freq_params.center_frequency1 = ap_info->freq_params.center_frequency1;
	wiphy_info->freq_params.center_frequency2 = ap_info->freq_params.center_frequency2;

	wiphy_info->freq_params.channel_type = ap_info->freq_params.channel_type;

	status = wifi_nrf_fmac_set_wiphy_params(fmac_dev_ctx,
						if_idx,
						wiphy_info);

	if (status == WIFI_NRF_STATUS_FAIL) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: wifi_nrf_fmac_set_wiphy_params failes\n",
				      __func__);
		goto out;
	}

	wifi_nrf_fmac_peers_flush(fmac_dev_ctx, if_idx);

	status = umac_cmd_cfg(fmac_dev_ctx,
			      start_ap_cmd,
			      sizeof(*start_ap_cmd));

out:
	if (wiphy_info) {
		wifi_nrf_osal_mem_free(fmac_dev_ctx->fpriv->opriv,
				       wiphy_info);
	}

	if (start_ap_cmd) {
		wifi_nrf_osal_mem_free(fmac_dev_ctx->fpriv->opriv,
				       start_ap_cmd);
	}

	return status;
}


enum wifi_nrf_status wifi_nrf_fmac_stop_ap(void *dev_ctx,
					   unsigned char if_idx)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct img_umac_cmd_stop_ap *stop_ap_cmd = NULL;
	struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx = NULL;

	fmac_dev_ctx = dev_ctx;

	stop_ap_cmd = wifi_nrf_osal_mem_zalloc(fmac_dev_ctx->fpriv->opriv,
					       sizeof(*stop_ap_cmd));

	if (!stop_ap_cmd) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Unable to allocate memory\n",
				      __func__);
		goto out;
	}

	stop_ap_cmd->umac_hdr.cmd_evnt = IMG_UMAC_CMD_STOP_AP;
	stop_ap_cmd->umac_hdr.ids.wdev_id = if_idx;
	stop_ap_cmd->umac_hdr.ids.valid_fields |= IMG_INDEX_IDS_WDEV_ID_VALID;

	wifi_nrf_fmac_peers_flush(fmac_dev_ctx, if_idx);

	status = umac_cmd_cfg(fmac_dev_ctx,
			      stop_ap_cmd,
			      sizeof(*stop_ap_cmd));

out:
	if (stop_ap_cmd) {
		wifi_nrf_osal_mem_free(fmac_dev_ctx->fpriv->opriv,
				       stop_ap_cmd);
	}

	return status;
}


enum wifi_nrf_status wifi_nrf_fmac_p2p_dev_start(void *dev_ctx,
						 unsigned char if_idx)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct img_umac_cmd_start_p2p_dev *start_p2p_dev_cmd = NULL;
	const struct wifi_nrf_osal_ops *osal_ops = NULL;
	struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx = NULL;

	fmac_dev_ctx = dev_ctx;

	osal_ops = fmac_dev_ctx->fpriv->opriv->ops;

	start_p2p_dev_cmd = wifi_nrf_osal_mem_zalloc(fmac_dev_ctx->fpriv->opriv,
						     sizeof(*start_p2p_dev_cmd));

	if (!start_p2p_dev_cmd) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Unable to allocate memory\n", __func__);
		goto out;
	}

	start_p2p_dev_cmd->umac_hdr.cmd_evnt = IMG_UMAC_CMD_START_P2P_DEVICE;
	start_p2p_dev_cmd->umac_hdr.ids.wdev_id = if_idx;
	start_p2p_dev_cmd->umac_hdr.ids.valid_fields |= IMG_INDEX_IDS_WDEV_ID_VALID;

	status = umac_cmd_cfg(fmac_dev_ctx,
			      start_p2p_dev_cmd,
			      sizeof(*start_p2p_dev_cmd));

out:
	if (start_p2p_dev_cmd) {
		wifi_nrf_osal_mem_free(fmac_dev_ctx->fpriv->opriv,
				       start_p2p_dev_cmd);
	}

	return status;
}


enum wifi_nrf_status wifi_nrf_fmac_p2p_dev_stop(void *dev_ctx,
						unsigned char if_idx)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct img_umac_cmd_stop_p2p_dev *stop_p2p_dev_cmd = NULL;
	struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx = NULL;

	fmac_dev_ctx = dev_ctx;

	stop_p2p_dev_cmd = wifi_nrf_osal_mem_zalloc(fmac_dev_ctx->fpriv->opriv,
						    sizeof(*stop_p2p_dev_cmd));

	if (!stop_p2p_dev_cmd) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Unable to allocate memory\n",
				      __func__);
		goto out;
	}

	stop_p2p_dev_cmd->umac_hdr.cmd_evnt = IMG_UMAC_CMD_STOP_P2P_DEVICE;
	stop_p2p_dev_cmd->umac_hdr.ids.wdev_id = if_idx;
	stop_p2p_dev_cmd->umac_hdr.ids.valid_fields |= IMG_INDEX_IDS_WDEV_ID_VALID;

	status = umac_cmd_cfg(fmac_dev_ctx,
			      stop_p2p_dev_cmd,
			      sizeof(*stop_p2p_dev_cmd));
out:
	if (stop_p2p_dev_cmd) {
		wifi_nrf_osal_mem_free(fmac_dev_ctx->fpriv->opriv,
				       stop_p2p_dev_cmd);
	}

	return status;
}


enum wifi_nrf_status wifi_nrf_fmac_p2p_roc_start(void *dev_ctx,
						 unsigned char if_idx,
						 struct remain_on_channel_info *roc_info)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct img_umac_cmd_remain_on_channel *roc_cmd = NULL;
	struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx = NULL;

	fmac_dev_ctx = dev_ctx;

	roc_cmd = wifi_nrf_osal_mem_zalloc(fmac_dev_ctx->fpriv->opriv,
					   sizeof(struct img_umac_cmd_remain_on_channel));

	if (!roc_cmd) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Unable to allocate memory\n",
				      __func__);
		goto out;
	}

	roc_cmd->umac_hdr.cmd_evnt = IMG_UMAC_CMD_REMAIN_ON_CHANNEL;
	roc_cmd->umac_hdr.ids.wdev_id = if_idx;
	roc_cmd->umac_hdr.ids.valid_fields |= IMG_INDEX_IDS_WDEV_ID_VALID;

	wifi_nrf_osal_mem_cpy(fmac_dev_ctx->fpriv->opriv,
			      &roc_cmd->info,
			      roc_info,
			      sizeof(roc_cmd->info));

	if (roc_info->dur != 0) {
		roc_cmd->valid_fields |= IMG_CMD_ROC_DURATION_VALID;
	}

	roc_cmd->info.img_freq_params.valid_fields = IMG_SET_FREQ_PARAMS_FREQ_VALID;
	roc_cmd->valid_fields |= IMG_CMD_ROC_FREQ_PARAMS_VALID;

	status = umac_cmd_cfg(fmac_dev_ctx,
			      roc_cmd,
			      sizeof(*roc_cmd));

out:
	if (roc_cmd) {
		wifi_nrf_osal_mem_free(fmac_dev_ctx->fpriv->opriv,
				       roc_cmd);
	}

	return status;
}


enum wifi_nrf_status wifi_nrf_fmac_p2p_roc_stop(void *dev_ctx,
						unsigned char if_idx,
						unsigned long long cookie)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct img_umac_cmd_cancel_remain_on_channel *cancel_roc_cmd = NULL;
	struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx = NULL;

	fmac_dev_ctx = dev_ctx;

	cancel_roc_cmd = wifi_nrf_osal_mem_zalloc(fmac_dev_ctx->fpriv->opriv,
						  sizeof(*cancel_roc_cmd));

	if (!cancel_roc_cmd) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Unable to allocate memory\n",
				      __func__);
		goto out;
	}

	cancel_roc_cmd->umac_hdr.cmd_evnt = IMG_UMAC_CMD_CANCEL_REMAIN_ON_CHANNEL;
	cancel_roc_cmd->umac_hdr.ids.wdev_id = if_idx;
	cancel_roc_cmd->umac_hdr.ids.valid_fields |= IMG_INDEX_IDS_WDEV_ID_VALID;
	cancel_roc_cmd->cookie = cookie;
	cancel_roc_cmd->valid_fields |= IMG_CMD_CANCEL_ROC_COOKIE_VALID;

	status = umac_cmd_cfg(fmac_dev_ctx,
			      cancel_roc_cmd,
			      sizeof(*cancel_roc_cmd));
out:
	if (cancel_roc_cmd) {
		wifi_nrf_osal_mem_free(fmac_dev_ctx->fpriv->opriv,
				       cancel_roc_cmd);
	}

	return status;
}


enum wifi_nrf_status wifi_nrf_fmac_mgmt_tx(void *dev_ctx,
					   unsigned char if_idx,
					   struct img_umac_mgmt_tx_info *mgmt_tx_info)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct img_umac_cmd_mgmt_tx *mgmt_tx_cmd = NULL;
	struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx = NULL;

	fmac_dev_ctx = dev_ctx;

	mgmt_tx_cmd = wifi_nrf_osal_mem_zalloc(fmac_dev_ctx->fpriv->opriv,
					       sizeof(*mgmt_tx_cmd));

	if (!mgmt_tx_cmd) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Unable to allocate memory\n",
				      __func__);
		goto out;
	}

	mgmt_tx_cmd->umac_hdr.cmd_evnt = IMG_UMAC_CMD_FRAME;
	mgmt_tx_cmd->umac_hdr.ids.wdev_id = if_idx;
	mgmt_tx_cmd->umac_hdr.ids.valid_fields |= IMG_INDEX_IDS_WDEV_ID_VALID;

	wifi_nrf_osal_mem_cpy(fmac_dev_ctx->fpriv->opriv,
			      &mgmt_tx_cmd->info,
			      mgmt_tx_info,
			      sizeof(mgmt_tx_cmd->info));

	mgmt_tx_cmd->valid_fields |= (IMG_CMD_FRAME_FREQ_VALID |
				      IMG_CMD_FRAME_DURATION_VALID |
				      IMG_CMD_SET_FRAME_FREQ_PARAMS_VALID);

	mgmt_tx_cmd->info.freq_params.valid_fields |=
		(IMG_SET_FREQ_PARAMS_FREQ_VALID |
		 IMG_SET_FREQ_PARAMS_CHANNEL_WIDTH_VALID |
		 IMG_SET_FREQ_PARAMS_CENTER_FREQ1_VALID |
		 IMG_SET_FREQ_PARAMS_CENTER_FREQ2_VALID |
		 IMG_SET_FREQ_PARAMS_CHANNEL_TYPE_VALID);

	status = umac_cmd_cfg(fmac_dev_ctx,
			      mgmt_tx_cmd,
			      sizeof(*mgmt_tx_cmd));

out:
	if (mgmt_tx_cmd) {
		wifi_nrf_osal_mem_free(fmac_dev_ctx->fpriv->opriv,
				       mgmt_tx_cmd);
	}

	return status;
}


enum wifi_nrf_status wifi_nrf_fmac_del_sta(void *dev_ctx,
					   unsigned char if_idx,
					   struct img_umac_del_sta_info *del_sta_info)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct img_umac_cmd_del_sta *del_sta_cmd = NULL;
	struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx = NULL;

	fmac_dev_ctx = dev_ctx;

	del_sta_cmd = wifi_nrf_osal_mem_zalloc(fmac_dev_ctx->fpriv->opriv,
					       sizeof(*del_sta_cmd));

	if (!del_sta_cmd) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Unable to allocate memory\n",
				      __func__);
		goto out;
	}

	del_sta_cmd->umac_hdr.cmd_evnt = IMG_UMAC_CMD_DEL_STATION;
	del_sta_cmd->umac_hdr.ids.wdev_id = if_idx;
	del_sta_cmd->umac_hdr.ids.valid_fields |= IMG_INDEX_IDS_WDEV_ID_VALID;

	wifi_nrf_osal_mem_cpy(fmac_dev_ctx->fpriv->opriv,
			      &del_sta_cmd->info,
			      del_sta_info,
			      sizeof(del_sta_cmd->info));

	if (!wifi_nrf_util_is_arr_zero(del_sta_info->mac_addr,
				       sizeof(del_sta_info->mac_addr))) {
		del_sta_cmd->valid_fields |= IMG_CMD_DEL_STATION_MAC_ADDR_VALID;
	}

	if (del_sta_info->mgmt_subtype) {
		del_sta_cmd->valid_fields |=
			IMG_CMD_DEL_STATION_MGMT_SUBTYPE_VALID;
	}

	if (del_sta_info->reason_code) {
		del_sta_cmd->valid_fields |=
			IMG_CMD_DEL_STATION_REASON_CODE_VALID;
	}

	status = umac_cmd_cfg(fmac_dev_ctx,
			      del_sta_cmd,
			      sizeof(*del_sta_cmd));

out:
	if (del_sta_cmd) {
		wifi_nrf_osal_mem_free(fmac_dev_ctx->fpriv->opriv,
				       del_sta_cmd);
	}

	return status;
}


enum wifi_nrf_status wifi_nrf_fmac_add_sta(void *dev_ctx,
					   unsigned char if_idx,
					   struct img_umac_add_sta_info *add_sta_info)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct img_umac_cmd_add_sta *add_sta_cmd = NULL;
	struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx = NULL;

	fmac_dev_ctx = dev_ctx;

	add_sta_cmd = wifi_nrf_osal_mem_zalloc(fmac_dev_ctx->fpriv->opriv,
					       sizeof(*add_sta_cmd));

	if (!add_sta_cmd) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Unable to allocate memory\n",
				      __func__);
		goto out;
	}

	add_sta_cmd->umac_hdr.cmd_evnt = IMG_UMAC_CMD_NEW_STATION;
	add_sta_cmd->umac_hdr.ids.wdev_id = if_idx;
	add_sta_cmd->umac_hdr.ids.valid_fields |= IMG_INDEX_IDS_WDEV_ID_VALID;

	wifi_nrf_osal_mem_cpy(fmac_dev_ctx->fpriv->opriv,
			      &add_sta_cmd->info,
			      add_sta_info,
			      sizeof(add_sta_cmd->info));

	if (add_sta_info->aid) {
		add_sta_cmd->valid_fields |= IMG_CMD_NEW_STATION_AID_VALID;
	}

	if (add_sta_info->sta_capability) {
		add_sta_cmd->valid_fields |=
			IMG_CMD_NEW_STATION_STA_CAPABILITY_VALID;
	}

	add_sta_cmd->valid_fields |= IMG_CMD_NEW_STATION_LISTEN_INTERVAL_VALID;

	if (add_sta_info->supp_rates.img_num_rates > 0) {
		add_sta_cmd->valid_fields |=
			IMG_CMD_NEW_STATION_SUPP_RATES_VALID;
	}

	if (add_sta_info->ext_capability.ext_capability_len > 0) {
		add_sta_cmd->valid_fields |=
			IMG_CMD_NEW_STATION_EXT_CAPABILITY_VALID;
	}

	if (add_sta_info->supported_channels.supported_channels_len > 0) {
		add_sta_cmd->valid_fields |=
			IMG_CMD_NEW_STATION_SUPPORTED_CHANNELS_VALID;
	}

	if (add_sta_info->supported_oper_classes.supported_oper_classes_len > 0) {
		add_sta_cmd->valid_fields |=
			IMG_CMD_NEW_STATION_SUPPORTED_OPER_CLASSES_VALID;
	}

	add_sta_cmd->valid_fields |= IMG_CMD_NEW_STATION_STA_FLAGS2_VALID;

	if (!wifi_nrf_util_is_arr_zero(add_sta_info->ht_capability,
				       sizeof(add_sta_info->ht_capability))) {
		add_sta_cmd->valid_fields |=
			IMG_CMD_NEW_STATION_HT_CAPABILITY_VALID;
	}

	if (!wifi_nrf_util_is_arr_zero(add_sta_info->vht_capability,
				       sizeof(add_sta_info->vht_capability))) {
		add_sta_cmd->valid_fields |=
			IMG_CMD_NEW_STATION_VHT_CAPABILITY_VALID;
	}

	if (add_sta_info->opmode_notif) {
		add_sta_cmd->valid_fields |=
			IMG_CMD_NEW_STATION_OPMODE_NOTIF_VALID;
	}

	if (add_sta_info->wme_uapsd_queues) {
		add_sta_cmd->valid_fields |=
			IMG_CMD_NEW_STATION_STA_WME_UAPSD_QUEUES_VALID;
	}

	if (add_sta_info->wme_max_sp) {
		add_sta_cmd->valid_fields |=
			IMG_CMD_NEW_STATION_STA_WME_MAX_SP_VALID;
	}

	status = umac_cmd_cfg(fmac_dev_ctx,
			      add_sta_cmd,
			      sizeof(*add_sta_cmd));

out:
	if (add_sta_cmd) {
		wifi_nrf_osal_mem_free(fmac_dev_ctx->fpriv->opriv,
				       add_sta_cmd);
	}

	return status;
}


enum wifi_nrf_status wifi_nrf_fmac_chg_sta(void *dev_ctx,
					   unsigned char if_idx,
					   struct img_umac_chg_sta_info *chg_sta_info)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct img_umac_cmd_chg_sta *chg_sta_cmd = NULL;
	struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx = NULL;

	fmac_dev_ctx = dev_ctx;

	chg_sta_cmd = wifi_nrf_osal_mem_zalloc(fmac_dev_ctx->fpriv->opriv,
					       sizeof(*chg_sta_cmd));

	if (!chg_sta_cmd) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Unable to allocate memory\n",
				      __func__);
		goto out;
	}

	chg_sta_cmd->umac_hdr.cmd_evnt = IMG_UMAC_CMD_SET_STATION;
	chg_sta_cmd->umac_hdr.ids.wdev_id = if_idx;
	chg_sta_cmd->umac_hdr.ids.valid_fields |= IMG_INDEX_IDS_WDEV_ID_VALID;

	wifi_nrf_osal_mem_cpy(fmac_dev_ctx->fpriv->opriv,
			      &chg_sta_cmd->info,
			      chg_sta_info,
			      sizeof(chg_sta_cmd->info));

	if (chg_sta_info->aid) {
		chg_sta_cmd->valid_fields |=
			IMG_CMD_SET_STATION_AID_VALID;
	}


	if (chg_sta_info->supported_channels.supported_channels_len > 0) {
		chg_sta_cmd->valid_fields |=
			IMG_CMD_SET_STATION_SUPPORTED_CHANNELS_VALID;
	}

	if (chg_sta_info->supported_oper_classes.supported_oper_classes_len > 0) {
		chg_sta_cmd->valid_fields |=
			IMG_CMD_SET_STATION_SUPPORTED_OPER_CLASSES_VALID;
	}

	chg_sta_cmd->valid_fields |= IMG_CMD_SET_STATION_STA_FLAGS2_VALID;

	if (chg_sta_info->opmode_notif) {
		chg_sta_cmd->valid_fields |=
			IMG_CMD_SET_STATION_OPMODE_NOTIF_VALID;
	}

	if (chg_sta_info->wme_max_sp) {
		chg_sta_cmd->valid_fields |=
			IMG_CMD_SET_STATION_STA_WME_MAX_SP_VALID;
	}

	status = umac_cmd_cfg(fmac_dev_ctx,
			      chg_sta_cmd,
			      sizeof(*chg_sta_cmd));

out:
	if (chg_sta_cmd) {
		wifi_nrf_osal_mem_free(fmac_dev_ctx->fpriv->opriv,
				       chg_sta_cmd);
	}

	return status;
}


enum wifi_nrf_status wifi_nrf_fmac_mgmt_frame_reg(void *dev_ctx,
						  unsigned char if_idx,
						  struct img_umac_mgmt_frame_info *frame_info)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct img_umac_cmd_mgmt_frame_reg *frame_reg_cmd = NULL;
	struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx = NULL;

	fmac_dev_ctx = dev_ctx;

	frame_reg_cmd = wifi_nrf_osal_mem_zalloc(fmac_dev_ctx->fpriv->opriv,
						 sizeof(*frame_reg_cmd));

	if (!frame_reg_cmd) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Unable to allocate memory\n",
				      __func__);
		goto out;
	}

	frame_reg_cmd->umac_hdr.cmd_evnt = IMG_UMAC_CMD_REGISTER_FRAME;
	frame_reg_cmd->umac_hdr.ids.wdev_id = if_idx;
	frame_reg_cmd->umac_hdr.ids.valid_fields |= IMG_INDEX_IDS_WDEV_ID_VALID;

	wifi_nrf_osal_mem_cpy(fmac_dev_ctx->fpriv->opriv,
			      &frame_reg_cmd->info,
			      frame_info,
			      sizeof(frame_reg_cmd->info));

	status = umac_cmd_cfg(fmac_dev_ctx,
			      frame_reg_cmd,
			      sizeof(*frame_reg_cmd));

out:
	if (frame_reg_cmd) {
		wifi_nrf_osal_mem_free(fmac_dev_ctx->fpriv->opriv,
				       frame_reg_cmd);
	}

	return status;
}


enum wifi_nrf_status wifi_nrf_fmac_mac_addr(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
					    unsigned char *addr)
{
	unsigned char vif_idx = 0;

	vif_idx = wifi_nrf_fmac_vif_idx_get(fmac_dev_ctx);

	if (vif_idx == MAX_NUM_VIFS) {
		return WIFI_NRF_STATUS_FAIL;
	}

	wifi_nrf_osal_mem_cpy(fmac_dev_ctx->fpriv->opriv,
			      addr,
			      fmac_dev_ctx->base_mac_addr,
			      IMG_ETH_ALEN);

	if (((unsigned short)addr[5] + vif_idx) > 0xff) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: MAC Address rollover!!\n",
				      __func__);
	}

	addr[5] += vif_idx;

	return WIFI_NRF_STATUS_SUCCESS;
}


unsigned char wifi_nrf_fmac_add_vif(void *dev_ctx,
				    void *os_vif_ctx,
				    struct img_umac_add_vif_info *vif_info)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct img_umac_cmd_add_vif *add_vif_cmd = NULL;
	struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx = NULL;
	struct wifi_nrf_fmac_vif_ctx *vif_ctx = NULL;
	unsigned char vif_idx = 0;

	fmac_dev_ctx = dev_ctx;

	switch (vif_info->iftype) {
	case IMG_IFTYPE_STATION:
	case IMG_IFTYPE_P2P_CLIENT:
	case IMG_IFTYPE_AP:
	case IMG_IFTYPE_P2P_GO:
		break;
	default:
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: VIF type not supported\n",
				      __func__);
		goto err;
	}


	if (wifi_nrf_fmac_vif_check_if_limit(fmac_dev_ctx,
					     vif_info->iftype)) {
		goto err;
	}

	vif_ctx =  wifi_nrf_osal_mem_zalloc(fmac_dev_ctx->fpriv->opriv,
					    sizeof(*vif_ctx));

	if (!vif_ctx) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Unable to allocate memory for VIF ctx\n",
				      __func__);
		goto err;
	}

	vif_ctx->fmac_dev_ctx = fmac_dev_ctx;
	vif_ctx->os_vif_ctx = os_vif_ctx;
	vif_ctx->if_type = vif_info->iftype;

	wifi_nrf_osal_mem_cpy(fmac_dev_ctx->fpriv->opriv,
			      vif_ctx->mac_addr,
			      vif_info->mac_addr,
			      sizeof(vif_ctx->mac_addr));

	vif_idx = wifi_nrf_fmac_vif_idx_get(fmac_dev_ctx);

	if (vif_idx == MAX_NUM_VIFS) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Unable to add additional VIF\n",
				      __func__);
		goto err;
	}

	/* We don't need to send a command to the RPU for the default interface,
	 * since the FW is adding that interface by default. We just need to
	 * send commands for non-default interfaces
	 */
	if (vif_idx != 0) {
		add_vif_cmd = wifi_nrf_osal_mem_zalloc(fmac_dev_ctx->fpriv->opriv,
						       sizeof(*add_vif_cmd));

		if (!add_vif_cmd) {
			wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
					      "%s: Unable to allocate memory for cmd\n",
					      __func__);
			goto err;
		}

		add_vif_cmd->umac_hdr.cmd_evnt = IMG_UMAC_CMD_NEW_INTERFACE;
		add_vif_cmd->umac_hdr.ids.wdev_id = vif_idx;
		add_vif_cmd->umac_hdr.ids.valid_fields |= IMG_INDEX_IDS_WDEV_ID_VALID;

		wifi_nrf_osal_mem_cpy(fmac_dev_ctx->fpriv->opriv,
				      &add_vif_cmd->info,
				      vif_info,
				      sizeof(add_vif_cmd->info));

		add_vif_cmd->valid_fields |= IMG_CMD_NEW_INTERFACE_IFTYPE_VALID;
		add_vif_cmd->valid_fields |= IMG_CMD_NEW_INTERFACE_IFNAME_VALID;
		add_vif_cmd->valid_fields |= IMG_CMD_NEW_INTERFACE_MAC_ADDR_VALID;

		status = umac_cmd_cfg(fmac_dev_ctx,
				      add_vif_cmd,
				      sizeof(*add_vif_cmd));

		if (status == WIFI_NRF_STATUS_FAIL) {
			wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
					      "%s: IMG_UMAC_CMD_NEW_INTERFACE failed\n",
					      __func__);
			goto err;
		}

	}

	fmac_dev_ctx->vif_ctx[vif_idx] = vif_ctx;

	wifi_nrf_fmac_vif_incr_if_type(fmac_dev_ctx, vif_ctx->if_type);

	goto out;
err:
	if (vif_ctx) {
		wifi_nrf_osal_mem_free(fmac_dev_ctx->fpriv->opriv,
				       vif_ctx);
	}

	vif_idx = MAX_NUM_VIFS;

out:
	if (add_vif_cmd) {
		wifi_nrf_osal_mem_free(fmac_dev_ctx->fpriv->opriv,
				       add_vif_cmd);
	}

	return vif_idx;
}


enum wifi_nrf_status wifi_nrf_fmac_del_vif(void *dev_ctx,
					   unsigned char if_idx)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx = NULL;
	struct wifi_nrf_fmac_vif_ctx *vif_ctx = NULL;
	struct img_umac_cmd_del_vif *del_vif_cmd = NULL;

	fmac_dev_ctx = dev_ctx;

	switch (fmac_dev_ctx->vif_ctx[if_idx]->if_type) {
	case IMG_IFTYPE_STATION:
	case IMG_IFTYPE_P2P_CLIENT:
	case IMG_IFTYPE_AP:
	case IMG_IFTYPE_P2P_GO:
		break;
	default:
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: VIF type not supported\n",
				      __func__);
		goto out;
	}

	vif_ctx = fmac_dev_ctx->vif_ctx[if_idx];

	if (!vif_ctx) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: VIF ctx does not exist\n",
				      __func__);
		goto out;
	}

	/* We should not send a command to the RPU for the default interface,
	 * since the FW is adding that interface by default. We just need to
	 * send commands for non-default interfaces
	 */
	if (if_idx != 0) {
		del_vif_cmd = wifi_nrf_osal_mem_zalloc(fmac_dev_ctx->fpriv->opriv,
						       sizeof(*del_vif_cmd));

		if (!del_vif_cmd) {
			wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
					      "%s: Unable to allocate memory for cmd\n",
					      __func__);
			goto out;
		}

		del_vif_cmd->umac_hdr.cmd_evnt = IMG_UMAC_CMD_DEL_INTERFACE;
		del_vif_cmd->umac_hdr.ids.wdev_id = if_idx;
		del_vif_cmd->umac_hdr.ids.valid_fields |= IMG_INDEX_IDS_WDEV_ID_VALID;

		status = umac_cmd_cfg(fmac_dev_ctx,
				      del_vif_cmd,
				      sizeof(*del_vif_cmd));

		if (status != WIFI_NRF_STATUS_SUCCESS) {
			wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
					      "%s: IMG_UMAC_CMD_DEL_INTERFACE failed\n",
					      __func__);
			goto out;
		}
	}

	wifi_nrf_fmac_vif_decr_if_type(fmac_dev_ctx, vif_ctx->if_type);

out:
	if (del_vif_cmd) {
		wifi_nrf_osal_mem_free(fmac_dev_ctx->fpriv->opriv,
				       del_vif_cmd);
	}

	return status;
}


enum wifi_nrf_status wifi_nrf_fmac_chg_vif(void *dev_ctx,
					   unsigned char if_idx,
					   struct img_umac_chg_vif_attr_info *vif_info)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct img_umac_cmd_chg_vif_attr *chg_vif_cmd = NULL;
	struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx = NULL;

	fmac_dev_ctx = dev_ctx;

	switch (vif_info->iftype) {
	case IMG_IFTYPE_STATION:
	case IMG_IFTYPE_P2P_CLIENT:
	case IMG_IFTYPE_AP:
	case IMG_IFTYPE_P2P_GO:
		break;
	default:
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: VIF type not supported\n", __func__);
		goto out;
	}

	if (wifi_nrf_fmac_vif_check_if_limit(fmac_dev_ctx,
					     vif_info->iftype)) {
		goto out;
	}

	chg_vif_cmd = wifi_nrf_osal_mem_zalloc(fmac_dev_ctx->fpriv->opriv,
					       sizeof(*chg_vif_cmd));

	if (!chg_vif_cmd) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Unable to allocate memory\n",
				      __func__);
		goto out;
	}

	chg_vif_cmd->umac_hdr.cmd_evnt = IMG_UMAC_CMD_SET_INTERFACE;
	chg_vif_cmd->umac_hdr.ids.wdev_id = if_idx;
	chg_vif_cmd->umac_hdr.ids.valid_fields |= IMG_INDEX_IDS_WDEV_ID_VALID;

	wifi_nrf_osal_mem_cpy(fmac_dev_ctx->fpriv->opriv,
			      &chg_vif_cmd->info,
			      vif_info,
			      sizeof(chg_vif_cmd->info));

	chg_vif_cmd->valid_fields |= IMG_SET_INTERFACE_IFTYPE_VALID;
	chg_vif_cmd->valid_fields |= IMG_SET_INTERFACE_USE_4ADDR_VALID;

	status = umac_cmd_cfg(fmac_dev_ctx,
			      chg_vif_cmd,
			      sizeof(*chg_vif_cmd));
out:
	if (chg_vif_cmd) {
		wifi_nrf_osal_mem_free(fmac_dev_ctx->fpriv->opriv,
				       chg_vif_cmd);
	}

	return status;
}


enum wifi_nrf_status wifi_nrf_fmac_chg_vif_state(void *dev_ctx,
						 unsigned char if_idx,
						 struct img_umac_chg_vif_state_info *vif_info)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct img_umac_cmd_chg_vif_state *chg_vif_state_cmd = NULL;
	struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx = NULL;
	struct wifi_nrf_fmac_vif_ctx *vif_ctx = NULL;
	unsigned int count = 100; /* used for handling timeout*/

	fmac_dev_ctx = dev_ctx;

	chg_vif_state_cmd = wifi_nrf_osal_mem_zalloc(fmac_dev_ctx->fpriv->opriv,
						     sizeof(*chg_vif_state_cmd));

	if (!chg_vif_state_cmd) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Unable to allocate memory\n",
				      __func__);
		goto out;
	}

	chg_vif_state_cmd->umac_hdr.cmd_evnt = IMG_UMAC_CMD_SET_IFFLAGS;
	chg_vif_state_cmd->umac_hdr.ids.wdev_id = if_idx;
	chg_vif_state_cmd->umac_hdr.ids.valid_fields |=
		IMG_INDEX_IDS_WDEV_ID_VALID;

	wifi_nrf_osal_mem_cpy(fmac_dev_ctx->fpriv->opriv,
			      &chg_vif_state_cmd->info,
			      vif_info,
			      sizeof(chg_vif_state_cmd->info));

	vif_ctx = fmac_dev_ctx->vif_ctx[if_idx];

	vif_ctx->ifflags = false;

	status = umac_cmd_cfg(fmac_dev_ctx,
			      chg_vif_state_cmd,
			      sizeof(*chg_vif_state_cmd));

	while (!vif_ctx->ifflags && (--count > 0))
		wifi_nrf_osal_sleep_ms(fmac_dev_ctx->fpriv->opriv,
				       100);

	if (count == 0) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: RPU is unresponsive for 1 sec\n",
				      __func__);
		goto out;
	}


	if (vif_ctx->if_type == IMG_IFTYPE_AP) {
		if (vif_info->state == 1) {
			fmac_dev_ctx->tx_config.peers[MAX_PEERS].peer_id = MAX_PEERS;
			fmac_dev_ctx->tx_config.peers[MAX_PEERS].if_idx = if_idx;
		} else if (vif_info->state == 0) {
			fmac_dev_ctx->tx_config.peers[MAX_PEERS].peer_id = -1;
			fmac_dev_ctx->tx_config.peers[MAX_PEERS].if_idx = if_idx;
		}
	}

out:
	if (chg_vif_state_cmd) {
		wifi_nrf_osal_mem_free(fmac_dev_ctx->fpriv->opriv,
				       chg_vif_state_cmd);
	}

	return status;
}


enum wifi_nrf_status wifi_nrf_fmac_suspend(void *dev_ctx)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct img_umac_cmd_suspend *suspend_cmd = NULL;
	struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx = NULL;

	fmac_dev_ctx = dev_ctx;

	suspend_cmd = wifi_nrf_osal_mem_zalloc(fmac_dev_ctx->fpriv->opriv,
					       sizeof(*suspend_cmd));

	if (!suspend_cmd) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Unable to allocate memory\n", __func__);
		goto out;
	}

	suspend_cmd->umac_hdr.cmd_evnt = IMG_UMAC_CMD_SUSPEND;

	status = umac_cmd_cfg(fmac_dev_ctx,
			      suspend_cmd,
			      sizeof(*suspend_cmd));
out:
	if (suspend_cmd) {
		wifi_nrf_osal_mem_free(fmac_dev_ctx->fpriv->opriv,
				       suspend_cmd);
	}

	return status;

}


enum wifi_nrf_status wifi_nrf_fmac_get_tx_power(void *dev_ctx,
						unsigned int if_idx)
{

	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct img_umac_cmd_get_tx_power *cmd = NULL;
	struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx = NULL;

	fmac_dev_ctx = dev_ctx;

	cmd = wifi_nrf_osal_mem_zalloc(fmac_dev_ctx->fpriv->opriv,
				       sizeof(*cmd));

	if (!cmd) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Unable to allocate memory\n",
				      __func__);
		goto out;
	}

	cmd->umac_hdr.cmd_evnt = IMG_UMAC_CMD_GET_TX_POWER;
	cmd->umac_hdr.ids.wdev_id = if_idx;
	cmd->umac_hdr.ids.valid_fields |= IMG_INDEX_IDS_WDEV_ID_VALID;

	status = umac_cmd_cfg(fmac_dev_ctx, cmd, sizeof(*cmd));

out:
	if (cmd) {
		wifi_nrf_osal_mem_free(fmac_dev_ctx->fpriv->opriv,
				       cmd);
	}

	return status;
}


enum wifi_nrf_status wifi_nrf_fmac_get_channel(void *dev_ctx,
					       unsigned int if_idx)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct img_umac_cmd_get_channel *cmd = NULL;
	struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx = NULL;

	fmac_dev_ctx = dev_ctx;

	cmd = wifi_nrf_osal_mem_zalloc(fmac_dev_ctx->fpriv->opriv,
				       sizeof(*cmd));

	if (!cmd) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Unable to allocate memory\n",
				      __func__);
		goto out;
	}

	cmd->umac_hdr.cmd_evnt = IMG_UMAC_CMD_GET_CHANNEL;
	cmd->umac_hdr.ids.wdev_id = if_idx;
	cmd->umac_hdr.ids.valid_fields |= IMG_INDEX_IDS_WDEV_ID_VALID;

	status = umac_cmd_cfg(fmac_dev_ctx,
			      cmd,
			      sizeof(*cmd));
out:
	if (cmd) {
		wifi_nrf_osal_mem_free(fmac_dev_ctx->fpriv->opriv,
				       cmd);
	}

	return status;
}


enum wifi_nrf_status wifi_nrf_fmac_get_station(void *dev_ctx,
					       unsigned int if_idx,
					       unsigned char *mac)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct img_umac_cmd_get_sta *cmd = NULL;
	struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx = NULL;

	fmac_dev_ctx = dev_ctx;

	cmd = wifi_nrf_osal_mem_zalloc(fmac_dev_ctx->fpriv->opriv,
				       sizeof(*cmd));

	if (!cmd) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Unable to allocate memory\n",
				      __func__);
		goto out;
	}

	cmd->umac_hdr.cmd_evnt = IMG_UMAC_CMD_GET_STATION;
	cmd->umac_hdr.ids.wdev_id = if_idx;
	cmd->umac_hdr.ids.valid_fields |= IMG_INDEX_IDS_WDEV_ID_VALID;

	wifi_nrf_osal_mem_cpy(fmac_dev_ctx->fpriv->opriv,
			      cmd->info.mac_addr,
			      mac,
			      IMG_ETH_ALEN);

	status = umac_cmd_cfg(fmac_dev_ctx,
			      cmd,
			      sizeof(*cmd));
out:
	if (cmd) {
		wifi_nrf_osal_mem_free(fmac_dev_ctx->fpriv->opriv,
				       cmd);
	}

	return status;
}


enum wifi_nrf_status wifi_nrf_fmac_resume(void *dev_ctx)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct img_umac_cmd_resume *resume_cmd = NULL;
	struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx = NULL;

	fmac_dev_ctx = dev_ctx;

	resume_cmd = wifi_nrf_osal_mem_zalloc(fmac_dev_ctx->fpriv->opriv,
					      sizeof(*resume_cmd));

	if (!resume_cmd) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Unable to allocate memory\n",
				      __func__);
		goto out;
	}

	resume_cmd->umac_hdr.cmd_evnt = IMG_UMAC_CMD_RESUME;

	status = umac_cmd_cfg(fmac_dev_ctx,
			      resume_cmd,
			      sizeof(*resume_cmd));
out:
	if (resume_cmd) {
		wifi_nrf_osal_mem_free(fmac_dev_ctx->fpriv->opriv,
				       resume_cmd);
	}

	return status;
}


enum wifi_nrf_status wifi_nrf_fmac_set_qos_map(void *dev_ctx,
					       unsigned char if_idx,
					       struct img_umac_qos_map_info *qos_info)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct img_umac_cmd_set_qos_map *set_qos_cmd = NULL;
	struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx = NULL;

	fmac_dev_ctx = dev_ctx;

	set_qos_cmd = wifi_nrf_osal_mem_zalloc(fmac_dev_ctx->fpriv->opriv,
					       sizeof(*set_qos_cmd));

	if (!set_qos_cmd) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Unable to allocate memory\n",
				      __func__);
		goto out;
	}

	set_qos_cmd->umac_hdr.cmd_evnt = IMG_UMAC_CMD_SET_QOS_MAP;
	set_qos_cmd->umac_hdr.ids.wdev_id = if_idx;
	set_qos_cmd->umac_hdr.ids.valid_fields |= IMG_INDEX_IDS_WDEV_ID_VALID;

	if (qos_info->qos_map_info_len) {
		wifi_nrf_osal_mem_cpy(fmac_dev_ctx->fpriv->opriv,
				      &set_qos_cmd->info.qos_map_info,
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
		wifi_nrf_osal_mem_free(fmac_dev_ctx->fpriv->opriv,
				       set_qos_cmd);
	}

	return status;
}


enum wifi_nrf_status wifi_nrf_fmac_set_power_save(void *dev_ctx,
						  unsigned char if_idx,
						  bool state)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct img_umac_cmd_set_power_save *set_ps_cmd = NULL;
	struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx = NULL;

	fmac_dev_ctx = dev_ctx;

	set_ps_cmd = wifi_nrf_osal_mem_zalloc(fmac_dev_ctx->fpriv->opriv,
					      sizeof(*set_ps_cmd));

	if (!set_ps_cmd) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Unable to allocate memory\n", __func__);
		goto out;
	}

	set_ps_cmd->umac_hdr.cmd_evnt = IMG_UMAC_CMD_SET_POWER_SAVE;
	set_ps_cmd->umac_hdr.ids.wdev_id = if_idx;
	set_ps_cmd->umac_hdr.ids.valid_fields |=
		IMG_INDEX_IDS_WDEV_ID_VALID;
	set_ps_cmd->info.ps_state = state;

	status = umac_cmd_cfg(fmac_dev_ctx,
			      set_ps_cmd,
			      sizeof(*set_ps_cmd));
out:
	if (set_ps_cmd) {
		wifi_nrf_osal_mem_free(fmac_dev_ctx->fpriv->opriv,
				       set_ps_cmd);
	}

	return status;
}


enum wifi_nrf_status wifi_nrf_fmac_set_wowlan(void *dev_ctx,
					      unsigned int var)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct img_umac_cmd_set_wowlan *set_wowlan = NULL;
	struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx = NULL;

	fmac_dev_ctx = dev_ctx;

	set_wowlan = wifi_nrf_osal_mem_zalloc(fmac_dev_ctx->fpriv->opriv,
					      sizeof(*set_wowlan));

	if (!set_wowlan) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Unable to allocate memory\n",
				      __func__);
		goto out;
	}

	set_wowlan->umac_hdr.cmd_evnt = IMG_UMAC_CMD_SET_WOWLAN;
	set_wowlan->info.img_flags = var;

	status = umac_cmd_cfg(fmac_dev_ctx,
			      set_wowlan,
			      sizeof(*set_wowlan));
out:
	if (set_wowlan) {
		wifi_nrf_osal_mem_free(fmac_dev_ctx->fpriv->opriv,
				       set_wowlan);
	}

	return status;
}


enum wifi_nrf_status wifi_nrf_fmac_conf_btcoex(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
					       struct rpu_btcoex *params)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;

	status = umac_cmd_btcoex(fmac_dev_ctx, params);

	return status;
}


enum wifi_nrf_status wifi_nrf_fmac_conf_ltf_gi(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
					       unsigned char he_ltf,
					       unsigned char he_gi,
					       unsigned char enabled)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;

	status = umac_cmd_he_ltf_gi(fmac_dev_ctx, he_ltf, he_gi, enabled);

	return status;
}
