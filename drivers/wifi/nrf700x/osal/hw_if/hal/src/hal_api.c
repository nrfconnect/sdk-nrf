/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief File containing API definitions for the
 * HAL Layer of the Wi-Fi driver.
 */

#include "queue.h"
#include "hal_structs.h"
#include "hal_common.h"
#include "hal_reg.h"
#include "hal_mem.h"
#include "hal_interrupt.h"
#include "pal.h"

#ifndef CONFIG_NRF700X_RADIO_TEST
static enum nrf_wifi_status
nrf_wifi_hal_rpu_pktram_buf_map_init(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	unsigned int pool_idx = 0;

	status = pal_rpu_addr_offset_get(hal_dev_ctx->hpriv->opriv,
					 RPU_MEM_PKT_BASE,
					 &hal_dev_ctx->addr_rpu_pktram_base,
					 hal_dev_ctx->curr_proc);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: pal_rpu_addr_offset_get failed\n",
				      __func__);
		goto out;
	}

	hal_dev_ctx->addr_rpu_pktram_base_tx = hal_dev_ctx->addr_rpu_pktram_base;

	hal_dev_ctx->addr_rpu_pktram_base_rx_pool[0] =
		 (hal_dev_ctx->addr_rpu_pktram_base + RPU_PKTRAM_SIZE) -
		 (CONFIG_NRF700X_RX_NUM_BUFS * CONFIG_NRF700X_RX_MAX_DATA_SIZE);

	for (pool_idx = 1; pool_idx < MAX_NUM_OF_RX_QUEUES; pool_idx++) {
		hal_dev_ctx->addr_rpu_pktram_base_rx_pool[pool_idx] =
			hal_dev_ctx->addr_rpu_pktram_base_rx_pool[pool_idx - 1] +
			(hal_dev_ctx->hpriv->cfg_params.rx_buf_pool[pool_idx - 1].num_bufs *
			 hal_dev_ctx->hpriv->cfg_params.rx_buf_pool[pool_idx - 1].buf_sz);
	}

	status = NRF_WIFI_STATUS_SUCCESS;
out:
	return status;
}


unsigned long nrf_wifi_hal_buf_map_rx(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx,
				      unsigned long buf,
				      unsigned int buf_len,
				      unsigned int pool_id,
				      unsigned int buf_id)
{
	struct nrf_wifi_hal_buf_map_info *rx_buf_info = NULL;
	unsigned long addr_to_map = 0;
	unsigned long bounce_buf_addr = 0;
	unsigned long rpu_addr = 0;

	rx_buf_info = &hal_dev_ctx->rx_buf_info[pool_id][buf_id];

	if (rx_buf_info->mapped) {
		nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: Called for already mapped RX buffer\n",
				      __func__);
		goto out;
	}

	rx_buf_info->virt_addr = buf;
	rx_buf_info->buf_len = buf_len;

	if (buf_len != hal_dev_ctx->hpriv->cfg_params.rx_buf_pool[pool_id].buf_sz) {
		nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: Invalid buf_len (%d) for pool_id (%d)\n",
				      __func__,
				      buf_len,
				      pool_id);
		goto out;
	}

	bounce_buf_addr = hal_dev_ctx->addr_rpu_pktram_base_rx_pool[pool_id] +
		(buf_id * buf_len);

	rpu_addr = RPU_MEM_PKT_BASE + (bounce_buf_addr - hal_dev_ctx->addr_rpu_pktram_base);

	hal_rpu_mem_write(hal_dev_ctx,
			  (unsigned int)rpu_addr,
			  (void *)buf,
			  hal_dev_ctx->hpriv->cfg_params.rx_buf_headroom_sz);

	addr_to_map = bounce_buf_addr + hal_dev_ctx->hpriv->cfg_params.rx_buf_headroom_sz;

	rx_buf_info->phy_addr = nrf_wifi_bal_dma_map(hal_dev_ctx->bal_dev_ctx,
						     addr_to_map,
						     buf_len,
						     NRF_WIFI_OSAL_DMA_DIR_FROM_DEV);

	if (!rx_buf_info->phy_addr) {
		nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: DMA map failed\n",
				      __func__);
		goto out;
	}

out:
	if (rx_buf_info->phy_addr) {
		rx_buf_info->mapped = true;
	}

	return rx_buf_info->phy_addr;
}


unsigned long nrf_wifi_hal_buf_unmap_rx(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx,
					unsigned int data_len,
					unsigned int pool_id,
					unsigned int buf_id)
{
	struct nrf_wifi_hal_buf_map_info *rx_buf_info = NULL;
	unsigned long unmapped_addr = 0;
	unsigned long virt_addr = 0;
	unsigned long rpu_addr = 0;

	rx_buf_info = &hal_dev_ctx->rx_buf_info[pool_id][buf_id];

	if (!rx_buf_info->mapped) {
		nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: Called for unmapped RX buffer\n",
				      __func__);
		goto out;
	}

	unmapped_addr = nrf_wifi_bal_dma_unmap(hal_dev_ctx->bal_dev_ctx,
					       rx_buf_info->phy_addr,
					       rx_buf_info->buf_len,
					       NRF_WIFI_OSAL_DMA_DIR_FROM_DEV);

	rpu_addr = RPU_MEM_PKT_BASE + (unmapped_addr - hal_dev_ctx->addr_rpu_pktram_base);

	if (data_len) {
		if (!unmapped_addr) {
			nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
					      "%s: DMA unmap failed\n",
					      __func__);
			goto out;
		}

		hal_rpu_mem_read(hal_dev_ctx,
				 (void *)(rx_buf_info->virt_addr +
					  hal_dev_ctx->hpriv->cfg_params.rx_buf_headroom_sz),
				 (unsigned int)rpu_addr,
				 data_len);
	}

	virt_addr = rx_buf_info->virt_addr;

	nrf_wifi_osal_mem_set(hal_dev_ctx->hpriv->opriv,
			      rx_buf_info,
			      0,
			      sizeof(*rx_buf_info));
out:
	return virt_addr;
}


unsigned long nrf_wifi_hal_buf_map_tx(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx,
				      unsigned long buf,
				      unsigned int buf_len,
				      unsigned int desc_id,
				      unsigned int token,
				      unsigned int buf_indx)
{
	struct nrf_wifi_hal_buf_map_info *tx_buf_info = NULL;
	unsigned long addr_to_map = 0;
	unsigned long bounce_buf_addr = 0;
	unsigned long tx_token_base_addr = hal_dev_ctx->addr_rpu_pktram_base_tx +
		(token * hal_dev_ctx->hpriv->cfg_params.max_ampdu_len_per_token);
	unsigned long rpu_addr = 0;

	tx_buf_info = &hal_dev_ctx->tx_buf_info[desc_id];

	if (tx_buf_info->mapped) {
		nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: Called for already mapped TX buffer\n",
				      __func__);
		goto out;
	}

	tx_buf_info->virt_addr = buf;

	if (buf_len > (hal_dev_ctx->hpriv->cfg_params.max_tx_frm_sz -
		       hal_dev_ctx->hpriv->cfg_params.tx_buf_headroom_sz)) {
		nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: Invalid TX buf_len (%d) for (%d)\n",
				      __func__,
				      buf_len,
				      desc_id);
		goto out;
	}

	if (buf_indx == 0) {
		hal_dev_ctx->tx_frame_offset = tx_token_base_addr;
	}

	bounce_buf_addr = hal_dev_ctx->tx_frame_offset;

	/* Align bounce buffer and buffer length to 4-byte boundary */
	bounce_buf_addr = (bounce_buf_addr + 3) & ~3;
	buf_len = (buf_len + 3) & ~3;

	hal_dev_ctx->tx_frame_offset += (bounce_buf_addr - hal_dev_ctx->tx_frame_offset) +
		buf_len + hal_dev_ctx->hpriv->cfg_params.tx_buf_headroom_sz;

	rpu_addr = RPU_MEM_PKT_BASE + (bounce_buf_addr - hal_dev_ctx->addr_rpu_pktram_base);

	nrf_wifi_osal_log_dbg(hal_dev_ctx->hpriv->opriv,
	       "%s: bounce_buf_addr: 0x%lx, rpu_addr: 0x%lx, buf_len: %d off:%d\n",
	       __func__,
	       bounce_buf_addr,
	       rpu_addr,
	       buf_len,
	       hal_dev_ctx->tx_frame_offset);

	hal_rpu_mem_write(hal_dev_ctx,
			  (unsigned int)rpu_addr,
			  (void *)buf,
			  buf_len);

	addr_to_map = bounce_buf_addr;

	tx_buf_info->phy_addr = nrf_wifi_bal_dma_map(hal_dev_ctx->bal_dev_ctx,
						     addr_to_map,
						     buf_len,
						     NRF_WIFI_OSAL_DMA_DIR_TO_DEV);

	if (!tx_buf_info->phy_addr) {
		nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: DMA map failed\n",
				      __func__);
		goto out;
	}
	tx_buf_info->buf_len = buf_len;

out:
	if (tx_buf_info->phy_addr) {
		tx_buf_info->mapped = true;
	}

	return tx_buf_info->phy_addr;
}


unsigned long nrf_wifi_hal_buf_unmap_tx(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx,
					unsigned int desc_id)
{
	struct nrf_wifi_hal_buf_map_info *tx_buf_info = NULL;
	unsigned long unmapped_addr = 0;
	unsigned long virt_addr = 0;

	tx_buf_info = &hal_dev_ctx->tx_buf_info[desc_id];

	if (!tx_buf_info->mapped) {
		nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: Called for unmapped TX buffer\n",
				      __func__);
		goto out;
	}

	unmapped_addr = nrf_wifi_bal_dma_unmap(hal_dev_ctx->bal_dev_ctx,
					       tx_buf_info->phy_addr,
					       tx_buf_info->buf_len,
					       NRF_WIFI_OSAL_DMA_DIR_TO_DEV);

	if (!unmapped_addr) {
		nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: DMA unmap failed\n",
				      __func__);
		goto out;
	}

	virt_addr = tx_buf_info->virt_addr;

	nrf_wifi_osal_mem_set(hal_dev_ctx->hpriv->opriv,
			      tx_buf_info,
			      0,
			      sizeof(*tx_buf_info));
out:
	return virt_addr;
}
#endif /* !CONFIG_NRF700X_RADIO_TEST */


#ifdef CONFIG_NRF_WIFI_LOW_POWER
enum nrf_wifi_status hal_rpu_ps_wake(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx)
{
	unsigned int reg_val = 0;
	unsigned int rpu_ps_state_mask = 0;
	unsigned long start_time_us = 0;
	unsigned long idle_time_start_us = 0;
	unsigned long idle_time_us = 0;
	unsigned long elapsed_time_sec = 0;
	unsigned long elapsed_time_usec = 0;
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;

	if (!hal_dev_ctx) {
		nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: Invalid parameters\n",
				      __func__);
		return status;
	}


	/* If the FW is not yet booted up (e.g. during the FW load stage of Host FW load)
	 * then skip the RPU wake attempt since RPU sleep/wake kicks in only after FW boot
	 */
	if (!hal_dev_ctx->rpu_fw_booted)
		return NRF_WIFI_STATUS_SUCCESS;

	if (hal_dev_ctx->rpu_ps_state == RPU_PS_STATE_AWAKE) {
		status = NRF_WIFI_STATUS_SUCCESS;

		goto out;
	}

	nrf_wifi_bal_rpu_ps_wake(hal_dev_ctx->bal_dev_ctx);

	start_time_us = nrf_wifi_osal_time_get_curr_us(hal_dev_ctx->hpriv->opriv);

	rpu_ps_state_mask = ((1 << RPU_REG_BIT_PS_STATE) |
			     (1 << RPU_REG_BIT_READY_STATE));

	/* Add a delay to avoid a race condition in the RPU */
	/* TODO: Reduce to 200 us after sleep has been stabilized */
	nrf_wifi_osal_delay_us(hal_dev_ctx->hpriv->opriv,
			       1000);

	do {
		/* Poll the RPU PS state */
		reg_val = nrf_wifi_bal_rpu_ps_status(hal_dev_ctx->bal_dev_ctx);

		if ((reg_val & rpu_ps_state_mask) == rpu_ps_state_mask) {
			status = NRF_WIFI_STATUS_SUCCESS;
			break;
		}

		idle_time_start_us = nrf_wifi_osal_time_get_curr_us(hal_dev_ctx->hpriv->opriv);

		do {
			idle_time_us = nrf_wifi_osal_time_elapsed_us(hal_dev_ctx->hpriv->opriv,
								     idle_time_start_us);
		} while ((idle_time_us / 1000) < RPU_PS_WAKE_INTERVAL_MS);

		elapsed_time_usec = nrf_wifi_osal_time_elapsed_us(hal_dev_ctx->hpriv->opriv,
								  start_time_us);
		elapsed_time_sec = (elapsed_time_usec / 1000000);
	} while (elapsed_time_sec < RPU_PS_WAKE_TIMEOUT_S);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: RPU is not ready for more than %d sec,"
				      "reg_val = 0x%X rpu_ps_state_mask = 0x%X\n",
				      __func__,
				      RPU_PS_WAKE_TIMEOUT_S,
				      reg_val,
				      rpu_ps_state_mask);
		goto out;
	}
	hal_dev_ctx->rpu_ps_state = RPU_PS_STATE_AWAKE;

out:
	if (!hal_dev_ctx->irq_ctx) {
		nrf_wifi_osal_timer_schedule(hal_dev_ctx->hpriv->opriv,
					     hal_dev_ctx->rpu_ps_timer,
					     CONFIG_NRF700X_RPU_PS_IDLE_TIMEOUT_MS);
	}
	return status;
}


static void hal_rpu_ps_sleep(unsigned long data)
{
	struct nrf_wifi_hal_dev_ctx *hal_dev_ctx = NULL;
	unsigned long flags = 0;

	hal_dev_ctx = (struct nrf_wifi_hal_dev_ctx *)data;

	nrf_wifi_osal_spinlock_irq_take(hal_dev_ctx->hpriv->opriv,
					hal_dev_ctx->rpu_ps_lock,
					&flags);

	nrf_wifi_bal_rpu_ps_sleep(hal_dev_ctx->bal_dev_ctx);

	hal_dev_ctx->rpu_ps_state = RPU_PS_STATE_ASLEEP;

	nrf_wifi_osal_spinlock_irq_rel(hal_dev_ctx->hpriv->opriv,
				       hal_dev_ctx->rpu_ps_lock,
				       &flags);
}


static enum nrf_wifi_status hal_rpu_ps_init(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;

	hal_dev_ctx->rpu_ps_lock = nrf_wifi_osal_spinlock_alloc(hal_dev_ctx->hpriv->opriv);

	if (!hal_dev_ctx->rpu_ps_lock) {
		nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: Unable to allocate lock\n",
				      __func__);
		goto out;
	}

	nrf_wifi_osal_spinlock_init(hal_dev_ctx->hpriv->opriv,
				    hal_dev_ctx->rpu_ps_lock);

	hal_dev_ctx->rpu_ps_timer = nrf_wifi_osal_timer_alloc(hal_dev_ctx->hpriv->opriv);

	if (!hal_dev_ctx->rpu_ps_timer) {
		nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: Unable to allocate timer\n",
				      __func__);
		nrf_wifi_osal_spinlock_free(hal_dev_ctx->hpriv->opriv,
					    hal_dev_ctx->rpu_ps_lock);
		goto out;
	}

	nrf_wifi_osal_timer_init(hal_dev_ctx->hpriv->opriv,
				 hal_dev_ctx->rpu_ps_timer,
				 hal_rpu_ps_sleep,
				 (unsigned long)hal_dev_ctx);

	hal_dev_ctx->rpu_ps_state = RPU_PS_STATE_ASLEEP;
	hal_dev_ctx->dbg_enable = true;

	status = NRF_WIFI_STATUS_SUCCESS;
out:
	return status;
}


static void hal_rpu_ps_deinit(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx)
{
	nrf_wifi_osal_timer_kill(hal_dev_ctx->hpriv->opriv,
				 hal_dev_ctx->rpu_ps_timer);

	nrf_wifi_osal_timer_free(hal_dev_ctx->hpriv->opriv,
				 hal_dev_ctx->rpu_ps_timer);

	nrf_wifi_osal_spinlock_free(hal_dev_ctx->hpriv->opriv,
				    hal_dev_ctx->rpu_ps_lock);
}


static void hal_rpu_ps_set_state(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx,
				 enum RPU_PS_STATE ps_state)
{
	hal_dev_ctx->rpu_ps_state = ps_state;
}

enum nrf_wifi_status nrf_wifi_hal_get_rpu_ps_state(
				struct nrf_wifi_hal_dev_ctx *hal_dev_ctx,
				int *rpu_ps_ctrl_state)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;

	if (!hal_dev_ctx) {
		nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: Invalid parameters\n",
				      __func__);
		goto out;
	}

	*rpu_ps_ctrl_state = hal_dev_ctx->rpu_ps_state;

	return NRF_WIFI_STATUS_SUCCESS;
out:
	return status;
}
#endif /* CONFIG_NRF_WIFI_LOW_POWER */


static bool hal_rpu_hpq_is_empty(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx,
				 struct host_rpu_hpq *hpq)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	unsigned int val = 0;

	status = hal_rpu_reg_read(hal_dev_ctx,
				  &val,
				  hpq->dequeue_addr);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: Read from dequeue address failed, val (0x%X)\n",
				      __func__,
				      val);
		return true;
	}

	if (val) {
		return false;
	}

	return true;
}


static enum nrf_wifi_status hal_rpu_ready(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx,
					  enum NRF_WIFI_HAL_MSG_TYPE msg_type)
{
	bool is_empty = false;
	struct host_rpu_hpq *avl_buf_q = NULL;

	if (msg_type == NRF_WIFI_HAL_MSG_TYPE_CMD_CTRL) {
		avl_buf_q = &hal_dev_ctx->rpu_info.hpqm_info.cmd_avl_queue;
	} else {
		nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: Invalid msg type %d\n",
				      __func__,
				      msg_type);

		return NRF_WIFI_STATUS_FAIL;
	}

	/* Check if any command pointers are available to post a message */
	is_empty = hal_rpu_hpq_is_empty(hal_dev_ctx,
					avl_buf_q);

	if (is_empty == true) {
		return NRF_WIFI_STATUS_FAIL;
	}

	return NRF_WIFI_STATUS_SUCCESS;
}


static enum nrf_wifi_status hal_rpu_ready_wait(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx,
					       enum NRF_WIFI_HAL_MSG_TYPE msg_type)
{
	unsigned long start_time_us = 0;
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;

	start_time_us = nrf_wifi_osal_time_get_curr_us(hal_dev_ctx->hpriv->opriv);

	while (hal_rpu_ready(hal_dev_ctx, msg_type) != NRF_WIFI_STATUS_SUCCESS) {
		if (nrf_wifi_osal_time_elapsed_us(hal_dev_ctx->hpriv->opriv,
						  start_time_us) >= MAX_HAL_RPU_READY_WAIT) {
			nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
					      "%s: Timed out waiting (msg_type = %d)\n",
					      __func__,
					      msg_type);
			goto out;
		}
	}

	status = NRF_WIFI_STATUS_SUCCESS;
out:
	return status;
}


static enum nrf_wifi_status hal_rpu_msg_trigger(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;

	status = hal_rpu_reg_write(hal_dev_ctx,
				   RPU_REG_INT_TO_MCU_CTRL,
				   (hal_dev_ctx->num_cmds | 0x7fff0000));

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: Writing to MCU cmd register failed\n",
				      __func__);
		goto out;
	}

	hal_dev_ctx->num_cmds++;
out:
	return status;
}


static enum nrf_wifi_status hal_rpu_msg_post(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx,
					     enum NRF_WIFI_HAL_MSG_TYPE msg_type,
					     unsigned int queue_id,
					     unsigned int msg_addr)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct host_rpu_hpq *busy_queue = NULL;

	if (queue_id >= MAX_NUM_OF_RX_QUEUES) {
		nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: Invalid queue_id (%d)\n",
				      __func__,
				      queue_id);
		goto out;
	}

	if ((msg_type == NRF_WIFI_HAL_MSG_TYPE_CMD_CTRL) ||
	    (msg_type == NRF_WIFI_HAL_MSG_TYPE_CMD_DATA_TX)) {
		busy_queue = &hal_dev_ctx->rpu_info.hpqm_info.cmd_busy_queue;
	} else if (msg_type == NRF_WIFI_HAL_MSG_TYPE_CMD_DATA_RX) {
		busy_queue = &hal_dev_ctx->rpu_info.hpqm_info.rx_buf_busy_queue[queue_id];
	} else {
		nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: Invalid msg_type (%d)\n",
				      __func__,
				      msg_type);
		goto out;
	}

	/* Copy the address, to which information was posted,
	 * to the busy queue.
	 */
	status = hal_rpu_hpq_enqueue(hal_dev_ctx,
				     busy_queue,
				     msg_addr);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: Queueing of message to RPU failed\n",
				      __func__);
		goto out;
	}

	if (msg_type != NRF_WIFI_HAL_MSG_TYPE_CMD_DATA_RX) {
		/* Indicate to the RPU that the information has been posted */
		status = hal_rpu_msg_trigger(hal_dev_ctx);

		if (status != NRF_WIFI_STATUS_SUCCESS) {
			nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
					      "%s: Posting command to RPU failed\n",
					      __func__);
			goto out;
		}
	}
out:
	return status;
}


static enum nrf_wifi_status hal_rpu_msg_get_addr(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx,
						 enum NRF_WIFI_HAL_MSG_TYPE msg_type,
						 unsigned int *msg_addr)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct host_rpu_hpq *avl_queue = NULL;

	if (msg_type == NRF_WIFI_HAL_MSG_TYPE_CMD_CTRL) {
		avl_queue = &hal_dev_ctx->rpu_info.hpqm_info.cmd_avl_queue;
	} else {
		nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: Invalid msg_type (%d)\n",
				      __func__,
				      msg_type);
		goto out;
	}

	status = hal_rpu_hpq_dequeue(hal_dev_ctx,
				     avl_queue,
				     msg_addr);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: Dequeue of address failed msg_addr 0x%X\n",
				      __func__,
				      *msg_addr);
		*msg_addr = 0;
		goto out;
	}
out:
	return status;
}


static enum nrf_wifi_status hal_rpu_msg_write(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx,
					      enum NRF_WIFI_HAL_MSG_TYPE msg_type,
					      void *msg,
					      unsigned int len)
{
	unsigned int msg_addr = 0;
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;

	/* Get the address from the RPU to which
	 * the command needs to be copied to
	 */
	status = hal_rpu_msg_get_addr(hal_dev_ctx,
				      msg_type,
				      &msg_addr);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: Getting address (0x%X) to post message failed\n",
				      __func__,
				      msg_addr);
		goto out;
	}

	/* Copy the information to the suggested address */
	status = hal_rpu_mem_write(hal_dev_ctx,
				   msg_addr,
				   msg,
				   len);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: Copying information to RPU failed\n",
				      __func__);
		goto out;
	}

	/* Post the updated information to the RPU */
	status = hal_rpu_msg_post(hal_dev_ctx,
				  msg_type,
				  0,
				  msg_addr);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: Posting command to RPU failed\n",
				      __func__);
		goto out;
	}

out:
	return status;
}


static enum nrf_wifi_status hal_rpu_cmd_process_queue(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_hal_msg *cmd = NULL;

	while ((cmd = nrf_wifi_utils_q_dequeue(hal_dev_ctx->hpriv->opriv,
					       hal_dev_ctx->cmd_q))) {
		status = hal_rpu_ready_wait(hal_dev_ctx,
					    NRF_WIFI_HAL_MSG_TYPE_CMD_CTRL);

		if (status != NRF_WIFI_STATUS_SUCCESS) {
			nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
					      "%s: Timeout waiting to get free cmd buff from RPU\n",
					      __func__);
			nrf_wifi_osal_mem_free(hal_dev_ctx->hpriv->opriv,
					       cmd);
			cmd = NULL;
			continue;
		}

		status = hal_rpu_msg_write(hal_dev_ctx,
					   NRF_WIFI_HAL_MSG_TYPE_CMD_CTRL,
					   cmd->data,
					   cmd->len);

		if (status != NRF_WIFI_STATUS_SUCCESS) {
			nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
					      "%s: Writing command to RPU failed\n",
					      __func__);
			nrf_wifi_osal_mem_free(hal_dev_ctx->hpriv->opriv,
					       cmd);
			cmd = NULL;
			continue;
		}

		/* Free the command data and command */
		nrf_wifi_osal_mem_free(hal_dev_ctx->hpriv->opriv,
				       cmd);
		cmd = NULL;
	}

	return status;
}


static enum nrf_wifi_status hal_rpu_cmd_queue(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx,
					      void *cmd,
					      unsigned int cmd_size)
{
	int len = 0;
	int size = 0;
	char *data = NULL;
	struct nrf_wifi_hal_msg *hal_msg = NULL;
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;

	len = cmd_size;
	data = cmd;

	if (len > hal_dev_ctx->hpriv->cfg_params.max_cmd_size) {
		while (len > 0) {
			if (len > hal_dev_ctx->hpriv->cfg_params.max_cmd_size) {
				size = hal_dev_ctx->hpriv->cfg_params.max_cmd_size;
			} else {
				size = len;
			}

			hal_msg = nrf_wifi_osal_mem_zalloc(hal_dev_ctx->hpriv->opriv,
							   sizeof(*hal_msg) + size);

			if (!hal_msg) {
				nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
						      "%s: Unable to alloc buff for frag HAL cmd\n",
						      __func__);
				status = NRF_WIFI_STATUS_FAIL;
				goto out;
			}

			nrf_wifi_osal_mem_cpy(hal_dev_ctx->hpriv->opriv,
					      hal_msg->data,
					      data,
					      size);

			hal_msg->len = size;

			status = nrf_wifi_utils_q_enqueue(hal_dev_ctx->hpriv->opriv,
							  hal_dev_ctx->cmd_q,
							  hal_msg);

			if (status != NRF_WIFI_STATUS_SUCCESS) {
				nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
						      "%s: Unable to queue frag HAL cmd\n",
						      __func__);
				goto out;
			}

			len -= size;
			data += size;
		}
	} else {
		hal_msg = nrf_wifi_osal_mem_zalloc(hal_dev_ctx->hpriv->opriv,
						   sizeof(*hal_msg) + len);

		if (!hal_msg) {
			nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
					      "%s: Unable to allocate buffer for HAL command\n",
					      __func__);
			status = NRF_WIFI_STATUS_FAIL;
			goto out;
		}

		nrf_wifi_osal_mem_cpy(hal_dev_ctx->hpriv->opriv,
				      hal_msg->data,
				      cmd,
				      len);

		hal_msg->len = len;

		status = nrf_wifi_utils_q_enqueue(hal_dev_ctx->hpriv->opriv,
						  hal_dev_ctx->cmd_q,
						  hal_msg);

		if (status != NRF_WIFI_STATUS_SUCCESS) {
			nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
					      "%s: Unable to queue fragmented command\n",
					      __func__);
			goto out;
		}
	}

	/* Free the original command data */
	nrf_wifi_osal_mem_free(hal_dev_ctx->hpriv->opriv,
			       cmd);

out:
	return status;
}


enum nrf_wifi_status nrf_wifi_hal_ctrl_cmd_send(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx,
						void *cmd,
						unsigned int cmd_size)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;

	nrf_wifi_osal_spinlock_take(hal_dev_ctx->hpriv->opriv,
				    hal_dev_ctx->lock_hal);

	status = hal_rpu_cmd_queue(hal_dev_ctx,
				   cmd,
				   cmd_size);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: Queueing of command failed\n",
				      __func__);
		goto out;
	}

	status = hal_rpu_cmd_process_queue(hal_dev_ctx);

out:
	nrf_wifi_osal_spinlock_rel(hal_dev_ctx->hpriv->opriv,
				   hal_dev_ctx->lock_hal);

	return status;
}


enum nrf_wifi_status nrf_wifi_hal_data_cmd_send(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx,
						enum NRF_WIFI_HAL_MSG_TYPE cmd_type,
						void *cmd,
						unsigned int cmd_size,
						unsigned int desc_id,
						unsigned int pool_id)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	unsigned int addr_base = 0;
	unsigned int max_cmd_size = 0;
	unsigned int addr = 0;
	unsigned int host_addr = 0;


	nrf_wifi_osal_spinlock_take(hal_dev_ctx->hpriv->opriv,
				    hal_dev_ctx->lock_hal);

	if (cmd_type == NRF_WIFI_HAL_MSG_TYPE_CMD_DATA_RX) {
		addr_base = hal_dev_ctx->rpu_info.rx_cmd_base;
		max_cmd_size = RPU_DATA_CMD_SIZE_MAX_RX;
	} else if (cmd_type == NRF_WIFI_HAL_MSG_TYPE_CMD_DATA_TX) {
		addr_base = hal_dev_ctx->rpu_info.tx_cmd_base;
		max_cmd_size = RPU_DATA_CMD_SIZE_MAX_TX;
	} else {
		nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: Invalid data command type %d\n",
				      __func__,
				      cmd_type);
	}

	addr = addr_base + (max_cmd_size * desc_id);
	host_addr = addr;

	/* This is a indrect write to core memory */
	if (cmd_type == NRF_WIFI_HAL_MSG_TYPE_CMD_DATA_RX) {
		host_addr &= RPU_ADDR_MASK_OFFSET;
		host_addr |= RPU_MCU_CORE_INDIRECT_BASE;
	}

	/* Copy the information to the suggested address */
	status = hal_rpu_mem_write(hal_dev_ctx,
				   host_addr,
				   cmd,
				   cmd_size);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: Copying data cmd(%d) to RPU failed\n",
				      __func__,
				      cmd_type);
		goto out;
	}

	/* Post the updated information to the RPU */
	status = hal_rpu_msg_post(hal_dev_ctx,
				  cmd_type,
				  pool_id,
				  addr);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: Posting RX buf info to RPU failed\n",
				      __func__);
		goto out;
	}
out:
	nrf_wifi_osal_spinlock_rel(hal_dev_ctx->hpriv->opriv,
				   hal_dev_ctx->lock_hal);


	return status;
}


static void event_tasklet_fn(unsigned long data)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_hal_dev_ctx *hal_dev_ctx = NULL;

	hal_dev_ctx = (struct nrf_wifi_hal_dev_ctx *)data;

	status = hal_rpu_eventq_process(hal_dev_ctx);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: Event queue processing failed\n",
				      __func__);
	}
}


enum nrf_wifi_status hal_rpu_eventq_process(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_SUCCESS;
	struct nrf_wifi_hal_msg *event = NULL;
	unsigned long flags = 0;
	void *event_data = NULL;
	unsigned int event_len = 0;

	while (1) {
		nrf_wifi_osal_spinlock_irq_take(hal_dev_ctx->hpriv->opriv,
						hal_dev_ctx->lock_rx,
						&flags);

		event = nrf_wifi_utils_q_dequeue(hal_dev_ctx->hpriv->opriv,
						 hal_dev_ctx->event_q);

		nrf_wifi_osal_spinlock_irq_rel(hal_dev_ctx->hpriv->opriv,
					       hal_dev_ctx->lock_rx,
					       &flags);

		if (!event) {
			goto out;
		}

		event_data = event->data;
		event_len = event->len;

		/* Process the event further */
		status = hal_dev_ctx->hpriv->intr_callbk_fn(hal_dev_ctx->mac_dev_ctx,
							    event_data,
							    event_len);

		if (status != NRF_WIFI_STATUS_SUCCESS) {
			nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
					      "%s: Interrupt callback failed\n",
					      __func__);
		}

		/* Free up the local buffer */
		nrf_wifi_osal_mem_free(hal_dev_ctx->hpriv->opriv,
				       event);
		event = NULL;
	}

out:
	return status;
}


void nrf_wifi_hal_proc_ctx_set(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx,
			       enum RPU_PROC_TYPE proc)
{
	hal_dev_ctx->curr_proc = proc;
}


struct nrf_wifi_hal_dev_ctx *nrf_wifi_hal_dev_add(struct nrf_wifi_hal_priv *hpriv,
						  void *mac_dev_ctx)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_hal_dev_ctx *hal_dev_ctx = NULL;
#ifndef CONFIG_NRF700X_RADIO_TEST
	unsigned int i = 0;
	unsigned int num_rx_bufs = 0;
	unsigned int size = 0;
#endif /* !CONFIG_NRF700X_RADIO_TEST */

	hal_dev_ctx = nrf_wifi_osal_mem_zalloc(hpriv->opriv,
					       sizeof(*hal_dev_ctx));

	if (!hal_dev_ctx) {
		nrf_wifi_osal_log_err(hpriv->opriv,
				      "%s: Unable to allocate hal_dev_ctx\n",
				      __func__);
		goto err;
	}

	hal_dev_ctx->hpriv = hpriv;
	hal_dev_ctx->mac_dev_ctx = mac_dev_ctx;
	hal_dev_ctx->idx = hpriv->num_devs++;

	hal_dev_ctx->num_cmds = RPU_CMD_START_MAGIC;

	hal_dev_ctx->cmd_q = nrf_wifi_utils_q_alloc(hpriv->opriv);

	if (!hal_dev_ctx->cmd_q) {
		nrf_wifi_osal_log_err(hpriv->opriv,
				      "%s: Unable to allocate command queue\n",
				      __func__);
		goto hal_dev_free;
	}

	hal_dev_ctx->event_q = nrf_wifi_utils_q_alloc(hpriv->opriv);

	if (!hal_dev_ctx->event_q) {
		nrf_wifi_osal_log_err(hpriv->opriv,
				      "%s: Unable to allocate event queue\n",
				      __func__);
		goto cmd_q_free;
	}

	hal_dev_ctx->lock_hal = nrf_wifi_osal_spinlock_alloc(hpriv->opriv);

	if (!hal_dev_ctx->lock_hal) {
		nrf_wifi_osal_log_err(hpriv->opriv,
				      "%s: Unable to allocate HAL lock\n", __func__);
		hal_dev_ctx = NULL;
		goto event_q_free;
	}

	nrf_wifi_osal_spinlock_init(hpriv->opriv,
				    hal_dev_ctx->lock_hal);

	hal_dev_ctx->lock_rx = nrf_wifi_osal_spinlock_alloc(hpriv->opriv);

	if (!hal_dev_ctx->lock_rx) {
		nrf_wifi_osal_log_err(hpriv->opriv,
				      "%s: Unable to allocate HAL lock\n",
				      __func__);
		goto lock_hal_free;
	}

	nrf_wifi_osal_spinlock_init(hpriv->opriv,
				    hal_dev_ctx->lock_rx);

	hal_dev_ctx->event_tasklet = nrf_wifi_osal_tasklet_alloc(hpriv->opriv,
		NRF_WIFI_TASKLET_TYPE_BH);

	if (!hal_dev_ctx->event_tasklet) {
		nrf_wifi_osal_log_err(hpriv->opriv,
				      "%s: Unable to allocate event_tasklet\n",
				      __func__);
		goto lock_rx_free;
	}

	nrf_wifi_osal_tasklet_init(hpriv->opriv,
				   hal_dev_ctx->event_tasklet,
				   event_tasklet_fn,
				   (unsigned long)hal_dev_ctx);

#ifdef CONFIG_NRF_WIFI_LOW_POWER
	status = hal_rpu_ps_init(hal_dev_ctx);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		nrf_wifi_osal_log_err(hpriv->opriv,
				      "%s: hal_rpu_ps_init failed\n",
				      __func__);
		goto tasklet_free;
	}
#endif /* CONFIG_NRF_WIFI_LOW_POWER */

	hal_dev_ctx->bal_dev_ctx = nrf_wifi_bal_dev_add(hpriv->bpriv,
							hal_dev_ctx);

	if (!hal_dev_ctx->bal_dev_ctx) {
		nrf_wifi_osal_log_err(hpriv->opriv,
				      "%s: nrf_wifi_bal_dev_add failed\n",
				      __func__);
		goto tasklet_free;
	}

	status = hal_rpu_irq_enable(hal_dev_ctx);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		nrf_wifi_osal_log_err(hpriv->opriv,
				      "%s: hal_rpu_irq_enable failed\n",
				      __func__);
		goto bal_dev_free;
	}

#ifndef CONFIG_NRF700X_RADIO_TEST
	for (i = 0; i < MAX_NUM_OF_RX_QUEUES; i++) {
		num_rx_bufs = hal_dev_ctx->hpriv->cfg_params.rx_buf_pool[i].num_bufs;

		size = (num_rx_bufs * sizeof(struct nrf_wifi_hal_buf_map_info));

		hal_dev_ctx->rx_buf_info[i] = nrf_wifi_osal_mem_zalloc(hpriv->opriv,
								       size);

		if (!hal_dev_ctx->rx_buf_info[i]) {
			nrf_wifi_osal_log_err(hpriv->opriv,
					      "%s: No space for RX buf info[%d]\n",
					      __func__,
					      i);
			goto bal_dev_free;
		}
	}
#ifdef CONFIG_NRF700X_DATA_TX
	size = (hal_dev_ctx->hpriv->cfg_params.max_tx_frms *
		sizeof(struct nrf_wifi_hal_buf_map_info));

	hal_dev_ctx->tx_buf_info = nrf_wifi_osal_mem_zalloc(hpriv->opriv,
							    size);

	if (!hal_dev_ctx->tx_buf_info) {
		nrf_wifi_osal_log_err(hpriv->opriv,
				      "%s: No space for TX buf info\n",
				      __func__);
		goto rx_buf_free;
	}
#endif /* CONFIG_NRF700X_DATA_TX */

	status = nrf_wifi_hal_rpu_pktram_buf_map_init(hal_dev_ctx);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		nrf_wifi_osal_log_err(hpriv->opriv,
				      "%s: Buffer map init failed\n",
				      __func__);
#ifdef CONFIG_NRF700X_DATA_TX
		goto tx_buf_free;
#endif /* CONFIG_NRF700X_DATA_TX */
	}
#endif /* !CONFIG_NRF700X_RADIO_TEST */

	return hal_dev_ctx;
#ifndef CONFIG_NRF700X_RADIO_TEST
#ifdef CONFIG_NRF700X_DATA_TX
tx_buf_free:
	nrf_wifi_osal_mem_free(hpriv->opriv,
			       hal_dev_ctx->tx_buf_info);
	hal_dev_ctx->tx_buf_info = NULL;
rx_buf_free:

	for (i = 0; i < MAX_NUM_OF_RX_QUEUES; i++) {
		nrf_wifi_osal_mem_free(hpriv->opriv,
						hal_dev_ctx->rx_buf_info[i]);
		hal_dev_ctx->rx_buf_info[i] = NULL;
	}
#endif /* CONFIG_NRF700X_DATA_TX */
#endif /* !CONFIG_NRF700X_RADIO_TEST */
bal_dev_free:
	nrf_wifi_bal_dev_rem(hal_dev_ctx->bal_dev_ctx);
tasklet_free:
	nrf_wifi_osal_tasklet_free(hpriv->opriv,
					hal_dev_ctx->event_tasklet);
lock_rx_free:
	nrf_wifi_osal_spinlock_free(hpriv->opriv,
					hal_dev_ctx->lock_rx);
lock_hal_free:
	nrf_wifi_osal_spinlock_free(hpriv->opriv,
					hal_dev_ctx->lock_hal);
event_q_free:
	nrf_wifi_utils_q_free(hpriv->opriv,
					hal_dev_ctx->event_q);
cmd_q_free:
	nrf_wifi_utils_q_free(hpriv->opriv,
					hal_dev_ctx->cmd_q);
hal_dev_free:
	nrf_wifi_osal_mem_free(hpriv->opriv,
					hal_dev_ctx);
	hal_dev_ctx = NULL;
err:
	return NULL;
}


void nrf_wifi_hal_dev_rem(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx)
{
	unsigned int i = 0;


	nrf_wifi_bal_dev_rem(hal_dev_ctx->bal_dev_ctx);

	nrf_wifi_osal_mem_free(hal_dev_ctx->hpriv->opriv,
			       hal_dev_ctx->tx_buf_info);
	hal_dev_ctx->tx_buf_info = NULL;

	for (i = 0; i < MAX_NUM_OF_RX_QUEUES; i++) {
		nrf_wifi_osal_mem_free(hal_dev_ctx->hpriv->opriv,
				       hal_dev_ctx->rx_buf_info[i]);
		hal_dev_ctx->rx_buf_info[i] = NULL;
	}

	nrf_wifi_osal_tasklet_kill(hal_dev_ctx->hpriv->opriv,
				   hal_dev_ctx->event_tasklet);

	nrf_wifi_osal_tasklet_free(hal_dev_ctx->hpriv->opriv,
				   hal_dev_ctx->event_tasklet);

	nrf_wifi_osal_spinlock_free(hal_dev_ctx->hpriv->opriv,
				    hal_dev_ctx->lock_hal);
	nrf_wifi_osal_spinlock_free(hal_dev_ctx->hpriv->opriv,
				    hal_dev_ctx->lock_rx);

	nrf_wifi_utils_q_free(hal_dev_ctx->hpriv->opriv,
			      hal_dev_ctx->event_q);

	nrf_wifi_utils_q_free(hal_dev_ctx->hpriv->opriv,
			      hal_dev_ctx->cmd_q);

#ifdef CONFIG_NRF_WIFI_LOW_POWER
	hal_rpu_ps_deinit(hal_dev_ctx);
#endif /* CONFIG_NRF_WIFI_LOW_POWER */

	hal_dev_ctx->hpriv->num_devs--;

	nrf_wifi_osal_mem_free(hal_dev_ctx->hpriv->opriv,
			       hal_dev_ctx);
}


enum nrf_wifi_status nrf_wifi_hal_dev_init(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;

#ifdef CONFIG_NRF_WIFI_LOW_POWER
	hal_dev_ctx->rpu_fw_booted = true;
#endif /* CONFIG_NRF_WIFI_LOW_POWER */

	status = nrf_wifi_bal_dev_init(hal_dev_ctx->bal_dev_ctx);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: nrf_wifi_bal_dev_init failed\n",
				      __func__);
		goto out;
	}

	/* Read the HPQM info for all the queues provided by the RPU
	 * (like command, event, RX buf queues etc)
	 */
	status = hal_rpu_mem_read(hal_dev_ctx,
				  &hal_dev_ctx->rpu_info.hpqm_info,
				  RPU_MEM_HPQ_INFO,
				  sizeof(hal_dev_ctx->rpu_info.hpqm_info));

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: Failed to get the HPQ info\n",
				      __func__);
		goto out;
	}

	status = hal_rpu_mem_read(hal_dev_ctx,
				  &hal_dev_ctx->rpu_info.rx_cmd_base,
				  RPU_MEM_RX_CMD_BASE,
				  sizeof(hal_dev_ctx->rpu_info.rx_cmd_base));

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: Reading the RX cmd base failed\n",
				      __func__);
		goto out;
	}

	hal_dev_ctx->rpu_info.tx_cmd_base = RPU_MEM_TX_CMD_BASE;
out:
	return status;
}


void nrf_wifi_hal_dev_deinit(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx)
{
	nrf_wifi_bal_dev_deinit(hal_dev_ctx->bal_dev_ctx);
}

enum nrf_wifi_status hal_rpu_recovery(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;

	if (!hal_dev_ctx->hpriv->rpu_recovery_callbk_fn) {
		nrf_wifi_osal_log_dbg(hal_dev_ctx->hpriv->opriv,
				      "%s: RPU recovery callback not registered",
				      __func__);
		goto out;
	}

	status = hal_dev_ctx->hpriv->rpu_recovery_callbk_fn(hal_dev_ctx->mac_dev_ctx, NULL, 0);
	if (status != NRF_WIFI_STATUS_SUCCESS) {
		nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: RPU recovery failed",
				      __func__);
		goto out;
	}

out:
	return status;
}

enum nrf_wifi_status nrf_wifi_hal_irq_handler(void *data)
{
	struct nrf_wifi_hal_dev_ctx *hal_dev_ctx = NULL;
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	unsigned long flags = 0;
#ifdef CONFIG_NRF_WIFI_LOW_POWER
	enum RPU_PS_STATE ps_state = RPU_PS_STATE_ASLEEP;
#endif /* CONFIG_NRF_WIFI_LOW_POWER */
	bool do_rpu_recovery = false;

	hal_dev_ctx = (struct nrf_wifi_hal_dev_ctx *)data;

	nrf_wifi_osal_spinlock_irq_take(hal_dev_ctx->hpriv->opriv,
					hal_dev_ctx->lock_rx,
					&flags);

#ifdef CONFIG_NRF_WIFI_LOW_POWER
	ps_state = hal_dev_ctx->rpu_ps_state;
	hal_rpu_ps_set_state(hal_dev_ctx,
			     RPU_PS_STATE_AWAKE);
#ifdef CONFIG_NRF_WIFI_LOW_POWER_DBG
	hal_dev_ctx->irq_ctx = true;
#endif /* CONFIG_NRF_WIFI_LOW_POWER_DBG */
#endif /* CONFIG_NRF_WIFI_LOW_POWER */

	status = hal_rpu_irq_process(hal_dev_ctx, &do_rpu_recovery);

#ifdef CONFIG_NRF_WIFI_LOW_POWER
	hal_rpu_ps_set_state(hal_dev_ctx,
			     ps_state);
#ifdef CONFIG_NRF_WIFI_LOW_POWER_DBG
	hal_dev_ctx->irq_ctx = false;
#endif /* CONFIG_NRF_WIFI_LOW_POWER_DBG */
#endif /* CONFIG_NRF_WIFI_LOW_POWER */

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		goto out;
	}

	if (do_rpu_recovery) {
		status = hal_rpu_recovery(hal_dev_ctx);

		if (status != NRF_WIFI_STATUS_SUCCESS) {
			nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
					      "%s: RPU recovery failed",
					      __func__);
		}
		/* RPU recovery is done, so ignore the rest of the processing */
		goto out;
	}

	nrf_wifi_osal_tasklet_schedule(hal_dev_ctx->hpriv->opriv,
				       hal_dev_ctx->event_tasklet);

out:
	nrf_wifi_osal_spinlock_irq_rel(hal_dev_ctx->hpriv->opriv,
				       hal_dev_ctx->lock_rx,
				       &flags);
	return status;
}


static int nrf_wifi_hal_poll_reg(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx,
				 unsigned int reg_addr,
				 unsigned int mask,
				 unsigned int req_value,
				 unsigned int poll_delay)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	unsigned int val = 0;
	unsigned int count = 50;

	do {
		status = hal_rpu_reg_read(hal_dev_ctx,
					  &val,
					  reg_addr);

		if (status != NRF_WIFI_STATUS_SUCCESS) {
			nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
					      "%s: Read from address (0x%X) failed, val (0x%X)\n",
					      __func__,
					      reg_addr,
					      val);
		}

		if ((val & mask) == req_value) {
			status = NRF_WIFI_STATUS_SUCCESS;
			break;
		}

		nrf_wifi_osal_sleep_ms(hal_dev_ctx->hpriv->opriv,
				       poll_delay);
	} while (count-- > 0);

	if (count == 0) {
		nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: Timed out polling on (0x%X)\n",
				      __func__,
				      reg_addr);

		status = NRF_WIFI_STATUS_FAIL;
		goto out;
	}
out:
	return status;
}


/* Perform MIPS reset */
enum nrf_wifi_status nrf_wifi_hal_proc_reset(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx,
					     enum RPU_PROC_TYPE rpu_proc)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;

	if ((rpu_proc != RPU_PROC_TYPE_MCU_LMAC) &&
	    (rpu_proc != RPU_PROC_TYPE_MCU_UMAC)) {
		nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: Unsupported RPU processor(%d)\n",
				      __func__,
				      rpu_proc);
		goto out;
	}

	/* Perform pulsed soft reset of MIPS */
	if (rpu_proc == RPU_PROC_TYPE_MCU_LMAC) {
		status = hal_rpu_reg_write(hal_dev_ctx,
					   RPU_REG_MIPS_MCU_CONTROL,
					   0x1);
	} else {
		status = hal_rpu_reg_write(hal_dev_ctx,
					   RPU_REG_MIPS_MCU2_CONTROL,
					   0x1);
	}

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: Pulsed soft reset of MCU failed for (%d) processor\n",
				      __func__,
				      rpu_proc);
		goto out;
	}


	/* Wait for it to come out of reset */
	if (rpu_proc == RPU_PROC_TYPE_MCU_LMAC) {
		status = nrf_wifi_hal_poll_reg(hal_dev_ctx,
					       RPU_REG_MIPS_MCU_CONTROL,
					       0x1,
					       0,
					       10);
	} else {
		status = nrf_wifi_hal_poll_reg(hal_dev_ctx,
					       RPU_REG_MIPS_MCU2_CONTROL,
					       0x1,
					       0,
					       10);
	}

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: MCU (%d) failed to come out of reset\n",
				      __func__,
				      rpu_proc);
		goto out;
	}

	/* MIPS will restart from it's boot exception registers
	 * and hit its default wait instruction
	 */
	if (rpu_proc == RPU_PROC_TYPE_MCU_LMAC) {
		status = nrf_wifi_hal_poll_reg(hal_dev_ctx,
					       0xA4000018,
					       0x1,
					       0x1,
					       10);
	} else {
		status = nrf_wifi_hal_poll_reg(hal_dev_ctx,
					       0xA4000118,
					       0x1,
					       0x1,
					       10);
	}
out:
	return status;
}


enum nrf_wifi_status nrf_wifi_hal_fw_chk_boot(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx,
					      enum RPU_PROC_TYPE rpu_proc)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	unsigned int addr = 0;
	unsigned int val = 0;
	unsigned int exp_val = 0;
	unsigned int i = 0;

	if (rpu_proc == RPU_PROC_TYPE_MCU_LMAC) {
		addr = RPU_MEM_LMAC_BOOT_SIG;
		exp_val = NRF_WIFI_LMAC_BOOT_SIG;
	} else if (rpu_proc == RPU_PROC_TYPE_MCU_UMAC) {
		addr = RPU_MEM_UMAC_BOOT_SIG;
		exp_val = NRF_WIFI_UMAC_BOOT_SIG;
	} else {
		nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: Invalid RPU processor (%d)\n",
				      __func__,
				      rpu_proc);
	}

	while (i < 1000) {
		status = hal_rpu_mem_read(hal_dev_ctx,
					  (unsigned char *)&val,
					  addr,
					  sizeof(val));

		if (status != NRF_WIFI_STATUS_SUCCESS) {
			nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
					      "%s: Reading of boot signature failed for RPU(%d)\n",
					      __func__,
					      rpu_proc);
		}

		if (val == exp_val) {
			break;
		}

		/* Sleep for 10 ms */
		nrf_wifi_osal_sleep_ms(hal_dev_ctx->hpriv->opriv,
				       10);

		i++;

	};

	if (i == 1000) {
		nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: Boot_sig check failed for RPU(%d), "
				      "Expected: 0x%X, Actual: 0x%X\n",
				      __func__,
				      rpu_proc,
				      exp_val,
				      val);
		status = NRF_WIFI_STATUS_FAIL;
		goto out;
	}

	status = NRF_WIFI_STATUS_SUCCESS;
out:
	return status;
}


struct nrf_wifi_hal_priv *
nrf_wifi_hal_init(struct nrf_wifi_osal_priv *opriv,
		  struct nrf_wifi_hal_cfg_params *cfg_params,
		  enum nrf_wifi_status (*intr_callbk_fn)(void *dev_ctx,
							 void *event_data,
							 unsigned int len),
		  enum nrf_wifi_status (*rpu_recovery_callbk_fn)(void *mac_ctx,
							     void *event_data,
							     unsigned int len))
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_hal_priv *hpriv = NULL;
	struct nrf_wifi_bal_cfg_params bal_cfg_params;

	hpriv = nrf_wifi_osal_mem_zalloc(opriv,
					 sizeof(*hpriv));

	if (!hpriv) {
		nrf_wifi_osal_log_err(opriv,
				      "%s: Unable to allocate memory for hpriv\n",
				      __func__);
		goto out;
	}

	hpriv->opriv = opriv;

	nrf_wifi_osal_mem_cpy(opriv,
			      &hpriv->cfg_params,
			      cfg_params,
			      sizeof(hpriv->cfg_params));

	hpriv->intr_callbk_fn = intr_callbk_fn;
	hpriv->rpu_recovery_callbk_fn = rpu_recovery_callbk_fn;

	status = pal_rpu_addr_offset_get(opriv,
					 RPU_ADDR_PKTRAM_START,
					 &hpriv->addr_pktram_base,
					 RPU_PROC_TYPE_MAX);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		nrf_wifi_osal_log_err(opriv,
				      "%s: pal_rpu_addr_offset_get failed\n",
				      __func__);
		goto out;
	}

	bal_cfg_params.addr_pktram_base = hpriv->addr_pktram_base;

	hpriv->bpriv = nrf_wifi_bal_init(opriv,
					 &bal_cfg_params,
					 &nrf_wifi_hal_irq_handler);

	if (!hpriv->bpriv) {
		nrf_wifi_osal_log_err(opriv,
				      "%s: Failed\n",
				      __func__);
		nrf_wifi_osal_mem_free(opriv,
				       hpriv);
		hpriv = NULL;
	}
out:
	return hpriv;
}


void nrf_wifi_hal_deinit(struct nrf_wifi_hal_priv *hpriv)
{
	nrf_wifi_bal_deinit(hpriv->bpriv);

	nrf_wifi_osal_mem_free(hpriv->opriv,
			       hpriv);
}


enum nrf_wifi_status nrf_wifi_hal_otp_info_get(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx,
					       struct host_rpu_umac_info *otp_info,
					       unsigned int *otp_flags)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;

	if (!hal_dev_ctx || !otp_info) {
		nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: Invalid parameters\n",
				      __func__);
		goto out;
	}

	status = hal_rpu_mem_read(hal_dev_ctx,
				  otp_info,
				  RPU_MEM_UMAC_BOOT_SIG,
				  sizeof(*otp_info));

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: OTP info get failed\n",
				      __func__);
		goto out;
	}

	status = hal_rpu_mem_read(hal_dev_ctx,
				  otp_flags,
				  RPU_MEM_OTP_INFO_FLAGS,
				  sizeof(*otp_flags));

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: OTP flags get failed\n",
				      __func__);
		goto out;
	}
out:
	return status;
}


enum nrf_wifi_status nrf_wifi_hal_otp_ft_prog_ver_get(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx,
						      unsigned int *ft_prog_ver)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;

	if (!hal_dev_ctx || !ft_prog_ver) {
		nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: Invalid parameters\n",
				      __func__);
		goto out;
	}

	status = hal_rpu_mem_read(hal_dev_ctx,
				  ft_prog_ver,
				  RPU_MEM_OTP_FT_PROG_VERSION,
				  sizeof(*ft_prog_ver));

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: FT program version get failed\n",
				      __func__);
		goto out;
	}
out:
	return status;
}
