/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <suit_storage.h>
#include <suit_plat_mem_util.h>

#if DT_NODE_EXISTS(DT_NODELABEL(cpuapp_suit_storage))
#define SUIT_STORAGE_OFFSET FIXED_PARTITION_OFFSET(cpuapp_suit_storage)
#define SUIT_STORAGE_SIZE   FIXED_PARTITION_SIZE(cpuapp_suit_storage)
#else
#define SUIT_STORAGE_OFFSET FIXED_PARTITION_OFFSET(suit_storage)
#define SUIT_STORAGE_SIZE   FIXED_PARTITION_SIZE(suit_storage)
#endif
#define SUIT_STORAGE_ADDRESS suit_plat_mem_nvm_ptr_get(SUIT_STORAGE_OFFSET)

/* The SUIT envelopes are defined inside the respective manfest_*.c files. */
extern uint8_t manifest_root_buf[];
extern const size_t manifest_root_len;
extern uint8_t manifest_app_buf[];
extern const size_t manifest_app_len;
extern uint8_t manifest_rad_buf[];
extern const size_t manifest_rad_len;

extern uint8_t manifest_app_posix_buf[];
extern const size_t manifest_app_posix_len;
extern uint8_t manifest_root_posix_buf[];
extern const size_t manifest_root_posix_len;

extern uint8_t manifest_app_posix_v2_buf[];
extern const size_t manifest_app_posix_v2_len;
extern uint8_t manifest_root_posix_v2_buf[];
extern const size_t manifest_root_posix_v2_len;

static const suit_manifest_class_id_t classes[] = {
	/* RFC4122 uuid5(nordic_vid, 'nRF54H20_sample_root') */
	{{0x3f, 0x6a, 0x3a, 0x4d, 0xcd, 0xfa, 0x58, 0xc5, 0xac, 0xce, 0xf9, 0xf5, 0x84, 0xc4, 0x11,
	  0x24}},
	/* RFC4122 uuid5(nordic_vid, 'nRF54H20_sample_app') */
	{{0x08, 0xc1, 0xb5, 0x99, 0x55, 0xe8, 0x5f, 0xbc, 0x9e, 0x76, 0x7b, 0xc2, 0x9c, 0xe1, 0xb0,
	  0x4d}},
	/* RFC4122 uuid5(nordic_vid, 'nRF54H20_sample_rad') */
	{{0x81, 0x6a, 0xa0, 0xa0, 0xaf, 0x11, 0x5e, 0xf2, 0x85, 0x8a, 0xfe, 0xb6, 0x68, 0xb2, 0xe9,
	  0xc9}},
	/* RFC4122 uuid5(nordic_vid, 'test_sample_root') */
	{{0x97, 0x05, 0x48, 0x23, 0x4c, 0x3d, 0x59, 0xa1, 0x89, 0x86, 0xa5, 0x46, 0x60, 0xa1, 0x4b,
	  0x0a}},
	/* RFC4122 uuid5(nordic_vid, 'test_sample_app') */
	{{0x5b, 0x46, 0x9f, 0xd1, 0x90, 0xee, 0x53, 0x9c, 0xa3, 0x18, 0x68, 0x1b, 0x03, 0x69, 0x5e,
	  0x36}},
};

static const suit_manifest_class_id_t *unsupported_classes[] = {
	&classes[0],
	&classes[1],
	&classes[2],
};

static const suit_manifest_class_id_t *supported_classes[] = {
	&classes[3],
	&classes[4],
};

static void test_suite_before(void *f)
{
	/* Execute SUIT storage init, so the MPI area is copied into a backup region. */
	int err = suit_storage_init();

	zassert_equal(SUIT_PLAT_SUCCESS, err, "Failed to init and backup suit storage (%d)", err);

	/* Clear the whole application area */
	const struct device *fdev = SUIT_PLAT_INTERNAL_NVM_DEV;

	zassert_not_null(fdev, "Unable to find a driver to erase storage area");

	err = flash_erase(fdev, SUIT_STORAGE_OFFSET, SUIT_STORAGE_SIZE);
	zassert_equal(0, err, "Unable to erase storage before test execution");

	err = suit_storage_init();
	zassert_equal(err, SUIT_PLAT_SUCCESS, "Failed to initialize SUIT storage module (%d).",
		      err);
}

ZTEST_SUITE(suit_storage_envelopes_tests, NULL, NULL, test_suite_before, NULL, NULL);

ZTEST(suit_storage_envelopes_tests, test_empty_envelope_get)
{
	const uint8_t *addr;
	size_t size;
	int rc = 0;

	for (size_t i = 0; i < ARRAY_SIZE(classes); i++) {
		rc = suit_storage_installed_envelope_get(NULL, NULL, NULL);
		zassert_not_equal(rc, SUIT_PLAT_SUCCESS,
				  "Arguments are incorrect, but %d envelope was received", i);

		rc = suit_storage_installed_envelope_get(NULL, NULL, &size);
		zassert_not_equal(rc, SUIT_PLAT_SUCCESS,
				  "Arguments are incorrect, but %d envelope was received (%d)", i,
				  size);

		rc = suit_storage_installed_envelope_get(NULL, &addr, NULL);
		zassert_not_equal(rc, SUIT_PLAT_SUCCESS,
				  "Arguments are incorrect, but %d envelope was received (0x%x)", i,
				  addr);

		rc = suit_storage_installed_envelope_get(&classes[i], NULL, NULL);
		zassert_not_equal(rc, SUIT_PLAT_SUCCESS,
				  "Arguments are incorrect, but %d envelope was received", i);

		rc = suit_storage_installed_envelope_get(&classes[i], NULL, &size);
		zassert_not_equal(rc, SUIT_PLAT_SUCCESS,
				  "Arguments are incorrect, but %d envelope was received (%d)", i,
				  size);

		rc = suit_storage_installed_envelope_get(&classes[i], &addr, NULL);
		zassert_not_equal(rc, SUIT_PLAT_SUCCESS,
				  "Arguments are incorrect, but %d envelope was received (0x%x)", i,
				  addr);

		rc = suit_storage_installed_envelope_get(&classes[i], &addr, &size);
		zassert_not_equal(rc, SUIT_PLAT_SUCCESS,
				  "Suit storage is empty, but %d envelope was received (0x%x, %d)",
				  i, addr, size);
	}
}

ZTEST(suit_storage_envelopes_tests, test_empty_envelope_set)
{
	const uint8_t *addr;
	size_t size;
	int rc = 0;

	suit_plat_mreg_t envelopes[] = {
		{
			.mem = manifest_root_posix_buf,
			.size = manifest_root_posix_len,
		},
		{
			.mem = manifest_app_posix_buf,
			.size = manifest_app_posix_len,
		},
	};

	for (size_t i = 0; i < ARRAY_SIZE(envelopes); i++) {
		rc = suit_storage_installed_envelope_get(supported_classes[i], &addr, &size);
		zassert_not_equal(rc, SUIT_PLAT_SUCCESS,
				  "Envelope %d was not installed, but was returned (0x%x, %d)", i,
				  addr, size);

		rc = suit_storage_install_envelope(supported_classes[i],
						   (uint8_t *)envelopes[i].mem, envelopes[i].size);
		zassert_equal(rc, SUIT_PLAT_SUCCESS, "Unable to install envelope %d", i);

		rc = suit_storage_installed_envelope_get(supported_classes[i], &addr, &size);
		zassert_equal(rc, SUIT_PLAT_SUCCESS,
			      "Envelope %d was installed, but was not returned (0x%x, %d)", i, addr,
			      size);
		zassert_true(size < (envelopes[i].size - 256),
			     "Envelope %d was installed, but the size was not reduced (%d >= %d)",
			     i, size, envelopes[i].size);
	}
}

ZTEST(suit_storage_envelopes_tests, test_empty_envelope_override)
{
	const uint8_t *addr;
	size_t size;
	int rc = 0;

	suit_plat_mreg_t envelopes[] = {
		{
			.mem = manifest_root_posix_buf,
			.size = manifest_root_posix_len,
		},
		{
			.mem = manifest_app_posix_buf,
			.size = manifest_app_posix_len,
		},
	};

	/* Verify that envelopes are not installed. */
	for (size_t i = 0; i < ARRAY_SIZE(envelopes); i++) {
		rc = suit_storage_installed_envelope_get(supported_classes[i], &addr, &size);
		zassert_not_equal(rc, SUIT_PLAT_SUCCESS,
				  "Envelope %d was not installed, but was returned (0x%x, %d)", i,
				  addr, size);
	}

	/* Install envelopes for the first time. */
	for (size_t i = 0; i < ARRAY_SIZE(envelopes); i++) {
		rc = suit_storage_install_envelope(supported_classes[i],
						   (uint8_t *)envelopes[i].mem, envelopes[i].size);
		zassert_equal(rc, SUIT_PLAT_SUCCESS, "Unable to install envelope %d", i);

		rc = suit_storage_installed_envelope_get(supported_classes[i], &addr, &size);
		zassert_equal(rc, SUIT_PLAT_SUCCESS,
			      "Envelope %d was installed, but was not returned (0x%x, %d)", i, addr,
			      size);
	}

	/* Install new version of the envelopes. */
	suit_plat_mreg_t envelopes_v2[] = {
		{
			.mem = manifest_root_posix_v2_buf,
			.size = manifest_root_posix_v2_len,
		},
		{
			.mem = manifest_app_posix_v2_buf,
			.size = manifest_app_posix_v2_len,
		},
	};

	for (size_t i = 0; i < ARRAY_SIZE(envelopes_v2); i++) {
		rc = suit_storage_install_envelope(
			supported_classes[i], (uint8_t *)envelopes_v2[i].mem, envelopes_v2[i].size);
		zassert_equal(rc, SUIT_PLAT_SUCCESS, "Unable to install envelope %d", i);

		rc = suit_storage_installed_envelope_get(supported_classes[i], &addr, &size);
		zassert_equal(rc, SUIT_PLAT_SUCCESS,
			      "Envelope %d was installed, but was not returned (0x%x, %d)", i, addr,
			      size);
	}
}

ZTEST(suit_storage_envelopes_tests, test_empty_envelope_set_unsupported_classes)
{
	const uint8_t *addr;
	size_t size;
	int rc = 0;

	suit_plat_mreg_t envelopes[] = {
		{
			.mem = manifest_root_buf,
			.size = manifest_root_len,
		},
		{
			.mem = manifest_app_buf,
			.size = manifest_app_len,
		},
		{
			.mem = manifest_rad_buf,
			.size = manifest_rad_len,
		},
	};

	for (size_t i = 0; i < ARRAY_SIZE(envelopes); i++) {
		rc = suit_storage_install_envelope(unsupported_classes[i],
						   (uint8_t *)envelopes[i].mem, envelopes[i].size);
		zassert_not_equal(rc, SUIT_PLAT_SUCCESS,
				  "Envelope with unsupported class ID shall fail %d", i);

		rc = suit_storage_installed_envelope_get(unsupported_classes[i], &addr, &size);
		zassert_not_equal(rc, SUIT_PLAT_SUCCESS,
				  "Envelope %d was not installed, but was returned (0x%x, %d)", i,
				  addr, size);
	}
}

ZTEST(suit_storage_envelopes_tests, test_empty_envelope_set_class_mismatch)
{
	const uint8_t *addr;
	size_t size;
	int rc = 0;

	suit_plat_mreg_t envelopes[] = {
		{
			.mem = manifest_root_buf,
			.size = manifest_root_len,
		},
		{
			.mem = manifest_app_buf,
			.size = manifest_app_len,
		},
	};

	for (size_t i = 0; i < ARRAY_SIZE(envelopes); i++) {
		rc = suit_storage_install_envelope(supported_classes[i],
						   (uint8_t *)envelopes[i].mem, envelopes[i].size);
		zassert_not_equal(rc, SUIT_PLAT_SUCCESS,
				  "Envelope with unmatched class ID shall fail %d", i);

		rc = suit_storage_installed_envelope_get(supported_classes[i], &addr, &size);
		zassert_not_equal(rc, SUIT_PLAT_SUCCESS,
				  "Envelope %d was not installed, but was returned (0x%x, %d)", i,
				  addr, size);
	}
}
