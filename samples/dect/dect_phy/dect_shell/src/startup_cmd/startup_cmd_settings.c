/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <zephyr/shell/shell.h>
#include <zephyr/settings/settings.h>

#include "desh_defines.h"
#include "desh_print.h"

#include "startup_cmd_ctrl.h"
#include "startup_cmd_settings.h"

#define STARTUP_CMD_SETTINGS_KEY "startup_cmd"
#define STARTUP_CMD_SETTINGS_VAL "data"

/* ****************************************************************************/

static struct startup_cmd_data startup_cmd_data;

/* ****************************************************************************/

/**@brief Callback when settings_load() is called. */
static int startup_cmd_settings_handler(
	const char *key, size_t len, settings_read_cb read_cb, void *cb_arg)
{
	int ret;

	if (strcmp(key, STARTUP_CMD_SETTINGS_VAL) == 0) {
		ret = read_cb(cb_arg, &startup_cmd_data, sizeof(startup_cmd_data));
		if (ret < 0) {
			printk("Failed to read dect startup_cmd_data, error: %d", ret);
			return ret;
		}
		return 0;
	}
	return -ENOENT;
}

/* ****************************************************************************/

char *startup_cmd_settings_cmd_str_get(uint8_t mem_slot)
{
	__ASSERT_NO_MSG(mem_slot >= 1 && mem_slot <= STARTUP_CMD_MAX_COUNT);
	return startup_cmd_data.cmd[mem_slot - 1].cmd_str;
}

int startup_cmd_settings_cmd_delay_get(int mem_slot)
{
	__ASSERT_NO_MSG(mem_slot >= 1 && mem_slot <= STARTUP_CMD_MAX_COUNT);
	return startup_cmd_data.cmd[mem_slot - 1].delay;
}

int startup_cmd_settings_starttime_get(void)
{
	return startup_cmd_data.starttime;
}


int startup_cmd_settings_data_read(struct startup_cmd_data *out_startup_cmd_data)
{
	memcpy(out_startup_cmd_data, &startup_cmd_data, sizeof(struct startup_cmd_data));
	return 0;
}

int startup_cmd_settings_cmd_data_read_by_memslot(
	struct startup_cmd *out_cmd_data, int mem_slot)
{
	if (mem_slot < 1 || mem_slot > STARTUP_CMD_MAX_COUNT) {
		return -EINVAL;
	}
	memcpy(out_cmd_data, &startup_cmd_data.cmd[mem_slot - 1], sizeof(struct startup_cmd));
	return 0;
}


int startup_cmd_settings_data_save(struct startup_cmd_data *new_startup_cmd_data)
{
	int ret = settings_save_one(STARTUP_CMD_SETTINGS_KEY "/" STARTUP_CMD_SETTINGS_VAL,
				    new_startup_cmd_data, sizeof(struct startup_cmd_data));

	if (ret) {
		desh_error("Cannot save startup_cmd settings, err: %d", ret);
		return ret;
	}

	startup_cmd_data = *new_startup_cmd_data;

	desh_print("startup_cmd settings saved");

	return ret;
}

bool startup_cmd_settings_enabled(void)
{
	for (uint8_t i = 0; i < STARTUP_CMD_MAX_COUNT; i++) {
		if (strlen(startup_cmd_data.cmd[i].cmd_str)) {
			return true;
		}
	}
	return false;
}

/* ****************************************************************************/

void startup_cmd_settings_data_print(void)
{
	int starttime = startup_cmd_settings_starttime_get();
	char *cmd_str;

	desh_print("startup_cmd config:");
	desh_print("  Start time: %d", starttime);

	for (uint8_t i = 1; i <= STARTUP_CMD_MAX_COUNT; i++) {
		desh_print("  Command #%d:", i);
		cmd_str = startup_cmd_settings_cmd_str_get(i);
		if (strlen(cmd_str)) {
			desh_print("    String:    %s", cmd_str);
			desh_print("    Delay:     %d", startup_cmd_settings_cmd_delay_get(i));
		} else {
			desh_print("    No command");
		}
	}
}

/* ****************************************************************************/

static int startup_cmd_settings_loaded(void)
{
	/* All loaded, let's make them effective. */
	startup_cmd_ctrl_settings_loaded();
	return 0;
}

static struct settings_handler cfg = { .name = STARTUP_CMD_SETTINGS_KEY,
				       .h_set = startup_cmd_settings_handler,
				       .h_commit = startup_cmd_settings_loaded };

int startup_cmd_settings_init(void)
{
	int err;

	memset(&startup_cmd_data, 0, sizeof(startup_cmd_data));
	startup_cmd_data.starttime = STARTUP_CMD_STARTTIME_DEFAULT;
	for (int i = 0; i < STARTUP_CMD_MAX_COUNT; i++) {
		startup_cmd_data.cmd[i].delay = STARTUP_CMD_DELAY_DEFAULT;
	}

	err = settings_subsys_init();
	if (err) {
		desh_error("(%s): Failed to initialize settings subsystem, error: %d",
			(__func__), err);
		return err;
	}
	err = settings_register(&cfg);
	if (err) {
		desh_error("(%s): Cannot register settings handler %d", (__func__), err);
		return err;
	}
	err = settings_load();
	if (err) {
		desh_error("(%s): Cannot load settings %d", (__func__), err);
		return err;
	}
	return 0;
}
