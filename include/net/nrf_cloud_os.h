/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NRF_CLOUD_OS_H__
#define NRF_CLOUD_OS_H__

/** @defgroup nrf_cloud_os nRF Cloud OS specifics
 * @{
 */

#include <stdlib.h>
#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Custom OS memory hooks for nRF Cloud library */
struct nrf_cloud_os_mem_hooks {
	void *(*malloc_fn)(size_t size);
	void *(*calloc_fn)(size_t count, size_t size);
	void (*free_fn)(void *ptr);
};

/**
 * @brief Initialize the used OS memory hooks. As a default, nRF Cloud library uses
 *        OS kernel heap memory (in other words, k_ prepending OS functions).
 *        This can be used to override those (for example, to be using
 *        OS system heap, that is standard C calloc/malloc/free).
 *
 * @note This API must be called before using nRF Cloud, even before nrf_cloud_init().
 *
 * @param[in] hooks Used memory alloc/free functions.
 */
void nrf_cloud_os_mem_hooks_init(struct nrf_cloud_os_mem_hooks *hooks);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* NRF_CLOUD_OS_H__ */
