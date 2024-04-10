/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <suit_storage_mpi.h>
#include <suit_storage_internal.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(suit_storage, CONFIG_SUIT_LOG_LEVEL);

#define SUIT_STORAGE_ADDRESS suit_plat_mem_nvm_ptr_get(SUIT_STORAGE_OFFSET)
#define SUIT_STORAGE_OFFSET  FIXED_PARTITION_OFFSET(suit_storage)
#define SUIT_STORAGE_SIZE    FIXED_PARTITION_SIZE(suit_storage)
#define SUIT_REPORT_SIZE     4096 /**< Size of a single SUIT report slot. */
#define SUIT_N_REPORTS	     2	  /**< Number of SUIT report slots. */

/* Update candidate metadata, aligned to the erase block size. */
union suit_update_candidate_entry {
	struct update_candidate_info update;
	uint8_t erase_block[EB_SIZE(struct update_candidate_info)];
};

/* SUIT envelope slot.
 *
 * Each slot contains a CBOR-encoded map with severed SUIT envelope.
 */
typedef uint8_t suit_envelope_encoded_t[CONFIG_SUIT_STORAGE_ENVELOPE_SIZE];

/* SUIT envelope slot, aligned to the erase block size. */
union suit_envelope_entry {
	suit_envelope_encoded_t envelope_encoded;
	uint8_t erase_block[EB_SIZE(suit_envelope_encoded_t)];
};

/* SUIT configuration area, set by application (i.e. thorough IPC service) and
 * checked using suit-condition-check-content.
 *
 * Currently both the service and the directive is not supported.
 * This definition allows to implement up to three 32-bit configuration values.
 * It is defined here, so the system will not be repartitioned once this feature
 * will be introduced in the future.
 */
typedef uint8_t suit_config_t[CONFIG_SUIT_STORAGE_CONFIG_SIZE];

/* SUIT configuration area, aligned to the erase block size. */
union suit_config_entry {
	suit_config_t config;
	uint8_t erase_block[EB_SIZE(suit_config_t)];
};

/* SUIT report binary. */
typedef uint8_t suit_report_t[SUIT_REPORT_SIZE];

/* SUIT rebort area, aligned to the erase block size. */
union suit_report_entry {
	suit_report_t report;
	uint8_t erase_block[EB_SIZE(suit_report_t)];
};

/* SUIT storage structure, that can be used to parse the NVM contents. */
struct suit_storage {
	/** Update candidate information. */
	union suit_update_candidate_entry update;
	/** SUIT configuration area */
	union suit_config_entry config;
	/** A copy of the configuration area to protect against random resets. */
	union suit_config_entry config_backup;
	/** The main storage for the SUIT envelopes. */
	union suit_envelope_entry envelopes[CONFIG_SUIT_STORAGE_N_ENVELOPES];
	/** Storage for the SUIT reports. */
	union suit_report_entry reports[SUIT_N_REPORTS];
};

static const suit_storage_mpi_t mpi_test_sample[] = {
	{
		.version = SUIT_MPI_INFO_VERSION,
		.downgrade_prevention_policy = SUIT_MPI_DOWNGRADE_PREVENTION_DISABLED,
		.independent_updateability_policy = SUIT_MPI_INDEPENDENT_UPDATE_ALLOWED,
		.signature_verification_policy = SUIT_MPI_SIGNATURE_CHECK_DISABLED,
		.reserved = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			     0xFF},
		/* RFC4122 uuid5(uuid.NAMESPACE_DNS, 'nordicsemi.com') */
		.vendor_id = {0x76, 0x17, 0xda, 0xa5, 0x71, 0xfd, 0x5a, 0x85, 0x8f, 0x94, 0xe2,
			      0x8d, 0x73, 0x5c, 0xe9, 0xf4},
		/* RFC4122 uuid5(nordic_vid, 'test_sample_root') */
		.class_id = {0x97, 0x05, 0x48, 0x23, 0x4c, 0x3d, 0x59, 0xa1, 0x89, 0x86, 0xa5, 0x46,
			     0x60, 0xa1, 0x4b, 0x0a},
	},
	{
		.version = SUIT_MPI_INFO_VERSION,
		.downgrade_prevention_policy = SUIT_MPI_DOWNGRADE_PREVENTION_DISABLED,
		.independent_updateability_policy = SUIT_MPI_INDEPENDENT_UPDATE_DENIED,
		.signature_verification_policy = SUIT_MPI_SIGNATURE_CHECK_DISABLED,
		.reserved = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			     0xFF},
		/* RFC4122 uuid5(uuid.NAMESPACE_DNS, 'nordicsemi.com') */
		.vendor_id = {0x76, 0x17, 0xda, 0xa5, 0x71, 0xfd, 0x5a, 0x85, 0x8f, 0x94, 0xe2,
			      0x8d, 0x73, 0x5c, 0xe9, 0xf4},
		/* RFC4122 uuid5(nordic_vid, 'test_sample_app') */
		.class_id = {0x5b, 0x46, 0x9f, 0xd1, 0x90, 0xee, 0x53, 0x9c, 0xa3, 0x18, 0x68, 0x1b,
			     0x03, 0x69, 0x5e, 0x36},
	},
	{
		.version = SUIT_MPI_INFO_VERSION,
		.downgrade_prevention_policy = SUIT_MPI_DOWNGRADE_PREVENTION_DISABLED,
		.independent_updateability_policy = SUIT_MPI_INDEPENDENT_UPDATE_ALLOWED,
		.signature_verification_policy = SUIT_MPI_SIGNATURE_CHECK_DISABLED,
		.reserved = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			     0xFF},
		/* RFC4122 uuid5(uuid.NAMESPACE_DNS, 'nordicsemi.com') */
		.vendor_id = {0x76, 0x17, 0xda, 0xa5, 0x71, 0xfd, 0x5a, 0x85, 0x8f, 0x94, 0xe2,
			      0x8d, 0x73, 0x5c, 0xe9, 0xf4},
		/* RFC4122 uuid5(nordic_vid, 'test_sample_recovery') */
		.class_id = {0x74, 0xa0, 0xc6, 0xe7, 0xa9, 0x2a, 0x56, 0x00, 0x9c, 0x5d, 0x30, 0xee,
			     0x87, 0x8b, 0x06, 0xba},
	}};

static suit_plat_err_t find_mpi_area(suit_manifest_role_t role, uint8_t **addr, size_t *size)
{
	int index = -1;

	const suit_storage_mpi_t *mpi_config = mpi_test_sample;

	switch (role) {
	case SUIT_MANIFEST_APP_ROOT:
		index = 0;
		break;
	case SUIT_MANIFEST_APP_LOCAL_1:
		index = 1;
		break;
	case SUIT_MANIFEST_APP_RECOVERY:
		index = 2;
		break;
	default:
		index = -1;
		break;
	}

	if (index == -1) {
		return SUIT_PLAT_ERR_OUT_OF_BOUNDS;
	}

	*addr = (uint8_t *)&mpi_config[index];
	*size = sizeof(mpi_config[index]);

	return SUIT_PLAT_SUCCESS;
}

static suit_plat_err_t find_manifest_area(suit_manifest_role_t role, const uint8_t **addr,
					  size_t *size)
{
	struct suit_storage *storage = (struct suit_storage *)SUIT_STORAGE_ADDRESS;
	int index = -1;

	switch (role) {
	case SUIT_MANIFEST_APP_ROOT:
		index = 0;
		break;
	case SUIT_MANIFEST_APP_LOCAL_1:
		index = 1;
		break;
	case SUIT_MANIFEST_APP_RECOVERY:
		index = 2;
		break;
	default:
		index = -1;
		break;
	}

	if (index == -1) {
		return SUIT_PLAT_ERR_OUT_OF_BOUNDS;
	}

	*addr = storage->envelopes[index].erase_block;
	*size = sizeof(storage->envelopes[index].erase_block);

	return SUIT_PLAT_SUCCESS;
}

static suit_plat_err_t find_report_area(size_t index, uint8_t **addr, size_t *size)
{
	struct suit_storage *storage = (struct suit_storage *)SUIT_STORAGE_ADDRESS;

	if (index >= SUIT_N_REPORTS) {
		return SUIT_PLAT_ERR_OUT_OF_BOUNDS;
	}

	*addr = storage->reports[index].erase_block;
	*size = sizeof(storage->reports[index].erase_block);

	return SUIT_PLAT_SUCCESS;
}

suit_plat_err_t suit_storage_init(void)
{
	suit_plat_err_t err = SUIT_PLAT_SUCCESS;
	suit_manifest_role_t roles[] = {
		SUIT_MANIFEST_APP_ROOT,
		SUIT_MANIFEST_APP_LOCAL_1,
		SUIT_MANIFEST_APP_RECOVERY,
	};

	if (sizeof(struct suit_storage) > SUIT_STORAGE_SIZE) {
		return SUIT_PLAT_ERR_NOMEM;
	}

	if (CEIL_DIV(SUIT_STORAGE_SIZE, SUIT_STORAGE_EB_SIZE) * SUIT_STORAGE_EB_SIZE !=
	    SUIT_STORAGE_SIZE) {
		return SUIT_PLAT_ERR_CRASH;
	}

	err = suit_storage_mpi_init();
	if (err != SUIT_PLAT_SUCCESS) {
		return err;
	}

	err = suit_storage_report_internal_init();
	if (err != SUIT_PLAT_SUCCESS) {
		return err;
	}

	err = suit_storage_update_init();
	if (err != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Unable to initialize update candidate staging area (err: %d)", err);
	}

	/* Bootstrap MPI entries. */
	for (size_t i = 0; i < ARRAY_SIZE(roles); i++) {
		uint8_t *mpi_addr;
		size_t mpi_size;

		err = find_mpi_area(roles[i], &mpi_addr, &mpi_size);
		if (err != SUIT_PLAT_SUCCESS) {
			continue;
		}

		err = suit_storage_mpi_configuration_load(roles[i], mpi_addr, mpi_size);
		if (err != SUIT_PLAT_SUCCESS) {
			return err;
		}
	}

	return SUIT_PLAT_SUCCESS;
}

suit_plat_err_t suit_storage_update_cand_get(const suit_plat_mreg_t **regions, size_t *len)
{
	struct suit_storage *storage = (struct suit_storage *)SUIT_STORAGE_ADDRESS;
	uint8_t *addr = storage->update.erase_block;
	size_t size = sizeof(storage->update.erase_block);

	return suit_storage_update_get(addr, size, regions, len);
}

suit_plat_err_t suit_storage_update_cand_set(suit_plat_mreg_t *regions, size_t len)
{
	struct suit_storage *storage = (struct suit_storage *)SUIT_STORAGE_ADDRESS;
	uint8_t *addr = storage->update.erase_block;
	size_t size = sizeof(storage->update.erase_block);

	return suit_storage_update_set(addr, size, regions, len);
}

suit_plat_err_t suit_storage_installed_envelope_get(const suit_manifest_class_id_t *id,
						    const uint8_t **addr, size_t *size)
{
	suit_manifest_role_t role;

	if ((id == NULL) || (addr == NULL) || (size == 0)) {
		return SUIT_PLAT_ERR_INVAL;
	}

	suit_plat_err_t err = suit_storage_mpi_role_get(id, &role);

	if (err != SUIT_PLAT_SUCCESS) {
		LOG_INF("Unable to find role for given class ID.");
		return err;
	}

	err = find_manifest_area(role, addr, size);
	if (err != SUIT_PLAT_SUCCESS) {
		LOG_INF("Unable to find area for envelope with role 0x%x.", role);
		return err;
	}

	LOG_DBG("Decode envelope with role: 0x%x address: 0x%lx", role, (intptr_t)(*addr));

	err = suit_storage_envelope_get(*addr, *size, id, addr, size);
	if (err != SUIT_PLAT_SUCCESS) {
		LOG_WRN("Unable to parse envelope with role 0x%x", role);
		return err;
	}

	LOG_DBG("Valid envelope with given class ID and role 0x%x found", role);

	return err;
}

suit_plat_err_t suit_storage_install_envelope(const suit_manifest_class_id_t *id, uint8_t *addr,
					      size_t size)
{
	suit_plat_err_t err;
	suit_manifest_role_t role;
	uint8_t *area_addr = NULL;
	size_t area_size = 0;

	err = suit_storage_mpi_role_get(id, &role);
	if (err != SUIT_PLAT_SUCCESS) {
		LOG_INF("Unable to find role for given class ID.");
		return err;
	}

	if ((addr == NULL) || (size == 0)) {
		return SUIT_PLAT_ERR_INVAL;
	}

	err = find_manifest_area(role, (const uint8_t **)&area_addr, &area_size);
	if (err != SUIT_PLAT_SUCCESS) {
		LOG_INF("Unable to find area for envelope with role 0x%x.", role);
		return err;
	}

	err = suit_storage_envelope_install(area_addr, area_size, id, addr, size);
	if (err != SUIT_PLAT_SUCCESS) {
		LOG_INF("Failed to install envelope with role 0x%x.", role);
		return err;
	}

	LOG_INF("Envelope with role 0x%x saved.", role);

	return err;
}

suit_plat_err_t suit_storage_report_clear(size_t index)
{
	uint8_t *area_addr = NULL;
	size_t area_size = 0;
	suit_plat_err_t err;

	err = find_report_area(index, &area_addr, &area_size);
	if (err != SUIT_PLAT_SUCCESS) {
		LOG_INF("Unable to find report area at index %d.", index);
		return err;
	}

	return suit_storage_report_internal_clear(area_addr, area_size);
}

suit_plat_err_t suit_storage_report_save(size_t index, const uint8_t *buf, size_t len)
{
	uint8_t *area_addr = NULL;
	size_t area_size = 0;
	suit_plat_err_t err;

	err = find_report_area(index, &area_addr, &area_size);
	if (err != SUIT_PLAT_SUCCESS) {
		LOG_INF("Unable to find area for report at index %d.", index);
		return err;
	}

	return suit_storage_report_internal_save(area_addr, area_size, buf, len);
}

suit_plat_err_t suit_storage_report_read(size_t index, const uint8_t **buf, size_t *len)
{
	uint8_t *area_addr = NULL;
	size_t area_size = 0;
	suit_plat_err_t err;

	err = find_report_area(index, &area_addr, &area_size);
	if (err != SUIT_PLAT_SUCCESS) {
		LOG_INF("Unable to find area with report at index %d.", index);
		return err;
	}

	return suit_storage_report_internal_read(area_addr, area_size, buf, len);
}
