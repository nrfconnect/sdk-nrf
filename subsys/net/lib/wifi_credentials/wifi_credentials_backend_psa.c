/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "psa/protected_storage.h"

#include "wifi_credentials_internal.h"

LOG_MODULE_REGISTER(wifi_credentials_backend, CONFIG_WIFI_CREDENTIALS_LOG_LEVEL);

int wifi_credentials_backend_init(void)
{
	psa_status_t ret;
	uint8_t buf[ENTRY_MAX_LEN];

	for (size_t i = 0; i < CONFIG_WIFI_CREDENTIALS_MAX_ENTRIES; ++i) {
		size_t length_read = 0;

		ret = psa_ps_get(i + CONFIG_WIFI_CREDENTIALS_BACKEND_PSA_UID_OFFSET, 0,
				 ARRAY_SIZE(buf), buf, &length_read);
		if (ret == PSA_SUCCESS && length_read >= sizeof(struct wifi_credentials_header)) {
			wifi_credentials_cache_ssid(i, (struct wifi_credentials_header *)buf);
		}
	}
	return 0;
}

int wifi_credentials_store_entry(size_t idx, const void *buf, size_t buf_len)
{
	return psa_ps_set(idx + CONFIG_WIFI_CREDENTIALS_BACKEND_PSA_UID_OFFSET, buf_len, buf,
			  PSA_STORAGE_FLAG_NONE);
}

int wifi_credentials_delete_entry(size_t idx)
{
	return psa_ps_remove(idx + CONFIG_WIFI_CREDENTIALS_BACKEND_PSA_UID_OFFSET);
}

int wifi_credentials_load_entry(size_t idx, void *buf, size_t buf_len)
{
	size_t length_read = 0;
	int ret = psa_ps_get(idx + CONFIG_WIFI_CREDENTIALS_BACKEND_PSA_UID_OFFSET, 0, buf_len, buf,
			     &length_read);

	if (ret) {
		return ret;
	}

	if (buf_len != length_read) {
		return -EFAULT;
	}

	return 0;
}
