/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief Support for in-place updateable componenet.
 *
 * SUIT manifest may instruct the platform that component is inactive, by overriding
 * image-size parameter with value 0. Such component may be updated in place,
 * also memory associated to such component may be utilized for other purposes,
 * like updating SDFW or SDFW_Recovery from external flash.
 */

#ifndef SUIT_PLAT_IPUC_H__
#define SUIT_PLAT_IPUC_H__

#include <suit_types.h>
#include <suit_plat_err.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Declare component as updateable.
 *
 * @param[in] handle  Handle to the updateable component.
 *
 * @retval SUIT_PLAT_ERR_INVAL  component handle is invalid or component is not supported.
 * @retval SUIT_PLAT_ERR_NOMEM  the maximum number of updateable components has been reached.
 * @retval SUIT_PLAT_SUCCESS updateable component was successfully declared.
 */
suit_plat_err_t suit_plat_ipuc_declare(suit_component_t handle);

/** @brief Revoke declaration of updateable component.
 *
 * @param[in] handle  Handle to the component that is no longer updateable.
 *
 * @retval SUIT_PLAT_ERR_NOT_FOUND component is not declared as IPUC
 * @retval SUIT_PLAT_SUCCESS declaration of updateable component was successfully revoked.
 */
suit_plat_err_t suit_plat_ipuc_revoke(suit_component_t handle);

/** @brief Writes a chunk of data to IPUC
 *
 * @note offset parameter equal to 0 resets the data transfer. That allows to restart a image
 * transfer, or even to transfer a new image.
 *
 * @param[in] handle		Handle to the updateable component.
 * @param[in] offset		Offset to write data chunk. Discontinuous writes are not supported.
 * @param[in] buffer		Pointer to source data chunk.
 * @param[in] chunk_size	Size of data chunk.
 * @param[in] last_chunk	True if this is the last chunk of image.
 *
 * @retval SUIT_PLAT_ERR_NOT_FOUND component is not declared as IPUC
 * @retval SUIT_PLAT_ERR_INCORRECT_STATE discontinued write attempt
 * @retval SUIT_PLAT_ERR_IO sink not found, sink does not support seek operation, seek operation
 * failed, streaming to sink failed
 * @retval SUIT_PLAT_SUCCESS opration completed successfully
 *
 */
suit_plat_err_t suit_plat_ipuc_write(suit_component_t handle, size_t offset, uintptr_t buffer,
				     size_t chunk_size, bool last_chunk);

/** @brief Gets size of image written to IPUC
 *
 * @param[in] handle		Handle to the updateable component.
 * @param[out] size		Size of image stored in IPUC
 *
 * @retval SUIT_PLAT_ERR_INVAL invalid parameter
 * @retval SUIT_PLAT_ERR_NOT_FOUND component is not declared as IPUC
 * @retval SUIT_PLAT_ERR_INCORRECT_STATE writing to IPUC is still in progress
 * @retval SUIT_PLAT_SUCCESS opration completed successfully
 */
suit_plat_err_t suit_plat_ipuc_get_stored_img_size(suit_component_t handle, size_t *size);

/** @brief Find an updateable component which is capable to store SDFW update candidate.
 *
 * @details Since Secure ROM imposes requirements related to SDFW candidate location, returned
 * pointer does not necessarily points to the begin of updateable component.
 *
 * @param[in] required_size Required size of area capable to store SDFW candidate.
 *
 * @returns Pointer to area capable to store SDFW candidate, 0 otherwise.
 */
uintptr_t suit_plat_ipuc_sdfw_mirror_addr(size_t required_size);

#ifdef __cplusplus
}
#endif

#endif /* SUIT_PLAT_IPUC_H__ */
