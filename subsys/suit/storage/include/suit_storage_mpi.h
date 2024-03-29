/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Manifest Provisioning Information storage
 *
 * @details This module implements a SUIT storage submodule, responsible for interpreting
 *          the Manifest Provisioning Information (MPI).
 *          This data should be programmed into the device during the device setup phase as it is
 *          essential for the SUIT modules to identify the manifest(s) to boot.
 *          Additionally, it configures the list of manifest class and vendor ID pairs that will
 *          be accepted to be processed.
 *          For each supported manifest, the MPI defines update, downgrade and signature
 *          verification policy.
 *          The location of the MPI structure inside the NVM memory is provided by the main SUIT
 *          storage module and passed to this module as part of the initialization procedure.
 */

#ifndef SUIT_STORAGE_MPI_H__
#define SUIT_STORAGE_MPI_H__

#include <suit_metadata.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SUIT_MPI_INFO_VERSION 1

#define SUIT_MPI_DOWNGRADE_PREVENTION_DISABLED 1
#define SUIT_MPI_DOWNGRADE_PREVENTION_ENABLED  2

#define SUIT_MPI_INDEPENDENT_UPDATE_DENIED  1
#define SUIT_MPI_INDEPENDENT_UPDATE_ALLOWED 2

#define SUIT_MPI_SIGNATURE_CHECK_DISABLED		    1
#define SUIT_MPI_SIGNATURE_CHECK_ENABLED_ON_UPDATE	    2
#define SUIT_MPI_SIGNATURE_CHECK_ENABLED_ON_UPDATE_AND_BOOT 3

typedef const struct {
	uint8_t version;
	uint8_t downgrade_prevention_policy;
	uint8_t independent_updateability_policy;
	uint8_t signature_verification_policy;

	uint8_t reserved[12];

	uint8_t vendor_id[16];
	uint8_t class_id[16];
} suit_storage_mpi_t;

/**
 * @brief Initialize the SUIT storage module managing Manifest Provisioning Information.
 *
 * @details This function will clear all data configured by @ref suit_storage_mpi_configuration_load
 *
 * @retval SUIT_PLAT_SUCCESS  if successfully initialized.
 */
suit_plat_err_t suit_storage_mpi_init(void);

/**
 * @brief Add a configuration for a manifest with the given role.
 *
 * @param[in]  role  Role of the manifest in the system.
 * @param[in]  addr  NVM address, under which the MPI information structure is stored.
 * @param[in]  size  Size of the NVM area, reserved to store MPI information structure
 *
 * @retval SUIT_PLAT_SUCCESS            on success
 * @retval SUIT_PLAT_ERR_INVAL          invalid parameter, i.e. null pointer
 * @retval SUIT_PLAT_ERR_EXISTS         manifest with given role already configured.
 * @retval SUIT_PLAT_ERR_OUT_OF_BOUNDS  unsupported manifest provisioning structure format.
 * @retval SUIT_PLAT_ERR_SIZE           unable to store more MPI configurations.
 * @retval SUIT_PLAT_ERR_NOT_FOUND      manifest provisioning structure erased (0xFF).
 * @retval SUIT_PLAT_ERR_UNSUPPORTED    MPI values are not supported in the provided role.
 */
suit_plat_err_t suit_storage_mpi_configuration_load(suit_manifest_role_t role, const uint8_t *addr,
						    size_t size);

/**
 * @brief Find a role for a manifest with given class ID.
 *
 * @param[in]   class_id  Manifest class ID.
 * @param[out]  role      Pointer to the role variable.
 *
 * @retval SUIT_PLAT_SUCCESS        on success
 * @retval SUIT_PLAT_ERR_INVAL      invalid parameter, i.e. null pointer
 * @retval SUIT_PLAT_ERR_NOT_FOUND  manifest with given manifest class ID not configured.
 */
suit_plat_err_t suit_storage_mpi_role_get(const suit_manifest_class_id_t *class_id,
					  suit_manifest_role_t *role);

/**
 * @brief Find a manifest class ID with the given role.
 *
 * @param[in]   role      Role of the manifest.
 * @param[out]  class_id  Pointer to the manifest class ID to set.
 *
 * @retval SUIT_PLAT_SUCCESS        on success
 * @retval SUIT_PLAT_ERR_INVAL      invalid parameter, i.e. null pointer
 * @retval SUIT_PLAT_ERR_NOT_FOUND  manifest with given role not configured.
 */
suit_plat_err_t suit_storage_mpi_class_get(suit_manifest_role_t role,
					   const suit_manifest_class_id_t **class_id);

/**
 * @brief Find a manifest provisioning information for a manifest with given class ID.
 *
 * @param[in]   class_id  Manifest class ID.
 * @param[out]  mpi       Pointer to the MPI structure to fill.
 *
 * @retval SUIT_PLAT_SUCCESS        on success
 * @retval SUIT_PLAT_ERR_INVAL      invalid parameter, i.e. null pointer
 * @retval SUIT_PLAT_ERR_NOT_FOUND  manifest with given manifest class ID not configured.
 */
suit_plat_err_t suit_storage_mpi_get(const suit_manifest_class_id_t *class_id,
				     suit_storage_mpi_t **mpi);

/**
 * @brief Gets an array of configured manifest class ids.
 *
 * @param[out]     class_info	Array of manifest class ids and roles
 * @param[in,out]  size		as input - maximal amount of elements an array can hold,
 *				as output - amount of stored elements
 *
 * @retval SUIT_PLAT_SUCCESS    on success
 * @retval SUIT_PLAT_ERR_INVAL  invalid parameter, i.e. null pointer
 * @retval SUIT_PLAT_ERR_SIZE   array too small to store all information
 */
suit_plat_err_t suit_storage_mpi_class_ids_get(suit_manifest_class_info_t *class_info,
					       size_t *size);

/**
 * @brief Convert value representing the downgrade prevention policy stored in MPI
 *        to the corresponding value of the suit_downgrade_prevention_policy_t enum.
 *
 * @param[in] mpi_policy The value stored in the MPI storage
 *
 * @retval Value of the suit_downgrade_prevention_policy_t enum
 */
suit_downgrade_prevention_policy_t suit_mpi_downgrade_prevention_policy_to_metadata(int mpi_policy);

/**
 * @brief Convert value representing the independent updateability policy stored in MPI
 *        to the corresponding value of the suit_independent_updateability_policy_t enum.
 *
 * @param[in] mpi_policy The value stored in the MPI storage
 *
 * @retval Value of the suit_independent_updateability_policy_t enum
 */
suit_independent_updateability_policy_t
suit_mpi_independent_updateability_policy_to_metadata(int mpi_policy);

/**
 * @brief Convert value representing the signature verification policy stored in MPI
 *        to the corresponding value of the suit_signature_verification_policy_t enum.
 *
 * @param[in] mpi_policy The value stored in the MPI storage
 *
 * @retval Value of the suit_signature_verification_policy_t enum
 */
suit_signature_verification_policy_t
suit_mpi_signature_verification_policy_to_metadata(int mpi_policy);

#ifdef __cplusplus
}
#endif

#endif /* SUIT_STORAGE_MPI_H__ */
