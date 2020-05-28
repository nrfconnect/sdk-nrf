/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <inttypes.h>

#include <zephyr/types.h>
#include <sys/byteorder.h>
#include <storage/flash_map.h>
#include <pm_config.h>
#include <fw_info.h>

#include "event_manager.h"
#include "config_event.h"
#include "hid_event.h"
#include "ble_event.h"

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

enum dfu_opt {
	DFU_OPT_START,
	DFU_OPT_DATA,
	DFU_OPT_SYNC,
	DFU_OPT_REBOOT,
	DFU_OPT_FWINFO,

	DFU_OPT_COUNT
};

const static char * const opt_descr[] = {
	[DFU_OPT_START] = "start",
	[DFU_OPT_DATA] = "data",
	[DFU_OPT_SYNC] = "sync",
	[DFU_OPT_REBOOT] = "reboot",
	[DFU_OPT_FWINFO] = "fwinfo"
};

static u8_t dfu_slot_id(void)
{
	if ((u32_t)(uintptr_t)dfu_slot_id < PM_S1_IMAGE_ADDRESS) {
		return PM_S1_IMAGE_ID;
	}

	return PM_S0_IMAGE_ID;
}

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
		err = flash_area_open(dfu_slot_id(), &flash_area);
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
		k_delayed_work_submit(&background_erase, K_NO_WAIT);
	} else {
		LOG_INF("Secondary image slot is clean");

		is_flash_area_clean = true;
		erase_offset = 0;

		flash_area_close(flash_area);
		flash_area = NULL;
	}
}

static void handle_dfu_data(const u8_t *data, const size_t size)
{
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

static void handle_dfu_start(const u8_t *data, const size_t size)
{
	u32_t length;
	u32_t csum;
	u32_t offset;

	size_t data_size = sizeof(length) + sizeof(csum) + sizeof(offset);

	BUILD_ASSERT(sizeof(length) == sizeof(img_length), "");
	BUILD_ASSERT(sizeof(csum) == sizeof(img_csum), "");
	BUILD_ASSERT(sizeof(offset) == sizeof(cur_offset), "");

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
			k_delayed_work_submit(&background_erase, K_NO_WAIT);
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
	int err = flash_area_open(dfu_slot_id(), &flash_area);

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

static void handle_dfu_sync(u8_t *data, size_t *size)
{
	LOG_INF("DFU sync requested");

	u8_t dfu_active = (flash_area != NULL) ? 0x01 : 0x00;

	size_t data_size = sizeof(dfu_active) + sizeof(img_length) +
			   sizeof(img_csum) + sizeof(cur_offset);

	*size = data_size;

	size_t pos = 0;

	data[pos] = dfu_active;
	pos += sizeof(dfu_active);

	sys_put_le32(img_length, &data[pos]);
	pos += sizeof(img_length);

	sys_put_le32(img_csum, &data[pos]);
	pos += sizeof(img_csum);

	sys_put_le32(cur_offset, &data[pos]);
	pos += sizeof(cur_offset);
}

static void handle_reboot_request(u8_t *data, size_t *size)
{
	LOG_INF("System reboot requested");

	*size = sizeof(bool);
	data[0] = true;

	k_delayed_work_submit(&reboot_request, REBOOT_REQUEST_TIMEOUT);
}

static void handle_image_info_request(u8_t *data, size_t *size)
{
	const struct fw_info *info;
	u8_t flash_area_id;

	if (dfu_slot_id() == PM_S1_IMAGE_ID) {
		info = fw_info_find(PM_S0_IMAGE_ADDRESS);
		flash_area_id = 0;
	} else {
		info = fw_info_find(PM_S1_IMAGE_ADDRESS);
		flash_area_id = 1;
	}

	if (info) {
		u32_t image_size = info->size;
		u32_t build_num = info->version;
		u8_t major = 0;
		u8_t minor = 0;
		u16_t revision = 0;

		LOG_INF("Primary slot| image size:%" PRIu32
			" major:%" PRIu8 " minor:%" PRIu8 " rev:0x%" PRIx16
			" build:0x%" PRIx32, image_size, major, minor,
			revision, build_num);

		size_t data_size = sizeof(flash_area_id) +
				   sizeof(image_size) +
				   sizeof(major) +
				   sizeof(minor) +
				   sizeof(revision) +
				   sizeof(build_num);
		size_t pos = 0;

		*size = data_size;
		data[pos] = flash_area_id;
		pos += sizeof(flash_area_id);

		sys_put_le32(image_size, &data[pos]);
		pos += sizeof(image_size);

		data[pos] = major;
		pos += sizeof(major);

		data[pos] = minor;
		pos += sizeof(minor);

		sys_put_le16(revision, &data[pos]);
		pos += sizeof(revision);

		sys_put_le32(build_num, &data[pos]);
		pos += sizeof(build_num);
	} else {
		LOG_ERR("Cannot obtain image information");
	}
}

static void update_config(const u8_t opt_id, const u8_t *data,
			  const size_t size)
{
	switch (opt_id) {
	case DFU_OPT_DATA:
		handle_dfu_data(data, size);
		break;

	case DFU_OPT_START:
		handle_dfu_start(data, size);
		break;

	default:
		/* Ignore unknown event. */
		LOG_WRN("Unknown DFU event");
		break;
	}
}

static void fetch_config(const u8_t opt_id, u8_t *data, size_t *size)
{
	switch (opt_id) {
	case DFU_OPT_REBOOT:
		handle_reboot_request(data, size);
		break;

	case DFU_OPT_FWINFO:
		handle_image_info_request(data, size);
		break;

	case DFU_OPT_SYNC:
		handle_dfu_sync(data, size);
		break;

	default:
		/* Ignore unknown event. */
		LOG_WRN("Unknown DFU event");
		break;
	}
}

static bool event_handler(const struct event_header *eh)
{
	if (is_hid_report_event(eh)) {
		device_in_use = true;

		return false;
	}

	GEN_CONFIG_EVENT_HANDLERS(STRINGIFY(MODULE), opt_descr, update_config,
				  fetch_config, false);

	if (is_ble_peer_event(eh)) {
		device_in_use = true;

		return false;
	}

	if (is_module_state_event(eh)) {
		const struct module_state_event *event =
			cast_module_state_event(eh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			k_delayed_work_init(&dfu_timeout, dfu_timeout_handler);
			k_delayed_work_init(&reboot_request, reboot_request_handler);
			k_delayed_work_init(&background_erase, background_erase_handler);

			k_delayed_work_submit(&background_erase, K_NO_WAIT);
		}
		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, hid_report_event);
EVENT_SUBSCRIBE(MODULE, config_event);
EVENT_SUBSCRIBE(MODULE, config_fetch_request_event);
EVENT_SUBSCRIBE(MODULE, module_state_event);
EVENT_SUBSCRIBE(MODULE, ble_peer_event);
