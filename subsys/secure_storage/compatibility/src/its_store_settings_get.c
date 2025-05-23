/* Copyright (c) 2025 Nordic Semiconductor ASA
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/secure_storage/its/store/settings_get.h>
#include <zephyr/sys/util.h>
#include <stdio.h>

/* prefix + '/' + 16-char hex UID */
BUILD_ASSERT(CONFIG_SECURE_STORAGE_ITS_STORE_SETTINGS_NAME_MAX_LEN ==
	     MAX(sizeof(CONFIG_PSA_PROTECTED_STORAGE_PREFIX),
		 sizeof(CONFIG_PSA_INTERNAL_TRUSTED_STORAGE_PREFIX)) - 1
	     + 1 + 16,
	     "CONFIG_SECURE_STORAGE_ITS_STORE_SETTINGS_NAME_MAX_LEN needs to be adjusted");

void secure_storage_its_store_settings_get_name(
	secure_storage_its_uid_t uid,
	char name[static SECURE_STORAGE_ITS_STORE_SETTINGS_NAME_BUF_SIZE])
{
	/* Both SECURE_STORAGE_ITS_CALLER_PSA_ITS and SECURE_STORAGE_ITS_CALLER_MBEDTLS
	 * indicate calls to the PSA ITS API.
	 */
	const char *prefix = (uid.caller_id == SECURE_STORAGE_ITS_CALLER_PSA_PS) ?
			     CONFIG_PSA_PROTECTED_STORAGE_PREFIX :
			     CONFIG_PSA_INTERNAL_TRUSTED_STORAGE_PREFIX;

	snprintf(name, SECURE_STORAGE_ITS_STORE_SETTINGS_NAME_BUF_SIZE,
		 "%s/%08x%08x", prefix, (unsigned int)(uid.uid >> 32),
		 (unsigned int)(uid.uid & 0xffffffff));
}
