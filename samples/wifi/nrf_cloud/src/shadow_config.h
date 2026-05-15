/* Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _SHADOW_CONFIG_H_
#define _SHADOW_CONFIG_H_

#include <zephyr/kernel.h>
#include <net/nrf_cloud_codec.h>


/**
 * @brief For MQTT, this function should be called when the NRF_CLOUD_EVT_TRANSPORT_CONNECTED
 *        event is received.
 */
void shadow_config_cloud_connected(void);

/**
 * @brief Send the reported device configuration.
 *
 * @return 0 on success, otherwise negative on error.
 */
int shadow_config_reported_send(void);

/**
 * @brief Callback to add the current device configuration to a JSON object.
 *        Pass to @ref nrf_cloud_shadow_config_delta_process.
 */
int shadow_config_add_cfg_data(struct nrf_cloud_obj *const cfg_obj);

/**
 * @brief Callback to process and apply an incoming device configuration from a JSON object.
 *        Pass to @ref nrf_cloud_shadow_config_delta_process.
 *
 * @retval 0       Success, accept the config.
 * @retval -EBADF  Invalid config data, reject the delta.
 * @retval -ENOMSG Expected key not found in the config object.
 */
int shadow_config_process_cfg(struct nrf_cloud_obj *const cfg_obj);

/**
 * @brief Process an incoming transform shadow event.
 *
 * @retval 0	   Success.
 * @retval -EBADF  Invalid config data.
 * @retval -ENOMSG The provided data did not contain JSON or the expected config data.
 * @retval -EINVAL Error; Invalid parameter.
 * @return A negative value indicates an error, the device should send its current config to
 *         the reported section of the shadow.
 */
int shadow_config_transform_process(struct nrf_cloud_obj *const transform_obj);

/**
 * @brief Wrapper to check if the transform shadow data has been received.
 *
 * @retval true If the transform shadow data has been received.
 * @retval false If the transform shadow data has not been received.
 */
bool shadow_transform_received(void);

#endif /* _SHADOW_CONFIG_H_ */
