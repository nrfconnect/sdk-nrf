/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief Header containing memory management function declarations for the nRF71 driver.
 */

#ifndef __MEM_MGMT_H__
#define __MEM_MGMT_H__

#include <common/status.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#define WORD_SIZE 4
#define WORD_ALIGNED(size) ROUND_UP(size, WORD_SIZE)

/**
 * @brief The type of memory pool to allocate from.
 */
enum nrf_wifi_mem_pool_type {
	/** The control plane memory pool. */
	NRF_WIFI_MEM_POOL_TYPE_CTRL,
	/** The data plane memory pool. */
	NRF_WIFI_MEM_POOL_TYPE_DATA,
};

/**
 * @brief Get handles to the driver control and data heaps.
 *
 * @param ctrl Output pointer for the control heap, or NULL.
 * @param data Output pointer for the data heap, or NULL.
 */
void nrf_wifi_mem_get_heaps(struct k_heap **ctrl, struct k_heap **data);

/**
 * @brief Allocate memory from a driver heap pool.
 *
 * @param pool_type Pool to allocate from.
 * @param size Number of bytes to allocate.
 *
 * @return Pointer to the allocated memory on success, NULL on failure.
 */
void *nrf_wifi_mem_alloc(enum nrf_wifi_mem_pool_type pool_type, size_t size);

/**
 * @brief Allocate zero-initialized memory from a driver heap pool.
 *
 * @param pool_type Pool to allocate from.
 * @param size Number of bytes to allocate.
 *
 * @return Pointer to the allocated memory on success, NULL on failure.
 */
void *nrf_wifi_mem_zalloc(enum nrf_wifi_mem_pool_type pool_type, size_t size);

/**
 * @brief Free memory allocated from a driver heap pool.
 *
 * @param pool_type Pool the memory was allocated from.
 * @param ptr Pointer to the memory to free. No operation if NULL.
 */
void nrf_wifi_mem_free(enum nrf_wifi_mem_pool_type pool_type, void *ptr);

/**
 * @brief Copy memory between two buffers.
 *
 * @param dest Destination buffer.
 * @param src Source buffer.
 * @param size Number of bytes to copy.
 */
void nrf_wifi_mem_cpy(void *dest, const void *src, size_t size);

/**
 * @brief Fill a block of memory with a byte value.
 *
 * @param ptr Pointer to the memory to fill.
 * @param value Byte value to write.
 * @param size Number of bytes to fill.
 */
void nrf_wifi_mem_set(void *ptr, int value, size_t size);

/**
 * @brief Compare two memory regions.
 *
 * @param addr1 First memory region.
 * @param addr2 Second memory region.
 * @param size Number of bytes to compare.
 *
 * @return Zero if the regions are equal, otherwise non-zero.
 */
int nrf_wifi_mem_cmp(const void *addr1, const void *addr2, size_t size);

#endif /* __MEM_MGMT_H__ */
