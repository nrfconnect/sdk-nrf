/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef ENTROPY_SER_H_
#define ENTROPY_SER_H_

#include <zephyr/types.h>

/**
 * @file
 * @defgroup entropy_ser Entropy driver serialization
 * @{
 * @brief Entropy serialization API.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**@brief Function for initializing the remote Entropy driver instance.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int entropy_remote_init(void);

/**@brief Function for getting entropy value from the remote driver.
 *
 * This function demonstrates simples use of a command.
 *
 * @param[out] buffer Received entropy data.
 * @param[in] length Requested entropy length.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int entropy_remote_get(uint8_t *buffer, size_t length);

/**@brief Function for getting entropy value from the remote driver.
 *
 * This function works identical as @ref entropy_remote_get, but also
 * demonstates how to encode parameters and decode results in the same function.
 *
 * @param[out] buffer Received entropy data.
 * @param[in] length Requested entropy length.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int entropy_remote_get_inline(uint8_t *buffer, size_t length);

/**@brief Function for getting entropy value from the remote driver in an
 * asynchronous way.
 *
 * This function demonstates how to use events to accomplish asynchronous API
 * calls.
 *
 * @param[in] length Requested entropy length.
 * @param[in] callback Results will be passed to this callback.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int entropy_remote_get_async(size_t length, void (*callback)(int result,
							     uint8_t *buffer,
							     size_t length));

/**@brief Function for getting entropy value in a callback from the remote
 * driver.
 *
 * This function executes synchronous callback to demonstrate reuse of
 * a thread that is waiting for a response.
 *
 * @param[in] length   Requested entropy length.
 * @param[in] callback Results will be passed to this callback.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int entropy_remote_get_cbk(uint16_t length, void (*callback)(int result,
							  uint8_t *buffer,
							  size_t length));
#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ENTROPY_SER_H_ */
