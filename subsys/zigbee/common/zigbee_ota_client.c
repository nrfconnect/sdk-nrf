/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <logging/log.h>
#include <dfu/mcuboot.h>
#include <dfu/dfu_target.h>
#include <power/reboot.h>
#include "pm_config.h"
#include <zboss_api.h>
#include <zb_error_handler.h>
#include <zb_nrf_platform.h>
#include <zigbee_ota.h>
#include <zigbee_ota_client.h>
#include <dk_buttons_and_leds.h>

LOG_MODULE_DECLARE(app);

#define MANDATORY_HEADER_LEN   sizeof(zb_zcl_ota_upgrade_file_header_t)
#define OPTIONAL_HEADER_LEN    sizeof(zb_zcl_ota_upgrade_file_header_optional_t)
#define TOTAL_HEADER_LEN       (MANDATORY_HEADER_LEN + OPTIONAL_HEADER_LEN)

#define MAGIC_WORD_SIZE 4

struct zb_ota_dfu_context {
	uint32_t        ota_header_size;
	uint32_t        bin_size;
	uint32_t        total_size;
	uint8_t         ota_header[TOTAL_HEADER_LEN];
	uint32_t        ota_header_fill_level;
	uint32_t        ota_firmware_fill_level;
	bool            mandatory_header_finished;
	bool            process_optional_header;
	bool            process_magic_word;
	bool            process_bin_image;
};

struct zb_ota_upgrade_attr {
	zb_ieee_addr_t upgrade_server;
	zb_uint32_t    file_offset;
	zb_uint32_t    file_version;
	zb_uint16_t    stack_version;
	zb_uint32_t    downloaded_file_ver;
	zb_uint32_t    downloaded_stack_ver;
	zb_uint8_t     image_status;
	zb_uint16_t    manufacturer;
	zb_uint16_t    image_type;
	zb_uint16_t    min_block_reque;
	zb_uint16_t    image_stamp;
	zb_uint16_t    server_addr;
	zb_uint8_t     server_ep;
};

struct zb_ota_basic_attr {
	zb_uint8_t zcl_version;
	zb_uint8_t power_source;
};

struct zb_ota_client_ctx {
	struct zb_ota_basic_attr   basic_attr;
	struct zb_ota_upgrade_attr ota_attr;
	struct k_timer             alarm;
};

union zb_ota_app_ver {
	uint32_t uint32_ver;
	struct {
		uint16_t revision;
		uint8_t minor;
		uint8_t major;
	};
};

static struct zb_ota_dfu_context ota_ctx;
static struct zb_ota_client_ctx dev_ctx;
static union zb_ota_app_ver app_image_ver;

/* Declare attribute list for Basic cluster. */
ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(ota_basic_attr_list,
				 &dev_ctx.basic_attr.zcl_version,
				 &dev_ctx.basic_attr.power_source);

/* OTA cluster attributes data. */
ZB_ZCL_DECLARE_OTA_UPGRADE_ATTRIB_LIST(ota_upgrade_attr_list,
				dev_ctx.ota_attr.upgrade_server,
				&dev_ctx.ota_attr.file_offset,
				&dev_ctx.ota_attr.file_version,
				&dev_ctx.ota_attr.stack_version,
				&dev_ctx.ota_attr.downloaded_file_ver,
				&dev_ctx.ota_attr.downloaded_stack_ver,
				&dev_ctx.ota_attr.image_status,
				&dev_ctx.ota_attr.manufacturer,
				&dev_ctx.ota_attr.image_type,
				&dev_ctx.ota_attr.min_block_reque,
				&dev_ctx.ota_attr.image_stamp,
				&dev_ctx.ota_attr.server_addr,
				&dev_ctx.ota_attr.server_ep,
				(uint16_t)CONFIG_ZIGBEE_OTA_HW_VERSION,
				CONFIG_ZIGBEE_OTA_DATA_BLOCK_SIZE,
				ZB_ZCL_OTA_UPGRADE_QUERY_TIMER_COUNT_DEF);

ZB_HA_DECLARE_OTA_UPGRADE_CLIENT_CLUSTER_LIST(ota_upgrade_client_clusters,
	ota_basic_attr_list, ota_upgrade_attr_list);

ZB_HA_DECLARE_OTA_UPGRADE_CLIENT_EP(ota_upgrade_client_ep,
				    CONFIG_ZIGBEE_OTA_ENDPOINT,
				    ota_upgrade_client_clusters);

/**@brief Function for initializing all OTA clusters attributes.
 *
 * @note This function shall be called after the dfu initialization.
 */
static void zb_ota_client_attr_init(void)
{
	struct mcuboot_img_header mcuboot_header;

	int err = boot_read_bank_header(PM_MCUBOOT_PRIMARY_ID,
					&mcuboot_header,
					sizeof(mcuboot_header));
	if (err) {
		LOG_WRN("Failed read app ver from image header (err %d)", err);
	} else {
		app_image_ver.major = mcuboot_header.h.v1.sem_ver.major;
		app_image_ver.minor = mcuboot_header.h.v1.sem_ver.minor;
		app_image_ver.revision = mcuboot_header.h.v1.sem_ver.revision;
	}

	/* Basic cluster attributes data. */
	dev_ctx.basic_attr.zcl_version  = ZB_ZCL_VERSION;
	dev_ctx.basic_attr.power_source = ZB_ZCL_BASIC_POWER_SOURCE_UNKNOWN;

	/* OTA cluster attributes data. */
	zb_ieee_addr_t addr = ZB_ZCL_OTA_UPGRADE_SERVER_DEF_VALUE;

	ZB_MEMCPY(dev_ctx.ota_attr.upgrade_server, addr,
		  sizeof(zb_ieee_addr_t));
	dev_ctx.ota_attr.file_offset = ZB_ZCL_OTA_UPGRADE_FILE_OFFSET_DEF_VALUE;
	dev_ctx.ota_attr.file_version = app_image_ver.uint32_ver;
	dev_ctx.ota_attr.stack_version =
		ZB_ZCL_OTA_UPGRADE_FILE_HEADER_STACK_PRO;
	dev_ctx.ota_attr.downloaded_file_ver  =
		ZB_ZCL_OTA_UPGRADE_DOWNLOADED_FILE_VERSION_DEF_VALUE;
	dev_ctx.ota_attr.downloaded_stack_ver =
		ZB_ZCL_OTA_UPGRADE_DOWNLOADED_STACK_DEF_VALUE;
	dev_ctx.ota_attr.image_status =
		ZB_ZCL_OTA_UPGRADE_IMAGE_STATUS_DEF_VALUE;
	dev_ctx.ota_attr.manufacturer = CONFIG_ZIGBEE_OTA_MANUFACTURER_ID;
	dev_ctx.ota_attr.image_type = CONFIG_ZIGBEE_OTA_IMAGE_TYPE;
	dev_ctx.ota_attr.min_block_reque = 0;
	dev_ctx.ota_attr.image_stamp = ZB_ZCL_OTA_UPGRADE_IMAGE_STAMP_MIN_VALUE;
}

uint32_t zb_ota_get_image_ver(void)
{
	return app_image_ver.uint32_ver;
}

void zb_ota_dfu_init(void)
{
	memset(&ota_ctx, 0, sizeof(ota_ctx));
}

void zb_ota_dfu_abort(void)
{
	LOG_INF("ABORT Zigbee DFU");
	/* Reset the context. */
	memset(&ota_ctx, 0, sizeof(ota_ctx));
	dfu_target_done(false);
}

/* Code for rebooting the chip. */
void zb_ota_reboot_application(zb_uint8_t param)
{
	ARG_UNUSED(param);

	LOG_INF("Reboot application.");
	int res = dfu_target_reset();

	if (res != 0) {
		LOG_ERR("Unable to reset DFU target");
	}
	sys_reboot(SYS_REBOOT_COLD);
}

void zb_ota_client_init(void)
{
	/* Marks the currently running image as confirmed. */
	if (!boot_is_img_confirmed()) {
		int ret = boot_write_img_confirmed();

		if (ret) {
			LOG_ERR("Couldn't confirm image: %d", ret);
		} else {
			LOG_INF("Marked image as OK");
		}
	}

	zb_ota_client_attr_init();

	/* Initialize periodic OTA server discovery. */
	zb_ota_client_periodical_discovery_srv_init();
}

void zb_ota_confirm_image(void)
{
	if (!boot_is_img_confirmed()) {
		int ret = boot_write_img_confirmed();

		if (ret) {
			LOG_ERR("Couldn't confirm image: %d", ret);
		} else {
			LOG_INF("Marked image as OK");
		}
	}
}

static void zb_ota_dfu_target_callback_handler(enum dfu_target_evt_id evt)
{
	switch (evt) {
	case DFU_TARGET_EVT_TIMEOUT:
		LOG_INF("DFU_TARGET_EVT_TIMEOUT");
		break;
	case DFU_TARGET_EVT_ERASE_DONE:
		LOG_INF("DFU_TARGET_EVT_ERASE_DONE");
		break;
	default:
		LOG_INF("DFU_TARGET_EVT_ERROR");
	}
}

static uint8_t zb_ota_dfu_target_init(const uint8_t *magic_word_buf)
{
	int err = 0;
	int img_type = dfu_target_img_type(magic_word_buf, MAGIC_WORD_SIZE);

	err = dfu_target_init(img_type, ota_ctx.bin_size,
			      zb_ota_dfu_target_callback_handler);
	if (err < 0) {
		LOG_ERR("dfu_target_init err %d", err);
		return err;
	}

	err = dfu_target_write(magic_word_buf, MAGIC_WORD_SIZE);
	if (err != 0) {
		LOG_ERR("dfu_target_write err %d", err);
		int res = dfu_target_done(false);

		if (res != 0) {
			LOG_ERR("Unable to free DFU target resources");
		}
		return err;
	}

	return err;
}

static zb_uint8_t zb_ota_process_mandatory_header(zb_uint8_t *data,
	uint32_t len, uint32_t *bytes_copied)
{
	uint32_t bytes_left =
		MANDATORY_HEADER_LEN - ota_ctx.ota_header_fill_level;
	*bytes_copied = MIN(bytes_left, len);

	LOG_INF("Process mandatory header.");
	LOG_INF("Bytes left: %d copy: %d", bytes_left, *bytes_copied);

	memcpy(&ota_ctx.ota_header[ota_ctx.ota_header_fill_level],
		data, *bytes_copied);
	ota_ctx.ota_header_fill_level += *bytes_copied;

	if (ota_ctx.ota_header_fill_level == MANDATORY_HEADER_LEN) {
		ota_ctx.mandatory_header_finished = true;

		/* All the mandatory header is copied. */
		zb_zcl_ota_upgrade_file_header_t *hdr =
			(zb_zcl_ota_upgrade_file_header_t *)ota_ctx.ota_header;
		ota_ctx.ota_header_size = hdr->header_length;
		ota_ctx.total_size = hdr->total_image_size;
		ota_ctx.bin_size = ota_ctx.total_size - ota_ctx.ota_header_size;

		if (hdr->header_length > MANDATORY_HEADER_LEN) {
			ota_ctx.process_optional_header = true;
		} else {
			ota_ctx.process_magic_word = true;
		}

		LOG_INF("Mandatory header received.");
		LOG_INF("\tHeader size:     %d", ota_ctx.ota_header_size);
		LOG_INF("\tFirmware size:   %d", ota_ctx.bin_size);
		LOG_INF("\tOptional header: %s",
			(ota_ctx.process_optional_header ? "true" : "false"));
	}

	return ZB_ZCL_OTA_UPGRADE_STATUS_OK;
}

static zb_uint8_t zb_ota_process_optional_header(zb_uint8_t *data, uint32_t len,
						 uint32_t *bytes_copied)
{
	uint32_t bytes_left =
		ota_ctx.ota_header_size - ota_ctx.ota_header_fill_level;
	*bytes_copied = MIN(bytes_left, len);

	LOG_INF("Process optional header.");
	LOG_INF("Bytes left: %d copy: %d", bytes_left, *bytes_copied);

	if (ota_ctx.ota_header_fill_level < TOTAL_HEADER_LEN) {
		uint32_t bytes_to_copy = MIN(*bytes_copied, TOTAL_HEADER_LEN -
						ota_ctx.ota_header_fill_level);
		memcpy(&ota_ctx.ota_header[ota_ctx.ota_header_fill_level],
			data, bytes_to_copy);
	}

	/* Current software should skip unsupported header fields. */
	ota_ctx.ota_header_fill_level += *bytes_copied;

	if (ota_ctx.ota_header_fill_level == ota_ctx.ota_header_size) {
		LOG_INF("Full header received. Continue downloading firmware.");
		ota_ctx.process_optional_header = false;
		ota_ctx.process_magic_word = true;
	}

	return ZB_ZCL_OTA_UPGRADE_STATUS_OK;
}

static zb_uint8_t zb_ota_process_magic_word(zb_uint8_t *data, uint32_t len,
					    uint32_t *bytes_copied)
{
	static uint8_t mgw_buf[MAGIC_WORD_SIZE];
	uint32_t bytes_left = 0;

	if (ota_ctx.ota_firmware_fill_level < MAGIC_WORD_SIZE) {
		bytes_left = MAGIC_WORD_SIZE - ota_ctx.ota_firmware_fill_level;
	}
	*bytes_copied = MIN(bytes_left, len);

	LOG_INF("Process magic word.");
	LOG_INF("Bytes left: %d copy: %d", bytes_left, *bytes_copied);

	memcpy(&mgw_buf[ota_ctx.ota_firmware_fill_level], data, *bytes_copied);
	ota_ctx.ota_firmware_fill_level += *bytes_copied;

	if (ota_ctx.ota_firmware_fill_level == MAGIC_WORD_SIZE) {
		int err = zb_ota_dfu_target_init(mgw_buf);

		ota_ctx.process_magic_word = false;
		if (err != 0) {
			return ZB_ZCL_OTA_UPGRADE_STATUS_ERROR;
		}

		ota_ctx.process_bin_image = true;
	}

	return ZB_ZCL_OTA_UPGRADE_STATUS_OK;
}

static zb_uint8_t zb_ota_process_firmware(zb_uint8_t *data, uint32_t len,
					  uint32_t *bytes_copied)
{
	int err = 0;

	LOG_INF("Process firmware.");
	LOG_INF("Bytes left: %d copy: %d",
		ota_ctx.bin_size - ota_ctx.ota_firmware_fill_level, len);

	*bytes_copied = len;
	ota_ctx.ota_firmware_fill_level += len;

	err = dfu_target_write(data, len);
	if (err != 0) {
		LOG_ERR("dfu_target_write err %d", err);

		err = dfu_target_done(false);
		if (err != 0) {
			LOG_ERR(
			 "Unable to free DFU target resources");
		}

		return ZB_ZCL_OTA_UPGRADE_STATUS_ERROR;
	}

	if (ota_ctx.bin_size == ota_ctx.ota_firmware_fill_level) {
		LOG_INF("Firmware downloaded.");
		ota_ctx.process_bin_image = false;
		ota_ctx.mandatory_header_finished = false;

		err = dfu_target_done(true);
		if (err != 0) {
			LOG_ERR("dfu_target_done error: %d",
				err);
			return ZB_ZCL_OTA_UPGRADE_STATUS_ERROR;
		}
	}

	return ZB_ZCL_OTA_UPGRADE_STATUS_OK;
}

zb_uint8_t zb_ota_process_chunk(const zb_zcl_ota_upgrade_value_param_t *ota,
				zb_bufid_t bufid)
{
	uint8_t ret = ZB_ZCL_OTA_UPGRADE_STATUS_OK;
	uint32_t bytes_consumed = 0;
	uint32_t bytes_copied = 0;

	if (ota->upgrade.receive.file_offset !=
	    ota_ctx.ota_header_fill_level + ota_ctx.ota_firmware_fill_level) {
		LOG_WRN("Unaligned OTA transfer. Expected: %d, received: %d",
			ota_ctx.ota_header_fill_level +
				ota_ctx.ota_firmware_fill_level,
			ota->upgrade.receive.file_offset);
		return ZB_ZCL_OTA_UPGRADE_STATUS_ERROR;
	}

	/* Process image header and save it in the memory. */
	if (ota_ctx.mandatory_header_finished == false) {
		ret = zb_ota_process_mandatory_header(
			&ota->upgrade.receive.block_data[bytes_consumed],
			ota->upgrade.receive.data_length - bytes_consumed,
			&bytes_copied);
		bytes_consumed += bytes_copied;
	}

	/* Processing optional header is activated when downloaded header
	 * length is greater than MANDATORY_HEADER_LEN.
	 */
	if (ota_ctx.process_optional_header) {
		ret = zb_ota_process_optional_header(
			&ota->upgrade.receive.block_data[bytes_consumed],
			ota->upgrade.receive.data_length - bytes_consumed,
			&bytes_copied);
		bytes_consumed += bytes_copied;
	}

	/* Validate magic word on the beginning of image and initialize
	 *  dfu target.
	 */
	if (ota_ctx.process_magic_word) {
		ret = zb_ota_process_magic_word(
			&ota->upgrade.receive.block_data[bytes_consumed],
			ota->upgrade.receive.data_length - bytes_consumed,
			&bytes_copied);
		bytes_consumed += bytes_copied;
	}

	/* Pass the image to the DFU target module. */
	if (ota_ctx.process_bin_image) {
		ret = zb_ota_process_firmware(
			&ota->upgrade.receive.block_data[bytes_consumed],
			ota->upgrade.receive.data_length - bytes_consumed,
			&bytes_copied);
		bytes_consumed += bytes_copied;
	}
	return ret;
}

static void zb_ota_server_discovery_handler(struct k_timer *work)
{
	/* The user callback function is not invoked upon OTA server discovery
	 * failure. This is the reason why an explicit attribute value check
	 * must be used.
	 */
	zb_zcl_attr_t *attr_desc = zb_zcl_get_attr_desc_a(
					CONFIG_ZIGBEE_OTA_ENDPOINT,
					ZB_ZCL_CLUSTER_ID_OTA_UPGRADE,
					ZB_ZCL_CLUSTER_CLIENT_ROLE,
					ZB_ZCL_ATTR_OTA_UPGRADE_SERVER_ID);

	/* A periodical discovery was started on an endpoint that does
	 * not implement OTA client. Abort the application.
	 */
	ZB_ASSERT(attr_desc);

	if (ZB_IS_64BIT_ADDR_UNKNOWN(attr_desc->data_p)) {
		/* Restart OTA server discovery. In case of OOM state,
		 * the discovery mechanism is restarted in the next interval.
		 */
		zigbee_get_out_buf_delayed(zb_zcl_ota_upgrade_init_client);
	} else {
		/* OTA server discovery is finished. Stop the timer. */
		zb_ota_client_periodical_discovery_srv_stop();
	}
}

/* Create an instance of timer for OTA server periodical discovery. */
void zb_ota_client_periodical_discovery_srv_init(void)
{
	k_timer_init(&dev_ctx.alarm, zb_ota_server_discovery_handler, NULL);
}

void zb_ota_client_periodical_discovery_srv_start(void)
{
	k_timer_start(&dev_ctx.alarm, K_NO_WAIT, K_HOURS(24));
}

void zb_ota_client_periodical_discovery_srv_stop(void)
{
	k_timer_stop(&dev_ctx.alarm);
}
