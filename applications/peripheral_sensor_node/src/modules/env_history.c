/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/fs/nvs.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

#include <caf/events/sensor_event.h>
#include <caf/events/power_event.h>

#include "app_sensor_event.h"

#define MODULE env_history
#include <caf/events/module_state_event.h>

LOG_MODULE_REGISTER(MODULE);

size_t strnlen(const char *s, size_t maxlen);

#define NVS_PARTITION		sensor_data_storage
#define NVS_PARTITION_DEVICE	FIXED_PARTITION_DEVICE(NVS_PARTITION)
#define NVS_PARTITION_OFFSET	FIXED_PARTITION_OFFSET(NVS_PARTITION)
#define NVS_PARTITION_SIZE	FIXED_PARTITION_SIZE(NVS_PARTITION)

#define BT_UUID_PSN_VAL \
	BT_UUID_128_ENCODE(0xde550000, 0xc9f9, 0x4f0d, 0xa6e1, 0x766437949322)

#define BT_UUID_PSN_READ_REQ_VAL \
	BT_UUID_128_ENCODE(0xde550002, 0xacb6, 0x4c73, 0x8445, 0x2563acbb43c2)

#define BT_UUID_PSN_READ_VAL \
	BT_UUID_128_ENCODE(0xde550004, 0xacb6, 0x4c73, 0x8445, 0x2563acbb43c2)

#define BT_UUID_PSN_SERVICE	BT_UUID_DECLARE_128(BT_UUID_PSN_VAL)
#define BT_UUID_PSN_READ_REQ	BT_UUID_DECLARE_128(BT_UUID_PSN_READ_REQ_VAL)
#define BT_UUID_PSN_READ	BT_UUID_DECLARE_128(BT_UUID_PSN_READ_VAL)

#define ENV_VALUES_CNT 4
#define ENV_NVS_ID 0x859
#define TMP_BUF_SIZE 64
#define XFER_BUF_SIZE 250
#define MTX_LOCK_WAIT_TIME K_MSEC(200)

#define PSN_ENV_SVC_ATTR_IDX 2

#define DATA_SIZE_MSG "use uint16_t little endian as input"
#define NO_DATA_MSG "No data"

static char xfer_buf[XFER_BUF_SIZE];
static struct nvs_fs fs = {
	.flash_device = NVS_PARTITION_DEVICE,
	.offset = NVS_PARTITION_OFFSET,
};

static int xfer_cnt;
static int xfer_read_id;
static uint16_t xfer_depth;
static uint32_t latest_id;
static K_MUTEX_DEFINE(nvs_mtx);

struct env_storage_chunk {
	uint32_t id;
	struct sensor_value values[ENV_VALUES_CNT];
};

size_t env_get_history(char *print_buf, const uint16_t depth)
{
	struct env_storage_chunk env_chunk;

	k_mutex_lock(&nvs_mtx, MTX_LOCK_WAIT_TIME);
	int rc = nvs_read_hist(&fs, ENV_NVS_ID, &env_chunk, sizeof(env_chunk), depth);

	k_mutex_unlock(&nvs_mtx);

	if (rc < 0) {
		return -EOVERFLOW;
	}
	int len = sprintf(print_buf, "%d ", env_chunk.id);

	for (size_t i = 0; i < ENV_VALUES_CNT; i++) {
		float tmp = sensor_value_to_double(&env_chunk.values[i]);

		len += sprintf(print_buf + len, "%.2f ", (double)tmp);
	}

	return len;
}

static ssize_t bt_psn_read_req(struct bt_conn *conn,
			const struct bt_gatt_attr *attr,
			const void *buf, uint16_t len, uint16_t offset, uint8_t flags);

BT_GATT_SERVICE_DEFINE(psn_env_svc,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_PSN_SERVICE),
		BT_GATT_CHARACTERISTIC(BT_UUID_PSN_READ,
				       BT_GATT_CHRC_NOTIFY,
				       BT_GATT_PERM_READ,
				       NULL,
				       NULL,
				       NULL),
		BT_GATT_CCC(NULL, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
		BT_GATT_CUD("Env read", BT_GATT_PERM_READ),
		BT_GATT_CHARACTERISTIC(BT_UUID_PSN_READ_REQ,
				       BT_GATT_CHRC_WRITE,
				       BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
				       NULL,
				       bt_psn_read_req,
				       NULL),
		BT_GATT_CCC(NULL, BT_GATT_PERM_READ),
		BT_GATT_CUD("Env read req", BT_GATT_PERM_READ),
);

static const struct bt_gatt_attr *psn_read_attr = &psn_env_svc.attrs[PSN_ENV_SVC_ATTR_IDX];

static void send_tail(struct bt_conn *conn, void *user_data)
{
	ARG_UNUSED(user_data);

	char tmp_buf[TMP_BUF_SIZE];
	int pos = 0;

	if (latest_id == 0) {
		bt_gatt_notify(conn, psn_read_attr, NO_DATA_MSG, sizeof(NO_DATA_MSG)-1);
		LOG_INF("%s", NO_DATA_MSG);
	}

	while (xfer_cnt < xfer_depth) {
		int len = env_get_history(tmp_buf, (latest_id - xfer_read_id));

		if (len < 0) {
			xfer_depth = xfer_cnt;
			break;
		}

		if ((XFER_BUF_SIZE - pos) > len) {
			memcpy(xfer_buf+pos, tmp_buf, len);
			pos += len;
			xfer_read_id--;
			xfer_cnt++;
		} else {
			break;
		}
	}

	if (pos > 0) {
		struct bt_gatt_notify_params params = {
			.attr = psn_read_attr,
			.data = xfer_buf,
			.len = pos,
			.func = send_tail,
		};
		bt_gatt_notify_cb(conn, &params);

		LOG_INF("size %d up to %d",  pos,  xfer_cnt);
	}
}

static ssize_t bt_psn_read_req(struct bt_conn *conn,
			const struct bt_gatt_attr *attr,
			const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	if (len != sizeof(uint16_t)) {
		bt_gatt_notify(conn, psn_read_attr, DATA_SIZE_MSG, sizeof(DATA_SIZE_MSG)-1);
		LOG_INF("%s", DATA_SIZE_MSG);
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	xfer_cnt = 0;
	xfer_depth = *((uint16_t *)buf);

	xfer_read_id = latest_id;
	send_tail(conn, NULL);

	return 0;
}

static void init(void)
{
	struct flash_pages_info info;

	if (!device_is_ready(fs.flash_device)) {
		LOG_ERR("Flash device %s is not ready.", fs.flash_device->name);
		return;
	}
	int rc = flash_get_page_info_by_offs(fs.flash_device, fs.offset, &info);

	if (rc) {
		LOG_ERR("Unable to get page info.");
		return;
	}

	fs.sector_size = info.size;
	fs.sector_count = NVS_PARTITION_SIZE / fs.sector_size;

	rc = nvs_mount(&fs);
	if (rc) {
		LOG_ERR("Flash Init failed.");
		return;
	}

	struct env_storage_chunk latest_env;

	rc = nvs_read(&fs, ENV_NVS_ID, &latest_env, sizeof(latest_env));
	if (rc > 0) {
		LOG_INF("NVS latest id %d", latest_env.id);
		latest_id = latest_env.id;
	} else {
		latest_id = 0;
	}

	module_set_state(MODULE_STATE_READY);
}

static bool handle_sensor_event(const struct sensor_event *event)
{
	size_t data_cnt = sensor_event_get_data_cnt(event);
	const struct sensor_value *data_ptr = sensor_event_get_data_ptr(event);

	if (!strcmp(event->descr, "env")) {
		struct env_storage_chunk env_chunk;

		memcpy(&env_chunk.values, data_ptr, data_cnt*sizeof(struct sensor_value));
		latest_id++;
		env_chunk.id = latest_id;

		k_mutex_lock(&nvs_mtx, MTX_LOCK_WAIT_TIME);
		nvs_write(&fs, ENV_NVS_ID, &env_chunk, sizeof(env_chunk));
		k_mutex_unlock(&nvs_mtx);
	}

	return false;
}


static bool app_event_handler(const struct app_event_header *aeh)
{
	if (is_sensor_event(aeh)) {
		return handle_sensor_event(cast_sensor_event(aeh));
	}

	if (is_module_state_event(aeh)) {
		const struct module_state_event *event = cast_module_state_event(aeh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			init();
		}

		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, module_state_event);
APP_EVENT_SUBSCRIBE(MODULE, sensor_event);
