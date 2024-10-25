/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <suit_storage_mpi.h>
#include <suit_plat_mem_util.h>
#include "test_common.h"

#define SUIT_STORAGE_NORDIC_OFFSET FIXED_PARTITION_OFFSET(cpusec_suit_storage)
#define SUIT_STORAGE_NORDIC_SIZE   FIXED_PARTITION_SIZE(cpusec_suit_storage)
#define SUIT_STORAGE_RAD_OFFSET	   FIXED_PARTITION_OFFSET(cpurad_suit_storage)
#define SUIT_STORAGE_RAD_SIZE	   FIXED_PARTITION_SIZE(cpurad_suit_storage)
#define SUIT_STORAGE_APP_OFFSET	   FIXED_PARTITION_OFFSET(cpuapp_suit_storage)
#define SUIT_STORAGE_APP_SIZE	   FIXED_PARTITION_SIZE(cpuapp_suit_storage)

#define SUIT_STORAGE_APP_MPI_SIZE 0xf0
#define SUIT_STORAGE_RAD_MPI_SIZE 0x90

/* clang-format off */

/* RFC4122 uuid5(uuid.NAMESPACE_DNS, 'nordicsemi.com') */
const suit_manifest_class_id_t nordic_vid = {{
	0x76, 0x17, 0xda, 0xa5, 0x71, 0xfd, 0x5a, 0x85,
	0x8f, 0x94, 0xe2, 0x8d, 0x73, 0x5c, 0xe9, 0xf4
}};

/* RFC4122 uuid5(nordic_vid, 'nRF54H20_nordic_top') */
const suit_manifest_class_id_t nordic_top_cid = {{
	0xf0, 0x3d, 0x38, 0x5e, 0xa7, 0x31, 0x56, 0x05,
	0xb1, 0x5d, 0x03, 0x7f, 0x6d, 0xa6, 0x09, 0x7f
}};

/* RFC4122 uuid5(nordic_vid, 'nRF54H20_sec') */
const suit_manifest_class_id_t nordic_sdfw_cid = {{
	0xd9, 0x6b, 0x40, 0xb7, 0x09, 0x2b, 0x5c, 0xd1,
	0xa5, 0x9f, 0x9a, 0xf8, 0x0c, 0x33, 0x7e, 0xba
}};

/* RFC4122 uuid5(nordic_vid, 'nRF54H20_sys') */
const suit_manifest_class_id_t nordic_scfw_cid = {{
	0xc0, 0x8a, 0x25, 0xd7, 0x35, 0xe6, 0x59, 0x2c,
	0xb7, 0xad, 0x43, 0xac, 0xc8, 0xd1, 0xd1, 0xc8
}};

/* RFC4122 uuid5(nordic_vid, 'test_sample_root') */
const suit_manifest_class_id_t app_root_cid = {{
	0x97, 0x05, 0x48, 0x23, 0x4c, 0x3d, 0x59, 0xa1,
	0x89, 0x86, 0xa5, 0x46, 0x60, 0xa1, 0x4b, 0x0a,
}};

/* RFC4122 uuid5(nordic_vid, 'test_sample_app') */
const suit_manifest_class_id_t app_local_cid = {{
	0x5b, 0x46, 0x9f, 0xd1, 0x90, 0xee, 0x53, 0x9c,
	0xa3, 0x18, 0x68, 0x1b, 0x03, 0x69, 0x5e, 0x36,
}};

/* clang-format on */

void erase_area_nordic(void)
{
	/* Clear the whole nordic area */
	const struct device *fdev = SUIT_PLAT_INTERNAL_NVM_DEV;

	zassert_not_null(fdev, "Unable to find a driver to modify MPI area");

	int err = flash_erase(fdev, SUIT_STORAGE_NORDIC_OFFSET, SUIT_STORAGE_NORDIC_SIZE);

	zassert_equal(0, err, "Unable to erase nordic area before test execution");
}

void erase_area_rad(void)
{
	/* Clear the whole radio area */
	const struct device *fdev = SUIT_PLAT_INTERNAL_NVM_DEV;

	zassert_not_null(fdev, "Unable to find a driver to modify MPI area");

	int err = flash_erase(fdev, SUIT_STORAGE_RAD_OFFSET, SUIT_STORAGE_RAD_SIZE);

	zassert_equal(0, err, "Unable to erase radio core area before test execution");
}

void erase_area_app(void)
{
	/* Clear the whole application area */
	const struct device *fdev = SUIT_PLAT_INTERNAL_NVM_DEV;

	zassert_not_null(fdev, "Unable to find a driver to modify MPI area");

	int err = flash_erase(fdev, SUIT_STORAGE_APP_OFFSET, SUIT_STORAGE_APP_SIZE);

	zassert_equal(0, err, "Unable to erase application area before test execution");
}

void write_area_app(void)
{
	/* clang-format off */
	uint8_t mpi_root[] = {
		/* Application root MPI area */
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
		/* Application recovery MPI area (48) */
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		/* Application local 1 MPI area */
		0x01, /* version */
		0x01, /* downgrade prevention disabled */
		0x01, /* Independent update denied */
		0x01, /* signature check disabled */
		/* reserved (12) */
		0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		/* RFC4122 uuid5(uuid.NAMESPACE_DNS, 'nordicsemi.com') */
		0x76, 0x17, 0xda, 0xa5, 0x71, 0xfd, 0x5a, 0x85,
		0x8f, 0x94, 0xe2, 0x8d, 0x73, 0x5c, 0xe9, 0xf4,
		/* RFC4122 uuid5(nordic_vid, 'test_sample_app') */
		0x5b, 0x46, 0x9f, 0xd1, 0x90, 0xee, 0x53, 0x9c,
		0xa3, 0x18, 0x68, 0x1b, 0x03, 0x69, 0x5e, 0x36,
	};
	uint8_t app_digest[] = {
		0x44, 0xdc, 0x3d, 0xc5, 0xf3, 0xab, 0x75, 0xa5,
		0x22, 0xd6, 0x42, 0xce, 0x92, 0xb8, 0xbe, 0xf4,
		0x60, 0xcd, 0x7c, 0xab, 0xfa, 0x18, 0x8b, 0x91,
		0x69, 0x32, 0x0a, 0x8b, 0x9e, 0x35, 0x28, 0xa9,
	};
	/* clang-format on */

	/* Write the sample application area (just the root MPI) and corresponding digest */
	const struct device *fdev = SUIT_PLAT_INTERNAL_NVM_DEV;

	zassert_not_null(fdev, "Unable to find a driver to modify MPI area");

	int err = flash_write(fdev, SUIT_STORAGE_APP_OFFSET, mpi_root, sizeof(mpi_root));

	zassert_equal(0, err,
		      "Unable to store application root MPI contents before test execution");

	err = flash_write(fdev, SUIT_STORAGE_APP_OFFSET + SUIT_STORAGE_APP_MPI_SIZE, app_digest,
			  sizeof(app_digest));
	zassert_equal(0, err, "Unable to store application MPI digest before test execution");
}

void assert_nordic_classes(void)
{
	suit_manifest_class_info_t class_infos[CONFIG_SUIT_STORAGE_N_ENVELOPES];
	size_t class_infos_len = CONFIG_SUIT_STORAGE_N_ENVELOPES;

	/* ASSERT that it is possible to fetch list of supported manifest classes and roles */
	int err = suit_storage_mpi_class_ids_get(class_infos, &class_infos_len);

	zassert_equal(err, SUIT_PLAT_SUCCESS,
		      "Failed to fetch list of supported manifest classes (%d).", err);
	/* ... and MPI reports at least 3 class IDs */
	zassert_true(class_infos_len >= 3,
		     "Invalid number of supported manifest classes (%d < %d).", class_infos_len, 3);
	/* ... and the Nordic top manifest class is supported */
	zassert_mem_equal(class_infos[0].vendor_id, &nordic_vid, sizeof(nordic_vid));
	zassert_mem_equal(class_infos[0].class_id, &nordic_top_cid, sizeof(nordic_top_cid));
	zassert_equal(class_infos[0].role, SUIT_MANIFEST_SEC_TOP, "Invalid class role returned: %d",
		      class_infos[0].role);
	/* ... and the Nordic secure domain update & recovery manifest class is supported */
	zassert_mem_equal(class_infos[1].vendor_id, &nordic_vid, sizeof(nordic_vid));
	zassert_mem_equal(class_infos[1].class_id, &nordic_sdfw_cid, sizeof(nordic_sdfw_cid));
	zassert_equal(class_infos[1].role, SUIT_MANIFEST_SEC_SDFW,
		      "Invalid class role returned: %d", class_infos[3].role);
	/* ... and the Nordic system controller manifest class is supported */
	zassert_mem_equal(class_infos[2].vendor_id, &nordic_vid, sizeof(nordic_vid));
	zassert_mem_equal(class_infos[2].class_id, &nordic_scfw_cid, sizeof(nordic_scfw_cid));
	zassert_equal(class_infos[2].role, SUIT_MANIFEST_SEC_SYSCTRL,
		      "Invalid class role returned: %d", class_infos[3].role);
}

void assert_sample_app_classes(void)
{
	suit_manifest_class_info_t class_infos[CONFIG_SUIT_STORAGE_N_ENVELOPES];
	size_t class_infos_len = CONFIG_SUIT_STORAGE_N_ENVELOPES;

	/* ASSERT that it is possible to fetch list of supported manifest classes and roles */
	int err = suit_storage_mpi_class_ids_get(class_infos, &class_infos_len);

	zassert_equal(err, SUIT_PLAT_SUCCESS,
		      "Failed to fetch list of supported manifest classes (%d).", err);
	/* ... and MPI reports 5 supported class IDs */
	zassert_equal(class_infos_len, 5,
		      "Invalid number of supported manifest classes (%d != %d).", class_infos_len,
		      5);
	/* ... and the sample application root manifest class is supported */
	zassert_mem_equal(class_infos[3].vendor_id, &nordic_vid, sizeof(nordic_vid));
	zassert_mem_equal(class_infos[3].class_id, &app_root_cid, sizeof(app_root_cid));
	zassert_equal(class_infos[3].role, SUIT_MANIFEST_APP_ROOT,
		      "Invalid class role returned: %d", class_infos[4].role);
	/* ... and the sample application local manifest class is supported */
	zassert_mem_equal(class_infos[4].vendor_id, &nordic_vid, sizeof(nordic_vid));
	zassert_mem_equal(class_infos[4].class_id, &app_local_cid, sizeof(app_local_cid));
	zassert_equal(class_infos[4].role, SUIT_MANIFEST_APP_LOCAL_1,
		      "Invalid class role returned: %d", class_infos[4].role);
}
