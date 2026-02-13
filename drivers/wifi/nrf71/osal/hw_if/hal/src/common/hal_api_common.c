/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @brief File containing API definitions for the
 * HAL Layer of the Wi-Fi driver.
 */

#include "queue.h"

#include "common/hal_api_common.h"


static enum nrf_wifi_status hal_rpu_msg_write(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx,
					      enum NRF_WIFI_HAL_MSG_TYPE msg_type,
					      void *msg,
					      unsigned int len)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;

	status = nrf_wifi_osal_ipc_send_msg(msg_type, msg, len);
	if (status != NRF_WIFI_STATUS_SUCCESS) {
		nrf_wifi_osal_log_err("%s: Sending message to RPU failed",
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

	while ((cmd = nrf_wifi_utils_ctrl_q_dequeue(hal_dev_ctx->cmd_q))) {
		status = hal_rpu_msg_write(hal_dev_ctx,
					   NRF_WIFI_HAL_MSG_TYPE_CMD_CTRL,
					   cmd->data,
					   cmd->len);

		if (status != NRF_WIFI_STATUS_SUCCESS) {
			nrf_wifi_osal_log_err("%s: Writing command to RPU failed",
					      __func__);
			nrf_wifi_osal_mem_free(cmd);
			cmd = NULL;
			continue;
		}

		/* Free the command data and command */
		nrf_wifi_osal_mem_free(cmd);
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

			hal_msg = nrf_wifi_osal_mem_zalloc(sizeof(*hal_msg) + size);

			if (!hal_msg) {
				nrf_wifi_osal_log_err("%s: Unable to alloc buff for frag HAL cmd",
						      __func__);
				status = NRF_WIFI_STATUS_FAIL;
				goto out;
			}

			nrf_wifi_osal_mem_cpy(hal_msg->data,
					      data,
					      size);

			hal_msg->len = size;

			status = nrf_wifi_utils_ctrl_q_enqueue(hal_dev_ctx->cmd_q,
							  hal_msg);

			if (status != NRF_WIFI_STATUS_SUCCESS) {
				nrf_wifi_osal_log_err("%s: Unable to queue frag HAL cmd",
						      __func__);
				goto out;
			}

			len -= size;
			data += size;
		}
	} else {
		hal_msg = nrf_wifi_osal_mem_zalloc(sizeof(*hal_msg) + len);

		if (!hal_msg) {
			nrf_wifi_osal_log_err("%s: Unable to allocate buffer for HAL command",
					      __func__);
			status = NRF_WIFI_STATUS_FAIL;
			goto out;
		}

		nrf_wifi_osal_mem_cpy(hal_msg->data,
				      cmd,
				      len);

		hal_msg->len = len;

		status = nrf_wifi_utils_ctrl_q_enqueue(hal_dev_ctx->cmd_q,
						  hal_msg);

		if (status != NRF_WIFI_STATUS_SUCCESS) {
			nrf_wifi_osal_log_err("%s: Unable to queue fragmented command",
					      __func__);
			goto out;
		}
	}

	/* Free the original command data */
	nrf_wifi_osal_mem_free(cmd);

out:
	return status;
}


enum nrf_wifi_status nrf_wifi_hal_ctrl_cmd_send(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx,
						void *cmd,
						unsigned int cmd_size)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;


#ifdef NRF_WIFI_CMD_EVENT_LOG
	nrf_wifi_osal_log_info("%s: caller %p",
			      __func__,
			      __builtin_return_address(0));
#else
	nrf_wifi_osal_log_dbg("%s: caller %p",
			     __func__,
			     __builtin_return_address(0));
#endif
	nrf_wifi_osal_spinlock_take(hal_dev_ctx->lock_hal);

	status = hal_rpu_cmd_queue(hal_dev_ctx,
				   cmd,
				   cmd_size);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		nrf_wifi_osal_log_err("%s: Queueing of command failed",
				      __func__);
		goto out;
	}

	status = hal_rpu_cmd_process_queue(hal_dev_ctx);

out:
	nrf_wifi_osal_spinlock_rel(hal_dev_ctx->lock_hal);

	return status;
}


enum nrf_wifi_status hal_rpu_eventq_process(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_SUCCESS;
	struct nrf_wifi_hal_msg *event = NULL;
	void *event_data = NULL;
	unsigned int event_len = 0;

	while (1) {
		event = nrf_wifi_utils_ctrl_q_dequeue(hal_dev_ctx->event_q);
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
			nrf_wifi_osal_log_err("%s: Interrupt callback failed",
					      __func__);
		}

		/* Free up the local buffer */
		nrf_wifi_osal_mem_free(event);
		event = NULL;
	}

out:
	return status;
}

static void hal_rpu_eventq_drain(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx)
{
	struct nrf_wifi_hal_msg *event = NULL;
	unsigned long flags = 0;

	while (1) {
		nrf_wifi_osal_spinlock_irq_take(hal_dev_ctx->lock_rx,
						&flags);

		event = nrf_wifi_utils_ctrl_q_dequeue(hal_dev_ctx->event_q);

		nrf_wifi_osal_spinlock_irq_rel(hal_dev_ctx->lock_rx,
					       &flags);

		if (!event) {
			goto out;
		}

		/* Free up the local buffer */
		nrf_wifi_osal_mem_free(event);
		event = NULL;
	}

out:
	return;
}

void nrf_wifi_hal_proc_ctx_set(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx,
			       enum RPU_PROC_TYPE proc)
{
	hal_dev_ctx->curr_proc = proc;
}


void nrf_wifi_hal_dev_rem(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx)
{
	unsigned int i = 0;

	nrf_wifi_osal_tasklet_kill(hal_dev_ctx->recovery_tasklet);
	nrf_wifi_osal_tasklet_free(hal_dev_ctx->recovery_tasklet);
	nrf_wifi_osal_spinlock_free(hal_dev_ctx->lock_recovery);

	nrf_wifi_osal_tasklet_kill(hal_dev_ctx->event_tasklet);

	nrf_wifi_osal_tasklet_free(hal_dev_ctx->event_tasklet);

	hal_rpu_eventq_drain(hal_dev_ctx);

	nrf_wifi_osal_spinlock_free(hal_dev_ctx->lock_hal);
	nrf_wifi_osal_spinlock_free(hal_dev_ctx->lock_rx);

	nrf_wifi_utils_ctrl_q_free(hal_dev_ctx->event_q);

	nrf_wifi_utils_ctrl_q_free(hal_dev_ctx->cmd_q);

#ifdef NRF_WIFI_LOW_POWER
	hal_rpu_ps_deinit(hal_dev_ctx);
#endif

	nrf_wifi_bal_dev_rem(hal_dev_ctx->bal_dev_ctx);

	nrf_wifi_osal_mem_free(hal_dev_ctx->tx_buf_info);
	hal_dev_ctx->tx_buf_info = NULL;

	for (i = 0; i < MAX_NUM_OF_RX_QUEUES; i++) {
		nrf_wifi_osal_mem_free(hal_dev_ctx->rx_buf_info[i]);
		hal_dev_ctx->rx_buf_info[i] = NULL;
	}

	hal_dev_ctx->hpriv->num_devs--;

	nrf_wifi_osal_mem_free(hal_dev_ctx);
}


enum nrf_wifi_status nrf_wifi_hal_dev_init(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;

#ifdef NRF_WIFI_LOW_POWER
	hal_dev_ctx->rpu_fw_booted = true;
#endif /* NRF_WIFI_LOW_POWER */

	status = nrf_wifi_bal_dev_init(hal_dev_ctx->bal_dev_ctx);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		nrf_wifi_osal_log_err("%s: nrf_wifi_bal_dev_init failed",
				      __func__);
		goto out;
	}

	nrf_wifi_hal_enable(hal_dev_ctx);
out:
	return status;
}

enum nrf_wifi_status nrf_wifi_hal_ipc_msg_handler(void *priv)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_hal_dev_ctx *hal_dev_ctx = (struct nrf_wifi_hal_dev_ctx *) priv;
	void *event_data = hal_dev_ctx->ipc_msg;
	/* IPC message is a pointer to PKTRAM address so the len is not relevant */
	unsigned int event_len = sizeof(event_data);

	nrf_wifi_osal_log_dbg("%s: IPC message received\n", __func__);
	status = hal_dev_ctx->hpriv->intr_callbk_fn(hal_dev_ctx->mac_dev_ctx,
						    event_data,
						    event_len);

	return status;
}

void nrf_wifi_hal_dev_deinit(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx)
{
	nrf_wifi_hal_disable(hal_dev_ctx);
	nrf_wifi_bal_dev_deinit(hal_dev_ctx->bal_dev_ctx);
	hal_rpu_eventq_drain(hal_dev_ctx);
}



struct nrf_wifi_hal_priv *
nrf_wifi_hal_init(struct nrf_wifi_hal_cfg_params *cfg_params,
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

	hpriv = nrf_wifi_osal_mem_zalloc(sizeof(*hpriv));

	if (!hpriv) {
		nrf_wifi_osal_log_err("%s: Unable to allocate memory for hpriv",
				      __func__);
		goto out;
	}

	nrf_wifi_osal_mem_cpy(&hpriv->cfg_params,
			      cfg_params,
			      sizeof(hpriv->cfg_params));

	hpriv->intr_callbk_fn = intr_callbk_fn;
	hpriv->rpu_recovery_callbk_fn = rpu_recovery_callbk_fn;

	ARG_UNUSED(status);
	/* PKTRAM base addr is not needed for IPC */
	hpriv->bpriv = nrf_wifi_bal_init(&bal_cfg_params, &nrf_wifi_hal_ipc_msg_handler);

	if (!hpriv->bpriv) {
		nrf_wifi_osal_log_err("%s: Failed",
				      __func__);
		nrf_wifi_osal_mem_free(hpriv);
		hpriv = NULL;
	}
out:
	return hpriv;
}


void nrf_wifi_hal_deinit(struct nrf_wifi_hal_priv *hpriv)
{
	nrf_wifi_bal_deinit(hpriv->bpriv);

	nrf_wifi_osal_mem_free(hpriv);
}


void nrf_wifi_hal_enable(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx)
{
	nrf_wifi_osal_spinlock_irq_take(hal_dev_ctx->lock_rx,
					NULL);
	hal_dev_ctx->hal_status = NRF_WIFI_HAL_STATUS_ENABLED;
	nrf_wifi_osal_spinlock_irq_rel(hal_dev_ctx->lock_rx,
				       NULL);
}

void nrf_wifi_hal_disable(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx)
{
	nrf_wifi_osal_spinlock_irq_take(hal_dev_ctx->lock_rx,
					NULL);
	hal_dev_ctx->hal_status = NRF_WIFI_HAL_STATUS_DISABLED;
	nrf_wifi_osal_spinlock_irq_rel(hal_dev_ctx->lock_rx,
				       NULL);
}

enum NRF_WIFI_HAL_STATUS nrf_wifi_hal_status_unlocked(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx)
{
	return hal_dev_ctx->hal_status;
}

__weak void hal_rpu_ps_deinit(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx)
{
	(void)hal_dev_ctx;
}
