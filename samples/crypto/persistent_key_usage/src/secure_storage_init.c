/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "settings/settings_file.h"
#include <zephyr/device.h>
#include <zephyr/fs/fs.h>
#include <zephyr/fs/littlefs.h>
#include <zephyr/logging/log.h>

#define STORAGE_PARTITION    storage_partition
#define STORAGE_PARTITION_ID FIXED_PARTITION_ID(STORAGE_PARTITION)

LOG_MODULE_REGISTER(persistent_key_usage_secure_storage, LOG_LEVEL_DBG);

/* LittleFS work area strcut */
FS_LITTLEFS_DECLARE_DEFAULT_CONFIG(cstorage);
static struct fs_mount_t littlefs_mnt = {
	.type = FS_LITTLEFS,
	.fs_data = &cstorage,
	.storage_dev = (void *)STORAGE_PARTITION_ID,
	.mnt_point = "/0",
};

static int setup_settings_backend(void)
{
	int rc;

	rc = fs_mount(&littlefs_mnt);
	if (rc != 0) {
		LOG_ERR("%s failed (ret %d)", __func__, rc);
		return rc;
	}
	rc = settings_subsys_init();
	if (rc != 0) {
		LOG_ERR("%s failed (ret %d)", __func__, rc);
		return rc;
	}

	return 0;
}

SYS_INIT(setup_settings_backend, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

#ifdef CONFIG_SECURE_STORAGE_BACKEND_AEAD_KEY_DERIVE_FROM_HUK
#include <nrf_cc3xx_platform.h>
#include <hw_unique_key.h>

#ifndef HUK_HAS_KMU
#include <zephyr/sys/reboot.h>
#endif

int write_huk(void)
{
	if (!hw_unique_key_are_any_written()) {
		LOG_INF("Writing random keys to KMU\n");
		int result = hw_unique_key_write_random();

		if (result != HW_UNIQUE_KEY_SUCCESS) {
			LOG_ERR("hw_unique_key_write_random returned error: %d\n", result);
			return 0;
		}
		LOG_INF("Success!\n\n");
#if !defined(HUK_HAS_KMU)
		/* Reboot to allow the bootloader to load the key into CryptoCell. */
		sys_reboot(0);
#endif /* !defined(HUK_HAS_KMU) */
	}

	return 0;
}
#endif /* CONFIG_SECURE_STORAGE_BACKEND_AEAD_KEY_DERIVE_FROM_HUK */
