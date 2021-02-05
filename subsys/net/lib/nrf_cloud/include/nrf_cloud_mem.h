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

#include <zephyr.h>

/**@brief Method to allocate zero-initialized memory of size COUNT*SIZE, for
 * internal use in the module.
 *
 * @param[in] COUNT Count of memory blocks requested.
 * @param[in] SIZE Size of memory block.
 *
 * @retval A valid pointer on SUCCESS, else, NULL.
 */
#define nrf_cloud_calloc(COUNT, SIZE) k_calloc(COUNT, SIZE)

/**@brief Method to allocate memory for internal use in the module.
 *
 * @param[in] SIZE Size of memory requested.
 *
 * @retval A valid pointer on SUCCESS, else, NULL.
 */
#define nrf_cloud_malloc(SIZE) k_malloc(SIZE)

/**@brief Method to free any allocated memory for internal use in the module.
 *
 * @param[in] MEMORY Memory to be freed.
 */
#define nrf_cloud_free(MEMORY) k_free(MEMORY)


#ifdef __cplusplus
}
#endif

#endif /* NRF_CLOUD_OS_H_ */
