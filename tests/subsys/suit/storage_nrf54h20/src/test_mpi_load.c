/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <suit_storage_mpi.h>
#include "test_common.h"

static void test_suite_before(void *f)
{
	/* Reinitialize MPI, so it clears all internal state variables. */
	suit_plat_err_t err = suit_storage_mpi_init();

	zassert_equal(err, SUIT_PLAT_SUCCESS, "Failed to init SUIT MPI module (%d)", err);
}

ZTEST_SUITE(suit_storage_nrf54h20_mpi_load_tests, NULL, NULL, test_suite_before, NULL, NULL);

ZTEST(suit_storage_nrf54h20_mpi_load_tests, test_vid_uuid_zeros)
{
	uint8_t mpi[] = {
		0x01, /* version */
		0x01, /* downgrade prevention disabled */
		0x02, /* Independent update allowed */
		0x01, /* signature check disabled */
		/* reserved (12) */
		0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		/* 0x00 * 16 */
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		/* RFC4122 uuid5(nordic_vid, 'test_sample_root') */
		0x97, 0x05, 0x48, 0x23, 0x4c, 0x3d, 0x59, 0xa1,
		0x89, 0x86, 0xa5, 0x46, 0x60, 0xa1, 0x4b, 0x0a,
	};

	suit_plat_err_t err =
		suit_storage_mpi_configuration_load(SUIT_MANIFEST_APP_ROOT, mpi, sizeof(mpi));
	zassert_equal(err, SUIT_PLAT_ERR_OUT_OF_BOUNDS,
		      "MPI with UUID filled with zeros should fail to load");
}

ZTEST(suit_storage_nrf54h20_mpi_load_tests, test_vid_uuid_ones)
{
	uint8_t mpi[] = {
		0x01, /* version */
		0x01, /* downgrade prevention disabled */
		0x02, /* Independent update allowed */
		0x01, /* signature check disabled */
		/* reserved (12) */
		0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		/* 0xFF * 16 */
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		/* RFC4122 uuid5(nordic_vid, 'test_sample_root') */
		0x97, 0x05, 0x48, 0x23, 0x4c, 0x3d, 0x59, 0xa1,
		0x89, 0x86, 0xa5, 0x46, 0x60, 0xa1, 0x4b, 0x0a,
	};

	suit_plat_err_t err =
		suit_storage_mpi_configuration_load(SUIT_MANIFEST_APP_ROOT, mpi, sizeof(mpi));
	zassert_equal(err, SUIT_PLAT_ERR_OUT_OF_BOUNDS,
		      "MPI with UUID filled with ones should fail to load");
}

ZTEST(suit_storage_nrf54h20_mpi_load_tests, test_cid_uuid_zeros)
{
	uint8_t mpi[] = {
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
		/* 0x00 * 16 */
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	};

	suit_plat_err_t err =
		suit_storage_mpi_configuration_load(SUIT_MANIFEST_APP_ROOT, mpi, sizeof(mpi));
	zassert_equal(err, SUIT_PLAT_ERR_OUT_OF_BOUNDS,
		      "MPI with UUID filled with zeros should fail to load");
}

ZTEST(suit_storage_nrf54h20_mpi_load_tests, test_cid_uuid_ones)
{
	uint8_t mpi[] = {
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
		/* 0xFF * 16 */
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	};

	suit_plat_err_t err =
		suit_storage_mpi_configuration_load(SUIT_MANIFEST_APP_ROOT, mpi, sizeof(mpi));
	zassert_equal(err, SUIT_PLAT_ERR_OUT_OF_BOUNDS,
		      "MPI with UUID filled with ones should fail to load");
}

ZTEST(suit_storage_nrf54h20_mpi_load_tests, test_recovery_without_updates)
{
	uint8_t mpi[] = {
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

	suit_plat_err_t err =
		suit_storage_mpi_configuration_load(SUIT_MANIFEST_APP_RECOVERY, mpi, sizeof(mpi));
	zassert_equal(err, SUIT_PLAT_ERR_UNSUPPORTED,
		      "It should not be possible to block application recovery updates");
}

ZTEST(suit_storage_nrf54h20_mpi_load_tests, test_root_without_updates)
{
	uint8_t mpi[] = {
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

	suit_plat_err_t err =
		suit_storage_mpi_configuration_load(SUIT_MANIFEST_APP_ROOT, mpi, sizeof(mpi));
	zassert_equal(err, SUIT_PLAT_ERR_UNSUPPORTED,
		      "It should not be possible to block application root updates");
}

ZTEST(suit_storage_nrf54h20_mpi_load_tests, test_nonerased_reserved)
{
	uint8_t mpi[] = {
		0x01, /* version */
		0x01, /* downgrade prevention disabled */
		0x02, /* Independent update allowed */
		0x01, /* signature check disabled */
		/* reserved (12) */
		0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE,
		/* RFC4122 uuid5(uuid.NAMESPACE_DNS, 'nordicsemi.com') */
		0x76, 0x17, 0xda, 0xa5, 0x71, 0xfd, 0x5a, 0x85,
		0x8f, 0x94, 0xe2, 0x8d, 0x73, 0x5c, 0xe9, 0xf4,
		/* RFC4122 uuid5(nordic_vid, 'test_sample_root') */
		0x97, 0x05, 0x48, 0x23, 0x4c, 0x3d, 0x59, 0xa1,
		0x89, 0x86, 0xa5, 0x46, 0x60, 0xa1, 0x4b, 0x0a,
	};

	suit_plat_err_t err =
		suit_storage_mpi_configuration_load(SUIT_MANIFEST_APP_ROOT, mpi, sizeof(mpi));
	zassert_equal(err, SUIT_PLAT_ERR_OUT_OF_BOUNDS,
		      "It should not be possible to load MPI with nonerase reserved fields");
}

ZTEST(suit_storage_nrf54h20_mpi_load_tests, test_empty_new_version)
{
	uint8_t mpi[] = {
		0x02, /* unsupported version */
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	};

	suit_plat_err_t err =
		suit_storage_mpi_configuration_load(SUIT_MANIFEST_APP_ROOT, mpi, sizeof(mpi));
	zassert_equal(err, SUIT_PLAT_ERR_OUT_OF_BOUNDS,
		      "It should not be possible to load MPI with unsupported version");
}

ZTEST(suit_storage_nrf54h20_mpi_load_tests, test_downgrade_prevention_zero)
{
	uint8_t mpi[] = {
		0x01, /* version */
		0x00, /* unknown downgrade prevention policy */
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

	suit_plat_err_t err =
		suit_storage_mpi_configuration_load(SUIT_MANIFEST_APP_ROOT, mpi, sizeof(mpi));
	zassert_equal(
		err, SUIT_PLAT_ERR_OUT_OF_BOUNDS,
		"It should not be possible to load MPI with unknown downgrade prevention policy");
}

ZTEST(suit_storage_nrf54h20_mpi_load_tests, test_independent_updateability_zero)
{
	uint8_t mpi[] = {
		0x01, /* version */
		0x01, /* downgrade prevention disabled */
		0x00, /* unknown independent update policy */
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

	suit_plat_err_t err =
		suit_storage_mpi_configuration_load(SUIT_MANIFEST_APP_ROOT, mpi, sizeof(mpi));
	zassert_equal(
		err, SUIT_PLAT_ERR_OUT_OF_BOUNDS,
		"It should not be possible to load MPI with unknown independent update policy");
}

ZTEST(suit_storage_nrf54h20_mpi_load_tests, test_signature_check_zero)
{
	uint8_t mpi[] = {
		0x01, /* version */
		0x01, /* downgrade prevention disabled */
		0x02, /* Independent update allowed */
		0x00, /* unknown signature check policy */
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

	suit_plat_err_t err =
		suit_storage_mpi_configuration_load(SUIT_MANIFEST_APP_ROOT, mpi, sizeof(mpi));
	zassert_equal(err, SUIT_PLAT_ERR_OUT_OF_BOUNDS,
		      "It should not be possible to load MPI with unknown signature check policy");
}

ZTEST(suit_storage_nrf54h20_mpi_load_tests, test_fake_nordic_top_manifest)
{
	uint8_t mpi[] = {
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
		/* RFC4122 uuid5(nordic_vid, 'nRF54H20_nordic_top') */
		0xf0, 0x3d, 0x38, 0x5e, 0xa7, 0x31, 0x56, 0x05,
		0xb1, 0x5d, 0x03, 0x7f, 0x6d, 0xa6, 0x09, 0x7f,
	};

	/* GIVEN the Nordic top manifest is already loaded */
	suit_plat_err_t err =
		suit_storage_mpi_configuration_load(SUIT_MANIFEST_SEC_TOP, mpi, sizeof(mpi));
	zassert_equal(err, SUIT_PLAT_SUCCESS,
		      "Unable to load Nordic top manifest prior test execution (err: %d)", err);

	/* WHEN application loads ROOT manifest with Nordic top class ID */
	err = suit_storage_mpi_configuration_load(SUIT_MANIFEST_APP_ROOT, mpi, sizeof(mpi));

	/* THEN application slot is not loaded */
	zassert_equal(err, SUIT_PLAT_ERR_EXISTS,
		      "It should not be possible to load root MPI with Nordic top class ID");
}

ZTEST(suit_storage_nrf54h20_mpi_load_tests, test_two_root_manifests)
{
	uint8_t mpi[] = {
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

	/* GIVEN the ROOT manifest is already loaded */
	suit_plat_err_t err =
		suit_storage_mpi_configuration_load(SUIT_MANIFEST_APP_ROOT, mpi, sizeof(mpi));
	zassert_equal(err, SUIT_PLAT_SUCCESS,
		      "Unable to load root manifest prior test execution (err: %d)", err);

	/* WHEN application loads ROOT manifest from a different slot */
	err = suit_storage_mpi_configuration_load(SUIT_MANIFEST_APP_ROOT, mpi, sizeof(mpi));

	/* THEN the second slot is not loaded */
	zassert_equal(err, SUIT_PLAT_ERR_EXISTS,
		      "It should not be possible to load root MPI twice");
}

ZTEST(suit_storage_nrf54h20_mpi_load_tests, test_too_many_roles)
{
	suit_manifest_role_t roles[] = {
		SUIT_MANIFEST_SEC_TOP,
		SUIT_MANIFEST_SEC_SDFW,
		SUIT_MANIFEST_SEC_SYSCTRL,
		SUIT_MANIFEST_APP_ROOT,
		SUIT_MANIFEST_APP_RECOVERY,
		SUIT_MANIFEST_APP_LOCAL_1,
		SUIT_MANIFEST_APP_LOCAL_2,
		SUIT_MANIFEST_APP_LOCAL_3,
		SUIT_MANIFEST_RAD_RECOVERY,
		SUIT_MANIFEST_RAD_LOCAL_1,
		SUIT_MANIFEST_RAD_LOCAL_2,
	};
	uint8_t mpi[] = {
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
	uint8_t mpi_array[sizeof(mpi) * ARRAY_SIZE(roles)];

	for (size_t i = 0; i < ARRAY_SIZE(roles); i++) {
		/* Fake CID so each entry is different */
		memcpy(&mpi_array[i * sizeof(mpi)], mpi, sizeof(mpi));
		mpi_array[i * sizeof(mpi) + sizeof(mpi) - 1] = roles[i];
	}

	/* Check that the test will not exceed the array with roles. */
	zassert_true(ARRAY_SIZE(roles) > CONFIG_SUIT_STORAGE_N_ENVELOPES,
		     "Unable to execute test: role list does not exceed MPI limits (%d >= %d)",
		     CONFIG_SUIT_STORAGE_N_ENVELOPES, ARRAY_SIZE(roles));

	/* GIVEN all slots in MPI are used */
	for (size_t i = 0; i < CONFIG_SUIT_STORAGE_N_ENVELOPES; i++) {
		suit_plat_err_t err = suit_storage_mpi_configuration_load(
			roles[i], &mpi_array[i * sizeof(mpi)], sizeof(mpi));
		zassert_equal(err, SUIT_PLAT_SUCCESS,
			      "Unable to load manifest with role %d at index %d prior test "
			      "execution (err: %d)",
			      roles[i], i, err);
	}

	for (size_t i = CONFIG_SUIT_STORAGE_N_ENVELOPES; i < ARRAY_SIZE(roles); i++) {
		/* WHEN application loads next manifest */
		suit_plat_err_t err = suit_storage_mpi_configuration_load(
			roles[i], &mpi_array[i * sizeof(mpi)], sizeof(mpi));

		/* THEN the it is not loaded */
		zassert_equal(
			err, SUIT_PLAT_ERR_SIZE,
			"It should not be possible to load more MPIs than configured (index: %d)",
			i);
	}
}
