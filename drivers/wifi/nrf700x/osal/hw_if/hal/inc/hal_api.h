/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief Header containing API declarations for the
 * HAL Layer of the Wi-Fi driver.
 */


#ifndef __HAL_API_H__
#define __HAL_API_H__

#include <stdbool.h>
#include "osal_api.h"
#include "rpu_if.h"
#include "bal_api.h"
#include "hal_structs.h"
#include "hal_mem.h"
#include "hal_reg.h"
#include "hal_fw_patch_loader.h"


/**
 * nrf_wifi_hal_init() - Initialize the HAL layer.
 *
 * @opriv: Pointer to the OSAL layer.
 * @cfg_params: Parameters needed to configure the HAL for WLAN operation.
 * @intr_callbk_fn: Pointer to the callback function which the user of this
 *                  layer needs to implement to handle events from the RPU.
 *
 * This API is used to initialize the HAL layer and is expected to be called
 * before using the HAL layer. This API returns a pointer to the HAL context
 * which might need to be passed to further API calls.
 *
 * Returns: Pointer to instance of HAL layer context.
 */
struct nrf_wifi_hal_priv *
nrf_wifi_hal_init(struct nrf_wifi_osal_priv *opriv,
		  struct nrf_wifi_hal_cfg_params *cfg_params,
		  enum nrf_wifi_status (*intr_callbk_fn)(void *mac_ctx,
							 void *event_data,
							 unsigned int len));

/**
 * nrf_wifi_hal_deinit() - Deinitialize the HAL layer.
 * @hpriv: Pointer to the HAL context returned by the @nrf_wifi_hal_init API.
 *
 * This API is used to deinitialize the HAL layer and is expected to be called
 * after done using the HAL layer.
 *
 * Returns: None.
 */
void nrf_wifi_hal_deinit(struct nrf_wifi_hal_priv *hpriv);


struct nrf_wifi_hal_dev_ctx *nrf_wifi_hal_dev_add(struct nrf_wifi_hal_priv *hpriv,
						  void *mac_dev_ctx);


void nrf_wifi_hal_dev_rem(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx);


enum nrf_wifi_status nrf_wifi_hal_dev_init(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx);


void nrf_wifi_hal_dev_deinit(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx);

/**
 * nrf_wifi_hal_ctrl_cmd_send() - Sends a control command to the RPU.
 * @hal_ctx: Pointer to HAL context.
 * @cmd: Pointer to command data.
 * @cmd_size: Size of the command data pointed to by @cmd.
 *
 * This function takes care of sending a command to the RPU. It does the
 * following:
 *
 *     - Fragments a command into smaller chunks if the size of a command is
 *       greater than %MAX_CMD_SIZE.
 *     - Queues up the command(s) to the HAL command queue.
 *     - Calls a function to further process the commands queued up in the HAL
 *       command queue which handles it by:
 *
 *           - Waiting for the RPU to be ready to handle a command.
 *           - Copies the command to the GRAM memory and indicates to the RPU
 *             that a command has been posted
 *
 * Return: Status
 *		Pass : %NRF_WIFI_STATUS_SUCCESS
 *		Error: %NRF_WIFI_STATUS_FAIL
 */
enum nrf_wifi_status nrf_wifi_hal_ctrl_cmd_send(struct nrf_wifi_hal_dev_ctx *hal_ctx,
						void *cmd,
						unsigned int cmd_size);


/**
 * nrf_wifi_hal_data_cmd_send() - Send a data command to the RPU.
 * @hal_ctx: Pointer to HAL context.
 * @cmd_type: Type of the data command to send to the RPU.
 * @data_cmd: The data command to be sent to the RPU.
 * @data_cmd_size: Size of the data command to be sent to the RPU.
 * @desc_id: Descriptor ID of the buffer being submitted to RPU.
 * @pool_id: Pool ID to which the buffer being submitted to RPU belongs.
 *
 * This function programs the relevant information about a data command,
 * to the RPU. These buffers are needed by the RPU to receive data and
 * management frames as well as to transmit data frames.
 *
 * Return: Status
 *		Pass : %NRF_WIFI_STATUS_SUCCESS
 *		Error: %NRF_WIFI_STATUS_FAIL
 */
enum nrf_wifi_status nrf_wifi_hal_data_cmd_send(struct nrf_wifi_hal_dev_ctx *hal_ctx,
						enum NRF_WIFI_HAL_MSG_TYPE cmd_type,
						void *data_cmd,
						unsigned int data_cmd_size,
						unsigned int desc_id,
						unsigned int pool_id);

/**
 * hal_rpu_eventq_process() - Process events from the RPU.
 * @hpriv: Pointer to HAL context.
 *
 * This function processes the events which have been queued into the event
 * queue by an ISR. It does the following:
 *
 *     - Dequeues an event from the event queue.
 *     - Calls hal_event_process to further process the event.
 *
 * Return: Status
 *		Pass: %NRF_WIFI_STATUS_SUCCESS
 *		Fail: %NRF_WIFI_STATUS_FAIL
 */
enum nrf_wifi_status hal_rpu_eventq_process(struct nrf_wifi_hal_dev_ctx *hal_ctx);


unsigned long nrf_wifi_hal_buf_map_rx(struct nrf_wifi_hal_dev_ctx *hal_ctx,
				      unsigned long buf,
				      unsigned int buf_len,
				      unsigned int pool_id,
				      unsigned int buf_id);

unsigned long nrf_wifi_hal_buf_unmap_rx(struct nrf_wifi_hal_dev_ctx *hal_ctx,
					unsigned int data_len,
					unsigned int pool_id,
					unsigned int buf_id);

unsigned long nrf_wifi_hal_buf_map_tx(struct nrf_wifi_hal_dev_ctx *hal_ctx,
				      unsigned long buf,
				      unsigned int buf_len,
				      unsigned int desc_id,
				      unsigned int token,
				      unsigned int buf_indx);

unsigned long nrf_wifi_hal_buf_unmap_tx(struct nrf_wifi_hal_dev_ctx *hal_ctx,
					unsigned int desc_id);

void nrf_wifi_hal_proc_ctx_set(struct nrf_wifi_hal_dev_ctx *hal_ctx,
			       enum RPU_PROC_TYPE proc);

enum nrf_wifi_status nrf_wifi_hal_proc_reset(struct nrf_wifi_hal_dev_ctx *hal_ctx,
					     enum RPU_PROC_TYPE rpu_proc);

enum nrf_wifi_status nrf_wifi_hal_fw_chk_boot(struct nrf_wifi_hal_dev_ctx *hal_ctx,
					      enum RPU_PROC_TYPE rpu_proc);


#ifdef CONFIG_NRF_WIFI_LOW_POWER
enum nrf_wifi_status hal_rpu_ps_wake(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx);
enum nrf_wifi_status nrf_wifi_hal_get_rpu_ps_state(
				struct nrf_wifi_hal_dev_ctx *hal_dev_ctx,
				int *rpu_ps_ctrl_state);
#endif /* CONFIG_NRF_WIFI_LOW_POWER */
#endif /* __HAL_API_H__ */


enum nrf_wifi_status nrf_wifi_hal_otp_info_get(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx,
					       struct host_rpu_umac_info *otp_info,
					       unsigned int *otp_flags);

enum nrf_wifi_status nrf_wifi_hal_otp_ft_prog_ver_get(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx,
						      unsigned int *ft_prog_ver);
