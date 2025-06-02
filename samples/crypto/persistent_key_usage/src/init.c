/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>

LOG_MODULE_DECLARE(persistent_key_usage, LOG_LEVEL_DBG);

#ifdef CONFIG_TRUSTED_STORAGE_STORAGE_BACKEND_SETTINGS

static int setup_settings_backend(void)
{
	int rc = settings_subsys_init();

	if (rc != 0) {
		LOG_ERR("%s failed (ret %d)", __func__, rc);
		return rc;
	}

	return 0;
}

SYS_INIT(setup_settings_backend, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

#endif /* CONFIG_TRUSTED_STORAGE_STORAGE_BACKEND_SETTINGS */

#if	defined(CONFIG_TRUSTED_STORAGE_BACKEND_AEAD_KEY_DERIVE_FROM_HUK) || \
	defined(CONFIG_SECURE_STORAGE_ITS_TRANSFORM_AEAD_KEY_PROVIDER_HUK_LIBRARY)

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
#ifndef HUK_HAS_KMU
		/* Reboot to allow the bootloader to load the key into CryptoCell. */
		sys_reboot(0);
#endif
	}

	return 0;
}
#endif
