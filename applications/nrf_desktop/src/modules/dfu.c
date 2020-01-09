/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <inttypes.h>

#include <zephyr/types.h>
#include <sys/byteorder.h>
#include <flash_map.h>
#include <pm_config.h>
#include <dfu/mcuboot.h>

#include "event_manager.h"
#include "config_event.h"
#include "hid_event.h"

#define MODULE dfu
#include "module_state_event.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_CONFIG_CHANNEL_DFU_LOG_LEVEL);


#define FLASH_PAGE_SIZE_LOG2	12
#define FLASH_PAGE_SIZE		BIT(FLASH_PAGE_SIZE_LOG2)
#define FLASH_PAGE_ID(off)	((off) >> FLASH_PAGE_SIZE_LOG2)
#define FLASH_CLEAN_VAL		UINT32_MAX
#define FLASH_READ_CHUNK_SIZE	(FLASH_PAGE_SIZE / 8)

#define DFU_TIMEOUT			K_SECONDS(2)
#define REBOOT_REQUEST_TIMEOUT		K_MSEC(250)
#define BACKGROUND_FLASH_ERASE_TIMEOUT	K_SECONDS(15)

static struct k_delayed_work dfu_timeout;
static struct k_delayed_work reboot_request;
static struct k_delayed_work background_erase;

static const struct flash_area *flash_area;
static u32_t cur_offset;
static u32_t img_csum;
static u32_t img_length;

static bool device_in_use;
static bool is_flash_area_clean;


static bool is_page_clean(const struct flash_area *fa, off_t off, size_t len)
{
	static const size_t chunk_size = FLASH_READ_CHUNK_SIZE;
	static const size_t chunk_cnt = FLASH_PAGE_SIZE / chunk_size;

	BUILD_ASSERT(chunk_size * chunk_cnt == FLASH_PAGE_SIZE);
	BUILD_ASSERT(chunk_size % sizeof(u32_t) == 0);

	u32_t buf[chunk_size / sizeof(u32_t)];

	int err;

	for (size_t i = 0; i < chunk_cnt; i++) {
		err = flash_area_read(fa, off + i * chunk_size, buf, chunk_size);

		if (err) {
			LOG_ERR("Cannot read flash");
			return false;
		}

		for (size_t j = 0; j < ARRAY_SIZE(buf); j++) {
			if (buf[j] != FLASH_CLEAN_VAL) {
				return false;
			}
		}
	}

	return true;
}

static void dfu_timeout_handler(struct k_work *work)
{
	LOG_WRN("DFU timed out");

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

static void background_erase_handler(struct k_work *work)
{
	static u32_t erase_offset;
	int err;

	__ASSERT_NO_MSG(!is_flash_area_clean);

	/* During page erase operation CPU stalls. As page erase takes tens of
	 * milliseconds let's perform it in background, when user is not
	 * interacting with the device.
	 */
	if (device_in_use) {
		device_in_use = false;
		k_delayed_work_submit(&background_erase,
				      BACKGROUND_FLASH_ERASE_TIMEOUT);
		return;
	}

	if (!flash_area) {
		err = flash_area_open(PM_MCUBOOT_SECONDARY_ID, &flash_area);
		if (err) {
			LOG_ERR("Cannot open flash area (%d)", err);
			flash_area = NULL;
			return;
		}
	}

	__ASSERT_NO_MSG(erase_offset + FLASH_PAGE_SIZE <= flash_area->fa_size);

	if (!is_page_clean(flash_area, erase_offset, FLASH_PAGE_SIZE)) {
		err = flash_area_erase(flash_area, erase_offset, FLASH_PAGE_SIZE);
		if (err) {
			LOG_ERR("Cannot erase page (%d)", err);
			flash_area_close(flash_area);
			flash_area = NULL;
			return;
		}
	}

	erase_offset += FLASH_PAGE_SIZE;

	if (erase_offset < flash_area->fa_size) {
		k_delayed_work_submit(&background_erase, 0);
	} else {
		LOG_INF("Secondary image slot is clean");

		is_flash_area_clean = true;
		erase_offset = 0;

		flash_area_close(flash_area);
		flash_area = NULL;
	}
}

static void handle_dfu_data(const struct config_event *event)
{
	const u8_t *data = event->dyndata.data;
	size_t size = event->dyndata.size;
	int err;

	if (!is_flash_area_clean) {
		LOG_WRN("Flash is not clean");
		return;
	}

	if (!flash_area) {
		LOG_WRN("DFU was not started");
		return;
	}

	LOG_DBG("DFU data received cur_offset:%" PRIu32, cur_offset);

	if (size == 0) {
		LOG_WRN("Invalid DFU data header");
		goto dfu_finish;
	}

	err = flash_area_write(flash_area, cur_offset, data, size);
	if (err) {
		LOG_ERR("Cannot write data (%d)", err);
		goto dfu_finish;
	}

	cur_offset += size;

	LOG_DBG("DFU chunk written");

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
	k_delayed_work_cancel(&dfu_timeout);
}

static void handle_dfu_start(const struct config_event *event)
{
	const u8_t *data = event->dyndata.data;
	size_t size = event->dyndata.size;

	u32_t length;
	u32_t csum;
	u32_t offset;

	size_t data_size = sizeof(length) + sizeof(csum) + sizeof(offset);

	BUILD_ASSERT_MSG(sizeof(length) == sizeof(img_length), "");
	BUILD_ASSERT_MSG(sizeof(csum) == sizeof(img_csum), "");
	BUILD_ASSERT_MSG(sizeof(offset) == sizeof(cur_offset), "");

	if (size < data_size) {
		LOG_WRN("Invalid DFU start header");
		return;
	}

	if (!is_flash_area_clean) {
		LOG_WRN("Flash is not clean yet.");
		return;
	}

	if (flash_area) {
		LOG_WRN("DFU already in progress");
		return;
	}

	size_t pos = 0;

	length = sys_get_le32(&data[pos]);
	pos += sizeof(length);

	csum = sys_get_le32(&data[pos]);
	pos += sizeof(csum);

	offset = sys_get_le32(&data[pos]);
	pos += sizeof(offset);

	LOG_INF("DFU start received img_length:%" PRIu32
		" img_csum:0x%" PRIx32 " offset:%" PRIu32,
		length, csum, offset);

	if (offset != 0) {
		if ((offset != cur_offset) ||
		    (length != img_length) ||
		    (csum != img_csum)) {
			LOG_WRN("Cannot restart DFU"
				" length: %" PRIu32 " %" PRIu32
				" csum: 0x%" PRIx32 " 0x%" PRIx32
				" offset: %" PRIu32 " %" PRIu32,
				length, img_length,
				csum, img_csum,
				offset, cur_offset);
			return;
		} else {
			LOG_INF("Restart DFU");
		}
	} else {
		if (cur_offset != 0) {
			k_delayed_work_submit(&background_erase, 0);
			is_flash_area_clean = false;
			cur_offset = 0;
			img_length = 0;
			img_csum = 0;

			return;
		} else {
			img_length = length;
			img_csum = csum;
		}
	}

	__ASSERT_NO_MSG(flash_area == NULL);
	int err = flash_area_open(PM_MCUBOOT_SECONDARY_ID, &flash_area);

	if (err) {
		LOG_ERR("Cannot open flash area (%d)", err);

		flash_area = NULL;
	} else if (flash_area->fa_size < img_length) {
		LOG_WRN("Insufficient space for DFU (%zu < %" PRIu32 ")",
			flash_area->fa_size, img_length);

		flash_area_close(flash_area);
		flash_area = NULL;
	} else {
		LOG_INF("DFU started");
		k_delayed_work_submit(&dfu_timeout, DFU_TIMEOUT);
	}
}

static void handle_dfu_sync(const struct config_fetch_request_event *event)
{
	LOG_INF("DFU sync requested");

	u8_t dfu_active = (flash_area != NULL) ? 0x01 : 0x00;

	size_t data_size = sizeof(dfu_active) + sizeof(img_length) +
			   sizeof(img_csum) + sizeof(cur_offset);

	struct config_fetch_event *fetch_event =
		new_config_fetch_event(data_size);
	fetch_event->id = event->id;
	fetch_event->recipient = event->recipient;
	fetch_event->channel_id = event->channel_id;

	size_t pos = 0;

	fetch_event->dyndata.data[pos] = dfu_active;
	pos += sizeof(dfu_active);

	sys_put_le32(img_length, &fetch_event->dyndata.data[pos]);
	pos += sizeof(img_length);

	sys_put_le32(img_csum, &fetch_event->dyndata.data[pos]);
	pos += sizeof(img_csum);

	sys_put_le32(cur_offset, &fetch_event->dyndata.data[pos]);
	pos += sizeof(cur_offset);

	EVENT_SUBMIT(fetch_event);
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
	u8_t flash_area_id = PM_MCUBOOT_PRIMARY_ID;
	int err = boot_read_bank_header(flash_area_id, &header,
					sizeof(header));

	if (!err) {
		LOG_INF("Primary slot| image size:%" PRIu32
			" major:%" PRIu8 " minor:%" PRIu8 " rev:0x%" PRIx16
			" build:0x%" PRIx32, header.h.v1.image_size,
			header.h.v1.sem_ver.major, header.h.v1.sem_ver.minor,
			header.h.v1.sem_ver.revision, header.h.v1.sem_ver.build_num);

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

	case DFU_START:
		handle_dfu_start(event);
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

	case DFU_SYNC:
		handle_dfu_sync(event);
		break;

	default:
		/* Ignore unknown event. */
		LOG_WRN("Unknown DFU event");
		break;
	}
}

static bool event_handler(const struct event_header *eh)
{
	if ((IS_ENABLED(CONFIG_DESKTOP_HID_MOUSE) && is_hid_mouse_event(eh)) ||
	    (IS_ENABLED(CONFIG_DESKTOP_HID_KEYBOARD) && is_hid_keyboard_event(eh))) {
		device_in_use = true;

		return false;
	}

	if (is_config_event(eh)) {
		handle_config_event(cast_config_event(eh));

		return false;
	}

	if (is_config_fetch_request_event(eh)) {
		handle_config_fetch_request_event(
				cast_config_fetch_request_event(eh));
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
			k_delayed_work_init(&background_erase, background_erase_handler);

			k_delayed_work_submit(&background_erase, 0);
		}
		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, hid_mouse_event);
EVENT_SUBSCRIBE(MODULE, hid_keyboard_event);
EVENT_SUBSCRIBE_EARLY(MODULE, config_event);
EVENT_SUBSCRIBE(MODULE, config_fetch_request_event);
EVENT_SUBSCRIBE(MODULE, module_state_event);
