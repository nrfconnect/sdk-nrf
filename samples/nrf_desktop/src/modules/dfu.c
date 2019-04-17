/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <inttypes.h>

#include <zephyr/types.h>
#include <misc/byteorder.h>
#include <flash_map.h>
#include <pm_config.h>
#include <dfu/mcuboot.h>
#include <bluetooth/conn.h>

#include "event_manager.h"
#include "config_event.h"
#include "ble_event.h"

#define MODULE dfu
#include "module_state_event.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_CONFIG_CHANNEL_DFU_LOG_LEVEL);


#if defined(CONFIG_BT_PERIPHERAL)
#define DEFAULT_LATENCY CONFIG_BT_PERIPHERAL_PREF_SLAVE_LATENCY
#else
#define DEFAULT_LATENCY 0
#endif


#define FLASH_PAGE_SIZE 0x1000

#define DFU_TIMEOUT K_SECONDS(2)

static struct bt_conn *active_conn;
static struct k_delayed_work dfu_timeout;

static const struct flash_area *flash_area;


static void set_ble_latency(bool dfu_start)
{
	if (!active_conn) {
		LOG_INF("No active_connection");
		return;
	}

	struct bt_conn_info info;

	int err = bt_conn_get_info(active_conn, &info);
	if (err) {
		LOG_WRN("Cannot get conn info (%d)", err);
		return;
	}

	if (IS_ENABLED(CONFIG_BT_PERIPHERAL) &&
	    (info.role == BT_CONN_ROLE_SLAVE)) {
		const struct bt_le_conn_param param = {
			.interval_min = info.le.interval,
			.interval_max = info.le.interval,
			.latency = (dfu_start) ? (0) : (DEFAULT_LATENCY),
			.timeout = info.le.timeout
		};

		err = bt_conn_le_param_update(active_conn, &param);
		if (err) {
			LOG_WRN("Cannot update parameters (%d)", err);
			return;
		}
	}

	LOG_INF("BLE latency %screased for DFU", dfu_start ? "de" : "in");
}

static void dfu_timeout_handler(struct k_work *work)
{
	LOG_WRN("DFU timed out");

	set_ble_latency(false);

	if (flash_area) {
		flash_area_close(flash_area);
		flash_area = NULL;
	}
}

static void handle_dfu_data(u8_t *data, size_t size)
{
	int err;

	u32_t img_length;
	u32_t chunk_offset;
	u32_t chunk_size;

	size_t min_size = sizeof(img_length) + sizeof(chunk_offset);

	if (size < min_size) {
		LOG_INF("Invalid DFU data header");
		return;
	}

	img_length = sys_get_le32(&data[0]);
	chunk_offset = sys_get_le32(&data[sizeof(img_length)]);
	chunk_size = size - min_size;

	LOG_INF("DFU data received img_length:%" PRIu32
		" chunk_offset:%" PRIu32 " chunk_size:%" PRIu32,
		img_length, chunk_offset, chunk_size);

	static size_t cur_offset;

	if (chunk_offset == 0) {
		set_ble_latency(true);

		if (cur_offset) {
			LOG_WRN("Previous DFU operation interrupted");
		}

		err = flash_area_open(PM_MCUBOOT_PARTITIONS_SECONDARY_ID, &flash_area);
		if (err) {
			LOG_ERR("Cannot open flash area (%d)", err);
			goto dfu_finish;
		}

		cur_offset = 0;
	}

	if (!flash_area) {
		LOG_WRN("DFU was not started");
		goto dfu_finish;
	}

	if (flash_area->fa_size < img_length) {
		LOG_WRN("Insufficient space for DFU");
		goto dfu_finish;
	}

	if (chunk_offset != cur_offset) {
		LOG_WRN("Invalid chunk sequence");
		goto dfu_finish;
	}

	if (!chunk_size) {
		LOG_WRN("No data to store");
		goto dfu_finish;
	}

	if (cur_offset % FLASH_PAGE_SIZE == 0) {
		err = flash_area_erase(flash_area, cur_offset, FLASH_PAGE_SIZE);
		if (err) {
			LOG_ERR("Cannot erase page (%d)", err);
			goto dfu_finish;
		}
	}

	void *chunk_data = &data[min_size];

	err = flash_area_write(flash_area, cur_offset, chunk_data, chunk_size);
	if (err) {
		LOG_ERR("Cannot write data (%d)", err);
		goto dfu_finish;
	}

	cur_offset += chunk_size;

	LOG_INF("DFU chunk written");
	if (img_length == cur_offset) {
		LOG_INF("DFU image written");
		boot_request_upgrade(false);

		goto dfu_finish;
	} else {
		k_delayed_work_submit(&dfu_timeout, DFU_TIMEOUT);
	}

	return;

dfu_finish:
	flash_area_close(flash_area);
	flash_area = NULL;
	set_ble_latency(false);
	k_delayed_work_cancel(&dfu_timeout);
}

static bool event_handler(const struct event_header *eh)
{
	if (is_config_event(eh)) {
		struct config_event *event = cast_config_event(eh);

		/* Accept only events coming from transport. */
		if ((event->store_needed) && (GROUP_FIELD_GET(event->id) == EVENT_GROUP_DFU)) {
			handle_dfu_data(event->dyndata.data, event->dyndata.size);
		}

		return false;
	}

	if (is_ble_peer_event(eh)) {
		struct ble_peer_event *event = cast_ble_peer_event(eh);

		switch (event->state) {
		case PEER_STATE_CONNECTED:
			active_conn = event->id;
			break;

		case PEER_STATE_DISCONNECTED:
			active_conn = NULL;
			break;

		case PEER_STATE_SECURED:
		case PEER_STATE_CONN_FAILED:
			/* No action */
			break;

		default:
			__ASSERT_NO_MSG(false);
			break;
		}

		return false;
	}

	if (is_module_state_event(eh)) {
		const struct module_state_event *event =
			cast_module_state_event(eh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			int err = boot_write_img_confirmed();
			if (err) {
				LOG_ERR("Cannot confirm a running image");
			}

			k_delayed_work_init(&dfu_timeout, dfu_timeout_handler);
		}
		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, module_state_event);
EVENT_SUBSCRIBE(MODULE, config_event);
EVENT_SUBSCRIBE(MODULE, ble_peer_event);
