/*
 * Copyright (c) 2026 Nordic Semiconductor
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/fs/fs.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/logging/log_backend.h>

#include "fs_dump_ble.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(fs_dump_ble, LOG_LEVEL_INF);

#define FS_DUMP_CMD_MAX_LEN 64
#define FS_DUMP_PATH_MAX_LEN 96
#define FS_DUMP_READ_CHUNK 8
#define FS_DUMP_MAX_FILE_BYTES CONFIG_LOG_BACKEND_FS_FILE_SIZE

#define BT_UUID_FS_DUMP_SERVICE_VAL \
	BT_UUID_128_ENCODE(0x3a0b25f0, 0x8d4b, 0x4cde, 0x9f80, 0x7d3cf6f8a100)
#define BT_UUID_FS_DUMP_CMD_VAL \
	BT_UUID_128_ENCODE(0x3a0b25f1, 0x8d4b, 0x4cde, 0x9f80, 0x7d3cf6f8a100)
#define BT_UUID_FS_DUMP_DATA_VAL \
	BT_UUID_128_ENCODE(0x3a0b25f2, 0x8d4b, 0x4cde, 0x9f80, 0x7d3cf6f8a100)

static struct bt_uuid_128 fs_dump_service_uuid = BT_UUID_INIT_128(
	BT_UUID_FS_DUMP_SERVICE_VAL);
static struct bt_uuid_128 fs_dump_cmd_uuid = BT_UUID_INIT_128(
	BT_UUID_FS_DUMP_CMD_VAL);
static struct bt_uuid_128 fs_dump_data_uuid = BT_UUID_INIT_128(
	BT_UUID_FS_DUMP_DATA_VAL);

#define BT_UUID_FS_DUMP_SERVICE (&fs_dump_service_uuid.uuid)
#define BT_UUID_FS_DUMP_CMD (&fs_dump_cmd_uuid.uuid)
#define BT_UUID_FS_DUMP_DATA (&fs_dump_data_uuid.uuid)

static K_MUTEX_DEFINE(fs_dump_lock);
static struct bt_conn *fs_dump_conn;
static bool fs_dump_notify_enabled;
static char fs_dump_cmd_buf[FS_DUMP_CMD_MAX_LEN + 1];
static uint8_t fs_dump_file_buf[FS_DUMP_MAX_FILE_BYTES];
K_THREAD_STACK_DEFINE(fs_dump_wq_stack, 2048);
static struct k_work_q fs_dump_wq;

static void fs_backend_pause(const struct log_backend **backend, bool *resume_needed)
{
	*backend = log_backend_get_by_name("log_backend_fs");
	*resume_needed = false;

	if ((*backend != NULL) && log_backend_is_active(*backend)) {
		log_backend_deactivate(*backend);
		*resume_needed = true;
	}
}

static void fs_backend_resume(const struct log_backend *backend, bool resume_needed)
{
	if (resume_needed && (backend != NULL)) {
		log_backend_activate(backend, backend->cb->ctx);
	}
}

static void fs_dump_work_handler(struct k_work *work);
static K_WORK_DEFINE(fs_dump_work, fs_dump_work_handler);

static const struct bt_gatt_attr *fs_dump_data_attr_get(void);

static size_t notify_payload_max(struct bt_conn *conn)
{
	uint16_t mtu = bt_gatt_get_mtu(conn);

	if (mtu <= 3U) {
		return 20U;
	}

	return mtu - 3U;
}

static int notify_raw(struct bt_conn *conn, const uint8_t *data, size_t len)
{
	return bt_gatt_notify(conn, fs_dump_data_attr_get(), data, len);
}

static int notify_text(struct bt_conn *conn, const char *text)
{
	size_t len = strlen(text);
	size_t pos = 0U;
	size_t chunk_max = notify_payload_max(conn);

	while (pos < len) {
		size_t chunk = MIN(chunk_max, len - pos);
		int err;
		int retries = 0;

		do {
			err = notify_raw(conn, (const uint8_t *)&text[pos], chunk);
			if ((err == -ENOMEM) || (err == -EAGAIN)) {
				k_sleep(K_MSEC(10));
				retries++;
			}
		} while (((err == -ENOMEM) || (err == -EAGAIN)) && (retries < 200));

		if (err) {
			LOG_ERR("notify failed err=%d pos=%u chunk=%u", err,
				(unsigned int)pos, (unsigned int)chunk);
			return err;
		}

		pos += chunk;
		k_sleep(K_MSEC(2));
	}

	return 0;
}

static void notify_error_line(struct bt_conn *conn, const char *msg, int err)
{
	char line[64];

	snprintk(line, sizeof(line), "ERR %s (%d)\n", msg, err);
	(void)notify_text(conn, line);
}

static void ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	ARG_UNUSED(attr);

	k_mutex_lock(&fs_dump_lock, K_FOREVER);
	fs_dump_notify_enabled = (value == BT_GATT_CCC_NOTIFY);
	k_mutex_unlock(&fs_dump_lock);

	LOG_INF("FS dump notifications %s", fs_dump_notify_enabled ? "enabled" : "disabled");
}

static ssize_t cmd_write(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	uint16_t copy_len;

	ARG_UNUSED(attr);
	ARG_UNUSED(flags);

	if (conn == NULL) {
		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	if (offset != 0U) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	copy_len = MIN(len, (uint16_t)FS_DUMP_CMD_MAX_LEN);

	k_mutex_lock(&fs_dump_lock, K_FOREVER);

	memcpy(fs_dump_cmd_buf, buf, copy_len);
	fs_dump_cmd_buf[copy_len] = '\0';

	if (fs_dump_conn != NULL) {
		bt_conn_unref(fs_dump_conn);
	}
	fs_dump_conn = bt_conn_ref(conn);

	k_mutex_unlock(&fs_dump_lock);

	(void)k_work_submit_to_queue(&fs_dump_wq, &fs_dump_work);

	return len;
}

BT_GATT_SERVICE_DEFINE(fs_dump_svc,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_FS_DUMP_SERVICE),
	BT_GATT_CHARACTERISTIC(BT_UUID_FS_DUMP_CMD,
			       BT_GATT_CHRC_WRITE | BT_GATT_CHRC_WRITE_WITHOUT_RESP,
			       BT_GATT_PERM_WRITE, NULL, cmd_write, NULL),
	BT_GATT_CHARACTERISTIC(BT_UUID_FS_DUMP_DATA, BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_NONE, NULL, NULL, NULL),
	BT_GATT_CCC(ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
);

static const struct bt_gatt_attr *fs_dump_data_attr_get(void)
{
	return &fs_dump_svc.attrs[4];
}

static const char *skip_spaces(const char *s)
{
	while ((*s == ' ') || (*s == '\t')) {
		s++;
	}

	return s;
}

static void trim_trailing(char *s)
{
	size_t len = strlen(s);

	while (len > 0U) {
		char c = s[len - 1U];

		if ((c != '\r') && (c != '\n') && (c != ' ') && (c != '\t')) {
			break;
		}

		s[len - 1U] = '\0';
		len--;
	}
}

static bool invalid_filename(const char *name)
{
	if ((name == NULL) || (name[0] == '\0')) {
		return true;
	}

	if ((strstr(name, "..") != NULL) || (strchr(name, '/') != NULL)) {
		return true;
	}

	return false;
}

static void handle_list(struct bt_conn *conn)
{
	const struct log_backend *fs_backend;
	bool fs_resume_needed;
	struct fs_dir_t dir;
	struct fs_dirent ent;
	size_t prefix_len = strlen(CONFIG_LOG_BACKEND_FS_FILE_PREFIX);
	bool listed = false;
	int rc;

	/* Flush pending logs first so LIST reflects recent file creation/rotation. */
	log_flush();
	fs_backend_pause(&fs_backend, &fs_resume_needed);

	rc = notify_text(conn, "OK LIST BEGIN\n");
	if (rc) {
		goto out;
	}

	fs_dir_t_init(&dir);
	rc = fs_opendir(&dir, CONFIG_LOG_BACKEND_FS_DIR);
	if (rc != 0) {
		notify_error_line(conn, "LIST opendir failed", rc);
		goto out;
	}

	while (true) {
		char line[64];

		rc = fs_readdir(&dir, &ent);
		if (rc != 0) {
			break;
		}

		if (ent.name[0] == '\0') {
			break;
		}

		if (ent.type != FS_DIR_ENTRY_FILE) {
			continue;
		}

		if (strncmp(ent.name, CONFIG_LOG_BACKEND_FS_FILE_PREFIX, prefix_len) != 0) {
			continue;
		}

		snprintk(line, sizeof(line), "FILE %.52s\n", ent.name);
		rc = notify_text(conn, line);
		if (rc) {
			(void)fs_closedir(&dir);
			goto out;
		}

		/* Keep LIST bounded and deterministic for demo and host tooling. */
		listed = true;
		break;
	}

	(void)fs_closedir(&dir);
	if (!listed) {
		(void)notify_text(conn, "FILE none\n");
	}
	(void)notify_text(conn, "OK LIST END\n");

out:
	fs_backend_resume(fs_backend, fs_resume_needed);
}

static void handle_dump(struct bt_conn *conn, const char *name)
{
	const struct log_backend *fs_backend;
	bool fs_resume_needed;
	struct fs_file_t file;
	struct fs_dirent ent;
	char path[FS_DUMP_PATH_MAX_LEN];
	char chunk_line[2 + (FS_DUMP_READ_CHUNK * 2) + 2];
	char control_line[64];
	int path_len;
	int total = 0;
	int rd;
	int rc;

	/* Flush pending logs before reading files to avoid partial/trailing records. */
	log_flush();
	fs_backend_pause(&fs_backend, &fs_resume_needed);

	if (invalid_filename(name)) {
		notify_error_line(conn, "DUMP invalid filename", -EINVAL);
		goto out;
	}

	path_len = snprintk(path, sizeof(path), "%s/%s", CONFIG_LOG_BACKEND_FS_DIR, name);
	if ((path_len <= 0) || (path_len >= (int)sizeof(path))) {
		notify_error_line(conn, "DUMP filename too long", -ENAMETOOLONG);
		goto out;
	}

	rd = fs_stat(path, &ent);
	if (rd != 0) {
		notify_error_line(conn, "DUMP stat failed", rd);
		goto out;
	}

	fs_file_t_init(&file);

	rd = fs_open(&file, path, FS_O_READ);
	if (rd != 0) {
		notify_error_line(conn, "DUMP open failed", rd);
		goto out;
	}

	snprintk(control_line, sizeof(control_line), "OK DUMP BEGIN %s\n", name);
	rc = notify_text(conn, control_line);
	if (rc) {
		(void)fs_close(&file);
		goto out;
	}

	int file_size = ent.size;
	if (file_size < 0) {
		notify_error_line(conn, "DUMP invalid size", -EINVAL);
		(void)fs_close(&file);
		goto out;
	}
	if (file_size > FS_DUMP_MAX_FILE_BYTES) {
		file_size = FS_DUMP_MAX_FILE_BYTES;
	}

	while (total < file_size) {
		rd = fs_read(&file, &fs_dump_file_buf[total], file_size - total);
		if (rd < 0) {
			notify_error_line(conn, "DUMP read failed", rd);
			(void)fs_close(&file);
			goto out;
		}
		if (rd == 0) {
			break;
		}
		total += rd;
	}

	for (int pos = 0; pos < total; pos += FS_DUMP_READ_CHUNK) {
		int chunk_len = MIN(FS_DUMP_READ_CHUNK, total - pos);

		chunk_line[0] = 'D';
		chunk_line[1] = ' ';

		for (int i = 0; i < chunk_len; i++) {
			snprintk(&chunk_line[2 + (i * 2)], 3, "%02x", fs_dump_file_buf[pos + i]);
		}

		chunk_line[2 + (chunk_len * 2)] = '\n';
		chunk_line[2 + (chunk_len * 2) + 1] = '\0';
		rc = notify_text(conn, chunk_line);
		if (rc) {
			(void)fs_close(&file);
			goto out;
		}
	}

	(void)fs_close(&file);

	snprintk(control_line, sizeof(control_line), "OK DUMP END %s\n", name);
	rc = notify_text(conn, control_line);

out:
	fs_backend_resume(fs_backend, fs_resume_needed);
}

static void fs_dump_work_handler(struct k_work *work)
{
	struct bt_conn *conn = NULL;
	char cmd[FS_DUMP_CMD_MAX_LEN + 1];
	const char *p;

	ARG_UNUSED(work);

	k_mutex_lock(&fs_dump_lock, K_FOREVER);

	if (fs_dump_conn != NULL) {
		if (fs_dump_notify_enabled) {
			conn = bt_conn_ref(fs_dump_conn);
		}

		/* Drop stored command reference; each write acquires a fresh one. */
		bt_conn_unref(fs_dump_conn);
		fs_dump_conn = NULL;
	}

	memcpy(cmd, fs_dump_cmd_buf, sizeof(cmd));

	k_mutex_unlock(&fs_dump_lock);

	if (conn == NULL) {
		return;
	}

	trim_trailing(cmd);
	p = skip_spaces(cmd);

	if (strcmp(p, "LIST") == 0) {
		handle_list(conn);
	} else if (strncmp(p, "DUMP", 4) == 0) {
		p = skip_spaces(p + 4);
		handle_dump(conn, p);
	} else {
		notify_error_line(conn, "Unknown command", -EINVAL);
	}

	bt_conn_unref(conn);
}

void fs_dump_ble_init(void)
{
	k_work_queue_start(&fs_dump_wq, fs_dump_wq_stack,
			   K_THREAD_STACK_SIZEOF(fs_dump_wq_stack),
			   K_PRIO_PREEMPT(7), NULL);
	LOG_INF("BLE FS dump service ready (commands: LIST, DUMP <file>)");
}
