/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <suit_storage_mpi.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(suit_storage_mpi, CONFIG_SUIT_LOG_LEVEL);

typedef struct {
	suit_manifest_role_t role;
	const uint8_t *addr;
	size_t size;
} suit_storage_mpi_entry_t;

static suit_storage_mpi_entry_t entries[CONFIG_SUIT_STORAGE_N_ENVELOPES];
static size_t entries_len;

/** @brief Detect if UUID contains only zeros or ones.
 *
 * @retval SUIT_PLAT_ERR_OUT_OF_BOUNDS  if the UUID contains only zeros or ones.
 * @retval SUIT_PLAT_SUCCESS            if the UUID contains at least two different bits.
 */
static suit_plat_err_t uuid_validate(const uint8_t *uuid)
{
	bool zeros = true;
	bool ones = true;

	for (size_t i = 0; i < sizeof(suit_uuid_t); i++) {
		if (uuid[i] != 0xFF) {
			ones = false;
		}

		if (uuid[i] != 0x00) {
			zeros = false;
		}
	}

	if (zeros || ones) {
		return SUIT_PLAT_ERR_OUT_OF_BOUNDS;
	}

	return SUIT_PLAT_SUCCESS;
}

suit_plat_err_t suit_storage_mpi_init(void)
{
	memset(entries, 0, sizeof(entries));
	entries_len = 0;

	return SUIT_PLAT_SUCCESS;
}

suit_plat_err_t suit_storage_mpi_configuration_load(suit_manifest_role_t role, const uint8_t *addr,
						    size_t size)
{
	const suit_manifest_class_id_t *new_class_id = NULL;
	suit_storage_mpi_t *mpi = (suit_storage_mpi_t *)addr;

	if ((role == SUIT_MANIFEST_UNKNOWN) || (addr == NULL) || (size == 0)) {
		return SUIT_PLAT_ERR_INVAL;
	}

	if (mpi->version != SUIT_MPI_INFO_VERSION) {
		/* Detect if the area is erased. */
		for (size_t i = 0; i < size; i++) {
			if (addr[i] != 0xFF) {
				return SUIT_PLAT_ERR_OUT_OF_BOUNDS;
			}
		}

		/* Use the dedicated error code - in some cases, the caller may decide to continue
		 * loading the MPI structures.
		 */
		return SUIT_PLAT_ERR_NOT_FOUND;
	}

	new_class_id = (const suit_manifest_class_id_t *)mpi->class_id;

	for (size_t i = 0; i < entries_len; i++) {
		const suit_storage_mpi_t *ex_mpi = (suit_storage_mpi_t *)entries[i].addr;
		const suit_manifest_class_id_t *ex_class_id =
			(const suit_manifest_class_id_t *)ex_mpi->class_id;

		if (entries[i].role == role) {
			LOG_ERR("Manifest with role 0x%x already configured at index %d", role, i);
			return SUIT_PLAT_ERR_EXISTS;
		}

		if (suit_metadata_uuid_compare(new_class_id, ex_class_id) == SUIT_PLAT_SUCCESS) {
			LOG_ERR("Manifest with given class ID already configured with different "
				"role at index %d",
				i);
			return SUIT_PLAT_ERR_EXISTS;
		}
	}

	if (ARRAY_SIZE(entries) <= entries_len) {
		LOG_ERR("Too small Manifest Provisioning Information array.");
		return SUIT_PLAT_ERR_SIZE;
	}

	/* Validate downgrade prevention policy value. */
	switch (mpi->downgrade_prevention_policy) {
	case SUIT_MPI_DOWNGRADE_PREVENTION_DISABLED:
	case SUIT_MPI_DOWNGRADE_PREVENTION_ENABLED:
		break;
	default:
		LOG_ERR("Invalid downgrade prevention policy value for role 0x%x: %d", role,
			mpi->downgrade_prevention_policy);
		return SUIT_PLAT_ERR_OUT_OF_BOUNDS;
	}

	/* Validate independent updateability policy value. */
	switch (mpi->independent_updateability_policy) {
	case SUIT_MPI_INDEPENDENT_UPDATE_DENIED:
		if ((role == SUIT_MANIFEST_APP_ROOT) || (role == SUIT_MANIFEST_APP_RECOVERY)) {
			LOG_ERR("It is incorrect to block independent updates for role %d", role);
			return SUIT_PLAT_ERR_UNSUPPORTED;
		}
		break;
	case SUIT_MPI_INDEPENDENT_UPDATE_ALLOWED:
		break;
	default:
		LOG_ERR("Invalid independent updateability policy value for role 0x%x: %d", role,
			mpi->independent_updateability_policy);
		return SUIT_PLAT_ERR_OUT_OF_BOUNDS;
	}

	/* Validate signature verification policy value. */
	switch (mpi->signature_verification_policy) {
	case SUIT_MPI_SIGNATURE_CHECK_DISABLED:
	case SUIT_MPI_SIGNATURE_CHECK_ENABLED_ON_UPDATE:
	case SUIT_MPI_SIGNATURE_CHECK_ENABLED_ON_UPDATE_AND_BOOT:
		break;
	default:
		LOG_ERR("Invalid signature verification policy value for role 0x%x: %d", role,
			mpi->signature_verification_policy);
		return SUIT_PLAT_ERR_OUT_OF_BOUNDS;
	}

	/* Validate reserved field value. */
	for (size_t i = 0; i < ARRAY_SIZE(mpi->reserved); i++) {
		if (mpi->reserved[i] != 0xFF) {
			LOG_ERR("Invalid value inside reserved field at index %d for role 0x%x "
				"(0x%x)",
				i, role, mpi->reserved[i]);
			return SUIT_PLAT_ERR_OUT_OF_BOUNDS;
		}
	}

	/* Validate vendor ID value. */
	if (uuid_validate(mpi->vendor_id) != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Invalid vendor UUID configured for role 0x%x", role);
		return SUIT_PLAT_ERR_OUT_OF_BOUNDS;
	}

	/* Validate class ID value. */
	if (uuid_validate(mpi->class_id) != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Invalid class UUID configured for role 0x%x", role);
		return SUIT_PLAT_ERR_OUT_OF_BOUNDS;
	}

	LOG_INF("Add manifest with role 0x%x and class ID at index %d:", role, entries_len);
	LOG_INF("\t%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
		new_class_id->raw[0], new_class_id->raw[1], new_class_id->raw[2],
		new_class_id->raw[3], new_class_id->raw[4], new_class_id->raw[5],
		new_class_id->raw[6], new_class_id->raw[7], new_class_id->raw[8],
		new_class_id->raw[9], new_class_id->raw[10], new_class_id->raw[11],
		new_class_id->raw[12], new_class_id->raw[13], new_class_id->raw[14],
		new_class_id->raw[15]);

	entries[entries_len].role = role;
	entries[entries_len].addr = addr;
	entries[entries_len].size = size;
	entries_len++;

	return SUIT_PLAT_SUCCESS;
}

suit_plat_err_t suit_storage_mpi_role_get(const suit_manifest_class_id_t *class_id,
					  suit_manifest_role_t *role)
{
	if ((class_id == NULL) || (role == NULL)) {
		return SUIT_PLAT_ERR_INVAL;
	}

	for (size_t i = 0; i < entries_len; i++) {
		if (entries[i].role != SUIT_MANIFEST_UNKNOWN) {
			suit_storage_mpi_t *mpi = (suit_storage_mpi_t *)entries[i].addr;

			if (suit_metadata_uuid_compare(
				    (const suit_manifest_class_id_t *)mpi->class_id, class_id) ==
			    SUIT_PLAT_SUCCESS) {
				*role = entries[i].role;
				return SUIT_PLAT_SUCCESS;
			}
		}
	}

	return SUIT_PLAT_ERR_NOT_FOUND;
}

suit_plat_err_t suit_storage_mpi_class_get(suit_manifest_role_t role,
					   const suit_manifest_class_id_t **class_id)
{
	if ((role == SUIT_MANIFEST_UNKNOWN) || (class_id == NULL)) {
		return SUIT_PLAT_ERR_INVAL;
	}

	for (size_t i = 0; i < entries_len; i++) {
		if (entries[i].role == role) {
			suit_storage_mpi_t *mpi = (suit_storage_mpi_t *)entries[i].addr;

			*class_id = (const suit_manifest_class_id_t *)mpi->class_id;

			return SUIT_PLAT_SUCCESS;
		}
	}

	return SUIT_PLAT_ERR_NOT_FOUND;
}

suit_plat_err_t suit_storage_mpi_get(const suit_manifest_class_id_t *class_id,
				     suit_storage_mpi_t **mpi)
{
	if ((class_id == NULL) || (mpi == NULL)) {
		return SUIT_PLAT_ERR_INVAL;
	}

	for (size_t i = 0; i < entries_len; i++) {
		if (entries[i].role != SUIT_MANIFEST_UNKNOWN) {
			*mpi = (suit_storage_mpi_t *)entries[i].addr;

			if (suit_metadata_uuid_compare(
				    (const suit_manifest_class_id_t *)(*mpi)->class_id, class_id) ==
			    SUIT_PLAT_SUCCESS) {
				return SUIT_PLAT_SUCCESS;
			}
		}
	}

	return SUIT_PLAT_ERR_NOT_FOUND;
}

suit_plat_err_t suit_storage_mpi_class_ids_get(suit_manifest_class_info_t *class_info, size_t *size)
{
	if ((class_info == NULL) || (size == NULL)) {
		return SUIT_PLAT_ERR_INVAL;
	}

	if (*size < entries_len) {
		return SUIT_PLAT_ERR_SIZE;
	}

	for (size_t i = 0; i < entries_len; i++) {
		suit_storage_mpi_t *mpi = (suit_storage_mpi_t *)entries[i].addr;

		class_info[i].vendor_id = (const suit_manifest_class_id_t *)mpi->vendor_id;
		class_info[i].class_id = (const suit_manifest_class_id_t *)mpi->class_id;
		class_info[i].role = entries[i].role;
	}

	*size = entries_len;

	return SUIT_PLAT_SUCCESS;
}

suit_downgrade_prevention_policy_t suit_mpi_downgrade_prevention_policy_to_metadata(int mpi_policy)
{
	switch (mpi_policy) {
	case SUIT_MPI_DOWNGRADE_PREVENTION_DISABLED:
		return SUIT_DOWNGRADE_PREVENTION_DISABLED;
	case SUIT_MPI_DOWNGRADE_PREVENTION_ENABLED:
		return SUIT_DOWNGRADE_PREVENTION_ENABLED;
	default:
		return SUIT_DOWNGRADE_PREVENTION_UNKNOWN;
	}
}

suit_independent_updateability_policy_t
suit_mpi_independent_updateability_policy_to_metadata(int mpi_policy)
{
	switch (mpi_policy) {
	case SUIT_MPI_INDEPENDENT_UPDATE_DENIED:
		return SUIT_INDEPENDENT_UPDATE_DENIED;
	case SUIT_MPI_INDEPENDENT_UPDATE_ALLOWED:
		return SUIT_INDEPENDENT_UPDATE_ALLOWED;
	default:
		return SUIT_INDEPENDENT_UPDATE_UNKNOWN;
	}
}

suit_signature_verification_policy_t
suit_mpi_signature_verification_policy_to_metadata(int mpi_policy)
{
	switch (mpi_policy) {
	case SUIT_MPI_SIGNATURE_CHECK_DISABLED:
		return SUIT_SIGNATURE_CHECK_DISABLED;
	case SUIT_MPI_SIGNATURE_CHECK_ENABLED_ON_UPDATE:
		return SUIT_SIGNATURE_CHECK_ENABLED_ON_UPDATE;
	case SUIT_MPI_SIGNATURE_CHECK_ENABLED_ON_UPDATE_AND_BOOT:
		return SUIT_SIGNATURE_CHECK_ENABLED_ON_UPDATE_AND_BOOT;
	default:
		return SUIT_SIGNATURE_CHECK_UNKNOWN;
	}
}
