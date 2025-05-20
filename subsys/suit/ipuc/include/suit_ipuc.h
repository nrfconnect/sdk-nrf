/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief Support for in-place updateable component.
 *
 * SUIT manifest may instruct the platform that component is inactive, by overriding
 * image-size parameter with value 0. Such component may be updated in place.
 */

#ifndef SUIT_IPUC_H__
#define SUIT_IPUC_H__

#include <stdint.h>
#include <zcbor_common.h>
#include <suit_plat_err.h>
#include <suit_metadata.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Get amount of declared in-place updateable components
 *
 * @param[out] count  Amount of declared in-place updateable components
 *
 * @retval SUIT_PLAT_ERR_INVAL	invalid parameter.
 * @retval SUIT_PLAT_ERR_IPC	error while communicating with secure domain.
 * @retval SUIT_PLAT_SUCCESS	operation completed successfully.
 */
suit_plat_err_t suit_ipuc_get_count(size_t *count);

/** @brief Get information about in-place updateable component
 *
 * @param[in] idx		0-based component number.
 * @param[out] component_id	component identifier.
 * @param[out] role		role of manifest which declared component as in-place updateable.
 *
 * @retval SUIT_PLAT_ERR_INVAL		invalid parameter.
 * @retval SUIT_PLAT_ERR_NOT_FOUND	component not declared as in-place updateable.
 * @retval SUIT_PLAT_ERR_IPC		error while communicating with secure domain.
 * @retval SUIT_PLAT_SUCCESS		operation completed successfully.
 */
suit_plat_err_t suit_ipuc_get_info(size_t idx, struct zcbor_string *component_id,
				   suit_manifest_role_t *role);

/** @brief Prepare in-place updateable component for future write operations.
 *
 * @param[in] component_id	component identifier.
 * @param[in] encryption_info	placeholder for future encryption-related parameters.
 * @param[in] compression_info	placeholder for future compression-related parameters.
 *
 * @retval SUIT_PLAT_ERR_INVAL		invalid parameter.
 * @retval SUIT_PLAT_ERR_NOT_FOUND	component not declared as in-place updateable.
 * @retval SUIT_PLAT_ERR_INCORRECT_STATE component in incorrect state, i.e. occupied by other client
 * @retval SUIT_PLAT_ERR_UNSUPPORTED	compression/encryption requested, unsupported component
 * type.
 * @retval SUIT_PLAT_ERR_IO		erase operation has failed
 * @retval SUIT_PLAT_ERR_IPC		error while communicating with secure domain.
 * @retval SUIT_PLAT_SUCCESS		operation completed successfully.
 */
suit_plat_err_t suit_ipuc_write_setup(struct zcbor_string *component_id,
				      struct zcbor_string *encryption_info,
				      struct zcbor_string *compression_info);

/** @brief Write data chunk to in-place updateable component.
 *
 * @param[in] component_id	component identifier.
 * @param[in] offset		Chunk offset in image.
 * @param[in] buffer		Address of memory buffer where image chunk is stored. Must be
 * dcache-line aligned.
 * @param[in] chunk_size	Size of image chunk, in bytes.
 * @param[in] last_chunk	'true' signalizes last write operation. No further write operations
 * will be accepted, till another suit_ipuc_write_setup() is called.
 *
 * @retval SUIT_PLAT_ERR_INVAL		invalid parameter.
 * @retval SUIT_PLAT_ERR_NOT_FOUND	component not declared as in-place updateable.
 * @retval SUIT_PLAT_ERR_INCORRECT_STATE component in incorrect state, i.e. occupied by other client
 * or not prepared for write.
 * @retval SUIT_PLAT_ERR_UNSUPPORTED	unsupported component type.
 * @retval SUIT_PLAT_ERR_IO		seek or write operation has failed.
 * @retval SUIT_PLAT_ERR_IPC		error while communicating with secure domain.
 * @retval SUIT_PLAT_SUCCESS		operation completed successfully.
 */
suit_plat_err_t suit_ipuc_write(struct zcbor_string *component_id, size_t offset, uintptr_t buffer,
				size_t chunk_size, bool last_chunk);

#ifdef __cplusplus
}
#endif

#endif /* SUIT_IPUC_H__ */
