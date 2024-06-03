/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <suit_mci.h>
#include <drivers/nrfx_common.h>
#include <suit_storage_mpi.h>
#include <suit_execution_mode.h>
#include <sdfw/lcs.h>
#include <zephyr/logging/log.h>
#include <sdfw/arbiter.h>

#define MANIFEST_PUBKEY_NRF_TOP_GEN0	 0x4000BB00
#define MANIFEST_PUBKEY_SYSCTRL_GEN0	 0x40082100
#define MANIFEST_PUBKEY_OEM_ROOT_GEN0	 0x4000AA00
#define MANIFEST_PUBKEY_APPLICATION_GEN0 0x40022100
#define MANIFEST_PUBKEY_RADIO_GEN0	 0x40032100
#define MANIFEST_PUBKEY_GEN_RANGE	 2

LOG_MODULE_REGISTER(suit_mci_nrf54h20, CONFIG_SUIT_LOG_LEVEL);

mci_err_t suit_mci_supported_manifest_class_ids_get(suit_manifest_class_info_t *class_info,
						    size_t *size)
{
	if (class_info == NULL || size == NULL) {
		return SUIT_PLAT_ERR_INVAL;
	}

	return suit_storage_mpi_class_ids_get(class_info, size);
}

mci_err_t suit_mci_invoke_order_get(const suit_manifest_class_id_t **class_id, size_t *size)
{
	if (class_id == NULL || size == NULL) {
		return SUIT_PLAT_ERR_INVAL;
	}
	size_t output_max_size = *size;
	size_t output_size = 2; /* Current number of elements on invocation order list */

	if (output_size > output_max_size) {
		return SUIT_PLAT_ERR_SIZE;
	}

	suit_execution_mode_t execution_mode = suit_execution_mode_get();

	switch (execution_mode) {
	case EXECUTION_MODE_INVOKE:
		if (suit_storage_mpi_class_get(SUIT_MANIFEST_SEC_TOP, &class_id[0]) !=
		    SUIT_PLAT_SUCCESS) {
			return SUIT_PLAT_ERR_NOT_FOUND;
		}

		if (suit_storage_mpi_class_get(SUIT_MANIFEST_APP_ROOT, &class_id[1]) !=
		    SUIT_PLAT_SUCCESS) {
			return SUIT_PLAT_ERR_NOT_FOUND;
		}
		break;

	case EXECUTION_MODE_INVOKE_RECOVERY:
		if (suit_storage_mpi_class_get(SUIT_MANIFEST_SEC_TOP, &class_id[0]) !=
		    SUIT_PLAT_SUCCESS) {
			return SUIT_PLAT_ERR_NOT_FOUND;
		}

		suit_plat_err_t ret =
			suit_storage_mpi_class_get(SUIT_MANIFEST_APP_RECOVERY, &class_id[1]);

		/* If recovery class ID is not configured, use application root manifest
		 * as the recovery manifest.
		 */
		if (ret == SUIT_PLAT_ERR_NOT_FOUND) {
			ret = suit_storage_mpi_class_get(SUIT_MANIFEST_APP_ROOT, &class_id[1]);
		}

		if (ret != SUIT_PLAT_SUCCESS) {
			return SUIT_PLAT_ERR_NOT_FOUND;
		}

		break;

	default:
		return SUIT_PLAT_ERR_INCORRECT_STATE;
	}

	*size = output_size;
	return SUIT_PLAT_SUCCESS;
}

mci_err_t suit_mci_downgrade_prevention_policy_get(const suit_manifest_class_id_t *class_id,
						   suit_downgrade_prevention_policy_t *policy)
{
	suit_storage_mpi_t *mpi = NULL;

	if (class_id == NULL || policy == NULL) {
		return SUIT_PLAT_ERR_INVAL;
	}

	if (suit_storage_mpi_get(class_id, &mpi) != SUIT_PLAT_SUCCESS) {
		return MCI_ERR_MANIFESTCLASSID;
	}

	*policy =
		suit_mpi_downgrade_prevention_policy_to_metadata(mpi->downgrade_prevention_policy);
	if (*policy == SUIT_DOWNGRADE_PREVENTION_UNKNOWN) {
		return SUIT_PLAT_ERR_OUT_OF_BOUNDS;
	}

	return SUIT_PLAT_SUCCESS;
}

mci_err_t suit_mci_independent_update_policy_get(const suit_manifest_class_id_t *class_id,
						 suit_independent_updateability_policy_t *policy)
{
	suit_storage_mpi_t *mpi = NULL;
	suit_manifest_role_t role = SUIT_MANIFEST_UNKNOWN;

	if (class_id == NULL || policy == NULL) {
		return SUIT_PLAT_ERR_INVAL;
	}

	if (suit_storage_mpi_get(class_id, &mpi) != SUIT_PLAT_SUCCESS) {
		return MCI_ERR_MANIFESTCLASSID;
	}

	if (suit_storage_mpi_role_get(class_id, &role) != SUIT_PLAT_SUCCESS) {
		return MCI_ERR_MANIFESTCLASSID;
	}

	*policy = suit_mpi_independent_updateability_policy_to_metadata(
		mpi->independent_updateability_policy);
	if (*policy == SUIT_INDEPENDENT_UPDATE_UNKNOWN) {
		return SUIT_PLAT_ERR_OUT_OF_BOUNDS;
	}

	/* Override independent updateability policy in recovery scenarios:
	 * If the update candidate was delivered by the recovery firmware,
	 * it must not update the recovery firmware.
	 * Invoke modes included, so the recovery firmware may reject the incorrect
	 * update candidate before resetting the SoC.
	 */
	switch (suit_execution_mode_get()) {
	case EXECUTION_MODE_INVOKE_RECOVERY:
	case EXECUTION_MODE_INSTALL_RECOVERY:
	case EXECUTION_MODE_POST_INVOKE_RECOVERY:
		if ((role == SUIT_MANIFEST_APP_RECOVERY) || (role == SUIT_MANIFEST_RAD_RECOVERY)) {
			*policy = SUIT_INDEPENDENT_UPDATE_DENIED;
		}
		break;

	case EXECUTION_MODE_FAIL_INSTALL_NORDIC_TOP:
		/* In this state, only the Nordic top manifest is accepted as an update candidate.*/
		if (role != SUIT_MANIFEST_SEC_TOP) {
			*policy = SUIT_INDEPENDENT_UPDATE_DENIED;
		}
		break;

	default:
		break;
	}

	return SUIT_PLAT_SUCCESS;
}

mci_err_t suit_mci_manifest_class_id_validate(const suit_manifest_class_id_t *class_id)
{
	if (class_id == NULL) {
		return SUIT_PLAT_ERR_INVAL;
	}

	suit_manifest_role_t role = SUIT_MANIFEST_UNKNOWN;
	suit_plat_err_t ret = suit_storage_mpi_role_get(class_id, &role);

	if (ret != SUIT_PLAT_SUCCESS) {
		return MCI_ERR_MANIFESTCLASSID;
	}

	return SUIT_PLAT_SUCCESS;
}

static bool skip_validation(suit_manifest_role_t role)
{
#ifdef CONFIG_SDFW_LCS
	/* Read the domain-specific LCS value. */
	enum lcs current_lcs = LCS_DISCARDED;

	switch (role) {
	case SUIT_MANIFEST_SEC_TOP:
	case SUIT_MANIFEST_SEC_SDFW:
	case SUIT_MANIFEST_SEC_SYSCTRL:
		current_lcs = lcs_get(LCS_DOMAIN_ID_SECURE);

		/* TODO:
		 * NCSDK-26255
		 * Once keys are provisioned, validation skip for Secure domain should be disabled.
		 * return false;
		 */

		LOG_WRN("SUIT: Validation skip is enabled for Secure domain.");
		break;

	case SUIT_MANIFEST_APP_ROOT:
	case SUIT_MANIFEST_APP_RECOVERY:
	case SUIT_MANIFEST_APP_LOCAL_1:
	case SUIT_MANIFEST_APP_LOCAL_2:
	case SUIT_MANIFEST_APP_LOCAL_3:
		current_lcs = lcs_get(LCS_DOMAIN_ID_APPLICATION);
		break;

	case SUIT_MANIFEST_RAD_RECOVERY:
	case SUIT_MANIFEST_RAD_LOCAL_1:
	case SUIT_MANIFEST_RAD_LOCAL_2:
		current_lcs = lcs_get(LCS_DOMAIN_ID_RADIOCORE);
		break;

	default:
		return false;
	}

	if ((current_lcs == LCS_ROT) || (current_lcs == LCS_ROT_DEBUG) ||
	    (current_lcs == LCS_EMPTY)) {
		return true;
	}
#endif /* CONFIG_SDFW_LCS */

	return false;
}

mci_err_t suit_mci_signing_key_id_validate(const suit_manifest_class_id_t *class_id,
					   uint32_t key_id)
{
	suit_manifest_role_t role = SUIT_MANIFEST_UNKNOWN;

	if (class_id == NULL) {
		return SUIT_PLAT_ERR_INVAL;
	}

	if (suit_storage_mpi_role_get(class_id, &role) != SUIT_PLAT_SUCCESS) {
		return MCI_ERR_MANIFESTCLASSID;
	}

	if (key_id == 0) {
		suit_storage_mpi_t *mpi = NULL;

		if (skip_validation(role)) {
			return SUIT_PLAT_SUCCESS;
		}

		if (suit_storage_mpi_get(class_id, &mpi) != SUIT_PLAT_SUCCESS) {
			return MCI_ERR_MANIFESTCLASSID;
		}

		if (mpi->signature_verification_policy == SUIT_MPI_SIGNATURE_CHECK_DISABLED) {
			return SUIT_PLAT_SUCCESS;
		} else if ((mpi->signature_verification_policy ==
			    SUIT_MPI_SIGNATURE_CHECK_ENABLED_ON_UPDATE) &&
			   (suit_execution_mode_get() == EXECUTION_MODE_INVOKE)) {
			/* By allowing key_id == 0 in the invoke path, the platform will verify
			 * the signature only during updates.
			 */
			return SUIT_PLAT_SUCCESS;
		}

		return MCI_ERR_WRONGKEYID;
	}

	switch (role) {
	case SUIT_MANIFEST_SEC_TOP:
	case SUIT_MANIFEST_SEC_SDFW:
		if (key_id >= MANIFEST_PUBKEY_NRF_TOP_GEN0 &&
		    key_id <= MANIFEST_PUBKEY_NRF_TOP_GEN0 + MANIFEST_PUBKEY_GEN_RANGE) {
			return SUIT_PLAT_SUCCESS;
		}
		break;

	case SUIT_MANIFEST_SEC_SYSCTRL:
		if (key_id >= MANIFEST_PUBKEY_SYSCTRL_GEN0 &&
		    key_id <= MANIFEST_PUBKEY_SYSCTRL_GEN0 + MANIFEST_PUBKEY_GEN_RANGE) {
			return SUIT_PLAT_SUCCESS;
		}
		break;

	case SUIT_MANIFEST_APP_ROOT:
		if (key_id >= MANIFEST_PUBKEY_OEM_ROOT_GEN0 &&
		    key_id <= MANIFEST_PUBKEY_OEM_ROOT_GEN0 + MANIFEST_PUBKEY_GEN_RANGE) {
			return SUIT_PLAT_SUCCESS;
		}
		break;

	case SUIT_MANIFEST_APP_RECOVERY:
	case SUIT_MANIFEST_APP_LOCAL_1:
	case SUIT_MANIFEST_APP_LOCAL_2:
	case SUIT_MANIFEST_APP_LOCAL_3:
		if (key_id >= MANIFEST_PUBKEY_APPLICATION_GEN0 &&
		    key_id <= MANIFEST_PUBKEY_APPLICATION_GEN0 + MANIFEST_PUBKEY_GEN_RANGE) {
			return SUIT_PLAT_SUCCESS;
		}
		break;

	case SUIT_MANIFEST_RAD_RECOVERY:
	case SUIT_MANIFEST_RAD_LOCAL_1:
	case SUIT_MANIFEST_RAD_LOCAL_2:
		if (key_id >= MANIFEST_PUBKEY_RADIO_GEN0 &&
		    key_id <= MANIFEST_PUBKEY_RADIO_GEN0 + MANIFEST_PUBKEY_GEN_RANGE) {
			return SUIT_PLAT_SUCCESS;
		}
		break;

	default:
		break;
	}

	return MCI_ERR_WRONGKEYID;
}

mci_err_t suit_mci_processor_start_rights_validate(const suit_manifest_class_id_t *class_id,
						   int processor_id)
{
	if (class_id == NULL) {
		return SUIT_PLAT_ERR_INVAL;
	}

	suit_manifest_role_t role = SUIT_MANIFEST_UNKNOWN;

	if (suit_storage_mpi_role_get(class_id, &role) != SUIT_PLAT_SUCCESS) {
		return MCI_ERR_MANIFESTCLASSID;
	}

	switch (role) {
	case SUIT_MANIFEST_UNKNOWN:
		return MCI_ERR_MANIFESTCLASSID;

	case SUIT_MANIFEST_SEC_TOP:
	case SUIT_MANIFEST_APP_ROOT:
	case SUIT_MANIFEST_SEC_SDFW:
		break;

	case SUIT_MANIFEST_SEC_SYSCTRL:
		/* Sys manifest */
		if (processor_id == NRF_PROCESSOR_SYSCTRL) {
			/* SysCtrl */
			return SUIT_PLAT_SUCCESS;
		}
		break;

	case SUIT_MANIFEST_APP_RECOVERY:
	case SUIT_MANIFEST_APP_LOCAL_1:
	case SUIT_MANIFEST_APP_LOCAL_2:
	case SUIT_MANIFEST_APP_LOCAL_3:
		/* App manifest */
		if (processor_id == NRF_PROCESSOR_APPLICATION) {
			/* Appcore */
			return SUIT_PLAT_SUCCESS;
		}
		break;

	case SUIT_MANIFEST_RAD_RECOVERY:
	case SUIT_MANIFEST_RAD_LOCAL_1:
	case SUIT_MANIFEST_RAD_LOCAL_2:
		/* Rad manifest */
		if (processor_id == NRF_PROCESSOR_RADIOCORE) {
			/* Radiocore */
			return SUIT_PLAT_SUCCESS;
		}
		break;

	default:
		break;
	}

	return MCI_ERR_NOACCESS;
}

mci_err_t suit_mci_memory_access_rights_validate(const suit_manifest_class_id_t *class_id,
						 void *address, size_t mem_size)
{
	if (class_id == NULL || address == NULL || mem_size == 0) {
		return SUIT_PLAT_ERR_INVAL;
	}

	suit_manifest_role_t role = SUIT_MANIFEST_UNKNOWN;

	if (suit_storage_mpi_role_get(class_id, &role) != SUIT_PLAT_SUCCESS) {
		return MCI_ERR_MANIFESTCLASSID;
	}

	switch (role) {
	case SUIT_MANIFEST_UNKNOWN:
		return MCI_ERR_MANIFESTCLASSID;

	case SUIT_MANIFEST_SEC_TOP:
		/* Nordic top manifest - ability to operate on memory ranges intentionally blocked
		 */
		return MCI_ERR_NOACCESS;

	case SUIT_MANIFEST_SEC_SDFW:
		/* Sec manifest - TODO - implement checks based on UICR/SICR
		 */
		return SUIT_PLAT_SUCCESS;

	case SUIT_MANIFEST_SEC_SYSCTRL:
		/* Sysctrl manifest - TODO - implement checks based on UICR/SICR
		 */
		return SUIT_PLAT_SUCCESS;

	case SUIT_MANIFEST_APP_ROOT:
		/* Root manifest - ability to operate on memory ranges intentionally blocked
		 */
		return MCI_ERR_NOACCESS;

	case SUIT_MANIFEST_APP_RECOVERY:
	case SUIT_MANIFEST_APP_LOCAL_1:
	case SUIT_MANIFEST_APP_LOCAL_2:
	case SUIT_MANIFEST_APP_LOCAL_3: {
		struct arbiter_mem_params_access mem_params = {
			.allowed_types = ARBITER_MEM_TYPE(RESERVED, FIXED),
			.access = {
					.owner = NRF_OWNER_APPLICATION,
					.permissions = ARBITER_MEM_PERM(READ, SECURE),
					.address = (uintptr_t)address,
					.size = mem_size,
				},
		};

		if (arbiter_mem_access_check(&mem_params) != ARBITER_STATUS_OK) {
			return MCI_ERR_NOACCESS;
		}

		return SUIT_PLAT_SUCCESS;
	}

	case SUIT_MANIFEST_RAD_RECOVERY:
	case SUIT_MANIFEST_RAD_LOCAL_1:
	case SUIT_MANIFEST_RAD_LOCAL_2: {
		struct arbiter_mem_params_access mem_params = {
			.allowed_types = ARBITER_MEM_TYPE(RESERVED, FIXED),
			.access = {
					.owner = NRF_OWNER_RADIOCORE,
					.permissions = ARBITER_MEM_PERM(READ, SECURE),
					.address = (uintptr_t)address,
					.size = mem_size,
				},
		};

		if (arbiter_mem_access_check(&mem_params) != ARBITER_STATUS_OK) {
			return MCI_ERR_NOACCESS;
		}

		return SUIT_PLAT_SUCCESS;
	}

	default:
		break;
	}

	return MCI_ERR_NOACCESS;
}

mci_err_t
suit_mci_platform_specific_component_rights_validate(const suit_manifest_class_id_t *class_id,
						     int platform_specific_component_number)
{
	if (class_id == NULL) {
		return SUIT_PLAT_ERR_INVAL;
	}

	suit_manifest_role_t role = SUIT_MANIFEST_UNKNOWN;

	if (suit_storage_mpi_role_get(class_id, &role) != SUIT_PLAT_SUCCESS) {
		return MCI_ERR_MANIFESTCLASSID;
	}

	if (role == SUIT_MANIFEST_SEC_SDFW) {
		/* The only manifest with ability to control platform specific components is secdom.
		 * 1 - SDFW
		 * 2 - SDFW Recovery
		 */
		if (platform_specific_component_number == 1 ||
		    platform_specific_component_number == 2) {
			return SUIT_PLAT_SUCCESS;
		}
	}

	return MCI_ERR_NOACCESS;
}

mci_err_t suit_mci_vendor_id_for_manifest_class_id_get(const suit_manifest_class_id_t *class_id,
						       const suit_uuid_t **vendor_id)
{
	suit_storage_mpi_t *mpi = NULL;

	if (class_id == NULL || vendor_id == NULL) {
		return SUIT_PLAT_ERR_INVAL;
	}

	if (suit_storage_mpi_get(class_id, &mpi) != SUIT_PLAT_SUCCESS) {
		return MCI_ERR_MANIFESTCLASSID;
	}

	/* Casting is done as a temporary solution until mpi refactoring */
	*vendor_id = (const suit_uuid_t *)mpi->vendor_id;
	return SUIT_PLAT_SUCCESS;
}

mci_err_t
suit_mci_manifest_parent_child_declaration_validate(const suit_manifest_class_id_t *parent_class_id,
						    const suit_manifest_class_id_t *child_class_id)
{
	if ((parent_class_id == NULL) || (child_class_id == NULL)) {
		return SUIT_PLAT_ERR_INVAL;
	}

	suit_manifest_role_t parent_role = SUIT_MANIFEST_UNKNOWN;

	if (suit_storage_mpi_role_get(parent_class_id, &parent_role) != SUIT_PLAT_SUCCESS) {
		return MCI_ERR_MANIFESTCLASSID;
	}

	suit_manifest_role_t child_role = SUIT_MANIFEST_UNKNOWN;

	if (suit_storage_mpi_role_get(child_class_id, &child_role) != SUIT_PLAT_SUCCESS) {
		return MCI_ERR_MANIFESTCLASSID;
	}

	if ((parent_role == SUIT_MANIFEST_APP_ROOT) &&
	    (((child_role >= SUIT_MANIFEST_APP_LOCAL_1) &&
	      (child_role <= SUIT_MANIFEST_APP_LOCAL_3)) ||
	     ((child_role >= SUIT_MANIFEST_RAD_LOCAL_1) &&
	      (child_role <= SUIT_MANIFEST_RAD_LOCAL_2)) ||
	     (child_role == SUIT_MANIFEST_SEC_TOP))) {
		return SUIT_PLAT_SUCCESS;
	}

	if ((parent_role == SUIT_MANIFEST_SEC_TOP) &&
	    ((child_role == SUIT_MANIFEST_SEC_SYSCTRL) || (child_role == SUIT_MANIFEST_SEC_SDFW))) {
		return SUIT_PLAT_SUCCESS;
	}

	if ((parent_role == SUIT_MANIFEST_APP_RECOVERY) &&
	    ((child_role == SUIT_MANIFEST_RAD_RECOVERY) ||
	     ((child_role >= SUIT_MANIFEST_APP_LOCAL_1) &&
	      (child_role <= SUIT_MANIFEST_APP_LOCAL_3)) ||
	     ((child_role >= SUIT_MANIFEST_RAD_LOCAL_1) &&
	      (child_role <= SUIT_MANIFEST_RAD_LOCAL_2)))) {
		return SUIT_PLAT_SUCCESS;
	}

	return MCI_ERR_NOACCESS;
}

mci_err_t
suit_mci_manifest_process_dependency_validate(const suit_manifest_class_id_t *parent_class_id,
					      const suit_manifest_class_id_t *child_class_id)
{
	if ((parent_class_id == NULL) || (child_class_id == NULL)) {
		return SUIT_PLAT_ERR_INVAL;
	}

	suit_execution_mode_t execution_mode = suit_execution_mode_get();

	suit_manifest_role_t parent_role = SUIT_MANIFEST_UNKNOWN;

	if (suit_storage_mpi_role_get(parent_class_id, &parent_role) != SUIT_PLAT_SUCCESS) {
		return MCI_ERR_MANIFESTCLASSID;
	}

	suit_manifest_role_t child_role = SUIT_MANIFEST_UNKNOWN;

	if (suit_storage_mpi_role_get(child_class_id, &child_role) != SUIT_PLAT_SUCCESS) {
		return MCI_ERR_MANIFESTCLASSID;
	}

	switch (execution_mode) {
	case EXECUTION_MODE_INVOKE:
		if ((parent_role == SUIT_MANIFEST_SEC_TOP) &&
		    ((child_role == SUIT_MANIFEST_SEC_SYSCTRL) ||
		     (child_role == SUIT_MANIFEST_SEC_SDFW))) {
			return SUIT_PLAT_SUCCESS;
		}

		if ((parent_role == SUIT_MANIFEST_APP_ROOT) &&
		    (((child_role >= SUIT_MANIFEST_APP_LOCAL_1) &&
		      (child_role <= SUIT_MANIFEST_APP_LOCAL_3)) ||
		     ((child_role >= SUIT_MANIFEST_RAD_LOCAL_1) &&
		      (child_role <= SUIT_MANIFEST_RAD_LOCAL_2)))) {
			return SUIT_PLAT_SUCCESS;
		}
		break;

	case EXECUTION_MODE_INSTALL:
		if ((parent_role == SUIT_MANIFEST_SEC_TOP) &&
		    ((child_role == SUIT_MANIFEST_SEC_SYSCTRL) ||
		     (child_role == SUIT_MANIFEST_SEC_SDFW))) {
			return SUIT_PLAT_SUCCESS;
		}

		if ((parent_role == SUIT_MANIFEST_APP_ROOT) &&
		    (((child_role >= SUIT_MANIFEST_APP_LOCAL_1) &&
		      (child_role <= SUIT_MANIFEST_APP_LOCAL_3)) ||
		     ((child_role >= SUIT_MANIFEST_RAD_LOCAL_1) &&
		      (child_role <= SUIT_MANIFEST_RAD_LOCAL_2)) ||
		     (child_role == SUIT_MANIFEST_SEC_TOP))) {
			return SUIT_PLAT_SUCCESS;
		}

		if ((parent_role == SUIT_MANIFEST_APP_RECOVERY) &&
		    (child_role == SUIT_MANIFEST_RAD_RECOVERY)) {
			return SUIT_PLAT_SUCCESS;
		}
		break;

	case EXECUTION_MODE_INSTALL_RECOVERY:
		if ((parent_role == SUIT_MANIFEST_SEC_TOP) &&
		    ((child_role == SUIT_MANIFEST_SEC_SYSCTRL) ||
		     (child_role == SUIT_MANIFEST_SEC_SDFW))) {
			return SUIT_PLAT_SUCCESS;
		}

		if ((parent_role == SUIT_MANIFEST_APP_ROOT) &&
		    (((child_role >= SUIT_MANIFEST_APP_LOCAL_1) &&
		      (child_role <= SUIT_MANIFEST_APP_LOCAL_3)) ||
		     ((child_role >= SUIT_MANIFEST_RAD_LOCAL_1) &&
		      (child_role <= SUIT_MANIFEST_RAD_LOCAL_2)) ||
		     (child_role == SUIT_MANIFEST_SEC_TOP))) {
			return SUIT_PLAT_SUCCESS;
		}
		break;

	case EXECUTION_MODE_INVOKE_RECOVERY:
		if ((parent_role == SUIT_MANIFEST_SEC_TOP) &&
		    ((child_role == SUIT_MANIFEST_SEC_SYSCTRL) ||
		     (child_role == SUIT_MANIFEST_SEC_SDFW))) {
			return SUIT_PLAT_SUCCESS;
		}

		if ((parent_role == SUIT_MANIFEST_APP_RECOVERY) &&
		    ((child_role == SUIT_MANIFEST_RAD_RECOVERY) ||
		     ((child_role >= SUIT_MANIFEST_APP_LOCAL_1) &&
		      (child_role <= SUIT_MANIFEST_APP_LOCAL_3)) ||
		     ((child_role >= SUIT_MANIFEST_RAD_LOCAL_1) &&
		      (child_role <= SUIT_MANIFEST_RAD_LOCAL_2)))) {
			return SUIT_PLAT_SUCCESS;
		}

		if ((parent_role == SUIT_MANIFEST_APP_ROOT) &&
		    (((child_role >= SUIT_MANIFEST_APP_LOCAL_1) &&
		      (child_role <= SUIT_MANIFEST_APP_LOCAL_3)) ||
		     ((child_role >= SUIT_MANIFEST_RAD_LOCAL_1) &&
		      (child_role <= SUIT_MANIFEST_RAD_LOCAL_2)))) {
			return SUIT_PLAT_SUCCESS;
		}
		break;

	case EXECUTION_MODE_FAIL_INSTALL_NORDIC_TOP:
		/* In this state it is prohibited to process OEM manifests. */
		if ((parent_role == SUIT_MANIFEST_SEC_TOP) &&
		    ((child_role == SUIT_MANIFEST_SEC_SYSCTRL) ||
		     (child_role == SUIT_MANIFEST_SEC_SDFW))) {
			return SUIT_PLAT_SUCCESS;
		}
		break;

	default:
		break;
	}

	return MCI_ERR_NOACCESS;
}

mci_err_t suit_mci_init(void)
{
	return SUIT_PLAT_SUCCESS;
}
