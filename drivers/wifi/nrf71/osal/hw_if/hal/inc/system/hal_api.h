/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file hal_api.h
 *
 * @brief Header containing API declarations for the HAL Layer of the Wi-Fi driver
 * in the system mode of operation.
 */

#ifndef __HAL_API_SYS_H__
#define __HAL_API_SYS_H__

#include "osal_api.h"
#include <nrf71_wifi_ctrl.h>
#include "bal_api.h"
#include "common/hal_structs_common.h"
#include "common/hal_api_common.h"

struct nrf_wifi_hal_dev_ctx *nrf_wifi_sys_hal_dev_add(struct nrf_wifi_hal_priv *hpriv,
						      void *mac_dev_ctx);


void nrf_wifi_sys_hal_lock_rx(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx);

void nrf_wifi_sys_hal_unlock_rx(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx);

/**
 * @brief Send a data command to the RPU.
 *
 * @param hal_ctx Pointer to HAL context.
 * @param cmd_type Type of the data command to send to the RPU.
 * @param data_cmd The data command to be sent to the RPU.
 * @param data_cmd_size Size of the data command to be sent to the RPU.
 * @param desc_id Descriptor ID of the buffer being submitted to RPU.
 * @param pool_id Pool ID to which the buffer being submitted to RPU belongs.
 *
 * This function programs the relevant information about a data command,
 * to the RPU. These buffers are needed by the RPU to receive data and
 * management frames as well as to transmit data frames.
 *
 * @return The status of the operation.
 */
enum nrf_wifi_status nrf_wifi_sys_hal_data_cmd_send(struct nrf_wifi_hal_dev_ctx *hal_ctx,
						    enum NRF_WIFI_HAL_MSG_TYPE cmd_type,
						    void *data_cmd,
						    unsigned int data_cmd_size,
						    unsigned int desc_id,
						    unsigned int pool_id);

/**
 * @brief Map a receive buffer for the Wi-Fi HAL.
 *
 * This function maps a receive buffer to the Wi-Fi HAL device context.
 *
 * @param hal_ctx   Pointer to the Wi-Fi HAL device context.
 * @param buf       The buffer to be mapped.
 * @param buf_len   The length of the buffer.
 * @param pool_id   The pool ID of the buffer.
 * @param buf_id    The buffer ID.
 *
 * @return The status of the operation.
 */
unsigned long nrf_wifi_sys_hal_buf_map_rx(struct nrf_wifi_hal_dev_ctx *hal_ctx,
					  unsigned long buf,
					  unsigned int buf_len,
					  unsigned int pool_id,
					  unsigned int buf_id);

/**
 * @brief Unmap a receive buffer from the Wi-Fi HAL.
 *
 * This function unmaps a receive buffer from the Wi-Fi HAL device context.
 *
 * @param hal_ctx     Pointer to the Wi-Fi HAL device context.
 * @param data_len    The length of the data.
 * @param pool_id     The pool ID of the buffer.
 * @param buf_id      The buffer ID.
 *
 * @return The status of the operation.
 */
unsigned long nrf_wifi_sys_hal_buf_unmap_rx(struct nrf_wifi_hal_dev_ctx *hal_ctx,
					    unsigned int data_len,
					    unsigned int pool_id,
					    unsigned int buf_id);

/**
 * @brief Map a transmit buffer for the Wi-Fi HAL.
 *
 * This function maps a transmit buffer to the Wi-Fi HAL device context.
 *
 * @param hal_ctx     Pointer to the Wi-Fi HAL device context.
 * @param buf         The buffer to be mapped.
 * @param buf_len     The length of the buffer.
 * @param desc_id     The descriptor ID.
 * @param token       The token.
 * @param buf_indx    The buffer index.
 *
 * @return The status of the operation.
 */
unsigned long nrf_wifi_sys_hal_buf_map_tx(struct nrf_wifi_hal_dev_ctx *hal_ctx,
					  unsigned long buf,
					  unsigned int buf_len,
					  unsigned int desc_id,
					  unsigned int token,
					  unsigned int buf_indx);

/**
 * @brief Unmap a transmit buffer from the Wi-Fi HAL.
 *
 * This function unmaps a transmit buffer from the Wi-Fi HAL device context.
 *
 * @param hal_ctx     Pointer to the Wi-Fi HAL device context.
 * @param desc_id     The descriptor ID.
 *
 * @return The status of the operation.
 */
unsigned long nrf_wifi_sys_hal_buf_unmap_tx(struct nrf_wifi_hal_dev_ctx *hal_ctx,
					    unsigned int desc_id);

#ifdef NRF71_SR_COEX_SLEEP_CTRL_GPIO_CTRL
/**
 * @brief Configure Sleep control GPIO control for coexistence.
 *
 * @param hal_dev_ctx     Pointer to the Wi-Fi HAL device context.
 * @param alt_swctrl1_function_bt_coex_status1 SWCTRL1 alternative function.
 * @param invert_bt_coex_grant_output BT coexistence grant polarity selection.
 *
 * This function configures the GPIO control register fields for coex.
 * - Configures the SWCTRL1 alternative function selection
 * - Configures BT coexistence grant polarity selection
 * @retval NRF_WIFI_STATUS_SUCCESS On success
 * @retval NRF_WIFI_STATUS_FAIL On failure to execute command
 */
enum nrf_wifi_status nrf_wifi_hal_coex_config_sleep_ctrl_gpio_ctrl(
			struct nrf_wifi_hal_dev_ctx *hal_dev_ctx,
			unsigned int alt_swctrl1_function_bt_coex_status1,
			unsigned int invert_bt_coex_grant_output);
#endif /* NRF71_SR_COEX_SLEEP_CTRL_GPIO_CTRL */


#ifdef NRF_WIFI_RX_BUFF_PROG_UMAC
/**
 * @brief Get the mapped address of a receive buffer.
 *
 * This function returns the mapped address of a receive buffer.
 *
 * @param hal_dev_ctx Pointer to the Wi-Fi HAL device context.
 * @param pool_id Pool ID of the buffer.
 * @param buf_id Buffer ID of the buffer.
 *
 * @return The mapped address of the buffer.
 */
unsigned long nrf_wifi_hal_get_buf_map_rx(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx,
					  unsigned int pool_id,
					  unsigned int buf_id);
#endif /*NRF_WIFI_RX_BUFF_PROG_UMAC */
#endif /* __HAL_API_SYS_H__ */
