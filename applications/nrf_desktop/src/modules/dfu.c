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
#define REBOOT_REQUEST_TIMEOUT K_MSEC(250)

static struct bt_conn *active_conn;
static struct k_delayed_work dfu_timeout;
static struct k_delayed_work reboot_request;

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

static void reboot_request_handler(struct k_work *work)
{
	LOG_INF("Handle reset request now");

	if (flash_area) {
		flash_area_close(flash_area);
		flash_area = NULL;
	}

	LOG_PANIC();
	sys_reboot(SYS_REBOOT_WARM);
}

static void handle_dfu_data(const struct config_event *event)
{
	const u8_t *data = event->dyndata.data;
	size_t size = event->dyndata.size;

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

		err = flash_area_open(PM_MCUBOOT_SECONDARY_ID, &flash_area);
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
		LOG_WRN("Insufficient space for DFU (%zu < %" PRIu32 ")",
			flash_area->fa_size, img_length);
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

	const void *chunk_data = &data[min_size];

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

static void handle_reboot_request(const struct config_fetch_request_event *event)
{
	LOG_INF("System reboot requested");

	struct config_fetch_event *fetch_event =
		new_config_fetch_event(0);

	fetch_event->id = event->id;
	fetch_event->recipient = event->recipient;
	fetch_event->channel_id = event->channel_id;

	EVENT_SUBMIT(fetch_event);

	k_delayed_work_submit(&reboot_request, REBOOT_REQUEST_TIMEOUT);
}

static void handle_image_info_request(const struct config_fetch_request_event *event)
{
	struct mcuboot_img_header header;
	int err = boot_read_bank_header(DT_FLASH_AREA_IMAGE_0_ID, &header,
					sizeof(header));

	if (!err) {
		LOG_INF("Slot 0| image size:%" PRIu32
			" major:%" PRIu8 " minor:%" PRIu8 " rev:0x%" PRIx16
			" build:0x%" PRIx32, header.h.v1.image_size,
			header.h.v1.sem_ver.major, header.h.v1.sem_ver.minor,
			header.h.v1.sem_ver.revision, header.h.v1.sem_ver.build_num);

		u8_t flash_area_id = DT_FLASH_AREA_IMAGE_0_ID;
		size_t data_size = sizeof(flash_area_id) +
				   sizeof(header.h.v1.image_size) +
				   sizeof(header.h.v1.sem_ver.major) +
				   sizeof(header.h.v1.sem_ver.minor) +
				   sizeof(header.h.v1.sem_ver.revision) +
				   sizeof(header.h.v1.sem_ver.build_num);
		struct config_fetch_event *fetch_event =
			new_config_fetch_event(data_size);

		fetch_event->id = event->id;
		fetch_event->recipient = event->recipient;
		fetch_event->channel_id = event->channel_id;

		size_t pos = 0;

		fetch_event->dyndata.data[pos] = flash_area_id;
		pos += sizeof(flash_area_id);

		sys_put_le32(header.h.v1.image_size, &fetch_event->dyndata.data[pos]);
		pos += sizeof(header.h.v1.image_size);

		fetch_event->dyndata.data[pos] = header.h.v1.sem_ver.major;
		pos += sizeof(header.h.v1.sem_ver.major);

		fetch_event->dyndata.data[pos] = header.h.v1.sem_ver.minor;
		pos += sizeof(header.h.v1.sem_ver.minor);

		sys_put_le16(header.h.v1.sem_ver.revision, &fetch_event->dyndata.data[pos]);
		pos += sizeof(header.h.v1.sem_ver.revision);

		sys_put_le32(header.h.v1.sem_ver.build_num, &fetch_event->dyndata.data[pos]);
		pos += sizeof(header.h.v1.sem_ver.build_num);

		EVENT_SUBMIT(fetch_event);
	} else {
		LOG_ERR("Cannot obtain image information");
	}
}

static void handle_config_event(const struct config_event *event)
{
	if (!event->store_needed) {
		/* Accept only events coming from transport. */
		return;
	}

	if (GROUP_FIELD_GET(event->id) != EVENT_GROUP_DFU) {
		/* Only DFU events. */
		return;
	}

	switch (TYPE_FIELD_GET(event->id)) {
	case DFU_DATA:
		handle_dfu_data(event);
		break;

	default:
		/* Ignore unknown event. */
		LOG_WRN("Unknown DFU event");
		break;
	}
}

static void handle_config_fetch_request_event(const struct config_fetch_request_event *event)
{
	if (GROUP_FIELD_GET(event->id) != EVENT_GROUP_DFU) {
		/* Only DFU events. */
		return;
	}

	switch (TYPE_FIELD_GET(event->id)) {
	case DFU_REBOOT:
		handle_reboot_request(event);
		break;

	case DFU_IMGINFO:
		handle_image_info_request(event);
		break;

	default:
		/* Ignore unknown event. */
		LOG_WRN("Unknown DFU event");
		break;
	}
}

static bool event_handler(const struct event_header *eh)
{
	if (is_config_event(eh)) {
		handle_config_event(cast_config_event(eh));

		return false;
	}

	if (is_config_fetch_request_event(eh)) {
		handle_config_fetch_request_event(
				cast_config_fetch_request_event(eh));
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
			k_delayed_work_init(&reboot_request, reboot_request_handler);
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
EVENT_SUBSCRIBE(MODULE, config_fetch_request_event);
EVENT_SUBSCRIBE(MODULE, ble_peer_event);
