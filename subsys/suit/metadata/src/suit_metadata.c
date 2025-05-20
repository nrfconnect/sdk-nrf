/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <suit_metadata.h>
#include <string.h>

suit_metadata_err_t suit_metadata_uuid_compare(const suit_uuid_t *uuid1, const suit_uuid_t *uuid2)
{
	if (NULL == uuid1 || NULL == uuid2) {
		return SUIT_PLAT_ERR_INVAL;
	}

	if (0 == memcmp(uuid1->raw, uuid2->raw, sizeof(((suit_uuid_t *)0)->raw))) {
		return SUIT_PLAT_SUCCESS;
	}

	return METADATA_ERR_COMPARISON_FAILED;
}

suit_metadata_err_t suit_metadata_version_from_array(suit_version_t *version, int32_t *array,
						     size_t array_len)
{
	if ((version == NULL) || (array == NULL)) {
		return SUIT_PLAT_ERR_INVAL;
	}

	if (array_len == 0) {
		return SUIT_PLAT_ERR_SIZE;
	}

	if (array[0] < 0) {
		return SUIT_PLAT_ERR_OUT_OF_BOUNDS;
	}

	memset(version, 0, sizeof(*version));
	version->type = SUIT_VERSION_RELEASE_NORMAL;

	for (int i = 0; i < array_len; i++) {
		if (array[i] >= 0) {
			switch (i) {
			case 0:
				version->major = array[i];
				break;
			case 1:
				version->minor = array[i];
				break;
			case 2:
				version->patch = array[i];
				break;
			default:
				return SUIT_PLAT_ERR_OUT_OF_BOUNDS;
			}
		} else if (array[i] >= SUIT_VERSION_RELEASE_ALPHA) {
			version->type = (suit_version_release_type_t)array[i];
			if ((array_len == (i + 2)) && (array[i + 1] >= 0)) {
				version->pre_release_number = array[i + 1];
				return SUIT_PLAT_SUCCESS;
			} else if (array_len == (i + 1)) {
				return SUIT_PLAT_SUCCESS;
			}

			/* If negative integer is found - the processing is finished. */
			return SUIT_PLAT_ERR_OUT_OF_BOUNDS;
		} else {
			return SUIT_PLAT_ERR_OUT_OF_BOUNDS;
		}
	}

	return SUIT_PLAT_SUCCESS;
}

const char *suit_role_name_get(suit_manifest_role_t role)
{
#ifdef CONFIG_LOG
	switch (role) {
	case SUIT_MANIFEST_UNKNOWN:
		return " (Unknown)";
	case SUIT_MANIFEST_SEC_TOP:
		return " (Nordic: Top manifest)";
	case SUIT_MANIFEST_SEC_SDFW:
		return " (Nordic: SDFW & Recovery)";
	case SUIT_MANIFEST_SEC_SYSCTRL:
		return " (Nordic: SCFW)";
	case SUIT_MANIFEST_APP_ROOT:
		return " (Application: Root)";
	case SUIT_MANIFEST_APP_RECOVERY:
		return " (Application: Recovery)";
	case SUIT_MANIFEST_APP_LOCAL_1:
		return " (Application: Local 1)";
	case SUIT_MANIFEST_APP_LOCAL_2:
		return " (Application: Local 2)";
	case SUIT_MANIFEST_APP_LOCAL_3:
		return " (Application: Local 3)";
	case SUIT_MANIFEST_RAD_RECOVERY:
		return " (Radio: Recovery)";
	case SUIT_MANIFEST_RAD_LOCAL_1:
		return " (Radio: Local 1)";
	case SUIT_MANIFEST_RAD_LOCAL_2:
		return " (Radio: Local 2)";
	default:
		break;
	}
#endif /* CONFIG_LOG */

	return "";
}

const char *suit_manifest_class_name_get(const suit_manifest_class_id_t *class_id)
{
#ifdef CONFIG_LOG
	const uint8_t nRF54H20_nordic_top[] = {0xf0, 0x3d, 0x38, 0x5e, 0xa7, 0x31, 0x56, 0x05,
					       0xb1, 0x5d, 0x03, 0x7f, 0x6d, 0xa6, 0x09, 0x7f};
	const uint8_t nRF54H20_sec[] = {0xd9, 0x6b, 0x40, 0xb7, 0x09, 0x2b, 0x5c, 0xd1,
					0xa5, 0x9f, 0x9a, 0xf8, 0x0c, 0x33, 0x7e, 0xba};
	const uint8_t nRF54H20_sys[] = {0xc0, 0x8a, 0x25, 0xd7, 0x35, 0xe6, 0x59, 0x2c,
					0xb7, 0xad, 0x43, 0xac, 0xc8, 0xd1, 0xd1, 0xc8};
	const uint8_t test_sample_root[] = {0x97, 0x05, 0x48, 0x23, 0x4c, 0x3d, 0x59, 0xa1,
					    0x89, 0x86, 0xa5, 0x46, 0x60, 0xa1, 0x4b, 0x0a};
	const uint8_t test_sample_app[] = {0x5b, 0x46, 0x9f, 0xd1, 0x90, 0xee, 0x53, 0x9c,
					   0xa3, 0x18, 0x68, 0x1b, 0x03, 0x69, 0x5e, 0x36};
	const uint8_t test_sample_recovery[] = {0x74, 0xa0, 0xc6, 0xe7, 0xa9, 0x2a, 0x56, 0x00,
						0x9c, 0x5d, 0x30, 0xee, 0x87, 0x8b, 0x06, 0xba};
	const uint8_t nRF54H20_sample_root[] = {0x3f, 0x6a, 0x3a, 0x4d, 0xcd, 0xfa, 0x58, 0xc5,
						0xac, 0xce, 0xf9, 0xf5, 0x84, 0xc4, 0x11, 0x24};
	const uint8_t nRF54H20_app_recovery[] = {0xa3, 0x7b, 0x77, 0xb0, 0x87, 0x0e, 0x57, 0x49,
						 0xa8, 0x64, 0xf1, 0x44, 0x4a, 0xaf, 0xf5, 0x47};
	const uint8_t nRF54H20_sample_app[] = {0x08, 0xc1, 0xb5, 0x99, 0x55, 0xe8, 0x5f, 0xbc,
					       0x9e, 0x76, 0x7b, 0xc2, 0x9c, 0xe1, 0xb0, 0x4d};
	const uint8_t nRF54H20_sample_app_local_2[] = {0x51, 0xde, 0x10, 0xb8, 0xee, 0x2e,
						       0x5b, 0x4b, 0x80, 0xee, 0x53, 0x4a,
						       0x4a, 0x3c, 0x04, 0xfc};
	const uint8_t nRF54H20_sample_app_local_3[] = {0x2d, 0xca, 0x15, 0xa5, 0xa3, 0x2e,
						       0x5a, 0x71, 0xbe, 0x54, 0xba, 0x07,
						       0xbb, 0xaf, 0xae, 0x27};
	const uint8_t nRF54H20_rad_recovery[] = {0x58, 0x00, 0x98, 0xae, 0xb2, 0x40, 0x50, 0x66,
						 0xaf, 0x45, 0x57, 0x33, 0x25, 0xfc, 0x39, 0xf6};
	const uint8_t nRF54H20_sample_rad[] = {0x81, 0x6a, 0xa0, 0xa0, 0xaf, 0x11, 0x5e, 0xf2,
					       0x85, 0x8a, 0xfe, 0xb6, 0x68, 0xb2, 0xe9, 0xc9};
	const uint8_t nRF54H20_sample_rad_local_2[] = {0x0a, 0xa4, 0xbe, 0x57, 0xb3, 0x1e,
						       0x57, 0x9c, 0xab, 0x81, 0x13, 0xbd,
						       0x90, 0xe1, 0x6e, 0xf7};

	if (class_id == NULL) {
		return "";
	}

	if (memcmp(class_id->raw, nRF54H20_nordic_top, sizeof(nRF54H20_nordic_top)) == 0) {
		return " (nRF54H20_nordic_top)";
	} else if (memcmp(class_id->raw, nRF54H20_sec, sizeof(nRF54H20_sec)) == 0) {
		return " (nRF54H20_sec)";
	} else if (memcmp(class_id->raw, nRF54H20_sys, sizeof(nRF54H20_sys)) == 0) {
		return " (nRF54H20_sys)";
	} else if (memcmp(class_id->raw, test_sample_root, sizeof(test_sample_root)) == 0) {
		return " (test_sample_root)";
	} else if (memcmp(class_id->raw, test_sample_app, sizeof(test_sample_app)) == 0) {
		return " (test_sample_app)";
	} else if (memcmp(class_id->raw, test_sample_recovery, sizeof(test_sample_recovery)) == 0) {
		return " (test_sample_recovery)";
	} else if (memcmp(class_id->raw, nRF54H20_sample_root, sizeof(nRF54H20_sample_root)) == 0) {
		return " (nRF54H20_sample_root)";
	} else if (memcmp(class_id->raw, nRF54H20_app_recovery, sizeof(nRF54H20_app_recovery)) ==
		   0) {
		return " (nRF54H20_app_recovery)";
	} else if (memcmp(class_id->raw, nRF54H20_sample_app, sizeof(nRF54H20_sample_app)) == 0) {
		return " (nRF54H20_sample_app)";
	} else if (memcmp(class_id->raw, nRF54H20_sample_app_local_2,
			  sizeof(nRF54H20_sample_app_local_2)) == 0) {
		return " (nRF54H20_sample_app_local_2)";
	} else if (memcmp(class_id->raw, nRF54H20_sample_app_local_3,
			  sizeof(nRF54H20_sample_app_local_3)) == 0) {
		return " (nRF54H20_sample_app_local_3)";
	} else if (memcmp(class_id->raw, nRF54H20_rad_recovery, sizeof(nRF54H20_rad_recovery)) ==
		   0) {
		return " (nRF54H20_rad_recovery)";
	} else if (memcmp(class_id->raw, nRF54H20_sample_rad, sizeof(nRF54H20_sample_rad)) == 0) {
		return " (nRF54H20_sample_rad)";
	} else if (memcmp(class_id->raw, nRF54H20_sample_rad_local_2,
			  sizeof(nRF54H20_sample_rad_local_2)) == 0) {
		return " (nRF54H20_sample_rad_local_2)";
	}
#endif /* CONFIG_LOG */

	return "";
}
