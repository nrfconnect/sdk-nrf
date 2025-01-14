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
 * @brief Process an incoming delta event.
 *
 * @retval 0	   Success, accept the delta.
 * @retval -EBADF  Invalid config data, reject the delta.
 * @retval -ENOMSG The provided data did not contain JSON or a config section.
 * @retval -EAGAIN Ignore delta event until the accepted shadow is received. MQTT only.
 * @retval -EINVAL Error; Invalid parameter.
 * @retval -ENOMEM Error; out of memory.
 */
int shadow_config_delta_process(struct nrf_cloud_obj *const delta_obj);

/**
 * @brief Process an incoming accepted shadow event.
 *
 * @retval 0	   Success.
 * @retval -EBADF  Invalid config data.
 * @retval -ENOMSG The provided data did not contain JSON or the expected config data.
 * @retval -EINVAL Error; Invalid parameter.
 * @return A negative value indicates an error, the device should send its current config to
 *         the reported section of the shadow.
 */
int shadow_config_accepted_process(struct nrf_cloud_obj *const accepted_obj);

#endif /* _SHADOW_CONFIG_H_ */
