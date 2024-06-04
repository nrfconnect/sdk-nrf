/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <suit_storage_mpi.h>
#include <suit_storage_nvv.h>
#include <suit_storage_internal.h>
#include <zephyr/logging/log.h>
#include <psa/crypto.h>

LOG_MODULE_REGISTER(suit_storage, CONFIG_SUIT_LOG_LEVEL);

/* SUIT storage partition is defined as reserved-memory region, thus it uses absolute addresses. */
#define SUIT_STORAGE_OFFSET                                                                        \
	(FIXED_PARTITION_OFFSET(suit_storage_partition) - DT_REG_ADDR(DT_CHOSEN(zephyr_flash)))
#define SUIT_STORAGE_SIZE	    FIXED_PARTITION_SIZE(suit_storage_partition)
#define SUIT_STORAGE_NORDIC_ADDRESS suit_plat_mem_nvm_ptr_get(SUIT_STORAGE_NORDIC_OFFSET)
#define SUIT_STORAGE_NORDIC_OFFSET  FIXED_PARTITION_OFFSET(cpusec_suit_storage)
#define SUIT_STORAGE_NORDIC_SIZE    FIXED_PARTITION_SIZE(cpusec_suit_storage)
#define SUIT_STORAGE_RAD_ADDRESS    suit_plat_mem_nvm_ptr_get(SUIT_STORAGE_RAD_OFFSET)
#define SUIT_STORAGE_RAD_OFFSET	    FIXED_PARTITION_OFFSET(cpurad_suit_storage)
#define SUIT_STORAGE_RAD_SIZE	    FIXED_PARTITION_SIZE(cpurad_suit_storage)
#define SUIT_STORAGE_APP_ADDRESS    suit_plat_mem_nvm_ptr_get(SUIT_STORAGE_APP_OFFSET)
#define SUIT_STORAGE_APP_OFFSET	    FIXED_PARTITION_OFFSET(cpuapp_suit_storage)
#define SUIT_STORAGE_APP_SIZE	    FIXED_PARTITION_SIZE(cpuapp_suit_storage)

BUILD_ASSERT((SUIT_STORAGE_OFFSET <= SUIT_STORAGE_NORDIC_OFFSET) &&
		     (SUIT_STORAGE_NORDIC_OFFSET + SUIT_STORAGE_NORDIC_SIZE <=
		      SUIT_STORAGE_OFFSET + SUIT_STORAGE_SIZE),
	     "Secure storage must be defined within SUIT storage partition");

BUILD_ASSERT((SUIT_STORAGE_OFFSET <= SUIT_STORAGE_RAD_OFFSET) &&
		     (SUIT_STORAGE_RAD_OFFSET + SUIT_STORAGE_RAD_SIZE <=
		      SUIT_STORAGE_OFFSET + SUIT_STORAGE_SIZE),
	     "Radiocore storage must be defined within SUIT storage partition");

BUILD_ASSERT((SUIT_STORAGE_OFFSET <= SUIT_STORAGE_APP_OFFSET) &&
		     (SUIT_STORAGE_APP_OFFSET + SUIT_STORAGE_APP_SIZE <=
		      SUIT_STORAGE_OFFSET + SUIT_STORAGE_SIZE),
	     "Application storage must be defined within SUIT storage partition");

typedef uint8_t suit_storage_digest_t[32];

struct suit_storage_nvv {
	/* Manifest Runtime Non Volatile Variables. */
	uint8_t var[EB_SIZE(suit_storage_nvv_t)];
	/* Manifest Runtime Non Volatile Variables digest. */
	uint8_t digest[EB_SIZE(suit_storage_digest_t)];
};

struct suit_storage_mpi_rad {
	/* Radio recovery manifest MPI. */
	uint8_t recovery[EB_SIZE(suit_storage_mpi_t)];
	/* Radio local A manifest MPI. */
	uint8_t local_a[EB_SIZE(suit_storage_mpi_t)];
	/* Radio local B manifest MPI. */
	uint8_t local_b[EB_SIZE(suit_storage_mpi_t)];
	/* Radio MPI digest. */
	uint8_t digest[EB_SIZE(suit_storage_digest_t)];
};

struct suit_storage_mpi_app {
	/* Application root manifest MPI. */
	uint8_t root[EB_SIZE(suit_storage_mpi_t)];
	/* Application recovery manifest MPI. */
	uint8_t recovery[EB_SIZE(suit_storage_mpi_t)];
	/* Application local A manifest MPI. */
	uint8_t local_a[EB_SIZE(suit_storage_mpi_t)];
	/* Application local B manifest MPI. */
	uint8_t local_b[EB_SIZE(suit_storage_mpi_t)];
	/* Application local spare manifest MPI. */
	uint8_t local_spare[EB_SIZE(suit_storage_mpi_t)];
	/* Application MPI digest. */
	uint8_t digest[EB_SIZE(suit_storage_digest_t)];
};

/* SUIT storage structure, that can be used to parse the NVM contents. */
struct suit_storage_nordic {
	/* Area accessible by the SECURE core debugger. */
	union {
		struct suit_storage_area_nordic {
			/* Radio MPI Backup and digest. */
			struct suit_storage_mpi_rad rad_mpi_bak;
			/* Application MPI Backup and digest. */
			struct suit_storage_mpi_app app_mpi_bak;

			/* The last SUIT update report. */
			uint8_t report_0[160];

			/* Reserved for Future Use. */
			uint8_t reserved[160];

			/* Installed envelope: Nordic Top */
			uint8_t top[EB_ALIGN(1280)];
			/* Installed envelope: Secure Domain Firmware (update) */
			uint8_t sdfw[EB_ALIGN(1024)];
			/* Installed envelope: System Controller Firmware */
			uint8_t scfw[EB_ALIGN(1024)];
		} nordic;
		uint8_t nordic_area[ACCESS_SIZE(struct suit_storage_area_nordic)];
	};
};

struct suit_storage_rad {
	/* Area accessible by the RADIO core debugger. */
	union {
		struct suit_storage_area_rad {
			/* Radio manifests provisioning information. */
			struct suit_storage_mpi_rad mpi;

			/* Reserved for Future Use. */
			uint8_t reserved[848];

			/* Installed envelope: Recovery */
			uint8_t recovery[EB_ALIGN(1024)];
			/* Installed envelope: Local A */
			uint8_t local_a[EB_ALIGN(1024)];
			/* Installed envelope: Local B */
			uint8_t local_b[EB_ALIGN(1024)];
		} rad;
		uint8_t rad_area[ACCESS_SIZE(struct suit_storage_area_rad)];
	};
};

struct suit_storage_app {
	/* Area accessible by the APP core debugger. */
	union {
		struct suit_storage_area_app {
			/* Application manifests provisioning information. */
			struct suit_storage_mpi_app mpi;

			/* Reserved for Future Use. */
			uint8_t reserved[560];

			/** Update candidate information. */
			uint8_t update_cand[EB_SIZE(struct update_candidate_info)];

			/* Manifest Runtime Non Volatile Variables Area */
			struct suit_storage_nvv nvv;
			struct suit_storage_nvv nvv_bak;

			/* Installed envelope: ROOT */
			uint8_t root[EB_ALIGN(2048)];
			/* Installed envelope: Recovery */
			uint8_t recovery[EB_ALIGN(2048)];
			/* Installed envelope: Local A */
			uint8_t local_a[EB_ALIGN(1024)];
			/* Installed envelope: Local B */
			uint8_t local_b[EB_ALIGN(1024)];
			/* Installed envelope: Local Spare RFU */
			uint8_t local_spare[EB_ALIGN(1024)];
		} app;
		uint8_t app_area[ACCESS_SIZE(struct suit_storage_area_app)];
	};
};

static const suit_storage_mpi_t mpi_nordic[] = {
	{
		.version = SUIT_MPI_INFO_VERSION,
		.downgrade_prevention_policy = SUIT_MPI_DOWNGRADE_PREVENTION_ENABLED,
		.independent_updateability_policy = SUIT_MPI_INDEPENDENT_UPDATE_ALLOWED,
		.signature_verification_policy =
			SUIT_MPI_SIGNATURE_CHECK_ENABLED_ON_UPDATE_AND_BOOT,
		.reserved = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			     0xFF},
		/* RFC4122 uuid5(uuid.NAMESPACE_DNS, 'nordicsemi.com') */
		.vendor_id = {0x76, 0x17, 0xda, 0xa5, 0x71, 0xfd, 0x5a, 0x85, 0x8f, 0x94, 0xe2,
			      0x8d, 0x73, 0x5c, 0xe9, 0xf4},
		/* RFC4122 uuid5(nordic_vid, 'nRF54H20_nordic_top') */
		.class_id = {0xf0, 0x3d, 0x38, 0x5e, 0xa7, 0x31, 0x56, 0x05, 0xb1, 0x5d, 0x03, 0x7f,
			     0x6d, 0xa6, 0x09, 0x7f},
	},
	{
		.version = SUIT_MPI_INFO_VERSION,
		.downgrade_prevention_policy = SUIT_MPI_DOWNGRADE_PREVENTION_ENABLED,
		.independent_updateability_policy = SUIT_MPI_INDEPENDENT_UPDATE_DENIED,
		.signature_verification_policy =
			SUIT_MPI_SIGNATURE_CHECK_ENABLED_ON_UPDATE_AND_BOOT,
		.reserved = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			     0xFF},
		/* RFC4122 uuid5(uuid.NAMESPACE_DNS, 'nordicsemi.com') */
		.vendor_id = {0x76, 0x17, 0xda, 0xa5, 0x71, 0xfd, 0x5a, 0x85, 0x8f, 0x94, 0xe2,
			      0x8d, 0x73, 0x5c, 0xe9, 0xf4},
		/* RFC4122 uuid5(nordic_vid, 'nRF54H20_sec') */
		.class_id = {0xd9, 0x6b, 0x40, 0xb7, 0x09, 0x2b, 0x5c, 0xd1, 0xa5, 0x9f, 0x9a, 0xf8,
			     0x0c, 0x33, 0x7e, 0xba},
	},
	{
		.version = SUIT_MPI_INFO_VERSION,
		.downgrade_prevention_policy = SUIT_MPI_DOWNGRADE_PREVENTION_ENABLED,
		.independent_updateability_policy = SUIT_MPI_INDEPENDENT_UPDATE_DENIED,
		.signature_verification_policy =
			SUIT_MPI_SIGNATURE_CHECK_ENABLED_ON_UPDATE_AND_BOOT,
		.reserved = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			     0xFF},
		/* RFC4122 uuid5(uuid.NAMESPACE_DNS, 'nordicsemi.com') */
		.vendor_id = {0x76, 0x17, 0xda, 0xa5, 0x71, 0xfd, 0x5a, 0x85, 0x8f, 0x94, 0xe2,
			      0x8d, 0x73, 0x5c, 0xe9, 0xf4},
		/* RFC4122 uuid5(nordic_vid, 'nRF54H20_sys') */
		.class_id = {0xc0, 0x8a, 0x25, 0xd7, 0x35, 0xe6, 0x59, 0x2c, 0xb7, 0xad, 0x43, 0xac,
			     0xc8, 0xd1, 0xd1, 0xc8},
	}};

static suit_plat_err_t find_manifest_area(suit_manifest_role_t role, const uint8_t **addr,
					  size_t *size)
{
	struct suit_storage_nordic *nordic_storage =
		(struct suit_storage_nordic *)SUIT_STORAGE_NORDIC_ADDRESS;
	struct suit_storage_rad *rad_storage = (struct suit_storage_rad *)SUIT_STORAGE_RAD_ADDRESS;
	struct suit_storage_app *app_storage = (struct suit_storage_app *)SUIT_STORAGE_APP_ADDRESS;

	switch (role) {
	case SUIT_MANIFEST_SEC_TOP:
		*addr = nordic_storage->nordic.top;
		*size = sizeof(nordic_storage->nordic.top);
		break;
	case SUIT_MANIFEST_SEC_SDFW:
		*addr = nordic_storage->nordic.sdfw;
		*size = sizeof(nordic_storage->nordic.sdfw);
		break;
	case SUIT_MANIFEST_SEC_SYSCTRL:
		*addr = nordic_storage->nordic.scfw;
		*size = sizeof(nordic_storage->nordic.scfw);
		break;
	case SUIT_MANIFEST_RAD_RECOVERY:
		*addr = rad_storage->rad.recovery;
		*size = sizeof(rad_storage->rad.recovery);
		break;
	case SUIT_MANIFEST_RAD_LOCAL_1:
		*addr = rad_storage->rad.local_a;
		*size = sizeof(rad_storage->rad.local_a);
		break;
	case SUIT_MANIFEST_RAD_LOCAL_2:
		*addr = rad_storage->rad.local_b;
		*size = sizeof(rad_storage->rad.local_b);
		break;
	case SUIT_MANIFEST_APP_ROOT:
		*addr = app_storage->app.root;
		*size = sizeof(app_storage->app.root);
		break;
	case SUIT_MANIFEST_APP_RECOVERY:
		*addr = app_storage->app.recovery;
		*size = sizeof(app_storage->app.recovery);
		break;
	case SUIT_MANIFEST_APP_LOCAL_1:
		*addr = app_storage->app.local_a;
		*size = sizeof(app_storage->app.local_a);
		break;
	case SUIT_MANIFEST_APP_LOCAL_2:
		*addr = app_storage->app.local_b;
		*size = sizeof(app_storage->app.local_b);
		break;
	case SUIT_MANIFEST_APP_LOCAL_3:
		*addr = app_storage->app.local_spare;
		*size = sizeof(app_storage->app.local_spare);
		break;
	default:
		return SUIT_PLAT_ERR_OUT_OF_BOUNDS;
	}

	return SUIT_PLAT_SUCCESS;
}

/** @brief Verify digest of the area.
 *
 * @param[in]  addr    Address of the area to hash.
 * @param[in]  size    Size of the area to hash.
 * @param[in]  digest  Pointer to the structure with the digest value.
 *
 * @retval SUIT_PLAT_SUCCESS             if the digest was successfully verified.
 * @retval SUIT_PLAT_ERR_INVAL           if one of the input arguments is not correct (i.e. NULL).
 * @retval SUIT_PLAT_ERR_AUTHENTICATION  if the digest value does not match with the area contents.
 * @retval SUIT_PLAT_ERR_CRASH           if failed to calculate digest value.
 */
static suit_plat_err_t sha256_check(const uint8_t *addr, size_t size,
				    const suit_storage_digest_t *exp_digest)
{
	const psa_algorithm_t psa_alg = PSA_ALG_SHA_256;
	const size_t exp_digest_length = PSA_HASH_LENGTH(psa_alg);
	suit_plat_err_t err = SUIT_PLAT_ERR_AUTHENTICATION;
	psa_hash_operation_t operation = {0};

	if ((addr == NULL) || (exp_digest == NULL)) {
		LOG_ERR("Invalid argument");
		return SUIT_PLAT_ERR_INVAL;
	}

	if (exp_digest_length != sizeof(*exp_digest)) {
		LOG_ERR("Invalid digest algorithm");
		return SUIT_PLAT_ERR_INVAL;
	}

	psa_status_t status = psa_crypto_init();

	if (status != PSA_SUCCESS) {
		LOG_ERR("Failed to init psa crypto: %d", status);
		return SUIT_PLAT_ERR_CRASH;
	}

	status = psa_hash_setup(&operation, psa_alg);
	if (status != PSA_SUCCESS) {
		LOG_ERR("Failed to setup hash algorithm: %d", status);
		return SUIT_PLAT_ERR_CRASH;
	}

	status = psa_hash_update(&operation, addr, size);
	if (status != PSA_SUCCESS) {
		LOG_ERR("Failed to calculate hash value: %d", status);
		return SUIT_PLAT_ERR_CRASH;
	}

	status = psa_hash_verify(&operation, *exp_digest, exp_digest_length);
	if (status == PSA_SUCCESS) {
		/* Digest calculation successful; expected digest matches calculated one */
		err = SUIT_PLAT_SUCCESS;
	} else {
		if (status == PSA_ERROR_INVALID_SIGNATURE) {
			/* Digest calculation successful but expected digest does not match
			 * calculated one
			 */
			err = SUIT_PLAT_ERR_AUTHENTICATION;
		} else {
			LOG_ERR("psa_hash_verify error: %d", status);
			err = SUIT_PLAT_ERR_CRASH;
		}
		/* In both cases psa_hash_verify enters error state and must be aborted */
		status = psa_hash_abort(&operation);
		if (status != PSA_SUCCESS) {
			LOG_ERR("psa_hash_abort error %d", status);
			if (err == SUIT_PLAT_SUCCESS) {
				err = SUIT_PLAT_ERR_CRASH;
			}
		}
	}

	return err;
}

/** @brief Calculate digest of the area.
 *
 * @param[in]   addr    Address of the area to hash.
 * @param[in]   size    Size of the area to hash.
 * @param[out]  digest  Pointer to the output structure to store the digest value.
 *
 * @retval SUIT_PLAT_SUCCESS    if the digest was successfully calculated.
 * @retval SUIT_PLAT_ERR_INVAL  if one of the input arguments is not correct (i.e. NULL).
 * @retval SUIT_PLAT_ERR_CRASH  if failed to calculate digest value.
 */
static suit_plat_err_t sha256_get(const uint8_t *addr, size_t size, suit_storage_digest_t *digest)
{
	const psa_algorithm_t psa_alg = PSA_ALG_SHA_256;
	size_t digest_length = PSA_HASH_LENGTH(psa_alg);
	suit_plat_err_t err = SUIT_PLAT_ERR_AUTHENTICATION;
	psa_hash_operation_t operation = {0};

	if ((addr == NULL) || (digest == NULL)) {
		LOG_ERR("Invalid argument");
		return SUIT_PLAT_ERR_INVAL;
	}

	if (digest_length != sizeof(*digest)) {
		LOG_ERR("Invalid digest algorithm");
		return SUIT_PLAT_ERR_INVAL;
	}

	psa_status_t status = psa_crypto_init();

	if (status != PSA_SUCCESS) {
		LOG_ERR("Failed to init psa crypto: %d", status);
		return SUIT_PLAT_ERR_CRASH;
	}

	status = psa_hash_setup(&operation, psa_alg);
	if (status != PSA_SUCCESS) {
		LOG_ERR("Failed to setup hash algorithm: %d", status);
		return SUIT_PLAT_ERR_CRASH;
	}

	status = psa_hash_update(&operation, addr, size);
	if (status != PSA_SUCCESS) {
		LOG_ERR("Failed to calculate hash value: %d", status);
		return SUIT_PLAT_ERR_CRASH;
	}

	status = psa_hash_finish(&operation, *digest, digest_length, &digest_length);
	if (status == PSA_SUCCESS) {
		/* Digest calculation successful; expected digest matches calculated one */
		err = SUIT_PLAT_SUCCESS;
	} else {
		LOG_ERR("psa_hash_finish error: %d", status);
		err = SUIT_PLAT_ERR_CRASH;

		/* psa_hash_finish enters error state and must be aborted */
		status = psa_hash_abort(&operation);
		if (status != PSA_SUCCESS) {
			LOG_ERR("psa_hash_abort error %d", status);
		}
	}

	return err;
}

/** @brief Helper function to override destination area with source contents.
 *
 * @param[in]  dst_addr  Address of the destination area.
 * @param[in]  src_addr  Address of the source area.
 * @param[in]  size      Size of the area.
 *
 * @retval SUIT_PLAT_SUCCESS           if area is successfully copied.
 * @retval SUIT_PLAT_ERR_HW_NOT_READY  if the flash controller device is not available.
 * @retval SUIT_PLAT_ERR_IO            if unable to modify the destination area.
 */
static suit_plat_err_t flash_cpy(uint8_t *dst_addr, uint8_t *src_addr, size_t size)
{
	const struct device *fdev = SUIT_PLAT_INTERNAL_NVM_DEV;

	if (!device_is_ready(fdev)) {
		return SUIT_PLAT_ERR_HW_NOT_READY;
	}

	int err = flash_erase(fdev, suit_plat_mem_nvm_offset_get(dst_addr), size);

	if (err == 0) {
		err = flash_write(fdev, suit_plat_mem_nvm_offset_get(dst_addr), src_addr, size);
	}

	if (err == 0) {
		return SUIT_PLAT_SUCCESS;
	} else {
		return SUIT_PLAT_ERR_IO;
	}
}

/** @brief Select the digest-protected area with valid sigest.
 *
 * @param[in,out]  area_addr    Address of the area.
 * @param[in,out]  backup_addr  Address of the backup.
 * @param[in]      area_size    Size of the area.
 *
 * @retval NULL         if area and backup are invalid.
 * @retval area_addr    if area is valid.
 * @retval backup_addr  if area is invalid, but backup is valid.
 */
static uint8_t *digest_struct_find(uint8_t *area_addr, uint8_t *backup_addr, size_t area_size)
{
	const suit_storage_digest_t *area_digest = NULL;
	const suit_storage_digest_t *backup_digest = NULL;
	suit_plat_err_t area_status = SUIT_PLAT_ERR_CRASH;
	suit_plat_err_t backup_status = SUIT_PLAT_ERR_CRASH;

	if ((area_addr == NULL) || (backup_addr == NULL) || (area_size == 0) ||
	    (area_size != EB_ALIGN(area_size))) {
		LOG_ERR("Invalid argument");
		return NULL;
	}

	area_digest = (suit_storage_digest_t *)&area_addr[area_size];
	area_status = sha256_check(area_addr, area_size, area_digest);

	if (area_status == SUIT_PLAT_SUCCESS) {
		return area_addr;
	}

	backup_digest = (suit_storage_digest_t *)&backup_addr[area_size];
	backup_status = sha256_check(backup_addr, area_size, backup_digest);

	if (backup_status == SUIT_PLAT_SUCCESS) {
		return backup_addr;
	}

	return NULL;
}

/** @brief Validate the integrity of the digest-protected area in NVM.
 *
 * @details This function calculates and compares digests of the area and the assigned backup.
 *          It is assumed, that the digest value of type @ref suit_storage_digest_t is stored right
 *          after the data.
 *          If the area are valid, the backup is updated.
 *          If the area is invalid, but a valid backup is found, the backup is copied into the area.
 *          If backup is invalid, but the area is valid, the area is copied into the backup.
 *          If both the area and the backup are invalid, this function will fail.
 *
 * @param[in,out]  area_addr    Address of the area.
 * @param[in,out]  backup_addr  Address of the backup.
 * @param[in]      area_size    Size of the area.
 *
 * @retval SUIT_PLAT_SUCCESS             if area is successfully verified or fixed.
 * @retval SUIT_PLAT_ERR_INVAL           if one of the input arguments is not correct (i.e. NULL).
 * @retval SUIT_PLAT_ERR_HW_NOT_READY    if the flash controller device is not available.
 * @retval SUIT_PLAT_ERR_IO              if unable to modify the area or the backup using flash
 *                                       controller device.
 * @retval SUIT_PLAT_ERR_CRASH           if failed to calculate digest value.
 * @retval SUIT_PLAT_ERR_AUTHENTICATION  if both the area and the backup are invalid.
 */
static suit_plat_err_t digest_struct_validate(uint8_t *area_addr, uint8_t *backup_addr,
					      size_t area_size)
{
	const size_t area_with_digest_size = area_size + EB_SIZE(suit_storage_digest_t);
	const suit_storage_digest_t *area_digest = NULL;
	const suit_storage_digest_t *backup_digest = NULL;
	suit_plat_err_t area_status = SUIT_PLAT_ERR_CRASH;
	suit_plat_err_t backup_status = SUIT_PLAT_ERR_CRASH;

	if ((area_addr == NULL) || (backup_addr == NULL) || (area_size == 0) ||
	    (area_size != EB_ALIGN(area_size))) {
		LOG_ERR("Invalid argument");
		return SUIT_PLAT_ERR_INVAL;
	}

	area_digest = (suit_storage_digest_t *)&area_addr[area_size];
	backup_digest = (suit_storage_digest_t *)&backup_addr[area_size];

	area_status = sha256_check(area_addr, area_size, area_digest);
	backup_status = sha256_check(backup_addr, area_size, backup_digest);

	if (area_status == SUIT_PLAT_SUCCESS) {
		if (backup_status == SUIT_PLAT_SUCCESS) {
			if (memcmp(area_digest, backup_digest, sizeof(suit_storage_digest_t)) ==
			    0) {
				return SUIT_PLAT_SUCCESS;
			}
		}

		/* We have a valid entry and backup digest does not match or differ. */
		LOG_INF("Backup area 0x%lx -> 0x%lx", (uintptr_t)area_addr, (uintptr_t)backup_addr);

		return flash_cpy(backup_addr, area_addr, area_with_digest_size);
	} else if (backup_status == SUIT_PLAT_SUCCESS) {
		/* Regular entry broken - restore from backup. */
		LOG_INF("Use backup 0x%lx -> 0x%lx", (uintptr_t)backup_addr, (uintptr_t)area_addr);

		return flash_cpy(area_addr, backup_addr, area_with_digest_size);
	}

	/* Both regular and backup entries broken - fail. */
	return SUIT_PLAT_ERR_AUTHENTICATION;
}

/** @brief Update the area digest value and create a new backup of the digest-protected area in NVM.
 *
 * @param[in,out]  area_addr    Address of the area.
 * @param[in,out]  backup_addr  Address of the backup.
 * @param[in]      area_size    Size of the area.
 *
 * @retval SUIT_PLAT_SUCCESS           if area is successfully updated.
 * @retval SUIT_PLAT_ERR_INVAL         if one of the input arguments is not correct (i.e. NULL).
 * @retval SUIT_PLAT_ERR_HW_NOT_READY  if the flash controller device is not available.
 * @retval SUIT_PLAT_ERR_IO            if unable to modify the area or the backup using flash
 *                                     controller device.
 * @retval SUIT_PLAT_ERR_CRASH         if both the area and the backup are invalid.
 */
static suit_plat_err_t digest_struct_commit(uint8_t *area_addr, uint8_t *backup_addr,
					    size_t area_size)
{
	const struct device *fdev = SUIT_PLAT_INTERNAL_NVM_DEV;
	const suit_storage_digest_t *area_digest = NULL;
	suit_storage_digest_t digest;

	if ((area_addr == NULL) || (backup_addr == NULL) || (area_size == 0) ||
	    (area_size != EB_ALIGN(area_size))) {
		LOG_ERR("Invalid argument");
		return SUIT_PLAT_ERR_INVAL;
	}

	area_digest = (suit_storage_digest_t *)&area_addr[area_size];

	if (!device_is_ready(fdev)) {
		fdev = NULL;
		return SUIT_PLAT_ERR_HW_NOT_READY;
	}

	suit_plat_err_t ret = sha256_get(area_addr, area_size, &digest);

	if (ret != SUIT_PLAT_SUCCESS) {
		return ret;
	}

	/* Override regular entry. */
	int err = flash_erase(fdev, suit_plat_mem_nvm_offset_get((uint8_t *)(*area_digest)),
			      EB_SIZE(suit_storage_digest_t));
	if (err != 0) {
		return SUIT_PLAT_ERR_IO;
	}

	err = flash_write(fdev, suit_plat_mem_nvm_offset_get((uint8_t *)(*area_digest)), digest,
			  sizeof(digest));
	if (err != 0) {
		return SUIT_PLAT_ERR_IO;
	}

	/* Verify the new area value and backup if digest is correct. */
	err = digest_struct_validate(area_addr, backup_addr, area_size);
	if (err == SUIT_PLAT_ERR_AUTHENTICATION) {
		err = SUIT_PLAT_ERR_CRASH;
	}

	return err;
}

/** @brief Initialize NVV area with default values.
 *
 * @details Ideally, the SUIT storage NVV module should place the default values (0xFF) and this
 *          function should append the correct digest at the end of it.
 *
 * @param[out]  area_addr    Address of the NVV area to initialize.
 * @param[out]  backup_addr  Address of the NVV backup area to initialize.
 *
 * @retval SUIT_PLAT_SUCCESS           if area is successfully initialized.
 * @retval SUIT_PLAT_ERR_INVAL         if one of the input arguments is not correct (i.e. NULL).
 * @retval SUIT_PLAT_ERR_HW_NOT_READY  if the flash controller device is not available.
 * @retval SUIT_PLAT_ERR_CRASH         if failed to calculate digest value.
 * @retval SUIT_PLAT_ERR_IO            if unable to modify the area using flash controller device.
 */
static suit_plat_err_t nvv_init(uint8_t *area_addr, uint8_t *backup_addr)
{
	size_t area_size = offsetof(struct suit_storage_nvv, digest);
	suit_plat_err_t ret = suit_storage_nvv_erase(area_addr, area_size);

	if (ret != SUIT_PLAT_SUCCESS) {
		return ret;
	}

	/* Update digest and backup. */
	return digest_struct_commit(area_addr, backup_addr, area_size);
}

static suit_plat_err_t find_mpi_area(suit_manifest_role_t role, uint8_t **addr, size_t *size)
{
	struct suit_storage_nordic *nordic_storage =
		(struct suit_storage_nordic *)SUIT_STORAGE_NORDIC_ADDRESS;
	struct suit_storage_rad *rad_storage = (struct suit_storage_rad *)SUIT_STORAGE_RAD_ADDRESS;
	struct suit_storage_app *app_storage = (struct suit_storage_app *)SUIT_STORAGE_APP_ADDRESS;
	struct suit_storage_mpi_app *app_mpi = (struct suit_storage_mpi_app *)digest_struct_find(
		(uint8_t *)&app_storage->app.mpi, (uint8_t *)&nordic_storage->nordic.app_mpi_bak,
		offsetof(struct suit_storage_mpi_app, digest));
	struct suit_storage_mpi_rad *rad_mpi = (struct suit_storage_mpi_rad *)digest_struct_find(
		(uint8_t *)&rad_storage->rad.mpi, (uint8_t *)&nordic_storage->nordic.rad_mpi_bak,
		offsetof(struct suit_storage_mpi_rad, digest));

	switch (role) {
	case SUIT_MANIFEST_SEC_TOP:
		*addr = (uint8_t *)&mpi_nordic[0];
		*size = sizeof(mpi_nordic[0]);
		break;
	case SUIT_MANIFEST_SEC_SDFW:
		*addr = (uint8_t *)&mpi_nordic[1];
		*size = sizeof(mpi_nordic[1]);
		break;
	case SUIT_MANIFEST_SEC_SYSCTRL:
		*addr = (uint8_t *)&mpi_nordic[2];
		*size = sizeof(mpi_nordic[2]);
		break;
	case SUIT_MANIFEST_RAD_RECOVERY:
		if (rad_mpi == NULL) {
			return SUIT_PLAT_ERR_NOT_FOUND;
		}
		*addr = rad_mpi->recovery;
		*size = sizeof(rad_mpi->recovery);
		break;
	case SUIT_MANIFEST_RAD_LOCAL_1:
		if (rad_mpi == NULL) {
			return SUIT_PLAT_ERR_NOT_FOUND;
		}
		*addr = rad_mpi->local_a;
		*size = sizeof(rad_mpi->local_a);
		break;
	case SUIT_MANIFEST_RAD_LOCAL_2:
		if (rad_mpi == NULL) {
			return SUIT_PLAT_ERR_NOT_FOUND;
		}
		*addr = rad_mpi->local_b;
		*size = sizeof(rad_mpi->local_b);
		break;
	case SUIT_MANIFEST_APP_ROOT:
		if (app_mpi == NULL) {
			return SUIT_PLAT_ERR_NOT_FOUND;
		}
		*addr = app_mpi->root;
		*size = sizeof(app_mpi->root);
		break;
	case SUIT_MANIFEST_APP_RECOVERY:
		if (app_mpi == NULL) {
			return SUIT_PLAT_ERR_NOT_FOUND;
		}
		*addr = app_mpi->recovery;
		*size = sizeof(app_mpi->recovery);
		break;
	case SUIT_MANIFEST_APP_LOCAL_1:
		if (app_mpi == NULL) {
			return SUIT_PLAT_ERR_NOT_FOUND;
		}
		*addr = app_mpi->local_a;
		*size = sizeof(app_mpi->local_a);
		break;
	case SUIT_MANIFEST_APP_LOCAL_2:
		if (app_mpi == NULL) {
			return SUIT_PLAT_ERR_NOT_FOUND;
		}
		*addr = app_mpi->local_b;
		*size = sizeof(app_mpi->local_b);
		break;
	case SUIT_MANIFEST_APP_LOCAL_3:
		if (app_mpi == NULL) {
			return SUIT_PLAT_ERR_NOT_FOUND;
		}
		*addr = app_mpi->local_spare;
		*size = sizeof(app_mpi->local_spare);
		break;
	default:
		return SUIT_PLAT_ERR_OUT_OF_BOUNDS;
	}

	return SUIT_PLAT_SUCCESS;
}

/** @brief Iterate through roles and try to initialize MPI areas.
 */
static suit_plat_err_t configure_manifests(suit_manifest_role_t *roles, size_t num_roles,
					   bool ignore_missing)
{
	suit_plat_err_t ret = SUIT_PLAT_SUCCESS;
	uint8_t *mpi_addr;
	size_t mpi_size;

	for (size_t i = 0; i < num_roles; i++) {
		ret = find_mpi_area(roles[i], &mpi_addr, &mpi_size);
		if (ret != SUIT_PLAT_SUCCESS) {
			LOG_ERR("Failed to locate MPI area for role 0x%x: %d", roles[i], ret);
			return ret;
		}

		ret = suit_storage_mpi_configuration_load(roles[i], mpi_addr, mpi_size);
		if (ret != SUIT_PLAT_SUCCESS) {
			if ((ret == SUIT_PLAT_ERR_NOT_FOUND) && (ignore_missing)) {
				LOG_INF("Skip MPI area for role 0x%x. Area load failed.", roles[i]);
				continue;
			} else {
				LOG_WRN("Failed to load MPI configuration for role 0x%x: %d",
					roles[i], ret);
				return ret;
			}
		}
	}

	return SUIT_PLAT_SUCCESS;
}

static suit_plat_err_t find_report_area(size_t index, uint8_t **addr, size_t *size)
{
	struct suit_storage_nordic *nordic_storage =
		(struct suit_storage_nordic *)SUIT_STORAGE_NORDIC_ADDRESS;

	switch (index) {
	case 0:
		*addr = nordic_storage->nordic.report_0;
		*size = ARRAY_SIZE(nordic_storage->nordic.report_0);
		break;
	default:
		return SUIT_PLAT_ERR_OUT_OF_BOUNDS;
	}

	return SUIT_PLAT_SUCCESS;
}

suit_plat_err_t suit_storage_init(void)
{
	struct suit_storage_nordic *nordic_storage =
		(struct suit_storage_nordic *)SUIT_STORAGE_NORDIC_ADDRESS;
	struct suit_storage_rad *rad_storage = (struct suit_storage_rad *)SUIT_STORAGE_RAD_ADDRESS;
	struct suit_storage_app *app_storage = (struct suit_storage_app *)SUIT_STORAGE_APP_ADDRESS;
	suit_plat_err_t ret = SUIT_PLAT_SUCCESS;
	suit_manifest_role_t nordic_roles[] = {
		SUIT_MANIFEST_SEC_TOP,
		SUIT_MANIFEST_SEC_SDFW,
		SUIT_MANIFEST_SEC_SYSCTRL,
	};

	if ((sizeof(struct suit_storage_nordic) > SUIT_STORAGE_NORDIC_SIZE) ||
	    (sizeof(struct suit_storage_rad) > SUIT_STORAGE_RAD_SIZE) ||
	    (sizeof(struct suit_storage_app) > SUIT_STORAGE_APP_SIZE)) {
		return SUIT_PLAT_ERR_NOMEM;
	}

	if ((EB_ALIGN(SUIT_STORAGE_NORDIC_SIZE) != SUIT_STORAGE_NORDIC_SIZE) ||
	    (EB_ALIGN(SUIT_STORAGE_RAD_SIZE) != SUIT_STORAGE_RAD_SIZE) ||
	    (EB_ALIGN(SUIT_STORAGE_APP_SIZE) != SUIT_STORAGE_APP_SIZE)) {
		return SUIT_PLAT_ERR_CRASH;
	}

	ret = suit_storage_mpi_init();
	if (ret != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Failed to initialize MPI submodule: %d", ret);
		return ret;
	}

	ret = suit_storage_report_internal_init();
	if (ret != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Failed to initialize report submodule: %d", ret);
		return ret;
	}

	ret = suit_storage_nvv_init();
	if (ret != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Failed to init NVV: %d", ret);
		return ret;
	}

	ret = digest_struct_validate((uint8_t *)&app_storage->app.nvv,
				     (uint8_t *)&app_storage->app.nvv_bak,
				     offsetof(struct suit_storage_nvv, digest));
	if (ret == SUIT_PLAT_ERR_AUTHENTICATION) {
		LOG_WRN("Failed to verify NVV (%d), load default values", ret);
		ret = nvv_init((uint8_t *)&app_storage->app.nvv,
			       (uint8_t *)&app_storage->app.nvv_bak);
	}

	if (ret != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Failed to initialize NVV with default values: %d", ret);
		return ret;
	}

	/* Bootstrap Nordic MPI entries. */
	ret = configure_manifests(nordic_roles, ARRAY_SIZE(nordic_roles), false);
	if (ret != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Failed to initialize Nordic MPIs: %d", ret);
		return ret;
	}

	ret = digest_struct_validate((uint8_t *)&rad_storage->rad.mpi,
				     (uint8_t *)&nordic_storage->nordic.rad_mpi_bak,
				     offsetof(struct suit_storage_mpi_rad, digest));
	if (ret == SUIT_PLAT_SUCCESS) {
		suit_manifest_role_t roles[] = {
			SUIT_MANIFEST_RAD_RECOVERY,
			SUIT_MANIFEST_RAD_LOCAL_1,
			SUIT_MANIFEST_RAD_LOCAL_2,
		};

		ret = configure_manifests(roles, ARRAY_SIZE(roles), true);
		if (ret != SUIT_PLAT_SUCCESS) {
			LOG_ERR("Failed to initialize Radio MPIs: %d", ret);
			return ret;
		}
	} else {
		LOG_WRN("Failed to verify radio MPI: %d", ret);
	}

	ret = digest_struct_validate((uint8_t *)&app_storage->app.mpi,
				     (uint8_t *)&nordic_storage->nordic.app_mpi_bak,
				     offsetof(struct suit_storage_mpi_app, digest));
	if (ret == SUIT_PLAT_SUCCESS) {
		suit_manifest_role_t essential_roles[] = {
			SUIT_MANIFEST_APP_ROOT,
		};
		suit_manifest_role_t optional_roles[] = {
			SUIT_MANIFEST_APP_RECOVERY,
			SUIT_MANIFEST_APP_LOCAL_1,
			SUIT_MANIFEST_APP_LOCAL_2,
			SUIT_MANIFEST_APP_LOCAL_3,
		};

		ret = configure_manifests(essential_roles, ARRAY_SIZE(essential_roles), false);
		if (ret != SUIT_PLAT_SUCCESS) {
			LOG_ERR("Failed to initialize essential Application MPIs: %d", ret);
			return ret;
		}

		ret = configure_manifests(optional_roles, ARRAY_SIZE(optional_roles), true);
		if (ret != SUIT_PLAT_SUCCESS) {
			LOG_ERR("Failed to initialize non-essential Application MPIs: %d", ret);
			return ret;
		}
	} else {
		LOG_ERR("Failed to verify application MPI: %d", ret);
		/* Lack of application MPI means lack of ROOT as well as recovery class IDs.
		 * In such case, the system is unable to boot anything except manifests controlled
		 * by Nordic.
		 */
		return ret;
	}

	return SUIT_PLAT_SUCCESS;
}

suit_plat_err_t suit_storage_update_cand_get(const suit_plat_mreg_t **regions, size_t *len)
{
	struct suit_storage_app *app_storage = (struct suit_storage_app *)SUIT_STORAGE_APP_ADDRESS;
	uint8_t *addr = app_storage->app.update_cand;
	size_t size = sizeof(app_storage->app.update_cand);

	return suit_storage_update_get(addr, size, regions, len);
}

suit_plat_err_t suit_storage_update_cand_set(suit_plat_mreg_t *regions, size_t len)
{
	struct suit_storage_app *app_storage = (struct suit_storage_app *)SUIT_STORAGE_APP_ADDRESS;
	uint8_t *addr = app_storage->app.update_cand;
	size_t size = sizeof(app_storage->app.update_cand);

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

suit_plat_err_t suit_storage_var_get(size_t index, uint32_t *value)
{
	struct suit_storage_app *app_storage = (struct suit_storage_app *)SUIT_STORAGE_APP_ADDRESS;
	size_t area_size = offsetof(struct suit_storage_nvv, digest);
	uint8_t *area_addr = (uint8_t *)digest_struct_find(
		(uint8_t *)&app_storage->app.nvv, (uint8_t *)&app_storage->app.nvv_bak,
		offsetof(struct suit_storage_nvv, digest));

	return suit_storage_nvv_get(area_addr, area_size, index, value);
}

suit_plat_err_t suit_storage_var_set(size_t index, uint32_t value)
{
	struct suit_storage_app *app_storage = (struct suit_storage_app *)SUIT_STORAGE_APP_ADDRESS;
	size_t area_size = offsetof(struct suit_storage_nvv, digest);
	uint8_t *backup_addr = (uint8_t *)&app_storage->app.nvv_bak;
	uint8_t *area_addr =
		(uint8_t *)digest_struct_find((uint8_t *)&app_storage->app.nvv, backup_addr,
					      offsetof(struct suit_storage_nvv, digest));

	/* It is not allowed to update NVVs if the area has incorrect digest. */
	if (area_addr == backup_addr) {
		return SUIT_PLAT_ERR_IO;
	}

	suit_plat_err_t err = suit_storage_nvv_set(area_addr, area_size, index, value);

	if (err != SUIT_PLAT_SUCCESS) {
		LOG_INF("Failed to update NVV at index %d", index);
		return err;
	}

	/* Update digest and backup. */
	return digest_struct_commit(area_addr, backup_addr, area_size);
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
