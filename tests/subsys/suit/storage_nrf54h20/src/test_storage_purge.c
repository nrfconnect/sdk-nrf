/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <suit_storage.h>
#include <suit_storage_mpi.h>
#include <suit_storage_nvv.h>
#include "test_common.h"

ZTEST_SUITE(suit_storage_nrf54h20_bare_recovery_tests, NULL, NULL, NULL, NULL, NULL);

static void assert_app_purged(void)
{
	/* ASSERT that the application MPI area is purged */
	assert_purged_mpi_area_app(SUIT_STORAGE_APP_ADDRESS,
				   SUIT_STORAGE_APP_MPI_SIZE + SUIT_STORAGE_DIGEST_SIZE);
	/* ... and the application MPI backup is purged */
	assert_purged_mpi_area_app(SUIT_STORAGE_NORDIC_ADDRESS + SUIT_STORAGE_RAD_MPI_SIZE +
					   SUIT_STORAGE_DIGEST_SIZE,
				   SUIT_STORAGE_APP_MPI_SIZE + SUIT_STORAGE_DIGEST_SIZE);
	/* ... and the application reserved area as well as update candidate is purged */
	assert_purged_reserved_uci_area_app(SUIT_STORAGE_APP_ADDRESS + SUIT_STORAGE_APP_MPI_SIZE +
						    SUIT_STORAGE_DIGEST_SIZE,
					    SUIT_STORAGE_APP_RFU_SIZE + SUIT_STORAGE_APP_UCI_SIZE);

	/* ... and NVV area digest does not match */
	/* ... and NVV area is initialized with default values (0xFF) and digest */
	zassert_mem_equal(SUIT_STORAGE_APP_NVV_ADDRESS, nvv_empty, SUIT_STORAGE_APP_NVV_SIZE / 2);
	/* ... and NVV area is copied into NVV backup area */
	zassert_mem_equal(SUIT_STORAGE_APP_NVV_ADDRESS + SUIT_STORAGE_APP_NVV_SIZE / 2, nvv_empty,
			  SUIT_STORAGE_APP_NVV_SIZE / 2);
}

static void assert_rad_purged(void)
{
	/* ASSERT that the radio MPI area is purged */
	assert_purged_mpi_area_rad(SUIT_STORAGE_RAD_ADDRESS,
				   SUIT_STORAGE_RAD_MPI_SIZE + SUIT_STORAGE_DIGEST_SIZE);
	/* ... and the radio MPI backup is purged */
	assert_purged_mpi_area_rad(SUIT_STORAGE_NORDIC_ADDRESS,
				   SUIT_STORAGE_RAD_MPI_SIZE + SUIT_STORAGE_DIGEST_SIZE);
}

ZTEST(suit_storage_nrf54h20_bare_recovery_tests, test_storage_purge_populated_storage)
{
	suit_plat_mreg_t update_candidate[1] = {{
		.mem = (uint8_t *)0xCAFEFECA,
		.size = 2044,
	}};

	/* GIVEN the device was provisioned with application and radio MPI with root and sample
	 * radio config
	 */
	erase_area_nordic();
	write_area_nordic_root();
	write_area_nordic_rad();
	/* ... and the MPI area matches the MPI backups */
	erase_area_rad();
	erase_area_app();
	write_area_app_root();
	write_area_rad();
	/* ... and NVV is present */
	write_area_app_nvv();
	/* ... and NVV backup is present */
	write_area_app_nvv_backup();
	/* ... and the update candidate information is present */
	zassert_equal(suit_storage_update_cand_set(update_candidate, ARRAY_SIZE(update_candidate)),
		      SUIT_PLAT_SUCCESS, "Failed to set update candidate information");

	/* WHEN storage application area is purged on uninitialized module */
	int err = suit_storage_purge(SUIT_MANIFEST_DOMAIN_APP);

	/* THEN the purge operation succeeds... */
	zassert_equal(err, SUIT_PLAT_SUCCESS, "Failed to purge application area (%d).", err);

	/* ... and all fields assigned to the application domain are purged */
	assert_app_purged();
	/* ... and the root manifest is not provisioned */
	/* ... and the radio manifest is provisioned */
	assert_sample_rad_class();
	/* ... and digest of the radio MPI matches... */
	assert_valid_mpi_area_rad(SUIT_STORAGE_RAD_ADDRESS,
				  SUIT_STORAGE_RAD_MPI_SIZE + SUIT_STORAGE_DIGEST_SIZE);
	/* ... and the radio MPI backup is still present */
	assert_valid_mpi_area_rad(SUIT_STORAGE_NORDIC_ADDRESS,
				  SUIT_STORAGE_RAD_MPI_SIZE + SUIT_STORAGE_DIGEST_SIZE);
	/* ... and Nordic class IDs are supported */
	assert_nordic_classes();

	/* WHEN storage radio area is purged afterwards */
	err = suit_storage_purge(SUIT_MANIFEST_DOMAIN_RAD);

	/* THEN the purge operation succeeds... */
	zassert_equal(err, SUIT_PLAT_SUCCESS, "Failed to purge radio area (%d).", err);
	/* ... and all fields assigned to the application domain are still purged */
	assert_app_purged();
	/* ... and all fields assigned to the radio domain are purged */
	assert_rad_purged();
	/* ... and only Nordic class IDs are supported */
	assert_only_nordic_classes();
}

ZTEST(suit_storage_nrf54h20_bare_recovery_tests, test_storage_purge_populated_storage_rad_first)
{
	suit_plat_mreg_t update_candidate[1] = {{
		.mem = (uint8_t *)0xCAFEFECA,
		.size = 2044,
	}};

	/* GIVEN the device was provisioned with application and radio MPI with root and sample
	 * radio config
	 */
	erase_area_nordic();
	write_area_nordic_root();
	write_area_nordic_rad();
	/* ... and the MPI area matches the MPI backups */
	erase_area_rad();
	erase_area_app();
	write_area_app_root();
	write_area_rad();
	/* ... and NVV is present */
	write_area_app_nvv();
	/* ... and NVV backup is present */
	write_area_app_nvv_backup();
	/* ... and the update candidate information is present */
	zassert_equal(suit_storage_update_cand_set(update_candidate, ARRAY_SIZE(update_candidate)),
		      SUIT_PLAT_SUCCESS, "Failed to set update candidate information");

	/* WHEN storage radio area is purged */
	int err = suit_storage_purge(SUIT_MANIFEST_DOMAIN_RAD);

	/* THEN the purge operation succeeds... */
	zassert_equal(err, SUIT_PLAT_SUCCESS, "Failed to purge radio area (%d).", err);

	/* ... and all fields assigned to the radio domain are purged */
	assert_rad_purged();
	/* ... and the radio manifest is not provisioned */
	/* ... and the root manifest is provisioned */
	assert_sample_root_class();
	/* ... and digest of the application MPI matches... */
	assert_valid_mpi_area_app(SUIT_STORAGE_APP_ADDRESS,
				  SUIT_STORAGE_APP_MPI_SIZE + SUIT_STORAGE_DIGEST_SIZE);
	/* ... and the application MPI backup is still present */
	assert_valid_mpi_area_app(SUIT_STORAGE_NORDIC_ADDRESS + SUIT_STORAGE_RAD_MPI_SIZE +
					  SUIT_STORAGE_DIGEST_SIZE,
				  SUIT_STORAGE_APP_MPI_SIZE + SUIT_STORAGE_DIGEST_SIZE);

	/* ... and the update candidate information is still present */
	const suit_plat_mreg_t *update_regions = NULL;
	size_t update_regions_len = 0;

	err = suit_storage_update_cand_get(&update_regions, &update_regions_len);
	zassert_equal(err, SUIT_PLAT_SUCCESS,
		      "Update candidate purged as part of radio domain purge.");
	zassert_equal(update_regions_len, 1, "Length of update candidate regions is invalid (%d).",
		      update_regions_len);
	for (size_t i = 0; i < ARRAY_SIZE(update_candidate); i++) {
		zassert_equal(update_regions[i].mem, update_candidate[i].mem,
			      "Update region %d set with wrong address (%p != %p)", i,
			      update_regions[i].mem, update_candidate[i].mem);
		zassert_equal(update_regions[i].size, update_candidate[i].size,
			      "Update region %d set with wrong size (0x%x != 0x%x)", i,
			      update_regions[i].size, update_candidate[i].size);
	}

	/* ... and the NVVs are still present */
	uint8_t constants[SUIT_STORAGE_NVV_N_VARS] = {
		0x00, 0x00, 0x00, 0x00, 0xAA, 0xAA, 0xAA, 0xAA,
		0xAA, 0x55, 0xAA, 0x55, 0x00, 0x00, 0x00, 0x01,
		0x01, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
		0xAB, 0xCD, 0xEF, 0x98, 0xFF, 0xFF, 0xFF, 0xFF,
	};

	for (size_t i = 0; i < SUIT_STORAGE_NVV_N_VARS; i++) {
		uint8_t value;

		/* WHEN the NVV value is read */
		err = suit_storage_var_get(i, &value);
		/* THEN the NVV read succeeds */
		zassert_equal(err, SUIT_PLAT_SUCCESS, "Failed to read NVV at index: %d (%d).", i,
			      err);
		/* ... and values in NVV area matches the preconfigured constants */
		zassert_equal(value, constants[i], "Invalid NVV value at index: %d (0x%X != 0x%X).",
			      i, value, constants[i]);
	}

	/* ... and Nordic class IDs are supported */
	assert_nordic_classes();

	/* WHEN storage application area is purged afterwards */
	err = suit_storage_purge(SUIT_MANIFEST_DOMAIN_APP);

	/* THEN the purge operation succeeds... */
	zassert_equal(err, SUIT_PLAT_SUCCESS, "Failed to purge application area (%d).", err);
	/* ... and all fields assigned to the application domain are purged */
	assert_app_purged();
	/* ... and all fields assigned to the radio domain are still purged */
	assert_rad_purged();
	/* ... and only Nordic class IDs are supported */
	assert_only_nordic_classes();
}

ZTEST(suit_storage_nrf54h20_bare_recovery_tests, test_storage_purge_empty_storage)
{
	/* GIVEN storage area is not initialized */
	erase_area_nordic();
	erase_area_rad();
	erase_area_app();

	/* WHEN storage radio area is purged */
	int err = suit_storage_purge(SUIT_MANIFEST_DOMAIN_RAD);

	/* THEN the purge operation succeeds... */
	zassert_equal(err, SUIT_PLAT_SUCCESS, "Failed to purge radio area (%d).", err);

	/* ... and all fields assigned to the application domain are purged */
	assert_app_purged();
	/* ... and all fields assigned to the radio domain are purged */
	assert_rad_purged();
	/* ... and only Nordic class IDs are supported */
	assert_only_nordic_classes();

	/* WHEN storage application area is purged afterwards */
	err = suit_storage_purge(SUIT_MANIFEST_DOMAIN_APP);

	/* THEN the purge operation succeeds... */
	zassert_equal(err, SUIT_PLAT_SUCCESS, "Failed to purge application area (%d).", err);
	/* ... and all fields assigned to the application domain are still purged */
	assert_app_purged();
	/* ... and all fields assigned to the radio domain are still purged */
	assert_rad_purged();
	/* ... and only Nordic class IDs are supported */
	assert_only_nordic_classes();
}

ZTEST(suit_storage_nrf54h20_bare_recovery_tests, test_storage_purge_update_only)
{
	suit_plat_mreg_t update_candidate[1] = {{
		.mem = (uint8_t *)0xCAFEFECA,
		.size = 2044,
	}};

	/* GIVEN the storage contains only the update candidate information */
	erase_area_nordic();
	erase_area_rad();
	erase_area_app();
	zassert_equal(suit_storage_update_cand_set(update_candidate, ARRAY_SIZE(update_candidate)),
		      SUIT_PLAT_SUCCESS, "Failed to set update candidate information");

	/* WHEN storage application area is purged */
	int err = suit_storage_purge(SUIT_MANIFEST_DOMAIN_APP);

	/* THEN the purge operation succeeds... */
	zassert_equal(err, SUIT_PLAT_SUCCESS, "Failed to purge application area (%d).", err);
	/* ... and all fields assigned to the application domain are purged */
	assert_app_purged();
	/* ... and all fields assigned to the radio domain are purged */
	assert_rad_purged();
	/* ... and only Nordic class IDs are supported */
	assert_only_nordic_classes();
}
