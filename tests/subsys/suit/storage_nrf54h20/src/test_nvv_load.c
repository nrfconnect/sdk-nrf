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

ZTEST_SUITE(suit_storage_nrf54h20_nvv_tests, NULL, NULL, NULL, NULL, NULL);

ZTEST(suit_storage_nrf54h20_nvv_tests, test_app_nvv_read)
{
	/* GIVEN the device is provisioned with application MPI with root config */
	erase_area_nordic();
	write_area_nordic_root();
	erase_area_rad();
	erase_area_app();
	write_area_app_root();
	/* .. and NVV is present */
	write_area_app_nvv();
	/* .. and NVV backup is present */
	write_area_app_nvv_backup();

	/* ... and storage module is initialized */
	int err = suit_storage_init();

	/* ... and digest of the application MPI matches... */
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
	/* ... and NVV area backup is not modified */
	zassert_mem_equal(SUIT_STORAGE_APP_NVV_ADDRESS + SUIT_STORAGE_APP_NVV_SIZE / 2, nvv_sample,
			  SUIT_STORAGE_APP_NVV_SIZE / 2);
	/* ... and initialization succeeds */
	zassert_equal(err, exp_err, "Failed to initialize SUIT storage (%d).", err);
	/* ... and Nordic class IDs are supported */
	assert_nordic_classes();
	/* ... and sample root class is supported */
	assert_sample_root_class();

	uint32_t constants[SUIT_STORAGE_NVV_N_VARS] = {
		0x00000000, 0xAAAAAAAA, 0x55AA55AA, 0x01000000,
		0x00000001, 0xFFFFFFFF, 0x98EFCDAB, 0xFFFFFFFF,
	};
	for (size_t i = 0; i < SUIT_STORAGE_NVV_N_VARS; i++) {
		uint32_t value;

		/* WHEN the NVV value is read */
		err = suit_storage_var_get(i, &value);
		/* THEN the NVV read succeeds */
		zassert_equal(err, SUIT_PLAT_SUCCESS, "Failed to read NVV at index: %d (%d).", i,
			      err);
		/* ... and values in NVV area matches the preconfigured constants */
		zassert_equal(value, constants[i], "Invalid NVV value at index: %d (0x%X != 0x%X).",
			      i, value, constants[i]);
	}
}

ZTEST(suit_storage_nrf54h20_nvv_tests, test_app_nvv_write)
{
	/* GIVEN the device is provisioned with application MPI with root config */
	erase_area_nordic();
	write_area_nordic_root();
	/* .. and NVV area is erased */
	erase_area_rad();
	erase_area_app();
	write_area_app_root();

	/* ... and storage module is initialized */
	int err = suit_storage_init();

	/* ... and digest of the application MPI matches... */
	/* ... and the application MPI is the same as application backup area */
	assert_valid_mpi_area_app(SUIT_STORAGE_APP_ADDRESS,
				  SUIT_STORAGE_APP_MPI_SIZE + SUIT_STORAGE_DIGEST_SIZE);
	assert_valid_mpi_area_app(SUIT_STORAGE_NORDIC_ADDRESS + SUIT_STORAGE_RAD_MPI_SIZE +
					  SUIT_STORAGE_DIGEST_SIZE,
				  SUIT_STORAGE_APP_MPI_SIZE + SUIT_STORAGE_DIGEST_SIZE);
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
	/* ... and sample root class is supported */
	assert_sample_root_class();

	/* ... and NVV is set with initial values */
	for (size_t i = 0; i < SUIT_STORAGE_NVV_N_VARS; i++) {
		uint32_t value;

		err = suit_storage_var_get(i, &value);
		zassert_equal(err, SUIT_PLAT_SUCCESS, "Failed to read NVV at index: %d (%d).", i,
			      err);
		zassert_equal(value, 0xFFFFFFFF,
			      "Invalid initial NVV value at index: %d (0x%X != 0x%X).", i, value,
			      0xFFFFFFFF);
	}

	/* WHEN NVV variables are updated */
	uint32_t constants[SUIT_STORAGE_NVV_N_VARS] = {
		0x00000000, 0xAAAAAAAA, 0x55AA55AA, 0x01000000,
		0x00000001, 0xFFFFFFFF, 0x98EFCDAB, 0xFFFFFFFF,
	};
	for (size_t i = 0; i < SUIT_STORAGE_NVV_N_VARS; i++) {
		err = suit_storage_var_set(i, constants[i]);
		zassert_equal(err, SUIT_PLAT_SUCCESS, "Failed to write NVV at index: %d (%d).", i,
			      err);
	}

	/* THEN value of NVV area matches the preconfigured constants */
	zassert_mem_equal(SUIT_STORAGE_APP_NVV_ADDRESS, nvv_sample, SUIT_STORAGE_APP_NVV_SIZE / 2);
	/* ... and value of NVV area backup is updated */
	zassert_mem_equal(SUIT_STORAGE_APP_NVV_ADDRESS + SUIT_STORAGE_APP_NVV_SIZE / 2, nvv_sample,
			  SUIT_STORAGE_APP_NVV_SIZE / 2);
}

ZTEST(suit_storage_nrf54h20_nvv_tests, test_app_nvv_ro_backup_read)
{
	/* GIVEN the device is provisioned with application MPI with root config */
	erase_area_nordic();
	write_area_nordic_root();
	erase_area_rad();
	erase_area_app();
	write_area_app_root();
	/* .. and NVV is present */
	write_area_app_nvv();
	/* .. and NVV backup is present */
	write_area_app_nvv_backup();

	/* ... and storage module is initialized */
	int err = suit_storage_init();

	/* ... and digest of the application MPI matches... */
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
	/* ... and NVV area backup is not modified */
	zassert_mem_equal(SUIT_STORAGE_APP_NVV_ADDRESS + SUIT_STORAGE_APP_NVV_SIZE / 2, nvv_sample,
			  SUIT_STORAGE_APP_NVV_SIZE / 2);
	/* ... and initialization succeeds */
	zassert_equal(err, exp_err, "Failed to initialize SUIT storage (%d).", err);
	/* ... and Nordic class IDs are supported */
	assert_nordic_classes();
	/* ... and sample root class is supported */
	assert_sample_root_class();

	/* ... and NVV backup is corrupted after initialization*/
	erase_area_app_nvv_backup();

	uint32_t constants[SUIT_STORAGE_NVV_N_VARS] = {
		0x00000000, 0xAAAAAAAA, 0x55AA55AA, 0x01000000,
		0x00000001, 0xFFFFFFFF, 0x98EFCDAB, 0xFFFFFFFF,
	};
	for (size_t i = 0; i < SUIT_STORAGE_NVV_N_VARS; i++) {
		uint32_t value;

		/* WHEN the NVV value is read */
		err = suit_storage_var_get(i, &value);
		/* THEN the NVV read succeeds */
		zassert_equal(err, SUIT_PLAT_SUCCESS, "Failed to read NVV at index: %d (%d).", i,
			      err);
		/* ... and values in NVV area matches the preconfigured constants */
		zassert_equal(value, constants[i], "Invalid NVV value at index: %d (0x%X != 0x%X).",
			      i, value, constants[i]);
	}
}

ZTEST(suit_storage_nrf54h20_nvv_tests, test_app_nvv_ro_backup_write)
{
	/* GIVEN the device is provisioned with application MPI with root config */
	erase_area_nordic();
	write_area_nordic_root();
	erase_area_rad();
	erase_area_app();
	write_area_app_root();
	/* .. and NVV is present */
	write_area_app_nvv();
	/* .. and NVV backup is present */
	write_area_app_nvv_backup();

	/* ... and storage module is initialized */
	int err = suit_storage_init();

	/* ... and digest of the application MPI matches... */
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
	/* ... and NVV area backup is not modified */
	zassert_mem_equal(SUIT_STORAGE_APP_NVV_ADDRESS + SUIT_STORAGE_APP_NVV_SIZE / 2, nvv_sample,
			  SUIT_STORAGE_APP_NVV_SIZE / 2);
	/* ... and initialization succeeds */
	zassert_equal(err, exp_err, "Failed to initialize SUIT storage (%d).", err);
	/* ... and Nordic class IDs are supported */
	assert_nordic_classes();
	/* ... and sample root class is supported */
	assert_sample_root_class();

	/* ... and NVV backup is corrupted after initialization*/
	erase_area_app_nvv_backup();

	/* WHEN NVV variable is updated */
	err = suit_storage_var_set(0, 0x00000000);

	/* THEN the NVV update succeeds */
	zassert_equal(err, SUIT_PLAT_SUCCESS, "Failed to write NVV at index: 0 (%d).", err);

	/* ... and NVV area backup is recovered */
	zassert_mem_equal(SUIT_STORAGE_APP_NVV_ADDRESS + SUIT_STORAGE_APP_NVV_SIZE / 2, nvv_sample,
			  SUIT_STORAGE_APP_NVV_SIZE / 2);
}

ZTEST(suit_storage_nrf54h20_nvv_tests, test_app_nvv_ro_read)
{
	/* GIVEN the device is provisioned with application MPI with root config */
	erase_area_nordic();
	write_area_nordic_root();
	erase_area_rad();
	erase_area_app();
	write_area_app_root();
	/* .. and NVV is present */
	write_area_app_nvv();
	/* .. and NVV backup is present */
	write_area_app_nvv_backup();

	/* ... and storage module is initialized */
	int err = suit_storage_init();

	/* ... and digest of the application MPI matches... */
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
	/* ... and NVV area backup is not modified */
	zassert_mem_equal(SUIT_STORAGE_APP_NVV_ADDRESS + SUIT_STORAGE_APP_NVV_SIZE / 2, nvv_sample,
			  SUIT_STORAGE_APP_NVV_SIZE / 2);
	/* ... and initialization succeeds */
	zassert_equal(err, exp_err, "Failed to initialize SUIT storage (%d).", err);
	/* ... and Nordic class IDs are supported */
	assert_nordic_classes();
	/* ... and sample root class is supported */
	assert_sample_root_class();

	/* ... and NVV area is corrupted after initialization*/
	erase_area_app_nvv();

	uint32_t constants[SUIT_STORAGE_NVV_N_VARS] = {
		0x00000000, 0xAAAAAAAA, 0x55AA55AA, 0x01000000,
		0x00000001, 0xFFFFFFFF, 0x98EFCDAB, 0xFFFFFFFF,
	};
	for (size_t i = 0; i < SUIT_STORAGE_NVV_N_VARS; i++) {
		uint32_t value;

		/* WHEN the NVV value is read */
		err = suit_storage_var_get(i, &value);
		/* THEN the NVV read succeeds */
		zassert_equal(err, SUIT_PLAT_SUCCESS, "Failed to read NVV at index: %d (%d).", i,
			      err);
		/* ... and NVV backup area is used */
		/* ... and values in NVV area matches the preconfigured constants */
		zassert_equal(value, constants[i], "Invalid NVV value at index: %d (0x%X != 0x%X).",
			      i, value, constants[i]);
	}
}

ZTEST(suit_storage_nrf54h20_nvv_tests, test_app_nvv_ro_write)
{
	/* GIVEN the device is provisioned with application MPI with root config */
	erase_area_nordic();
	write_area_nordic_root();
	erase_area_rad();
	erase_area_app();
	write_area_app_root();
	/* .. and NVV is present */
	write_area_app_nvv();
	/* .. and NVV backup is present */
	write_area_app_nvv_backup();

	/* ... and storage module is initialized */
	int err = suit_storage_init();

	/* ... and digest of the application MPI matches... */
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
	/* ... and NVV area backup is not modified */
	zassert_mem_equal(SUIT_STORAGE_APP_NVV_ADDRESS + SUIT_STORAGE_APP_NVV_SIZE / 2, nvv_sample,
			  SUIT_STORAGE_APP_NVV_SIZE / 2);
	/* ... and initialization succeeds */
	zassert_equal(err, exp_err, "Failed to initialize SUIT storage (%d).", err);
	/* ... and Nordic class IDs are supported */
	assert_nordic_classes();
	/* ... and sample root class is supported */
	assert_sample_root_class();

	/* ... and NVV area is corrupted after initialization*/
	erase_area_app_nvv();

	/* WHEN NVV variable is updated */
	err = suit_storage_var_set(0, 0xABABABAB);

	/* THEN the NVV update fails */
	zassert_equal(err, SUIT_PLAT_ERR_IO, "NVV corrupted by writing to the read-only area");

	/* ... and NVV backup is not updated */
	zassert_mem_equal(SUIT_STORAGE_APP_NVV_ADDRESS + SUIT_STORAGE_APP_NVV_SIZE / 2, nvv_sample,
			  SUIT_STORAGE_APP_NVV_SIZE / 2);
}
