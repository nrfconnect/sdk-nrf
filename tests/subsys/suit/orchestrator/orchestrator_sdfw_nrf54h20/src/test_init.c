/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>

#include <suit_orchestrator.h>
#include <suit_storage.h>
#include <suit_execution_mode.h>

#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <suit_plat_mem_util.h>

#define SUIT_STORAGE_APP_MPI_SIZE 0xf0
#define SUIT_STORAGE_OFFSET	  FIXED_PARTITION_OFFSET(cpuapp_suit_storage)
#define SUIT_STORAGE_SIZE	  FIXED_PARTITION_SIZE(cpuapp_suit_storage)
#define SUIT_BACKUP_OFFSET	  FIXED_PARTITION_OFFSET(cpusec_suit_storage)
#define SUIT_BACKUP_SIZE	  FIXED_PARTITION_SIZE(cpusec_suit_storage)

/* Valid envelope */
extern const uint8_t manifest_valid_buf[];
extern const size_t manifest_valid_len;

static void setup_erased_flash(void *f)
{
	/* Clear the whole application area */
	const struct device *fdev = SUIT_PLAT_INTERNAL_NVM_DEV;

	zassert_not_null(fdev, "Unable to find a driver to erase storage area");

	int err = flash_erase(fdev, SUIT_STORAGE_OFFSET, SUIT_STORAGE_SIZE);

	zassert_equal(0, err, "Unable to erase storage before test execution");

	err = flash_erase(fdev, SUIT_BACKUP_OFFSET, SUIT_BACKUP_SIZE);
	zassert_equal(0, err, "Unable to erase storage backup before test execution");

	suit_plat_err_t ret = suit_storage_report_clear(0);

	zassert_equal(SUIT_PLAT_SUCCESS, ret,
		      "Unable to clear recovery flag before test execution");
}

static void write_empty_mpi_area_app(void)
{
	/* Digest of the content defined in assert_empty_mpi_area_app(). */
	uint8_t app_digest[] = {
		0xd6, 0xc4, 0x94, 0x17, 0xb1, 0xca, 0x0a, 0x67,
		0x14, 0xdc, 0xde, 0x2b, 0x40, 0x01, 0x0c, 0xb7,
		0x49, 0x6d, 0x05, 0xdf, 0x7f, 0x8c, 0x8b, 0x1b,
		0x98, 0x14, 0x09, 0x7e, 0x9d, 0x62, 0xc8, 0xe1,
	};

	/* Write the digest of application area filled with 0xFF */
	const struct device *fdev = SUIT_PLAT_INTERNAL_NVM_DEV;

	zassert_not_null(fdev, "Unable to find a driver to modify MPI area");

	int err = flash_write(fdev, SUIT_STORAGE_OFFSET + SUIT_STORAGE_APP_MPI_SIZE, app_digest,
			      sizeof(app_digest));

	zassert_equal(0, err, "Unable to store application MPI digest before test execution");
}

static void write_mpi_area_app_root(void)
{
	uint8_t mpi_root[] = {
		0x01, /* version */
		0x01, /* downgrade prevention disabled */
		0x02, /* Independent update allowed */
		0x01, /* signature check disabled */
		/* reserved (12) */
		0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		/* RFC4122 uuid5(uuid.NAMESPACE_DNS, 'nordicsemi.com') */
		0x76, 0x17, 0xda, 0xa5, 0x71, 0xfd, 0x5a, 0x85,
		0x8f, 0x94, 0xe2, 0x8d, 0x73, 0x5c, 0xe9, 0xf4,
		/* RFC4122 uuid5(nordic_vid, 'test_sample_root') */
		0x97, 0x05, 0x48, 0x23, 0x4c, 0x3d, 0x59, 0xa1,
		0x89, 0x86, 0xa5, 0x46, 0x60, 0xa1, 0x4b, 0x0a,
	};
	/* Digest of the content defined in assert_valid_mpi_area_app(). */
	uint8_t app_digest[] = {
		0xa9, 0xa0, 0xee, 0x40, 0x5f, 0xad, 0x2b, 0xeb,
		0x66, 0x50, 0xdd, 0xa9, 0x97, 0x11, 0x72, 0x98,
		0x2b, 0x17, 0x45, 0x90, 0x16, 0xe1, 0xc7, 0xf5,
		0xc1, 0xdc, 0x3f, 0xb4, 0x58, 0x96, 0x1e, 0x44,
	};

	/* Write the sample application area (just the root MPI) and corresponding digest */
	const struct device *fdev = SUIT_PLAT_INTERNAL_NVM_DEV;

	zassert_not_null(fdev, "Unable to find a driver to modify MPI area");

	int err = flash_write(fdev, SUIT_STORAGE_OFFSET, mpi_root, sizeof(mpi_root));

	zassert_equal(0, err,
		      "Unable to store application root MPI contents before test execution");

	err = flash_write(fdev, SUIT_STORAGE_OFFSET + SUIT_STORAGE_APP_MPI_SIZE, app_digest,
			  sizeof(app_digest));
	zassert_equal(0, err, "Unable to store application MPI digest before test execution");
}

static void write_mpi_area_app_unupdateable_root(void)
{
	uint8_t mpi_root[] = {
		0x01, /* version */
		0x01, /* downgrade prevention disabled */
		0x01, /* Independent update disabled */
		0x01, /* signature check disabled */
		/* reserved (12) */
		0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		/* RFC4122 uuid5(uuid.NAMESPACE_DNS, 'nordicsemi.com') */
		0x76, 0x17, 0xda, 0xa5, 0x71, 0xfd, 0x5a, 0x85,
		0x8f, 0x94, 0xe2, 0x8d, 0x73, 0x5c, 0xe9, 0xf4,
		/* RFC4122 uuid5(nordic_vid, 'test_sample_root') */
		0x97, 0x05, 0x48, 0x23, 0x4c, 0x3d, 0x59, 0xa1,
		0x89, 0x86, 0xa5, 0x46, 0x60, 0xa1, 0x4b, 0x0a,
	};
	/* Digest of the content defined in assert_valid_mpi_area_app() and
	 * modified independent updateability policy value.
	 */
	uint8_t app_digest[] = {
		0xae, 0x12, 0x12, 0x86, 0xdb, 0x15, 0x34, 0x99,
		0xa4, 0x6d, 0xcf, 0xfc, 0x7c, 0x73, 0x4f, 0xf9,
		0x2e, 0x67, 0xcb, 0xfc, 0x30, 0xdd, 0xb1, 0x65,
		0xa3, 0x9e, 0x72, 0x41, 0xd3, 0x1b, 0x4a, 0x91,
	};

	/* Write the sample application area (just the root MPI) and corresponding digest */
	const struct device *fdev = SUIT_PLAT_INTERNAL_NVM_DEV;

	zassert_not_null(fdev, "Unable to find a driver to modify MPI area");

	int err = flash_write(fdev, SUIT_STORAGE_OFFSET, mpi_root, sizeof(mpi_root));

	zassert_equal(0, err,
		      "Unable to store application root MPI contents before test execution");

	err = flash_write(fdev, SUIT_STORAGE_OFFSET + SUIT_STORAGE_APP_MPI_SIZE, app_digest,
			  sizeof(app_digest));
	zassert_equal(0, err, "Unable to store application MPI digest before test execution");
}

static void write_mpi_area_app_unsupported_version(void)
{
	uint8_t mpi_root[] = {
		0x02, /* version */
		0x01, /* downgrade prevention disabled */
		0x02, /* Independent update allowed */
		0x01, /* signature check disabled */
		/* reserved (12) */
		0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		/* RFC4122 uuid5(uuid.NAMESPACE_DNS, 'nordicsemi.com') */
		0x76, 0x17, 0xda, 0xa5, 0x71, 0xfd, 0x5a, 0x85,
		0x8f, 0x94, 0xe2, 0x8d, 0x73, 0x5c, 0xe9, 0xf4,
		/* RFC4122 uuid5(nordic_vid, 'test_sample_root') */
		0x97, 0x05, 0x48, 0x23, 0x4c, 0x3d, 0x59, 0xa1,
		0x89, 0x86, 0xa5, 0x46, 0x60, 0xa1, 0x4b, 0x0a,
	};
	/* Digest of the content defined in assert_valid_mpi_area_app() and
	 * modified mpi structure version value.
	 */
	uint8_t app_digest[] = {
		0x97, 0xcf, 0x84, 0xc1, 0xc8, 0x5e, 0x1c, 0x83,
		0xbe, 0xd3, 0xb5, 0xe0, 0x03, 0x66, 0xc7, 0xfb,
		0x5f, 0xa6, 0x9f, 0x69, 0xa1, 0xaa, 0xf9, 0xc3,
		0x1f, 0x6d, 0xa4, 0x6f, 0x50, 0xb0, 0x5f, 0x6c,
	};

	/* Write the sample application area (just the root MPI) and corresponding digest */
	const struct device *fdev = SUIT_PLAT_INTERNAL_NVM_DEV;

	zassert_not_null(fdev, "Unable to find a driver to modify MPI area");

	int err = flash_write(fdev, SUIT_STORAGE_OFFSET, mpi_root, sizeof(mpi_root));

	zassert_equal(0, err,
		      "Unable to store application root MPI contents before test execution");

	err = flash_write(fdev, SUIT_STORAGE_OFFSET + SUIT_STORAGE_APP_MPI_SIZE, app_digest,
			  sizeof(app_digest));
	zassert_equal(0, err, "Unable to store application MPI digest before test execution");
}

ZTEST_SUITE(orchestrator_nrf54h20_init_tests, NULL, NULL, setup_erased_flash, NULL, NULL);

ZTEST(orchestrator_nrf54h20_init_tests, test_no_mpi)
{
	/* GIVEN empty flash (suit storage and backup is erased)... */
	/* ... and update candidate flag is not set... */
	/* ... and emergency flag is not set */

	/* WHEN orchestrator is initialized */
	int err = suit_orchestrator_init();

	/* THEN failed state is triggered... */
	zassert_equal(EXECUTION_MODE_FAIL_NO_MPI, suit_execution_mode_get(),
		      "Lack of MPIs not detected");
	/* ... and orchestrator is initialized */
	zassert_equal(0, err, "Orchestrator not initialized");
}

ZTEST(orchestrator_nrf54h20_init_tests, test_no_root_mpi)
{
	/* GIVEN empty flash (suit storage and empty MPI area with digest)... */
	write_empty_mpi_area_app();
	/* ... and update candidate flag is not set... */
	/* ... and emergency flag is not set */

	/* WHEN orchestrator is initialized */
	int err = suit_orchestrator_init();

	/* THEN failed state is triggered... */
	zassert_equal(EXECUTION_MODE_FAIL_MPI_INVALID_MISSING, suit_execution_mode_get(),
		      "Lack of ROOT MPIs not detected");
	/* ... and orchestrator is initialized */
	zassert_equal(0, err, "Orchestrator not initialized");
}

ZTEST(orchestrator_nrf54h20_init_tests, test_invalid_mpi_version)
{
	/* GIVEN empty flash (suit storage and root MPI with incorrect version... */
	write_mpi_area_app_unsupported_version();
	/* ... and update candidate flag is not set... */
	/* ... and emergency flag is not set */

	/* WHEN orchestrator is initialized */
	int err = suit_orchestrator_init();

	/* THEN failed state is triggered... */
	zassert_equal(EXECUTION_MODE_FAIL_MPI_INVALID, suit_execution_mode_get(),
		      "Malformed ROOT MPI not detected");
	/* ... and orchestrator is initialized */
	zassert_equal(0, err, "Orchestrator not initialized");
}

ZTEST(orchestrator_nrf54h20_init_tests, test_unupdateable_root)
{
	/* GIVEN empty flash (suit storage and root MPI without updateability flag set... */
	write_mpi_area_app_unupdateable_root();
	/* ... and update candidate flag is not set... */
	/* ... and emergency flag is not set */

	/* WHEN orchestrator is initialized */
	int err = suit_orchestrator_init();

	/* THEN failed state is triggered... */
	zassert_equal(EXECUTION_MODE_FAIL_MPI_UNSUPPORTED, suit_execution_mode_get(),
		      "Non-updateable ROOT MPI not detected");
	/* ... and orchestrator is initialized */
	zassert_equal(0, err, "Orchestrator not initialized");
}

ZTEST(orchestrator_nrf54h20_init_tests, test_valid_root)
{
	/* GIVEN empty flash (suit storage and valid root MPI... */
	write_mpi_area_app_root();
	/* ... and update candidate flag is not set... */
	/* ... and emergency flag is not set */

	/* WHEN orchestrator is initialized */
	int err = suit_orchestrator_init();

	/* THEN failed state is triggered... */
	zassert_equal(EXECUTION_MODE_INVOKE, suit_execution_mode_get(),
		      "Non-updateable ROOT MPI not detected");
	/* ... and orchestrator is initialized */
	zassert_equal(0, err, "Orchestrator not initialized");
}
