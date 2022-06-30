/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <zephyr/shell/shell.h>
#include <zephyr/settings/settings.h>

#include "mosh_defines.h"
#include "mosh_print.h"

#include "startup_cmd_ctrl.h"
#include "startup_cmd_settings.h"

#define STARTUP_CMD_SETTINGS_KEY "startup_cmd_settings"

/* ****************************************************************************/

#define STARTUP_CMD_STARTTIME "starttime"
#define STARTUP_CMD_1_KEY     "startup_cmd_1"
#define STARTUP_CMD_2_KEY     "startup_cmd_2"
#define STARTUP_CMD_3_KEY     "startup_cmd_3"

/* ****************************************************************************/

struct startup_cmd_t {
	int starttime;
	char cmd1_str[STARTUP_CMD_MAX_LEN + 1];
	char cmd2_str[STARTUP_CMD_MAX_LEN + 1];
	char cmd3_str[STARTUP_CMD_MAX_LEN + 1];

};
static struct startup_cmd_t startup_cmd_data;

/* ****************************************************************************/

/**@brief Callback when settings_load() is called. */
static int startup_cmd_settings_handler(const char *key, size_t len,
			       settings_read_cb read_cb, void *cb_arg)
{
	int err;

	if (strcmp(key, STARTUP_CMD_STARTTIME) == 0) {
		err = read_cb(cb_arg, &startup_cmd_data.starttime,
			      sizeof(startup_cmd_data.starttime));
		if (err < 0) {
			mosh_error("Failed to read starttime, error: %d", err);
			return err;
		}
	} else if (strcmp(key, STARTUP_CMD_1_KEY) == 0) {
		err = read_cb(cb_arg, &startup_cmd_data.cmd1_str,
			      sizeof(startup_cmd_data.cmd1_str));
		if (err < 0) {
			mosh_error("Failed to read set_cmd #1, error: %d", err);
			return err;
		}
	} else if (strcmp(key, STARTUP_CMD_2_KEY) == 0) {
		err = read_cb(cb_arg, &startup_cmd_data.cmd2_str,
			      sizeof(startup_cmd_data.cmd2_str));
		if (err < 0) {
			mosh_error("Failed to read set_cmd #2, error: %d", err);
			return err;
		}
	} else if (strcmp(key, STARTUP_CMD_3_KEY) == 0) {
		err = read_cb(cb_arg, &startup_cmd_data.cmd3_str,
			      sizeof(startup_cmd_data.cmd3_str));
		if (err < 0) {
			mosh_error("Failed to read set_cmd #3, error: %d", err);
			return err;
		}
	}

	return 0;
}

/* ****************************************************************************/

char *startup_cmd_settings_get(uint8_t mem_slot)
{
	if (mem_slot == 1) {
		return startup_cmd_data.cmd1_str;
	} else if (mem_slot == 2) {
		return startup_cmd_data.cmd2_str;
	}
	__ASSERT_NO_MSG(mem_slot == 3);
	return startup_cmd_data.cmd3_str;
}

int startup_cmd_settings_save(const char *cmd_str, int mem_slot)
{
	int err = 0;
	const char *key;
	int len = strlen(cmd_str);
	char *tmp_ptr = NULL;

	__ASSERT_NO_MSG(mem_slot >= 1 && mem_slot <= STARTUP_CMD_MAX_COUNT);

	if (len > STARTUP_CMD_MAX_LEN) {
		mosh_error("%s: Too long cmd, max is %d", (__func__), STARTUP_CMD_MAX_LEN);
		return -EINVAL;
	}

	if (mem_slot == 1) {
		key = STARTUP_CMD_SETTINGS_KEY "/" STARTUP_CMD_1_KEY;
		tmp_ptr = startup_cmd_data.cmd1_str;
	} else if (mem_slot == 2) {
		key = STARTUP_CMD_SETTINGS_KEY "/" STARTUP_CMD_2_KEY;
		tmp_ptr = startup_cmd_data.cmd2_str;
	} else if (mem_slot == 3) {
		key = STARTUP_CMD_SETTINGS_KEY "/" STARTUP_CMD_3_KEY;
		tmp_ptr = startup_cmd_data.cmd3_str;
	} else {
		mosh_error("%s: unsupported memory slot %d", (__func__), mem_slot);
		err = -EINVAL;
		return err;
	}
	err = settings_save_one(key, cmd_str, len + 1);
	if (err) {
		mosh_error("%s: cannot save command %s to settings."
			   "err %d from settings_save_one()",
			   (__func__), cmd_str, err);
		return err;
	}
	strcpy(tmp_ptr, cmd_str);
	mosh_print("%s: key %s with value %s saved", (__func__), key, cmd_str);

	return 0;
}

bool startup_cmd_settings_enabled(void)
{
	if (strlen(startup_cmd_data.cmd1_str) ||
	    strlen(startup_cmd_data.cmd2_str) ||
	    strlen(startup_cmd_data.cmd3_str)) {
		return true;
	} else {
		return false;
	}
}

/* ****************************************************************************/

int startup_cmd_settings_starttime_set(int starttime)
{
	int ret;
	char *key;

	key = STARTUP_CMD_SETTINGS_KEY "/" STARTUP_CMD_STARTTIME;
	ret = settings_save_one(key, &starttime, sizeof(starttime));
	if (ret) {
		mosh_error("Cannot save start time %d to settings", starttime);
		return ret;
	}
	startup_cmd_data.starttime = starttime;

	mosh_print("%s: key %s with value %d saved", (__func__), key, starttime);
	return 0;
}

int startup_cmd_settings_starttime_get(void)
{
	return startup_cmd_data.starttime;
}

/* ****************************************************************************/

void startup_cmd_settings_conf_shell_print(void)
{
	int starttime = startup_cmd_settings_starttime_get();

	mosh_print("startup_cmd config:");
	if (starttime < 0) {
		mosh_print("  Start time: default");
	} else {
		mosh_print("  Start time: %d", starttime);
	}
	mosh_print("  Command #1: %s", startup_cmd_settings_get(1));
	mosh_print("  Command #2: %s", startup_cmd_settings_get(2));
	mosh_print("  Command #3: %s", startup_cmd_settings_get(3));
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
	startup_cmd_data.starttime = STARTUP_CMD_STARTTIME_NOT_SET;

	err = settings_subsys_init();
	if (err) {
		mosh_error("Failed to initialize settings subsystem, error: %d", err);
		return err;
	}
	err = settings_register(&cfg);
	if (err) {
		mosh_error("Cannot register settings handler %d", err);
		return err;
	}
	err = settings_load();
	if (err) {
		mosh_error("Cannot load settings %d", err);
		return err;
	}
	return 0;
}
