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

ZTEST_SUITE(suit_storage_nrf54h20_init_tests, NULL, NULL, NULL, NULL, NULL);

ZTEST(suit_storage_nrf54h20_init_tests, test_empty_storage)
{
	/* GIVEN the whole SUIT storage area is erased (unprovisioned device) */
	erase_area_nordic();
	erase_area_rad();
	erase_area_app();

	/* WHEN storage module is initialized */
	int err = suit_storage_init();

	/* THEN digest of the application MPI and it's backup does not match... */
	int exp_err = SUIT_PLAT_ERR_AUTHENTICATION;
	/* ... and an error code is returned */
	zassert_equal(err, exp_err, "Unexpected error code returned (%d).", err);
	/* ... and NVV area digest does not match */
	/* ... and NVV area is initialized with default values (0xFF) and digest */
	zassert_mem_equal(SUIT_STORAGE_APP_NVV_ADDRESS, nvv_empty, SUIT_STORAGE_APP_NVV_SIZE / 2);
	/* ... and NVV area is copied into NVV backup area */
	zassert_mem_equal(SUIT_STORAGE_APP_NVV_ADDRESS + SUIT_STORAGE_APP_NVV_SIZE / 2, nvv_empty,
			  SUIT_STORAGE_APP_NVV_SIZE / 2);
	/* ... and Nordic class IDs are supported */
	assert_nordic_classes();
}

ZTEST(suit_storage_nrf54h20_init_tests, test_empty_app_rad_with_digest)
{
	/* GIVEN the device is provisioned with empty application and radio MPI */
	erase_area_nordic();
	erase_area_rad();
	erase_area_app();
	write_empty_area_app();
	write_empty_area_rad();

	/* WHEN storage module is initialized */
	int err = suit_storage_init();

	/* THEN digest of the application MPI matches... */
	/* ... and the application MPI is copied into application backup area */
	assert_empty_mpi_area_app(SUIT_STORAGE_APP_ADDRESS,
				  SUIT_STORAGE_APP_MPI_SIZE + SUIT_STORAGE_DIGEST_SIZE);
	assert_empty_mpi_area_app(SUIT_STORAGE_NORDIC_ADDRESS + SUIT_STORAGE_RAD_MPI_SIZE +
					  SUIT_STORAGE_DIGEST_SIZE,
				  SUIT_STORAGE_APP_MPI_SIZE + SUIT_STORAGE_DIGEST_SIZE);
	/* ... and digest of the radio MPI matches... */
	/* ... and the radio MPI is copied into radio backup area */
	assert_empty_mpi_area_rad(SUIT_STORAGE_RAD_ADDRESS,
				  SUIT_STORAGE_RAD_MPI_SIZE + SUIT_STORAGE_DIGEST_SIZE);
	assert_empty_mpi_area_rad(SUIT_STORAGE_NORDIC_ADDRESS,
				  SUIT_STORAGE_RAD_MPI_SIZE + SUIT_STORAGE_DIGEST_SIZE);
	/* ... and parsing of the root MPI entry fails */
	int exp_err = SUIT_PLAT_ERR_NOT_FOUND;
	/* ... and an error code is returned */
	zassert_equal(err, exp_err, "Unexpected error code returned (%d).", err);
	/* ... and NVV area digest does not match */
	/* ... and NVV area is initialized with default values (0xFF) and digest */
	zassert_mem_equal(SUIT_STORAGE_APP_NVV_ADDRESS, nvv_empty, SUIT_STORAGE_APP_NVV_SIZE / 2);
	/* ... and NVV area is copied into NVV backup area */
	zassert_mem_equal(SUIT_STORAGE_APP_NVV_ADDRESS + SUIT_STORAGE_APP_NVV_SIZE / 2, nvv_empty,
			  SUIT_STORAGE_APP_NVV_SIZE / 2);
	/* ... and Nordic class IDs are supported */
	assert_nordic_classes();
}

ZTEST(suit_storage_nrf54h20_init_tests, test_with_root_rad)
{
	/* GIVEN the device is provisioned with application MPI with root config and sample radio
	 * MPI entry
	 */
	erase_area_nordic();
	erase_area_rad();
	erase_area_app();
	write_area_app_root();
	write_area_rad();

	/* WHEN storage module is initialized */
	int err = suit_storage_init();

	/* THEN digest of the application MPI matches... */
	/* ... and the application MPI is copied into application backup area */
	assert_valid_mpi_area_app(SUIT_STORAGE_APP_ADDRESS,
				  SUIT_STORAGE_APP_MPI_SIZE + SUIT_STORAGE_DIGEST_SIZE);
	assert_valid_mpi_area_app(SUIT_STORAGE_NORDIC_ADDRESS + SUIT_STORAGE_RAD_MPI_SIZE +
					  SUIT_STORAGE_DIGEST_SIZE,
				  SUIT_STORAGE_APP_MPI_SIZE + SUIT_STORAGE_DIGEST_SIZE);
	/* ... and digest of the radio MPI matches... */
	/* ... and the radio MPI is copied into radio backup area */
	assert_valid_mpi_area_rad(SUIT_STORAGE_RAD_ADDRESS,
				  SUIT_STORAGE_RAD_MPI_SIZE + SUIT_STORAGE_DIGEST_SIZE);
	assert_valid_mpi_area_rad(SUIT_STORAGE_NORDIC_ADDRESS,
				  SUIT_STORAGE_RAD_MPI_SIZE + SUIT_STORAGE_DIGEST_SIZE);
	/* ... and parsing of the root MPI succeeds */
	int exp_err = SUIT_PLAT_SUCCESS;
	/* ... and NVV area digest does not match */
	/* ... and NVV area is initialized with default values (0xFF) and digest */
	zassert_mem_equal(SUIT_STORAGE_APP_NVV_ADDRESS, nvv_empty, SUIT_STORAGE_APP_NVV_SIZE / 2);
	/* ... and NVV area is copied into NVV backup area */
	zassert_mem_equal(SUIT_STORAGE_APP_NVV_ADDRESS + SUIT_STORAGE_APP_NVV_SIZE / 2, nvv_empty,
			  SUIT_STORAGE_APP_NVV_SIZE / 2);
	/* ... and initialization succeeds */
	zassert_equal(err, exp_err, "Failed to initialize SUIT storage (%d).", err);
	/* ... and Nordic class IDs are supported */
	assert_nordic_classes();
	/* ... and sample root and radio classes are supported */
	assert_sample_root_rad_class();
}

ZTEST(suit_storage_nrf54h20_init_tests, test_with_root_rad_backup)
{
	/* GIVEN the device was provisioned with application and radio MPI with root and sample
	 * radio config
	 */
	erase_area_nordic();
	write_area_nordic_root();
	write_area_nordic_rad();
	/* .. and the application and radio area was erased after the backup was created */
	erase_area_rad();
	erase_area_app();

	/* WHEN storage module is initialized */
	int err = suit_storage_init();

	/* THEN digest of the application MPI does not match... */
	/* ... and the application MPI is copied from backup area */
	assert_valid_mpi_area_app(SUIT_STORAGE_APP_ADDRESS,
				  SUIT_STORAGE_APP_MPI_SIZE + SUIT_STORAGE_DIGEST_SIZE);
	assert_valid_mpi_area_app(SUIT_STORAGE_NORDIC_ADDRESS + SUIT_STORAGE_RAD_MPI_SIZE +
					  SUIT_STORAGE_DIGEST_SIZE,
				  SUIT_STORAGE_APP_MPI_SIZE + SUIT_STORAGE_DIGEST_SIZE);
	/* ... and digest of the radio MPI does not match... */
	/* ... and the radio MPI is copied from backup area */
	assert_valid_mpi_area_rad(SUIT_STORAGE_RAD_ADDRESS,
				  SUIT_STORAGE_RAD_MPI_SIZE + SUIT_STORAGE_DIGEST_SIZE);
	assert_valid_mpi_area_rad(SUIT_STORAGE_NORDIC_ADDRESS,
				  SUIT_STORAGE_RAD_MPI_SIZE + SUIT_STORAGE_DIGEST_SIZE);
	/* ... and parsing of the root MPI succeeds */
	int exp_err = SUIT_PLAT_SUCCESS;
	/* ... and NVV area digest does not match */
	/* ... and NVV area is initialized with default values (0xFF) and digest */
	zassert_mem_equal(SUIT_STORAGE_APP_NVV_ADDRESS, nvv_empty, SUIT_STORAGE_APP_NVV_SIZE / 2);
	/* ... and NVV area is copied into NVV backup area */
	zassert_mem_equal(SUIT_STORAGE_APP_NVV_ADDRESS + SUIT_STORAGE_APP_NVV_SIZE / 2, nvv_empty,
			  SUIT_STORAGE_APP_NVV_SIZE / 2);
	/* ... and initialization succeeds */
	zassert_equal(err, exp_err, "Failed to initialize SUIT storage (%d).", err);
	/* ... and Nordic class IDs are supported */
	assert_nordic_classes();
	/* ... and sample root and radio classes are supported */
	assert_sample_root_rad_class();
}

ZTEST(suit_storage_nrf54h20_init_tests, test_with_old_root_rad_backup)
{
	/* GIVEN the device was provisioned with application MPI with old root and sample radio
	 * config
	 */
	erase_area_nordic();
	write_area_nordic_old_root();
	write_area_nordic_old_rad();
	/* .. and the device is provisioned with new application MPI with root config without radio
	 * config
	 */
	erase_area_rad();
	erase_area_app();
	write_area_app_empty_nvv_backup();
	write_area_app_nvv();
	write_area_app_root();
	write_area_rad();

	/* WHEN storage module is initialized */
	int err = suit_storage_init();

	/* THEN digest of the application MPI matches... */
	/* ... and the application MPI is copied into application backup area */
	assert_valid_mpi_area_app(SUIT_STORAGE_APP_ADDRESS,
				  SUIT_STORAGE_APP_MPI_SIZE + SUIT_STORAGE_DIGEST_SIZE);
	assert_valid_mpi_area_app(SUIT_STORAGE_NORDIC_ADDRESS + SUIT_STORAGE_RAD_MPI_SIZE +
					  SUIT_STORAGE_DIGEST_SIZE,
				  SUIT_STORAGE_APP_MPI_SIZE + SUIT_STORAGE_DIGEST_SIZE);
	/* ... and digest of the radio MPI matches... */
	/* ... and the radio MPI is copied into radio backup area */
	assert_valid_mpi_area_rad(SUIT_STORAGE_RAD_ADDRESS,
				  SUIT_STORAGE_RAD_MPI_SIZE + SUIT_STORAGE_DIGEST_SIZE);
	assert_valid_mpi_area_rad(SUIT_STORAGE_NORDIC_ADDRESS,
				  SUIT_STORAGE_RAD_MPI_SIZE + SUIT_STORAGE_DIGEST_SIZE);
	/* ... and parsing of the root MPI succeeds */
	int exp_err = SUIT_PLAT_SUCCESS;
	/* ... and NVV area digest does matches */
	/* ... and NVV area is not modified */
	zassert_mem_equal(SUIT_STORAGE_APP_NVV_ADDRESS, nvv_sample, SUIT_STORAGE_APP_NVV_SIZE / 2);
	/* ... and NVV area backup is updated */
	zassert_mem_equal(SUIT_STORAGE_APP_NVV_ADDRESS + SUIT_STORAGE_APP_NVV_SIZE / 2, nvv_sample,
			  SUIT_STORAGE_APP_NVV_SIZE / 2);
	/* ... and initialization succeeds */
	zassert_equal(err, exp_err, "Failed to initialize SUIT storage (%d).", err);
	/* ... and Nordic class IDs are supported */
	assert_nordic_classes();
	/* ... and sample root and radio classes are supported */
	assert_sample_root_rad_class();
}

ZTEST(suit_storage_nrf54h20_init_tests, test_app_with_old_root_new_rad_nvv_backup)
{
	/* GIVEN the device was provisioned with application MPI with old root and sample radio
	 * config
	 */
	erase_area_nordic();
	write_area_nordic_old_root();
	write_area_nordic_rad();
	/* .. and the device is provisioned with new application MPI with root config without radio
	 * config
	 */
	erase_area_rad();
	erase_area_app();
	write_area_app_empty_nvv_backup();
	write_area_app_nvv();
	write_area_app_root();

	/* WHEN storage module is initialized */
	int err = suit_storage_init();

	/* THEN digest of the application MPI matches... */
	/* ... and the application MPI is copied into application backup area */
	assert_valid_mpi_area_app(SUIT_STORAGE_APP_ADDRESS,
				  SUIT_STORAGE_APP_MPI_SIZE + SUIT_STORAGE_DIGEST_SIZE);
	assert_valid_mpi_area_app(SUIT_STORAGE_NORDIC_ADDRESS + SUIT_STORAGE_RAD_MPI_SIZE +
					  SUIT_STORAGE_DIGEST_SIZE,
				  SUIT_STORAGE_APP_MPI_SIZE + SUIT_STORAGE_DIGEST_SIZE);
	/* ... and digest of the radio MPI does not match... */
	/* ... and the radio MPI is copied from backup area */
	assert_valid_mpi_area_rad(SUIT_STORAGE_RAD_ADDRESS,
				  SUIT_STORAGE_RAD_MPI_SIZE + SUIT_STORAGE_DIGEST_SIZE);
	assert_valid_mpi_area_rad(SUIT_STORAGE_NORDIC_ADDRESS,
				  SUIT_STORAGE_RAD_MPI_SIZE + SUIT_STORAGE_DIGEST_SIZE);
	/* ... and parsing of the root MPI succeeds */
	int exp_err = SUIT_PLAT_SUCCESS;
	/* ... and NVV area digest does matches */
	/* ... and NVV area is not modified */
	zassert_mem_equal(SUIT_STORAGE_APP_NVV_ADDRESS, nvv_sample, SUIT_STORAGE_APP_NVV_SIZE / 2);
	/* ... and NVV area backup is updated */
	zassert_mem_equal(SUIT_STORAGE_APP_NVV_ADDRESS + SUIT_STORAGE_APP_NVV_SIZE / 2, nvv_sample,
			  SUIT_STORAGE_APP_NVV_SIZE / 2);
	/* ... and initialization succeeds */
	zassert_equal(err, exp_err, "Failed to initialize SUIT storage (%d).", err);
	/* ... and Nordic class IDs are supported */
	assert_nordic_classes();
	/* ... and sample root and radio classes are supported */
	assert_sample_root_rad_class();
}

ZTEST(suit_storage_nrf54h20_init_tests, test_app_corrupted_nvv)
{
	/* GIVEN the device is provisioned with application MPI with root config */
	erase_area_nordic();
	write_area_nordic_root();
	/* .. and NVV area is erased */
	erase_area_rad();
	erase_area_app();
	write_area_app_root();
	/* .. and NVV backup is present */
	write_area_app_nvv_backup();

	/* WHEN storage module is initialized */
	int err = suit_storage_init();

	/* THEN digest of the application MPI matches... */
	/* ... and the application MPI is the same as application backup area */
	assert_valid_mpi_area_app(SUIT_STORAGE_APP_ADDRESS,
				  SUIT_STORAGE_APP_MPI_SIZE + SUIT_STORAGE_DIGEST_SIZE);
	assert_valid_mpi_area_app(SUIT_STORAGE_NORDIC_ADDRESS + SUIT_STORAGE_RAD_MPI_SIZE +
					  SUIT_STORAGE_DIGEST_SIZE,
				  SUIT_STORAGE_APP_MPI_SIZE + SUIT_STORAGE_DIGEST_SIZE);
	/* ... and parsing of the root MPI succeeds */
	int exp_err = SUIT_PLAT_SUCCESS;
	/* ... and NVV area digest does not match */
	/* ... and NVV area is recovered from backup */
	zassert_mem_equal(SUIT_STORAGE_APP_NVV_ADDRESS, nvv_sample, SUIT_STORAGE_APP_NVV_SIZE / 2);
	/* ... and NVV area backup is not modified */
	zassert_mem_equal(SUIT_STORAGE_APP_NVV_ADDRESS + SUIT_STORAGE_APP_NVV_SIZE / 2, nvv_sample,
			  SUIT_STORAGE_APP_NVV_SIZE / 2);
	/* ... and initialization succeeds */
	zassert_equal(err, exp_err, "Failed to initialize SUIT storage (%d).", err);
	/* ... and Nordic class IDs are supported */
	assert_nordic_classes();
	/* ... and sample root class is supported */
	assert_sample_root_class();
}

ZTEST(suit_storage_nrf54h20_init_tests, test_app_corrupted_nvv_backup)
{
	/* GIVEN the device is provisioned with application MPI with root config */
	erase_area_nordic();
	write_area_nordic_root();
	/* .. and NVV backup is erased */
	erase_area_rad();
	erase_area_app();
	write_area_app_root();
	/* .. and NVV is present */
	write_area_app_nvv();

	/* WHEN storage module is initialized */
	int err = suit_storage_init();

	/* THEN digest of the application MPI matches... */
	/* ... and the application MPI is the same as application backup area */
	assert_valid_mpi_area_app(SUIT_STORAGE_APP_ADDRESS,
				  SUIT_STORAGE_APP_MPI_SIZE + SUIT_STORAGE_DIGEST_SIZE);
	assert_valid_mpi_area_app(SUIT_STORAGE_NORDIC_ADDRESS + SUIT_STORAGE_RAD_MPI_SIZE +
					  SUIT_STORAGE_DIGEST_SIZE,
				  SUIT_STORAGE_APP_MPI_SIZE + SUIT_STORAGE_DIGEST_SIZE);
	/* ... and parsing of the root MPI succeeds */
	int exp_err = SUIT_PLAT_SUCCESS;
	/* ... and NVV area digest matches */
	/* ... and NVV area is not modified */
	zassert_mem_equal(SUIT_STORAGE_APP_NVV_ADDRESS, nvv_sample, SUIT_STORAGE_APP_NVV_SIZE / 2);
	/* ... and NVV area backup is updated */
	zassert_mem_equal(SUIT_STORAGE_APP_NVV_ADDRESS + SUIT_STORAGE_APP_NVV_SIZE / 2, nvv_sample,
			  SUIT_STORAGE_APP_NVV_SIZE / 2);
	/* ... and initialization succeeds */
	zassert_equal(err, exp_err, "Failed to initialize SUIT storage (%d).", err);
	/* ... and Nordic class IDs are supported */
	assert_nordic_classes();
	/* ... and sample root class is supported */
	assert_sample_root_class();
}
