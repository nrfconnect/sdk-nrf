/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <inttypes.h>

#include <zephyr/types.h>
#include <sys/byteorder.h>
#include <storage/flash_map.h>
#include <pm_config.h>

#include "event_manager.h"
#include "config_event.h"
#include "hid_event.h"
#include "ble_event.h"
#include "dfu_lock.h"

#define MODULE dfu
#include "module_state_event.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_CONFIG_CHANNEL_DFU_LOG_LEVEL);


/* DFU state values must match with values used by the host. */
#define DFU_STATE_INACTIVE 0x00
#define DFU_STATE_ACTIVE   0x01
#define DFU_STATE_STORING  0x02
#define DFU_STATE_CLEANING 0x03


#define FLASH_PAGE_SIZE_LOG2	12
#define FLASH_PAGE_SIZE		BIT(FLASH_PAGE_SIZE_LOG2)
#define FLASH_PAGE_ID(off)	((off) >> FLASH_PAGE_SIZE_LOG2)
#define FLASH_CLEAN_VAL		UINT32_MAX
#define FLASH_READ_CHUNK_SIZE	(FLASH_PAGE_SIZE / 8)

#define DFU_TIMEOUT			K_SECONDS(2)
#define REBOOT_REQUEST_TIMEOUT		K_MSEC(250)
#define BACKGROUND_FLASH_ERASE_TIMEOUT	K_SECONDS(15)
#define BACKGROUND_FLASH_STORE_TIMEOUT	K_MSEC(5)

/* Keep small to avoid blocking the workqueue for long periods of time. */
#define STORE_CHUNK_SIZE		16 /* bytes */

#define SYNC_BUFFER_SIZE (CONFIG_DESKTOP_CONFIG_CHANNEL_DFU_SYNC_BUFFER_SIZE * sizeof(uint32_t)) /* bytes */

#if CONFIG_SECURE_BOOT
 #include <fw_info.h>
 #define IMAGE0_ID		PM_S0_IMAGE_ID
 #define IMAGE0_ADDRESS		PM_S0_IMAGE_ADDRESS
 #define IMAGE1_ID		PM_S1_IMAGE_ID
 #define IMAGE1_ADDRESS		PM_S1_IMAGE_ADDRESS
#elif CONFIG_BOOTLOADER_MCUBOOT
 #include <dfu/mcuboot.h>
 #define IMAGE0_ID		PM_MCUBOOT_PRIMARY_ID
 #define IMAGE0_ADDRESS		PM_MCUBOOT_PRIMARY_ADDRESS
 #define IMAGE1_ID		PM_MCUBOOT_SECONDARY_ID
 #define IMAGE1_ADDRESS		PM_MCUBOOT_SECONDARY_ADDRESS
#else
 #error Bootloader not supported.
#endif

static struct k_delayed_work dfu_timeout;
static struct k_delayed_work reboot_request;
static struct k_delayed_work background_erase;
static struct k_delayed_work background_store;

static const struct flash_area *flash_area;
static uint32_t cur_offset;
static uint32_t img_csum;
static uint32_t img_length;

static uint16_t store_offset;
static uint16_t sync_offset;
static char sync_buffer[SYNC_BUFFER_SIZE] __aligned(4);

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

static uint8_t dfu_slot_id(void)
{
#if CONFIG_BOOTLOADER_MCUBOOT
	/* MCUBoot always puts new image in the secondary slot. */
	return IMAGE1_ID;
#else
	BUILD_ASSERT(IMAGE0_ADDRESS < IMAGE1_ADDRESS);
	if ((uint32_t)(uintptr_t)dfu_slot_id < IMAGE1_ADDRESS) {
		return IMAGE1_ID;
	}

	return IMAGE0_ID;
#endif
}

static bool is_page_clean(const struct flash_area *fa, off_t off, size_t len)
{
	static const size_t chunk_size = FLASH_READ_CHUNK_SIZE;
	static const size_t chunk_cnt = FLASH_PAGE_SIZE / chunk_size;

	BUILD_ASSERT(chunk_size * chunk_cnt == FLASH_PAGE_SIZE);
	BUILD_ASSERT(chunk_size % sizeof(uint32_t) == 0);

	uint32_t buf[chunk_size / sizeof(uint32_t)];

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

static void terminate_dfu(void)
{
	__ASSERT_NO_MSG(flash_area != NULL);

	flash_area_close(flash_area);
	k_delayed_work_cancel(&dfu_timeout);
	k_delayed_work_cancel(&background_store);
	dfu_unlock(MODULE_ID(MODULE));
	flash_area = NULL;
	sync_offset = 0;
}

static void dfu_timeout_handler(struct k_work *work)
{
	LOG_WRN("DFU timed out");

	terminate_dfu();
}

static void reboot_request_handler(struct k_work *work)
{
	LOG_INF("Handle reset request now");

	if (flash_area) {
		terminate_dfu();
	}

	LOG_PANIC();
	sys_reboot(SYS_REBOOT_WARM);
}

static void background_erase_handler(struct k_work *work)
{
	static uint32_t erase_offset;
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

		dfu_unlock(MODULE_ID(MODULE));
		is_flash_area_clean = true;
		erase_offset = 0;

		flash_area_close(flash_area);
		flash_area = NULL;
	}
}

static void complete_dfu_data_store(void)
{
	cur_offset += sync_offset;
	sync_offset = 0;
	store_offset = 0;

	LOG_DBG("DFU data store complete: %" PRIu32, cur_offset);

	if (cur_offset == img_length) {
		LOG_INF("DFU image written");
#ifdef CONFIG_BOOTLOADER_MCUBOOT
		int err = boot_request_upgrade(false);
		if (err) {
			LOG_ERR("Cannot request the image upgrade (err:%d)", err);
		}
#endif
		terminate_dfu();
	}
}

static void store_dfu_data_chunk(void)
{
	/* Some flash may require word alignment. */
	BUILD_ASSERT((STORE_CHUNK_SIZE % sizeof(uint32_t)) == 0);
	BUILD_ASSERT((sizeof(sync_buffer) % sizeof(uint32_t)) == 0);

	__ASSERT_NO_MSG(store_offset <= sync_offset);
	__ASSERT_NO_MSG(flash_area != NULL);

	size_t store_size = STORE_CHUNK_SIZE;

	if (sync_offset - store_offset < store_size) {
		store_size = sync_offset - store_offset;
	}
	if ((store_size > sizeof(uint32_t)) &&
	    ((store_size % sizeof(uint32_t)) != 0)) {
		/* Required by some flash drivers. */
		store_size = (store_size / sizeof(uint32_t)) * sizeof(uint32_t);
	}

	LOG_DBG("DFU data store chunk: %" PRIu32, cur_offset + store_offset);
	int err = flash_area_write(flash_area, cur_offset + store_offset,
				   &sync_buffer[store_offset], store_size);
	if (err) {
		LOG_ERR("Cannot write data (%d)", err);
		terminate_dfu();
	} else {
		store_offset += store_size;
	}
}

static void background_store_handler(struct k_work *work)
{
	if (device_in_use) {
		device_in_use = false;
	} else {
		store_dfu_data_chunk();
	}

	if (store_offset < sync_offset) {
		k_delayed_work_submit(&background_store, BACKGROUND_FLASH_STORE_TIMEOUT);
		k_delayed_work_submit(&dfu_timeout, DFU_TIMEOUT);
	} else {
		complete_dfu_data_store();
	}
}

static bool is_dfu_data_store_active(void)
{
	return k_delayed_work_pending(&background_store);
}

static void start_dfu_data_store(void)
{
	LOG_DBG("DFU data store start: %" PRIu32 " %" PRIu32, cur_offset, sync_offset);
	store_offset = 0;
	k_delayed_work_submit(&background_store, K_NO_WAIT);
}

static void handle_dfu_data(const uint8_t *data, size_t size)
{
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
		terminate_dfu();
		return;
	}

	if (size > sizeof(sync_buffer) - sync_offset) {
		LOG_WRN("Chunk size truncated");
		size = sizeof(sync_buffer) - sync_offset;
	}
	memcpy(&sync_buffer[sync_offset], data, size);

	sync_offset += size;

	LOG_DBG("DFU chunk collected");

	k_delayed_work_submit(&dfu_timeout, DFU_TIMEOUT);

	return;
}

static void handle_dfu_start(const uint8_t *data, const size_t size)
{
	uint32_t length;
	uint32_t csum;
	uint32_t offset;

	size_t data_size = sizeof(length) + sizeof(csum) + sizeof(offset);

	BUILD_ASSERT(sizeof(length) == sizeof(img_length), "");
	BUILD_ASSERT(sizeof(csum) == sizeof(img_csum), "");
	BUILD_ASSERT(sizeof(offset) == sizeof(cur_offset), "");

	sync_offset = 0;

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

	if (!dfu_lock(MODULE_ID(MODULE))) {
		LOG_WRN("DFU already started by another module");
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

			dfu_unlock(MODULE_ID(MODULE));
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
		dfu_unlock(MODULE_ID(MODULE));
	} else if (flash_area->fa_size < img_length) {
		LOG_WRN("Insufficient space for DFU (%zu < %" PRIu32 ")",
			flash_area->fa_size, img_length);

		terminate_dfu();
	} else {
		LOG_INF("DFU started");
		k_delayed_work_submit(&dfu_timeout, DFU_TIMEOUT);
	}
}

static void handle_dfu_sync(uint8_t *data, size_t *size)
{
	LOG_INF("DFU sync requested");
	uint16_t sync_buffer_size = sizeof(sync_buffer);

	bool storing_data = (sync_offset > 0);
	bool dfu_active = (flash_area != NULL);

	if (storing_data && !is_dfu_data_store_active()) {
		start_dfu_data_store();
	}

	uint8_t dfu_state;

	if (!is_flash_area_clean) {
		dfu_state = DFU_STATE_CLEANING;
	} else if (storing_data) {
		dfu_state = DFU_STATE_STORING;
	} else if (dfu_active) {
		dfu_state = DFU_STATE_ACTIVE;
	} else {
		dfu_state = DFU_STATE_INACTIVE;
	}

	size_t data_size = sizeof(dfu_state) + sizeof(img_length) +
			   sizeof(img_csum) + sizeof(cur_offset) +
			   sizeof(sync_buffer_size);

	*size = data_size;

	size_t pos = 0;

	data[pos] = dfu_state;
	pos += sizeof(dfu_state);

	sys_put_le32(img_length, &data[pos]);
	pos += sizeof(img_length);

	sys_put_le32(img_csum, &data[pos]);
	pos += sizeof(img_csum);

	sys_put_le32(cur_offset, &data[pos]);
	pos += sizeof(cur_offset);

	sys_put_le16(sync_buffer_size, &data[pos]);
	pos += sizeof(sync_buffer_size);

	__ASSERT_NO_MSG(pos == data_size);
}

static void handle_reboot_request(uint8_t *data, size_t *size)
{
	LOG_INF("System reboot requested");

	*size = sizeof(bool);
	data[0] = true;

	k_delayed_work_submit(&reboot_request, REBOOT_REQUEST_TIMEOUT);
}

#if CONFIG_SECURE_BOOT
static void handle_image_info_request(uint8_t *data, size_t *size)
{
	const struct fw_info *info;
	uint8_t flash_area_id;

	if (dfu_slot_id() == IMAGE1_ID) {
		info = fw_info_find(IMAGE0_ADDRESS);
		flash_area_id = 0;
	} else {
		info = fw_info_find(IMAGE1_ADDRESS);
		flash_area_id = 1;
	}

	if (info) {
		uint32_t image_size = info->size;
		uint32_t build_num = info->version;
		uint8_t major = 0;
		uint8_t minor = 0;
		uint16_t revision = 0;

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
#elif CONFIG_BOOTLOADER_MCUBOOT
static void handle_image_info_request(uint8_t *data, size_t *size)
{
	struct mcuboot_img_header header;
	uint8_t flash_area_id;
	uint8_t bank_header_area_id;

	if (dfu_slot_id() == IMAGE1_ID) {
		flash_area_id = 0;
		bank_header_area_id = IMAGE0_ID;
	} else {
		flash_area_id = 1;
		bank_header_area_id = IMAGE1_ID;
	}

	int err = boot_read_bank_header(bank_header_area_id, &header,
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
		size_t pos = 0;

		*size = data_size;

		data[pos] = flash_area_id;
		pos += sizeof(flash_area_id);

		sys_put_le32(header.h.v1.image_size, &data[pos]);
		pos += sizeof(header.h.v1.image_size);

		data[pos] = header.h.v1.sem_ver.major;
		pos += sizeof(header.h.v1.sem_ver.major);

		data[pos] = header.h.v1.sem_ver.minor;
		pos += sizeof(header.h.v1.sem_ver.minor);

		sys_put_le16(header.h.v1.sem_ver.revision, &data[pos]);
		pos += sizeof(header.h.v1.sem_ver.revision);

		sys_put_le32(header.h.v1.sem_ver.build_num, &data[pos]);
		pos += sizeof(header.h.v1.sem_ver.build_num);
	} else {
		LOG_ERR("Cannot obtain image information");
	}
}
#endif

static void update_config(const uint8_t opt_id, const uint8_t *data,
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

static void fetch_config(const uint8_t opt_id, uint8_t *data, size_t *size)
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
				  fetch_config);

	if (is_ble_peer_event(eh)) {
		device_in_use = true;

		return false;
	}

	if (is_module_state_event(eh)) {
		const struct module_state_event *event =
			cast_module_state_event(eh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
#if CONFIG_BOOTLOADER_MCUBOOT
			int err = boot_write_img_confirmed();

			if (err) {
				LOG_ERR("Cannot confirm a running image");
			}
#endif
			k_delayed_work_init(&dfu_timeout, dfu_timeout_handler);
			k_delayed_work_init(&reboot_request, reboot_request_handler);
			k_delayed_work_init(&background_erase, background_erase_handler);
			k_delayed_work_init(&background_store, background_store_handler);

			if (!dfu_lock(MODULE_ID(MODULE))) {
				/* Should not happen. */
				__ASSERT_NO_MSG(false);
			} else {
				k_delayed_work_submit(&background_erase,
						      K_NO_WAIT);
			}
		}
		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, hid_report_event);
EVENT_SUBSCRIBE_EARLY(MODULE, config_event);
EVENT_SUBSCRIBE(MODULE, module_state_event);
EVENT_SUBSCRIBE(MODULE, ble_peer_event);
