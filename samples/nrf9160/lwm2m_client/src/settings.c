/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */


#include <zephyr.h>
#include <settings/settings.h>

#include "settings.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(app_fota_settings, CONFIG_APP_LOG_LEVEL);

static struct update_counter uc;

int fota_update_counter_read(struct update_counter *update_counter)
{
	memcpy(update_counter, &uc, sizeof(uc));
	return 0;
}

int fota_update_counter_update(enum counter_type type, uint32_t new_value)
{
	if (type == COUNTER_UPDATE) {
		uc.update = new_value;
	} else {
		uc.current = new_value;
	}

	return settings_save_one("fota/counter", &uc, sizeof(uc));
}

static int set(const char *key, size_t len_rd, settings_read_cb read_cb,
	       void *cb_arg)
{
	int len;
	int key_len;
	const char *next;

	if (!key) {
		return -ENOENT;
	}

	key_len = settings_name_next(key, &next);

	if (!strncmp(key, "counter", key_len)) {
		len = read_cb(cb_arg, &uc, sizeof(uc));
		if (len < sizeof(uc)) {
			LOG_ERR("Unable to read update counter.  Resetting.");
			memset(&uc, 0, sizeof(uc));
		}

		return 0;
	}

	return -ENOENT;
}

static struct settings_handler fota_settings = {
	.name = "fota",
	.h_set = set,
};

int fota_settings_init(void)
{
	int err;

	err = settings_subsys_init();
	if (err) {
		LOG_ERR("settings_subsys_init failed (err %d)", err);
		return err;
	}

	err = settings_register(&fota_settings);
	if (err) {
		LOG_ERR("settings_register failed (err %d)", err);
		return err;
	}

	return 0;
}
