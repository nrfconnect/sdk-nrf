/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * Minimal fakes required to link nrf_cloud_codec.c in the test environment.
 *
 * nrf_cloud_codec.c calls three helper functions declared in
 * nrf_cloud_codec_internal.h and implemented in nrf_cloud_codec_internal.c.
 * Including the full nrf_cloud_codec_internal.c would pull in modem, FOTA,
 * Bluetooth, and other subsystems that are not needed for pure JSON codec
 * tests.  The three helpers are simple cJSON wrappers, so we reproduce them
 * here using the same logic as the originals.
 *
 * nrf_cloud_codec.c also references the nrf_cloud_calloc / nrf_cloud_free /
 * nrf_cloud_malloc memory wrappers from nrf_cloud_mem.c; those are provided
 * here as thin wrappers around the standard C library allocator (the Zephyr
 * kernel heap is not available in this minimal test configuration).
 */

#include <stdbool.h>
#include <stdlib.h>
#include <cJSON.h>
#include <errno.h>
#include <nrf_cloud_codec_internal.h>
#include <nrf_cloud_mem.h>

/*
 * cJSON helper fakes (mirrors of nrf_cloud_codec_internal.c).
 *
 * The real implementations live in nrf_cloud_codec_internal.c, which cannot
 * be compiled here because it pulls in modem_info, alerts, logging, and other
 * subsystems unavailable in this minimal test config.
 *
 * Signature correctness is enforced at compile time via the
 * nrf_cloud_codec_internal.h include above.  Behavioral divergence (e.g. a
 * changed error code) would not be caught here; if the real implementations
 * change materially, these fakes must be updated to match.
 */

int get_string_from_obj(const cJSON *const obj, const char *const key, char **string_out)
{
	if (!obj) {
		return -ENOENT;
	}

	cJSON *item = cJSON_GetObjectItem(obj, key);

	if (!cJSON_IsString(item)) {
		return item ? -ENOMSG : -ENODEV;
	}

	*string_out = item->valuestring;
	return 0;
}

int get_num_from_obj(const cJSON *const obj, const char *const key, double *num_out)
{
	if (!obj) {
		return -ENOENT;
	}

	cJSON *item = cJSON_GetObjectItem(obj, key);

	if (!cJSON_IsNumber(item)) {
		return item ? -ENOMSG : -ENODEV;
	}

	*num_out = item->valuedouble;
	return 0;
}

int get_bool_from_obj(const cJSON *const obj, const char *const key, bool *bool_out)
{
	if (!obj) {
		return -ENOENT;
	}

	cJSON *item = cJSON_GetObjectItem(obj, key);

	if (!cJSON_IsBool(item)) {
		return item ? -ENOMSG : -ENODEV;
	}

	*bool_out = (bool)cJSON_IsTrue(item);
	return 0;
}

/* Memory wrapper fakes (mirrors of nrf_cloud_mem.c) */

void *nrf_cloud_calloc(size_t count, size_t size)
{
	return calloc(count, size);
}

void *nrf_cloud_malloc(size_t size)
{
	return malloc(size);
}

void nrf_cloud_free(void *ptr)
{
	free(ptr);
}
