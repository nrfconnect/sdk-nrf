/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>
#include <dfu/dfu_target.h>
#include "slm_at_fota.h"
#include "slm_settings.h"

LOG_MODULE_REGISTER(slm_settings, CONFIG_SLM_LOG_LEVEL);

static int settings_set(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg)
{
	if (!strcmp(name, "modem_full_fota")) {
		if (len != sizeof(slm_modem_full_fota))
			return -EINVAL;
		if (read_cb(cb_arg, &slm_modem_full_fota, len) > 0)
			return 0;
	}
	/* Simply ignore obsolete settings that are not in use anymore.
	 * settings_delete() does not completely remove settings.
	 */
	return 0;
}

static struct settings_handler slm_settings_conf = {
	.name = "slm",
	.h_set = settings_set
};

int slm_settings_init(void)
{
	int ret;

	ret = settings_subsys_init();
	if (ret) {
		LOG_ERR("Init setting failed: %d", ret);
		return ret;
	}
	ret = settings_register(&slm_settings_conf);
	if (ret) {
		LOG_ERR("Register setting failed: %d", ret);
		return ret;
	}
	ret = settings_load_subtree("slm");
	if (ret) {
		LOG_ERR("Load setting failed: %d", ret);
	}

	return ret;
}

int slm_settings_fota_save(void)
{
	return settings_save_one("slm/modem_full_fota",
		&slm_modem_full_fota, sizeof(slm_modem_full_fota));
}
