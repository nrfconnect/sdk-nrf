/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SUIT_MEMPTR_STORAGE_H__
#define SUIT_MEMPTR_STORAGE_H__

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <suit_plat_err.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int suit_memptr_storage_err_t;

/**< Invalid record */
#define SUIT_MEMPTR_STORAGE_ERR_INVALID_RECORD	   1
/**< Write to unallocated record */
#define SUIT_MEMPTR_STORAGE_ERR_UNALLOCATED_RECORD 2

typedef void *memptr_storage_handle_t;

/**
 * @brief Get the memptr storage object
 *
 * @param handle Memptr storage handle
 *
 * @retval SUIT_PLAT_SUCCESS     on success
 * @retval SUIT_PLAT_ERR_INVAL   invalid parameter, i.e. null pointer
 * @retval SUIT_PLAT_ERR_NOMEM   no free records were found
 */
suit_memptr_storage_err_t suit_memptr_storage_get(memptr_storage_handle_t *handle);

/**
 * @brief Release storage record
 *
 * @param handle Memptr storage handle
 *
 * @retval SUIT_PLAT_SUCCESS    on success
 * @retval SUIT_PLAT_ERR_INVAL  invalid parameter, i.e. null pointer
 */
suit_memptr_storage_err_t suit_memptr_storage_release(memptr_storage_handle_t handle);

/**
 * @brief
 *
 * @param handle Memptr storage handle
 * @param payload_ptr Payload data pointer to be stored
 * @param payload_size Payload data size
 *
 * @retval SUIT_PLAT_SUCCESS    on success
 * @retval SUIT_PLAT_ERR_INVAL  invalid parameter, i.e. null pointer
 * @retval SUIT_MEMPTR_STORAGE_ERR_UNALLOCATED_RECORD Attempt to write to unallocated record.
 */
suit_memptr_storage_err_t suit_memptr_storage_ptr_store(memptr_storage_handle_t handle,
							const uint8_t *payload_ptr,
							size_t payload_size);

/**
 * @brief Get the memptr ptr object
 *
 * @param handle Memptr storage handle
 * @param payload_ptr Pointer to payload pointer
 * @param payload_size Pointer to payload size
 *
 * @retval SUIT_PLAT_SUCCESS    on success
 * @retval SUIT_PLAT_ERR_INVAL  invalid parameter, i.e. null pointer
 * @retval SUIT_MEMPTR_STORAGE_ERR_UNALLOCATED_RECORD Attempt to read from unallocated record.
 */
suit_memptr_storage_err_t suit_memptr_storage_ptr_get(memptr_storage_handle_t handle,
						      const uint8_t **payload_ptr,
						      size_t *payload_size);

#ifdef __cplusplus
}
#endif

#endif /* SUIT_MEMPTR_STORAGE_H__ */
