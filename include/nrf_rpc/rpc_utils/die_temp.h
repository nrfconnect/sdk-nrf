/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NRF_RPC_DIE_TEMP_H_
#define NRF_RPC_DIE_TEMP_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup nrf_rpc_utils nRF RPC utility commands
 * @{
 * @defgroup nrf_rpc_die_temp nRF RPC die temperature commands
 * @{
 */

/** @brief Die temperature error codes. */
enum nrf_rpc_die_temp_error {
	/** Success - temperature reading is valid */
	NRF_RPC_DIE_TEMP_SUCCESS = 0,
	/** Temperature sensor device not ready */
	NRF_RPC_DIE_TEMP_ERR_NOT_READY = 1,
	/** Failed to fetch sample from temperature sensor */
	NRF_RPC_DIE_TEMP_ERR_FETCH_FAILED = 2,
	/** Failed to get temperature value from sensor channel */
	NRF_RPC_DIE_TEMP_ERR_CHANNEL_GET_FAILED = 3,
};

/** @brief Get die temperature from the remote server.
 *
 * @param[out] temperature Pointer to store temperature in hundredths of degree Celsius
 *                         (e.g., 2550 = 25.50°C). Value is only valid if function
 *                         returns NRF_RPC_DIE_TEMP_SUCCESS.
 *
 * @return Error code from enum nrf_rpc_die_temp_error on server side, or negative
 *         errno on local RPC error:
 * @retval NRF_RPC_DIE_TEMP_SUCCESS                    Temperature value is valid
 * @retval NRF_RPC_DIE_TEMP_ERR_NOT_READY              Sensor not ready
 * @retval NRF_RPC_DIE_TEMP_ERR_FETCH_FAILED           Sample fetch failed
 * @retval NRF_RPC_DIE_TEMP_ERR_CHANNEL_GET_FAILED     Channel get failed
 * @retval -EINVAL                                     Invalid parameter (temperature is NULL)
 * @retval -EBADMSG                                    RPC communication error
 */
int nrf_rpc_die_temp_get(int32_t *temperature);

/**
 * @}
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* NRF_RPC_DIE_TEMP_H_ */
