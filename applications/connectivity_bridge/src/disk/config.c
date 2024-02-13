/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/fs/fs.h>
#include <zephyr/settings/settings.h>
#include <stdio.h>
#include <stdlib.h>

#define MODULE file_config
#include "module_state_event.h"
#include "fs_event.h"
#include "ble_ctrl_event.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_BRIDGE_MSC_LOG_LEVEL);

#if CONFIG_FS_FATFS_LFN
#define FILE_NAME         "Config.txt"
#else
#define FILE_NAME         "CONFIG.TXT"
#endif

#define SETTINGS_FILE_LINE_LEN_MAX 128

#define SETTINGS_KEY "cfg"

#define SETTINGS_DISP_NAME_MAX_LEN sizeof(union cfg_opt_display_name_len)
#define SETTINGS_VALUE_MAX_LEN sizeof(union cfg_opt_value_len)

/* List of configuration options
 * All options are treated as strings.
 * The handler functions parses according to desired type.
 * Null-terminator is always added to string length.
 *
 * To add a new configuration option, add a new list item according to format
 * described below, and a corresponding callback function.
 * The callback function will be triggered when a user changes the value
 * in the configuration .txt file and unmounts the USB drive.
 *
 * Format: (_key, _presentation_name, _size, _default, _callback, _enabled)
 */
#define CONFIG_LIST \
	X(ble_enable, BLE_ENABLED, 1, "0", ble_enable_opt_cb, (IS_ENABLED(CONFIG_BRIDGE_BLE_ENABLE) && !IS_ENABLED(CONFIG_BRIDGE_BLE_ALWAYS_ON))) \
	X(ble_name, BLE_NAME, CONFIG_BT_DEVICE_NAME_MAX, CONFIG_BT_DEVICE_NAME, ble_name_opt_cb, IS_ENABLED(CONFIG_BT_DEVICE_NAME_DYNAMIC))

struct cfg_option {
	const char *display_name;
	const char *settings_key;
	char *settings_value;
	const size_t settings_value_max_len;
	bool enabled;
	bool (*value_change_cb)(char *opt);
};

/* Define config value strings */
#define X(_key, _name, _size, _default, _cb, _enabled)\
		static char _key##_var[_size + 1] = _default;
	CONFIG_LIST
#undef X

/* Forward declare callback functions */
#define X(_key, _name, _size, _default, _cb, _enabled)\
		static bool _cb(char *);
	CONFIG_LIST
#undef X

/* Determine length of display name strings */
union cfg_opt_display_name_len {
#define X(_key, _name, _size, _default, _cb, _enabled)\
		uint8_t _key[sizeof(STRINGIFY(_name)) + 1];
	CONFIG_LIST
#undef X
};

/* Determine length of config value strings */
union cfg_opt_value_len {
#define X(_key, _name, _size, _default, _cb, _enabled)\
		uint8_t _key[_size + 1];
	CONFIG_LIST
#undef X
};

/* Determine length of settings key strings */
union cfg_opt_key_len {
#define X(_key, _name, _size, _default, _cb, _enabled)\
		uint8_t _key[sizeof(STRINGIFY(_key)) + 1];
	CONFIG_LIST
#undef X
};

BUILD_ASSERT(sizeof(union cfg_opt_key_len) <= SETTINGS_MAX_NAME_LEN);
BUILD_ASSERT(sizeof(union cfg_opt_value_len) <= SETTINGS_MAX_VAL_LEN);
BUILD_ASSERT((SETTINGS_DISP_NAME_MAX_LEN + SETTINGS_VALUE_MAX_LEN + sizeof("=\r\n")) <= SETTINGS_FILE_LINE_LEN_MAX);

/* Define boolean config value metadata structs */
static struct cfg_option configs[] = {
#define X(_key, _name, _size, _default, _cb, _enabled) \
		{.display_name = STRINGIFY(_name),\
		.settings_key = STRINGIFY(_key),\
		.settings_value = _key##_var,\
		.settings_value_max_len = _size,\
		.enabled = _enabled,\
		.value_change_cb = _cb},
	CONFIG_LIST
#undef X
};

static const char file_contents_header[] =
"=========================\r\n"
"          Configuration options\r\n"
"==========================================\r\n"
"The parameters below can be changed at runtime.\r\n"
"\r\n"
"NOTE: For changes to take effect,\r\n"
"safely disconnect (unmount) the drive and disconnect the USB cable.\r\n"
"==========================================\r\n";

static bool ble_enable_opt_cb(char *opt)
{
	enum ble_ctrl_cmd cmd;

	if (strcmp(opt, "0") == 0) {
		cmd = BLE_CTRL_DISABLE;
	} else if (strcmp(opt, "1") == 0) {
		cmd = BLE_CTRL_ENABLE;
	} else {
		LOG_WRN("Unrecognized ble_enable_opt: %s", opt);
		return false;
	}

	struct ble_ctrl_event *event = new_ble_ctrl_event();

	event->cmd = cmd;
	APP_EVENT_SUBMIT(event);

	return true;
}

static bool ble_name_opt_cb(char *opt)
{
	static char ble_name_buffer[CONFIG_BT_DEVICE_NAME_MAX + 1];

	__ASSERT_NO_MSG(strlen(opt) < sizeof(ble_name_buffer));

	strcpy(ble_name_buffer, opt);

	struct ble_ctrl_event *event = new_ble_ctrl_event();

	event->cmd = BLE_CTRL_NAME_UPDATE;
	event->param.name_update = ble_name_buffer;
	APP_EVENT_SUBMIT(event);

	return true;
}

static void save_settings(void)
{
	int err;

	for (int i = 0; i < ARRAY_SIZE(configs); ++i) {
		if (!configs[i].enabled) {
			/* Option not active */
			continue;
		}

		char key[SETTINGS_MAX_NAME_LEN + 1];

		err = snprintf(key,
				sizeof(key),
				SETTINGS_KEY "/%s",
				configs[i].settings_key);
		if (err < 0 || err > sizeof(key)) {
			LOG_ERR("Option format err: %s",
				configs[i].settings_key);
			continue;
		}

		err = settings_save_one(
				key,
				configs[i].settings_value,
				strlen(configs[i].settings_value));
		if (err) {
			LOG_ERR("settings_save_one: %d", err);
		}
	}
}

static int settings_set(const char *key, size_t len_rd,
			settings_read_cb read_cb, void *cb_arg)
{
	for (int i = 0; i < ARRAY_SIZE(configs); ++i) {
		if (!configs[i].enabled) {
			/* Option not active */
			continue;
		}

		if (strcmp(key, configs[i].settings_key) != 0) {
			continue;
		}

		char cfg_val[SETTINGS_VALUE_MAX_LEN];
		ssize_t len = read_cb(cb_arg, cfg_val, sizeof(cfg_val));

		if (len == 0) {
			continue;
		}

		if (len > configs[i].settings_value_max_len) {
			LOG_WRN("Setting value overflow");
			len = configs[i].settings_value_max_len;
		}

		/* null terminator included in string length */
		cfg_val[len] = '\0';

		if (strcmp(cfg_val, configs[i].settings_value) == 0) {
			/* No change */
			continue;
		}

		bool cfg_valid;

		cfg_valid = configs[i].value_change_cb(cfg_val);
		if (cfg_valid) {
			strcpy(configs[i].settings_value, cfg_val);
		}
	}

	return 0;
}

static void settings_init(void)
{
	int err;

	err = settings_subsys_init();
	if (err) {
		return;
	}

	static struct settings_handler sh = {
		.name = SETTINGS_KEY,
		.h_set = settings_set,
	};

	err = settings_register(&sh);
	if (err) {
		return;
	}

	err = settings_load();
	if (err) {
		return;
	}
}

/*
 * Read a buf_size-sized chunk from file into buf.
 * if a newline (\n or \r\n) is found in buf, it is replaced
 * with null terminator and the function returns true.
 * If no newline is found, or no data can be read, false is returned.
 * The file position is moved to after the newline character (when found).
 */
static bool read_line(struct fs_file_t *file, char *buf, size_t buf_size)
{
	ssize_t bytes_read;
	ssize_t newline_pos;

	bytes_read = fs_read(file, buf, buf_size);
	if (bytes_read <= 0) {
		return false;
	} else if (bytes_read < buf_size) {
		/* End of file: insert newline in case it is missing */
		buf[bytes_read] = '\n';
	}

	newline_pos = 0;

	for (int i = 1; i < bytes_read; ++i) {
		if (buf[i] == '\n') {
			newline_pos = i;
			break;
		}
	}

	if (newline_pos) {
		int err;

		err = fs_seek(file, newline_pos - bytes_read + 1, FS_SEEK_CUR);
		if (err) {
			LOG_ERR("fs_seek: %d", err);
			return false;
		}

		if (buf[newline_pos - 1] == '\r') {
			buf[newline_pos - 1] = '\0';
		} else {
			buf[newline_pos] = '\0';
		}

		return true;
	}

	return false;
}

/*
 * Check if buf contains configuration option 'name',
 * following the format "name=value".
 * If name matches, *val is updated to point after the '=' character
 * and the function returns true.
 */
static bool read_line_setting_val(char *buf, const char *name, char **val)
{
	if (strncmp(buf, name, strlen(name)) != 0) {
		return false;
	}

	for (int i = 0; i < (strlen(buf) - 1); ++i) {
		if (buf[i] == '=') {
			*val = &buf[i + 1];
			return true;
		}
	}

	return false;
}

static void parse_file(const char *mnt_point)
{
	struct fs_file_t file;
	char fname[SETTINGS_FILE_LINE_LEN_MAX];
	char linebuf[SETTINGS_FILE_LINE_LEN_MAX];
	int err;
	bool changes_made;

	err = snprintf(fname, sizeof(fname), "%s/" FILE_NAME, mnt_point);
	if (err <= 0 || err > sizeof(fname)) {
		LOG_ERR("filename format error");
		return;
	}

	fs_file_t_init(&file);
	err = fs_open(&file, fname, FS_O_CREATE | FS_O_RDWR);
	if (err) {
		LOG_ERR("fs_open: %d", err);
		return;
	}

	changes_made = false;

	while (read_line(&file, linebuf, sizeof(linebuf))) {
		/* For every line in file, check against all config options */
		for (int i = 0; i < ARRAY_SIZE(configs); ++i) {
			char *cfg_val;

			if (!read_line_setting_val(linebuf, configs[i].display_name, &cfg_val)) {
				/* Line does not match this config option */
				continue;
			}

			if (strcmp(cfg_val, configs[i].settings_value) == 0) {
				/* No change */
				break;
			}

			if (strlen(cfg_val) > configs[i].settings_value_max_len) {
				/* Values exceeding expected length are truncated */
				cfg_val[configs[i].settings_value_max_len] = '\0';
			}

			bool cfg_valid;

			cfg_valid = configs[i].value_change_cb(cfg_val);
			if (cfg_valid) {
				strcpy(configs[i].settings_value, cfg_val);
				changes_made = true;
			}

			break;
		}
	}

	if (changes_made) {
		save_settings();
	}

	err = fs_close(&file);
	if (err) {
		return;
	}
}

static bool app_event_handler(const struct app_event_header *aeh)
{
	if (is_fs_event(aeh)) {
		const struct fs_event *event =
			cast_fs_event(aeh);

		if (event->req == FS_REQUEST_CREATE_FILE) {
			int err;

			err = fs_event_helper_file_write(
				event->mnt_point,
				FILE_NAME,
				file_contents_header,
				strlen(file_contents_header));
			__ASSERT_NO_MSG(err == 0);

			/* Populate file with all options in list */

			for (int i = 0; i < ARRAY_SIZE(configs); ++i) {
				char linebuf[SETTINGS_DISP_NAME_MAX_LEN + SETTINGS_VALUE_MAX_LEN + sizeof("=\r\n")];
				int len;

				if (!configs[i].enabled) {
					continue;
				}

				len = snprintf(
					linebuf,
					sizeof(linebuf),
					"%s=%s\r\n",
					configs[i].display_name,
					configs[i].settings_value);
				if (len <= 0 || len > sizeof(linebuf)) {
					__ASSERT_NO_MSG(false);
					return false;
				}

				err = fs_event_helper_file_write(
					event->mnt_point,
					FILE_NAME,
					linebuf,
					len);
				__ASSERT_NO_MSG(err == 0);
			}
		} else if (event->req == FS_REQUEST_PARSE_FILE) {
			parse_file(event->mnt_point);
		}

		return false;
	}

	if (is_module_state_event(aeh)) {
		const struct module_state_event *event =
			cast_module_state_event(aeh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			settings_init();
		}

		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, module_state_event);
APP_EVENT_SUBSCRIBE(MODULE, fs_event);
