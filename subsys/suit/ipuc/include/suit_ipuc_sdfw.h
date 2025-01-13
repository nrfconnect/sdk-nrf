/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief Support for in-place updateable component.
 *
 * SUIT manifest may instruct the platform that component is inactive, by overriding
 * image-size parameter with value 0. Such component may be updated in place,
 * also memory associated to such component may be utilized for other purposes,
 * like updating SDFW or SDFW_Recovery from external flash.
 */

#ifndef SUIT_IPUC_SDFW_H__
#define SUIT_IPUC_SDFW_H__

#include <suit_types.h>
#include <suit_plat_err.h>
#include <suit_metadata.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Declare component as updateable.
 *
 * @param[in] handle  Handle to the updateable component.
 *
 * @retval SUIT_PLAT_ERR_INVAL	component handle is invalid or component is not supported.
 * @retval SUIT_PLAT_ERR_NOMEM	the maximum number of updateable components has been reached.
 * @retval SUIT_PLAT_SUCCESS	updateable component was successfully declared.
 */
suit_plat_err_t suit_ipuc_sdfw_declare(suit_component_t handle, suit_manifest_role_t role);

/** @brief Revoke declaration of updateable component.
 *
 * @param[in] handle  Handle to the component that is no longer updateable.
 *
 * @retval SUIT_PLAT_ERR_NOT_FOUND	component is not declared as IPUC
 * @retval SUIT_PLAT_SUCCESS		declaration of updateable component was
 * successfully revoked.
 */
suit_plat_err_t suit_ipuc_sdfw_revoke(suit_component_t handle);

/** @brief Get amount of declared in-place updateable components
 *
 * @param[out] count  Amount of declared in-place updateable components
 *
 * @retval SUIT_PLAT_ERR_INVAL	invalid parameter.
 * @retval SUIT_PLAT_ERR_IPC	error while communicating with secure domain.
 * @retval SUIT_PLAT_SUCCESS	operation completed successfully.
 */
suit_plat_err_t suit_ipuc_sdfw_get_count(size_t *count);

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
suit_plat_err_t suit_ipuc_sdfw_get_info(size_t idx, struct zcbor_string *component_id,
					suit_manifest_role_t *role);

/** @brief Prepare in-place updateable component for future write operations in secure domain.
 *
 * @param[in] ipc_client_id	client identifier for future write operations.
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
 * @retval SUIT_PLAT_SUCCESS		operation completed successfully.
 */
suit_plat_err_t suit_ipuc_sdfw_write_setup(int ipc_client_id, struct zcbor_string *component_id,
					   struct zcbor_string *encryption_info,
					   struct zcbor_string *compression_info);

/** @brief Write data chunk to in-place updateable component in secure domain.
 *
 * @param[in] ipc_client_id	client identifier. Must be the same value as provided in
 * suit_ipuc_sdfw_write_setup()
 * @param[in] component_id	component identifier.
 * @param[in] offset		Chunk offset in image.
 * @param[in] buffer		Address of memory buffer where image chunk is stored. Must be
 * dcache-line aligned. Dcache must be flushed/invalidated.
 * @param[in] chunk_size	Size of image chunk, in bytes.
 * @param[in] last_chunk	'true' signalizes last write operation. No further write operations
 * will be accepted, till another suit_ipuc_sdfw_write_setup() is called.
 *
 * @retval SUIT_PLAT_ERR_INVAL		invalid parameter.
 * @retval SUIT_PLAT_ERR_NOT_FOUND	component not declared as in-place updateable.
 * @retval SUIT_PLAT_ERR_INCORRECT_STATE component in incorrect state, i.e. occupied by other client
 * or not prepared for write.
 * @retval SUIT_PLAT_ERR_UNSUPPORTED	unsupported component type.
 * @retval SUIT_PLAT_ERR_IO		seek or write operation has failed.
 * @retval SUIT_PLAT_SUCCESS		operation completed successfully.
 */
suit_plat_err_t suit_ipuc_sdfw_write(int ipc_client_id, struct zcbor_string *component_id,
				     size_t offset, uintptr_t buffer, size_t chunk_size,
				     bool last_chunk);

/**
 * @brief Check if the digest calculated over stored data matches provided one.
 *
 * @param[in] component_id	component identifier.
 * @param[in] alg_id		algorithm used to calculate the digest.
 * @param[in] digest		CBOR string containing the digest to compare against.
 *
 * @retval SUIT_PLAT_ERR_INVAL		invalid parameter or digest does not match.
 * @retval SUIT_PLAT_ERR_NOT_FOUND	component not declared as in-place updateable.
 * @retval SUIT_PLAT_ERR_INCORRECT_STATE component in incorrect state, i.e. write operations in
 * progress. Digest may be calculated on completely transferred image.
 * @retval SUIT_PLAT_ERR_UNSUPPORTED	unsupported component type.
 * @retval SUIT_PLAT_ERR_IO		digest calculation operation has failed.
 * @retval SUIT_PLAT_SUCCESS		operation completed successfully.
 */
suit_plat_err_t suit_ipuc_sdfw_digest_compare(struct zcbor_string *component_id,
					      enum suit_cose_alg alg_id,
					      struct zcbor_string *digest);

/** @brief Find an updateable component which is capable to store SDFW update candidate.
 *
 * @details Since Secure ROM imposes requirements related to SDFW candidate location, returned
 * pointer does not necessarily points to the begin of updateable component.
 *
 * @param[in] required_size Required size of area capable to store SDFW candidate.
 *
 * @returns Pointer to area capable to store SDFW candidate, 0 otherwise.
 */
uintptr_t suit_ipuc_sdfw_mirror_addr(size_t required_size);

#ifdef __cplusplus
}
#endif

#endif /* SUIT_IPUC_SDFW_H__ */
