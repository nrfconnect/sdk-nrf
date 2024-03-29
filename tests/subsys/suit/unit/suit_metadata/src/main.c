/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <suit_metadata.h>
#include <mocks.h>

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))
#endif /* ARRAY_SIZE */

typedef struct {
	int32_t array[5];
	size_t array_size;
	suit_version_t exp_version;
} version_test_vect;

static int32_t test_version[] = {0, 0, 0, 0, 0};
static suit_version_t sem_version;

static void test_before(void *data)
{
	/* Reset common test structures */
	memset(&test_version, 0, sizeof(test_version));
	memset(&sem_version, 0, sizeof(sem_version));

	/* Reset mocks */
	mocks_reset();

	/* Reset common FFF internal structures */
	FFF_RESET_HISTORY();
}

ZTEST_SUITE(suit_metadata_version_from_array_tests, NULL, NULL, test_before, NULL, NULL);

ZTEST(suit_metadata_version_from_array_tests, test_null_semver)
{
	int32_t *array = test_version;
	suit_version_t *version = &sem_version;

	/* GIVEN the pointer to the output structure points to the NULL. */
	version = NULL;

	/* WHEN the version conversion is attempted. */
	int ret = suit_metadata_version_from_array(version, array, ARRAY_SIZE(test_version));

	/* THEN manifest version conversion fails */
	zassert_equal(SUIT_PLAT_ERR_INVAL, ret, "Failed to catch null output argument");
}

ZTEST(suit_metadata_version_from_array_tests, test_null_array)
{
	int32_t *array = test_version;
	suit_version_t *version = &sem_version;

	/* GIVEN the pointer to the input array points to the NULL. */
	array = NULL;

	/* WHEN the version conversion is attempted. */
	int ret = suit_metadata_version_from_array(version, array, ARRAY_SIZE(test_version));

	/* THEN manifest version conversion fails */
	zassert_equal(SUIT_PLAT_ERR_INVAL, ret, "Failed to catch null input argument");
}

ZTEST(suit_metadata_version_from_array_tests, test_too_small_array)
{
	int32_t *array = test_version;
	suit_version_t *version = &sem_version;

	/* GIVEN the input array size is zero. */
	int array_size = 0;

	/* WHEN the version conversion is attempted. */
	int ret = suit_metadata_version_from_array(version, array, array_size);

	/* THEN manifest version conversion fails */
	zassert_equal(SUIT_PLAT_ERR_SIZE, ret, "Failed to catch too small input array");
}

ZTEST(suit_metadata_version_from_array_tests, test_invalid_release_type)
{
	int32_t *array = test_version;
	suit_version_t *version = &sem_version;
	int32_t invalid_version_types[] = {
		3, 2, 1, -4, -5, -6,
	};

	for (int i = 0; i < ARRAY_SIZE(invalid_version_types); i++) {
		/* GIVEN the input array describes unsupported release type. */
		array[2] = invalid_version_types[i];

		/* WHEN the version conversion is attempted. */
		int ret =
			suit_metadata_version_from_array(version, array, ARRAY_SIZE(test_version));

		/* THEN manifest version conversion fails */
		zassert_equal(SUIT_PLAT_ERR_OUT_OF_BOUNDS, ret,
			      "Failed to catch unsupported release type");
	}
}

ZTEST(suit_metadata_version_from_array_tests, test_negative_release_values)
{
	int32_t *array = test_version;
	suit_version_t *version = &sem_version;
	int32_t invalid_versions[3][3] = {
		{-1, 2, 3},
		{1, -2, -3},
		{1, 2, -4},
	};

	for (int i = 0; i < ARRAY_SIZE(invalid_versions); i++) {
		/* GIVEN the input array describes negative release. */
		array = invalid_versions[i];

		/* WHEN the version conversion is attempted. */
		int ret = suit_metadata_version_from_array(version, array,
							   ARRAY_SIZE(invalid_versions[0]));

		/* THEN manifest version conversion fails */
		zassert_equal(SUIT_PLAT_ERR_OUT_OF_BOUNDS, ret,
			      "Failed to catch negative release value");
	}
}

ZTEST(suit_metadata_version_from_array_tests, test_negative_prerelease_values)
{
	int32_t *array = test_version;
	suit_version_t *version = &sem_version;
	int32_t invalid_versions[3][4] = {
		{-1, 2, -1, 3},
		{1, -2, -1, 3},
		{1, 2, -1, -3},
	};

	for (int i = 0; i < ARRAY_SIZE(invalid_versions); i++) {
		/* GIVEN the input array describes negative pre-release. */
		array = invalid_versions[i];

		/* WHEN the version conversion is attempted. */
		int ret = suit_metadata_version_from_array(version, array,
							   ARRAY_SIZE(invalid_versions[0]));

		/* THEN manifest version conversion fails */
		zassert_equal(SUIT_PLAT_ERR_OUT_OF_BOUNDS, ret,
			      "Failed to catch negative pre-release value");
	}
}

ZTEST(suit_metadata_version_from_array_tests, test_too_long_release_version)
{
	int32_t *array = test_version;
	suit_version_t *version = &sem_version;
	int32_t invalid_version[] = {1, 2, 3, 0};

	/* GIVEN the input array describes negative pre-release. */
	array = invalid_version;

	/* WHEN the version conversion is attempted. */
	int ret = suit_metadata_version_from_array(version, array, ARRAY_SIZE(invalid_version));

	/* THEN manifest version conversion fails */
	zassert_equal(SUIT_PLAT_ERR_OUT_OF_BOUNDS, ret,
		      "Failed to catch 4-digit regular release value");
}

ZTEST(suit_metadata_version_from_array_tests, test_valid_release_versions)
{
	int32_t *array = test_version;
	suit_version_t *version = &sem_version;
	version_test_vect versions[] = {
		{.array = {1, 0, 0, 0, 0},
		 .array_size = 1,
		 .exp_version = {
				.type = SUIT_VERSION_RELEASE_NORMAL,
				.major = 1,
				.minor = 0,
				.patch = 0,
				.pre_release_number = 0,
			}},
		{.array = {1, 2, 0, 0, 0},
		 .array_size = 2,
		 .exp_version = {
				.type = SUIT_VERSION_RELEASE_NORMAL,
				.major = 1,
				.minor = 2,
				.patch = 0,
				.pre_release_number = 0,
			}},
		{.array = {1, 2, 3, 0, 0},
		 .array_size = 3,
		 .exp_version = {
				.type = SUIT_VERSION_RELEASE_NORMAL,
				.major = 1,
				.minor = 2,
				.patch = 3,
				.pre_release_number = 0,
			}},
		{.array = {0, 0, 0, 0, 0},
		 .array_size = 3,
		 .exp_version = {
				.type = SUIT_VERSION_RELEASE_NORMAL,
				.major = 0,
				.minor = 0,
				.patch = 0,
				.pre_release_number = 0,
			}},
		{.array = {0, 0, -1, 0, 0},
		 .array_size = 3,
		 .exp_version = {
				.type = SUIT_VERSION_RELEASE_RC,
				.major = 0,
				.minor = 0,
				.patch = 0,
				.pre_release_number = 0,
			}},
		{.array = {0, 0, -2, 0, 0},
		 .array_size = 3,
		 .exp_version = {
				.type = SUIT_VERSION_RELEASE_BETA,
				.major = 0,
				.minor = 0,
				.patch = 0,
				.pre_release_number = 0,
			}},
		{.array = {0, 0, -3, 0, 0},
		 .array_size = 4,
		 .exp_version = {
				.type = SUIT_VERSION_RELEASE_ALPHA,
				.major = 0,
				.minor = 0,
				.patch = 0,
				.pre_release_number = 0,
			}},
		{.array = {3, 2, -1, 1, 0},
		 .array_size = 4,
		 .exp_version = {
				.type = SUIT_VERSION_RELEASE_RC,
				.major = 3,
				.minor = 2,
				.patch = 0,
				.pre_release_number = 1,
			}},
		{.array = {3, 2, -2, 1, 0},
		 .array_size = 4,
		 .exp_version = {
				.type = SUIT_VERSION_RELEASE_BETA,
				.major = 3,
				.minor = 2,
				.patch = 0,
				.pre_release_number = 1,
			}},
		{.array = {3, 2, 1, -3, 1},
		 .array_size = 5,
		 .exp_version = {
				.type = SUIT_VERSION_RELEASE_ALPHA,
				.major = 3,
				.minor = 2,
				.patch = 1,
				.pre_release_number = 1,
			}},
		{.array = {3, 2, -3, 1, 0},
		 .array_size = 4,
		 .exp_version = {
				.type = SUIT_VERSION_RELEASE_ALPHA,
				.major = 3,
				.minor = 2,
				.patch = 0,
				.pre_release_number = 1,
			}},
		{.array = {3, -3, 1, 0, 0},
		 .array_size = 3,
		 .exp_version = {
				.type = SUIT_VERSION_RELEASE_ALPHA,
				.major = 3,
				.minor = 0,
				.patch = 0,
				.pre_release_number = 1,
			}},
		{.array = {3, -3, 0, 0, 0},
		 .array_size = 2,
		 .exp_version = {
				.type = SUIT_VERSION_RELEASE_ALPHA,
				.major = 3,
				.minor = 0,
				.patch = 0,
				.pre_release_number = 0,
			}},
	};

	for (int i = 0; i < ARRAY_SIZE(versions); i++) {
		/* GIVEN the input array describes a valid release. */
		array = versions[i].array;

		printk("\tTEST: %d.%d.%d.%d\n", array[0], array[1], array[2], array[3]);

		/* WHEN the version conversion is attempted. */
		int ret = suit_metadata_version_from_array(version, array, versions[i].array_size);

		/* THEN manifest version conversion succeeds ... */
		zassert_equal(SUIT_PLAT_SUCCESS, ret, "Failed to catch negative release value");

		/* ... and the version has the expected value */
		zassert_equal(version->type, versions[i].exp_version.type,
			      "Incorrect version type returned");
		zassert_equal(version->major, versions[i].exp_version.major,
			      "Incorrect major version returned");
		zassert_equal(version->minor, versions[i].exp_version.minor,
			      "Incorrect minor version returned");
		zassert_equal(version->patch, versions[i].exp_version.patch,
			      "Incorrect patch version returned");
		zassert_equal(version->pre_release_number,
			      versions[i].exp_version.pre_release_number,
			      "Incorrect pre-release version number returned");
	}
}
