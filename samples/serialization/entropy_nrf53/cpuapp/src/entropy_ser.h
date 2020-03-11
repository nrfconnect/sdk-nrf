/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
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
 * @param[out] buffer Received entropy data.
 * @param[in] length Requested entropy length.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int entropy_remote_get(u8_t *buffer, u16_t length);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ENTROPY_SER_H_ */
