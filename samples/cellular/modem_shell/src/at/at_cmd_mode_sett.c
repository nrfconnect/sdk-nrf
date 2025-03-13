/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <zephyr/drivers/uart.h>
#include <zephyr/settings/settings.h>

#include "mosh_print.h"
#include "at_cmd_mode_sett.h"

#define AT_CMD_MODE_SETT_KEY "at_cmd_mode_settings"

#define AT_CMD_MODE_SETT_AUTOSTART "autostart_enabled"
#define AT_CMD_MODE_SETT_ECHO_ON "echo_on"

static bool sett_autostart_enabled;

extern bool at_cmd_mode_echo_on;

/******************************************************************************/

/**@brief Callback when settings_load() is called. */
static int at_cmd_mode_sett_handler(const char *key, size_t len, settings_read_cb read_cb,
				    void *cb_arg)
{
	int ret;

	if (strcmp(key, AT_CMD_MODE_SETT_AUTOSTART) == 0) {
		ret = read_cb(cb_arg, &sett_autostart_enabled, sizeof(sett_autostart_enabled));
		if (ret < 0) {
			mosh_error("Failed to read sett_autostart_enabled, error: %d", ret);
			return ret;
		}
	} else if (strcmp(key, AT_CMD_MODE_SETT_ECHO_ON) == 0) {
		ret = read_cb(cb_arg, &at_cmd_mode_echo_on, sizeof(at_cmd_mode_echo_on));
		if (ret < 0) {
			mosh_error("Failed to read at_cmd_mode_echo_on, error: %d", ret);
			return ret;
		}
	}
	return 0;
}

/******************************************************************************/

int at_cmd_mode_sett_autostart_enabled(bool enabled)
{
	const char *key_enabled = AT_CMD_MODE_SETT_KEY "/" AT_CMD_MODE_SETT_AUTOSTART;
	int err;

	sett_autostart_enabled = enabled;

	mosh_print("at_cmd_mode autostarting %s", ((enabled == true) ? "enabled" : "disabled"));

	err = settings_save_one(key_enabled, &sett_autostart_enabled,
				sizeof(sett_autostart_enabled));
	if (err) {
		mosh_error("sett_autostart_enabled: err %d from settings_save_one()", err);
		return err;
	}

	return 0;
}

bool at_cmd_mode_sett_is_autostart_enabled(void)
{
	return sett_autostart_enabled;
}

int at_cmd_mode_sett_echo_on(bool enabled, bool print_status)
{
	const char *key_enabled = AT_CMD_MODE_SETT_KEY "/" AT_CMD_MODE_SETT_ECHO_ON;
	int err;

	at_cmd_mode_echo_on = enabled;

	if (print_status) {
		mosh_print("at_cmd_mode echo_%s", (enabled ? "on" : "off"));
	}

	err = settings_save_one(key_enabled, &at_cmd_mode_echo_on, sizeof(at_cmd_mode_echo_on));
	if (err) {
		if (print_status) {
			mosh_error("sett_echo_on: err %d from settings_save_one()", err);
		}
		return err;
	}

	return 0;
}

/******************************************************************************/

static struct settings_handler cfg = { .name = AT_CMD_MODE_SETT_KEY,
				       .h_set = at_cmd_mode_sett_handler };

int at_cmd_mode_sett_init(void)
{
	int ret = 0;

	sett_autostart_enabled = false;

	ret = settings_subsys_init();
	if (ret) {
		mosh_error("Failed to initialize settings subsystem, error: %d", ret);
		return ret;
	}

	ret = settings_register(&cfg);
	if (ret) {
		mosh_error("Cannot register settings handler %d", ret);
		return ret;
	}

	ret = settings_load();
	if (ret) {
		mosh_error("Cannot load settings %d", ret);
		return ret;
	}
	return ret;
}
