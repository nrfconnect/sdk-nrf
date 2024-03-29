/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SUIT_SERVICE_H__
#define SUIT_SERVICE_H__

#include <stdlib.h>
#include <stdbool.h>

#include <suit_plat_err.h>
#include <suit_mreg.h>
#include <suit_metadata.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int suit_ssf_err_t;

/** Container for manifest class id and manifest role for ssfipc. */
typedef struct {
	suit_manifest_role_t role;
	suit_uuid_t vendor_id;
	suit_manifest_class_id_t class_id;
	suit_downgrade_prevention_policy_t downgrade_prevention_policy;
	suit_independent_updateability_policy_t independent_updateability_policy;
	suit_signature_verification_policy_t signature_verification_policy;
} suit_ssf_manifest_class_info_t;

#define SUIT_SSF_MISSING_COMPONENT 1 /**< The component in question was not found. */
#define SUIT_SSF_FAIL_CONDITION 2 /**< The test failed (i.e. digest comparison). */
#define SUIT_SSF_ERR_MANIFESTCLASSID 3 /**< Invalid or unsupported manifest class id */

/** @brief Check if the installed component digest matches.
 * @details The purpose of this function is to check if the specific component is installed in
 *          the system.
 *          The function checks the input digest against the value stored inside the manifest(s)
 *          that were used to boot the system.
 *          If the input component boundaries differ from the one that is installed, the function
 *          will return false, even if the overlapping binary contents matches.
 *          This behaviour is mandatory, so an attacker cannot check each byte of the memory
 *          against the dictionary of the known hashes and read back the whole memory.
 *
 * @param[in] component_id  Pointer to the structure with the component ID.
 * @param[in] alg_id        ID of an algorithm that was used to calculate the digest.
 * @param[in] digest        The digest to check.
 *
 * @retval SUIT_PLAT_SUCCESS if successful.
 * @retval SUIT_PLAT_ERR_INVAL if input parameters are invalid.
 * @retval SUIT_PLAT_ERR_UNSUPPORTED if specified digest type is not supported.
 * @retval SUIT_SSF_FAIL_CONDITION if the digest does not match.
 * @retval SUIT_SSF_MISSING_COMPONENT if the digest of the specified memory is unknown.
 * @retval SUIT_PLAT_ERR_IPC in case of SSF protocol error.
 */
suit_ssf_err_t suit_check_installed_component_digest(suit_plat_mreg_t *component_id, int alg_id,
						     suit_plat_mreg_t *digest);

/** @brief Trigger the system update procedure.
 *
 * @details This function is going to reboot the system.
 *          There are no means to pause or abort the upgrade after this function is called.
 *
 * @note The envelope must contain at least authentication wrapper and the SUIT manifest.
 *
 * @param[in]  regions  List of update candidate memory regions (envelope, caches).
 *                      By convention, the first region holds the SUIT envelope.
 * @param[in]  len      Length of the memory regions list.
 *
 * @retval SUIT_PLAT_SUCCESS if successful.
 * @retval SUIT_PLAT_ERR_INVAL if input parameters are invalid.
 * @retval SUIT_PLAT_ERR_NOMEM if update candidate has too many regions.
 * @retval SUIT_PLAT_ERR_HW_NOT_READY if device managing SUIT storage is not initialized.
 * @retval SUIT_PLAT_ERR_IO if failed to save the update candidate info.
 * @retval SUIT_PLAT_ERR_IPC in case of SSF protocol error.
 */
suit_ssf_err_t suit_trigger_update(suit_plat_mreg_t *regions, size_t len);

/** @brief Read the information about the manifest, that was used to boot the system.
 *
 * @details In the SUIT-based system, the firmware is identified by its digest.
 *          There is no semantic version attached to the firmware.
 *          This function allows to inform an application about the current revision and digest
 *          of the manifest.
 *          This data may be forwarded to the tools that would like to read the device state.
 *
 * @param[in] manifest_class_id  Manifest class ID.
 * @param[out] seq_num  Pointer to the memory to store the current root manifest sequence number
 *                      value.
 * @param[out] version  Manifest semantic version.
 * @param[out] status   Digest verification status
 * @param[out] alg_id   ID of an algorithm that was used to calculate the digest.
 * @param[out] digest   Pointer to the memory to store the digest of the current root manifest
 *                      value.
 *
 * @retval SUIT_PLAT_SUCCESS if successful.
 * @retval SUIT_PLAT_ERR_INVAL if input parameters are invalid.
 * @retval SUIT_PLAT_ERR_SIZE if the manifest version output buffer is too small.
 * @retval SUIT_PLAT_ERR_NOMEM if the manifest digest output buffer is too small.
 * @retval SUIT_PLAT_ERR_HW_NOT_READY if device manging SUIT storage is not initialized.
 * @retval SUIT_PLAT_ERR_BUSY if SUIT decoder is currently busy.
 * @retval SUIT_PLAT_ERR_NOT_FOUND if SUIT storage does not contain a valid manifest with
 *                                 the specified class ID.
 * @retval SUIT_PLAT_ERR_IPC in case of SSF protocol error.
 */
suit_ssf_err_t suit_get_installed_manifest_info(suit_manifest_class_id_t *manifest_class_id,
						unsigned int *seq_num, suit_semver_raw_t *version,
						suit_digest_status_t *status, int *alg_id,
						suit_plat_mreg_t *digest);

/** @brief Check validity and read information about the update candidate.
 *
 * @details In the SUIT-based system, the firmware is identified by its digest.
 *          This function allows to inform an application about the revision and digest
 *          of the update candidate.
 *
 * @param[out] manifest_class_id  Manifest class ID.
 * @param[out] seq_num  Pointer to the memory to store the current root manifest sequence number
 *                      value.
 * @param[out] version  Manifest semantic version.
 * @param[out] alg_id   ID of an algorithm that was used to calculate the digest.
 * @param[out] digest   Pointer to the memory to store the digest of the current root manifest
 *                      value.
 *
 * @retval SUIT_PLAT_SUCCESS if successful.
 * @retval SUIT_PLAT_ERR_INVAL if input parameters are invalid.
 * @retval SUIT_PLAT_ERR_SIZE if the manifest version output buffer is too small.
 * @retval SUIT_PLAT_ERR_NOMEM if the manifest digest or class ID is invalid.
 * @retval SUIT_PLAT_ERR_HW_NOT_READY if device manging SUIT storage is not initialized.
 * @retval SUIT_PLAT_ERR_BUSY if SUIT decoder is currently busy.
 * @retval SUIT_PLAT_ERR_NOT_FOUND if SUIT storage does not contain information about the update
 *                                 candidate.
 * @retval SUIT_PLAT_ERR_IPC in case of SSF protocol error.
 */
suit_ssf_err_t suit_get_install_candidate_info(suit_manifest_class_id_t *manifest_class_id,
					       unsigned int *seq_num, suit_semver_raw_t *version,
					       int *alg_id, suit_plat_mreg_t *digest);

/** @brief Get an array of supported manifest roles.
 *
 * @param[out]     roles Array of structures containing manifest roles.
 * @param[in,out]  size	 As input - maximal amount of elements an array can hold,
 *                       as output - amount of stored elements.
 *
 * @retval SUIT_PLAT_SUCCESS if successful.
 * @retval SUIT_PLAT_ERR_INVAL if input parameters are invalid.
 * @retval SUIT_PLAT_ERR_SIZE if the input array is too small to store all information.
 * @retval SUIT_PLAT_ERR_IPC in case of SSF protocol error.
 */
suit_ssf_err_t suit_get_supported_manifest_roles(suit_manifest_role_t *roles, size_t *size);

/** @brief Get class info (class id, vendor ID, role) for a given, supported manifest role.
 *         Prior to calling this function, @ref suit_get_supported_manifest_roles should be
 *         called to get the supported roles.
 *
 * @param[in]   role       Manifest role.
 * @param[out]  class_info Manifest class info (class id, role).
 *
 * @retval SUIT_PLAT_SUCCESS if successful.
 * @retval SUIT_PLAT_ERR_INVAL if input parameters are invalid.
 * @retval SUIT_PLAT_ERR_CBOR_DECODING if the data received from SDFW is malformed.
 * @retval SUIT_PLAT_ERR_NOT_FOUND if the given role is not supported.
 * @retval SUIT_PLAT_ERR_IPC in case of SSF protocol error.
 */
suit_ssf_err_t suit_get_supported_manifest_info(suit_manifest_role_t role,
						suit_ssf_manifest_class_info_t *class_info);

#ifdef __cplusplus
}
#endif

#endif /* SUIT_SERVICE_H__ */
