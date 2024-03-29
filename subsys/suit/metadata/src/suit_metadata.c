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
