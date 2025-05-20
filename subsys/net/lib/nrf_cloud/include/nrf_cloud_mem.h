/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NRF_CLOUD_MEM_H_
#define NRF_CLOUD_MEM_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/kernel.h>
#include <net/nrf_cloud_os.h>

/** @brief Allocate zero-initialized memory of size count*size for
 * internal use in the module.
 *
 * @param[in] count Count of memory blocks requested.
 * @param[in] size Size of memory block.
 *
 * @retval A valid pointer on SUCCESS, else, NULL.
 */
void *nrf_cloud_calloc(size_t count, size_t size);

/** @brief Allocate memory for internal use in the module.
 *
 * @param[in] size Size of memory requested.
 *
 * @retval A valid pointer on SUCCESS, else, NULL.
 */
void *nrf_cloud_malloc(size_t size);

/** @brief Free any allocated memory for internal use in the module.
 *
 * @param[in] memory Memory to be freed.
 */
void nrf_cloud_free(void *memory);

#ifdef __cplusplus
}
#endif

#endif /* NRF_CLOUD_MEM_H_ */
