/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <fs/fs.h>
#include <settings/settings.h>
#include <stdio.h>
#include <stdlib.h>

#define MODULE file_config
#include "module_state_event.h"
#include "fs_event.h"
#include "ble_ctrl_event.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_BRIDGE_MSC_LOG_LEVEL);

#if CONFIG_FS_FATFS_LFN
#define FILE_NAME         "Config.txt"
#else
#define FILE_NAME         "CONFIG.TXT"
#endif

#define SETTINGS_KEY "bridgecfg"

#define SETTINGS_KEY_BLE "ble"
#define SETTINGS_NAME_BLE "BLE_ENABLED"

static const char file_contents_header[] =
"==========================================\r\n"
"  Nordic Thingy:91 Configuration options\r\n"
"==========================================\r\n"
"The parameters below can be changed at runtime.\r\n"
"\r\n"
"NOTE: For changes to take effect,\r\n"
"safely disconnect (unmount) the drive and disconnect the USB cable.\r\n"
"==========================================\r\n";

static bool ble_enabled;

static void send_ble_ctrl_event(bool enable)
{
	struct ble_ctrl_event *event = new_ble_ctrl_event();

	event->cmd = enable ? BLE_CTRL_ENABLE : BLE_CTRL_DISABLE;
	EVENT_SUBMIT(event);
}

static void save_settings(void)
{
	int err;

	err = settings_save_one(
			SETTINGS_KEY "/" SETTINGS_KEY_BLE,
			&ble_enabled, sizeof(ble_enabled));
	if (err) {
		LOG_ERR("settings_save_one: %d", err);
	}
}

static int settings_set(const char *key, size_t len_rd,
			settings_read_cb read_cb, void *cb_arg)
{
	if (strcmp(key, SETTINGS_KEY_BLE) == 0) {
		bool val = 0;
		ssize_t len = read_cb(cb_arg, &val, sizeof(val));

		if (len == sizeof(val)) {
			ble_enabled = val;
		}
	}

	return 0;
}

static int commit(void)
{
	if (ble_enabled) {
		send_ble_ctrl_event(true);
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
		.h_commit = commit,
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
/**
 * Read a buf_size-sized chunk from file into buf.
 * if a newline (\n or \r\n) is found in buf, it is replaced
 * with null terminator and the function returns true.
 * If no newline is found, or no data can be read, false is returned.
 * The file position is moved to after the newline character (when found).
 */
static bool read_line(struct fs_file_t *file, char *buf, size_t buf_size)
{
	ssize_t bytes_read;
	ssize_t pos;

	bytes_read = fs_read(file, buf, buf_size);
	if (bytes_read <= 0) {
		return false;
	}

	pos = 0;

	for (int i = 1; i < bytes_read; ++i) {
		if (buf[i] == '\n') {
			pos = i;
			break;
		}
	}

	if (pos) {
		int err;

		err = fs_seek(file, pos - bytes_read + 1, FS_SEEK_CUR);
		if (err) {
			LOG_ERR("fs_seek: %d", err);
			return false;
		}

		if (buf[pos - 1] == '\r') {
			buf[pos - 1] = '\0';
		} else {
			buf[pos] = '\0';
		}

		return true;
	}

	return false;
}

static bool read_line_setting_val(char *buf, const char *key, char **val)
{
	if (strncmp(buf, key, strlen(key)) != 0) {
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
	char fname[128];
	char linebuf[128];
	int err;

	err = snprintf(fname, sizeof(fname), "%s/" FILE_NAME, mnt_point);
	if (err <= 0 || err > sizeof(fname)) {
		return;
	}

	err = fs_open(&file, fname);
	if (err) {
		LOG_ERR("fs_open: %d", err);
		return;
	}

	while (read_line(&file, linebuf, sizeof(linebuf))) {
#if CONFIG_BRIDGE_BLE_ENABLE
		char *ptr;
		u32_t ble_enabled_new = ble_enabled;

		if (read_line_setting_val(linebuf, SETTINGS_NAME_BLE, &ptr)) {
			ble_enabled_new = strtol(ptr, NULL, 10);
		}

		if (ble_enabled_new != ble_enabled) {
			ble_enabled = ble_enabled_new;

			save_settings();
			send_ble_ctrl_event(ble_enabled);
		}
#endif
	}

	err = fs_close(&file);
	if (err) {
		return;
	}
}

static bool event_handler(const struct event_header *eh)
{
	if (is_fs_event(eh)) {
		const struct fs_event *event =
			cast_fs_event(eh);

		if (event->req == FS_REQUEST_CREATE_FILE) {
			int err;

			err = fs_event_helper_file_write(
				event->mnt_point,
				FILE_NAME,
				file_contents_header,
				strlen(file_contents_header));
			__ASSERT_NO_MSG(err == 0);

#if CONFIG_BRIDGE_BLE_ENABLE && !CONFIG_BRIDGE_BLE_ALWAYS_ON
			char str[sizeof(SETTINGS_NAME_BLE) + 16];
			int len;

			len = snprintf(str, sizeof(str), SETTINGS_NAME_BLE "=%d\r\n", ble_enabled);
			if (len <= 0 || len > sizeof(str)) {
				__ASSERT_NO_MSG(false);
				return false;
			}

			err = fs_event_helper_file_write(
				event->mnt_point,
				FILE_NAME,
				str,
				len);
			__ASSERT_NO_MSG(err == 0);
#endif /* CONFIG_BRIDGE_BLE_ENABLE */
		} else if (event->req == FS_REQUEST_PARSE_FILE) {
			parse_file(event->mnt_point);
		}

		return false;
	}

	if (is_module_state_event(eh)) {
		const struct module_state_event *event =
			cast_module_state_event(eh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			settings_init();
		}
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, module_state_event);
EVENT_SUBSCRIBE(MODULE, fs_event);
