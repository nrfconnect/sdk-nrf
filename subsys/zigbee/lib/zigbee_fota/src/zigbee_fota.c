/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <logging/log.h>
#include <dfu/mcuboot.h>
#include <dfu/dfu_target.h>
#include <dfu/dfu_target_mcuboot.h>
#include <zboss_api.h>
#include <zigbee/zigbee_error_handler.h>
#include <zb_nrf_platform.h>
#include <zigbee/zigbee_fota.h>
#include "zigbee_ota.h"
#include <pm_config.h>

LOG_MODULE_REGISTER(zigbee_fota, CONFIG_ZIGBEE_FOTA_LOG_LEVEL);

#define MANDATORY_HEADER_LEN   sizeof(zb_zcl_ota_upgrade_file_header_t)
#define OPTIONAL_HEADER_LEN    sizeof(zb_zcl_ota_upgrade_file_header_optional_t)
#define TOTAL_HEADER_LEN       (MANDATORY_HEADER_LEN + OPTIONAL_HEADER_LEN)

#define MAGIC_WORD_SIZE 32

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

static uint8_t zb_ota_buf[CONFIG_ZIGBEE_FOTA_DATA_BLOCK_SIZE];
static struct zb_ota_dfu_context ota_ctx;
static struct zb_ota_client_ctx dev_ctx;
static zigbee_fota_callback_t callback;

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
				(uint16_t)CONFIG_ZIGBEE_FOTA_HW_VERSION,
				CONFIG_ZIGBEE_FOTA_DATA_BLOCK_SIZE,
				ZB_ZCL_OTA_UPGRADE_QUERY_TIMER_COUNT_DEF);

ZB_HA_DECLARE_OTA_UPGRADE_CLIENT_CLUSTER_LIST(ota_upgrade_client_clusters,
	ota_basic_attr_list, ota_upgrade_attr_list);

ZB_HA_DECLARE_OTA_UPGRADE_CLIENT_EP(zigbee_fota_client_ep,
				    CONFIG_ZIGBEE_FOTA_ENDPOINT,
				    ota_upgrade_client_clusters);

static void send_evt(zb_uint8_t id)
{
	__ASSERT(id != ZIGBEE_FOTA_EVT_PROGRESS, "use send_progress");
	const struct zigbee_fota_evt evt = {
		.id = (enum zigbee_fota_evt_id)id
	};
	callback(&evt);
}

static void send_progress(zb_uint8_t progress)
{
#ifdef CONFIG_ZIGBEE_FOTA_PROGRESS_EVT
	const struct zigbee_fota_evt evt = { .id = ZIGBEE_FOTA_EVT_PROGRESS,
					 .dl.progress = progress };
	callback(&evt);
#endif /* CONFIG_ZIGBEE_FOTA_PROGRESS_EVT */
}

/**@brief Function for initializing all OTA clusters attributes.
 *
 * @note This function shall be called after the dfu initialization.
 */
static void ota_client_attr_init(void)
{
	struct mcuboot_img_header mcuboot_header;
	union zb_ota_app_ver app_image_ver = {0};

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
	dev_ctx.ota_attr.manufacturer = CONFIG_ZIGBEE_FOTA_MANUFACTURER_ID;
	dev_ctx.ota_attr.image_type = CONFIG_ZIGBEE_FOTA_IMAGE_TYPE;
	dev_ctx.ota_attr.min_block_reque = 0;
	dev_ctx.ota_attr.image_stamp = ZB_ZCL_OTA_UPGRADE_IMAGE_STAMP_MIN_VALUE;
}

static void ota_dfu_reset(void)
{
	memset(&ota_ctx, 0, sizeof(ota_ctx));
}

static void ota_dfu_target_callback_handler(enum dfu_target_evt_id evt)
{
	switch (evt) {
	case DFU_TARGET_EVT_TIMEOUT:
		send_evt(ZIGBEE_FOTA_EVT_ERASE_PENDING);
		break;
	case DFU_TARGET_EVT_ERASE_DONE:
		send_evt(ZIGBEE_FOTA_EVT_ERASE_DONE);
		break;
	default:
		send_evt(ZIGBEE_FOTA_EVT_ERROR);
	}
}

static uint8_t ota_dfu_target_init(const uint8_t *magic_word_buf)
{
	int err = 0;
	int img_type = dfu_target_img_type(magic_word_buf, MAGIC_WORD_SIZE);

	err = dfu_target_mcuboot_set_buf(zb_ota_buf, sizeof(zb_ota_buf));
	if (err < 0) {
		LOG_ERR("dfu_target_mcuboot_set_buf err %d", err);
		return err;
	}

	err = dfu_target_init(img_type, ota_ctx.bin_size,
			      ota_dfu_target_callback_handler);
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

static zb_uint8_t ota_process_mandatory_header(zb_uint8_t *data, uint32_t len,
					       uint32_t *bytes_copied)
{
	uint32_t bytes_left =
		MANDATORY_HEADER_LEN - ota_ctx.ota_header_fill_level;
	*bytes_copied = MIN(bytes_left, len);

	LOG_DBG("Process mandatory header.");
	LOG_DBG("Bytes left: %d copy: %d", bytes_left, *bytes_copied);

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

static zb_uint8_t ota_process_optional_header(zb_uint8_t *data, uint32_t len,
					      uint32_t *bytes_copied)
{
	uint32_t bytes_left =
		ota_ctx.ota_header_size - ota_ctx.ota_header_fill_level;
	*bytes_copied = MIN(bytes_left, len);

	LOG_DBG("Process optional header.");
	LOG_DBG("Bytes left: %d copy: %d", bytes_left, *bytes_copied);

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

static zb_uint8_t ota_process_magic_word(zb_uint8_t *data, uint32_t len,
					 uint32_t *bytes_copied)
{
	static uint8_t mgw_buf[MAGIC_WORD_SIZE];
	uint32_t bytes_left = 0;

	if (ota_ctx.ota_firmware_fill_level < MAGIC_WORD_SIZE) {
		bytes_left = MAGIC_WORD_SIZE - ota_ctx.ota_firmware_fill_level;
	}
	*bytes_copied = MIN(bytes_left, len);

	LOG_DBG("Process magic word.");
	LOG_DBG("Bytes left: %d copy: %d", bytes_left, *bytes_copied);

	memcpy(&mgw_buf[ota_ctx.ota_firmware_fill_level], data, *bytes_copied);
	ota_ctx.ota_firmware_fill_level += *bytes_copied;

	if (ota_ctx.ota_firmware_fill_level == MAGIC_WORD_SIZE) {
		int err = ota_dfu_target_init(mgw_buf);

		ota_ctx.process_magic_word = false;
		if (err != 0) {
			return ZB_ZCL_OTA_UPGRADE_STATUS_ERROR;
		}

		ota_ctx.process_bin_image = true;
	}

	return ZB_ZCL_OTA_UPGRADE_STATUS_OK;
}

static zb_uint8_t ota_process_firmware(zb_uint8_t *data, uint32_t len,
				       uint32_t *bytes_copied)
{
	int err = 0;

	LOG_DBG("Process firmware.");
	LOG_DBG("Bytes left: %d copy: %d",
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

	send_progress(ota_ctx.ota_firmware_fill_level * 100 /
		      (ota_ctx.bin_size + 1));
	return ZB_ZCL_OTA_UPGRADE_STATUS_OK;
}

/** @brief Code to process the incoming Zigbee OTA frame
 *
 *  @param ota   Pointer to the zb_zcl_ota_upgrade_value_param_t structure,
 *               passed from the handler
 *  @param bufid ZBOSS buffer id
 *
 *  @return ZB_ZCL_OTA_UPGRADE_STATUS_BUSY if OTA has to be suspended,
 *          ZB_ZCL_OTA_UPGRADE_STATUS_OK otherwise
 */
static zb_uint8_t ota_process_chunk(
	const zb_zcl_ota_upgrade_value_param_t *ota, zb_bufid_t bufid)
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
	if (!ota_ctx.mandatory_header_finished) {
		ret = ota_process_mandatory_header(
			&ota->upgrade.receive.block_data[bytes_consumed],
			ota->upgrade.receive.data_length - bytes_consumed,
			&bytes_copied);
		bytes_consumed += bytes_copied;
	}

	/* Processing optional header is activated when downloaded header
	 * length is greater than MANDATORY_HEADER_LEN.
	 */
	if (ota_ctx.process_optional_header) {
		ret = ota_process_optional_header(
			&ota->upgrade.receive.block_data[bytes_consumed],
			ota->upgrade.receive.data_length - bytes_consumed,
			&bytes_copied);
		bytes_consumed += bytes_copied;
	}

	/* Validate magic word on the beginning of image and initialize
	 *  dfu target.
	 */
	if (ota_ctx.process_magic_word) {
		ret = ota_process_magic_word(
			&ota->upgrade.receive.block_data[bytes_consumed],
			ota->upgrade.receive.data_length - bytes_consumed,
			&bytes_copied);
		bytes_consumed += bytes_copied;
	}

	/* Pass the image to the DFU target module. */
	if (ota_ctx.process_bin_image) {
		ret = ota_process_firmware(
			&ota->upgrade.receive.block_data[bytes_consumed],
			ota->upgrade.receive.data_length - bytes_consumed,
			&bytes_copied);
		bytes_consumed += bytes_copied;
	}
	return ret;
}

static void ota_server_discovery_handler(struct k_timer *work)
{
	/* The user callback function is not invoked upon OTA server discovery
	 * failure. This is the reason why an explicit attribute value check
	 * must be used.
	 */
	zb_zcl_attr_t *attr_desc = zb_zcl_get_attr_desc_a(
					CONFIG_ZIGBEE_FOTA_ENDPOINT,
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
		k_timer_stop(&dev_ctx.alarm);
	}
}

void zigbee_fota_abort(void)
{
	LOG_INF("ABORT Zigbee DFU");
	/* Reset the context. */
	ota_dfu_reset();
	dfu_target_done(false);
}

int zigbee_fota_init(zigbee_fota_callback_t client_callback)
{
	if (client_callback == NULL) {
		return -EINVAL;
	}

	callback = client_callback;
	ota_client_attr_init();

	/* Initialize periodic OTA server discovery. */
	k_timer_init(&dev_ctx.alarm, ota_server_discovery_handler, NULL);

	return 0;
}


void zigbee_fota_signal_handler(zb_bufid_t bufid)
{
	zb_zdo_app_signal_hdr_t  *sig_handler = NULL;
	zb_zdo_app_signal_type_t  sig = zb_get_app_signal(bufid, &sig_handler);
	zb_ret_t                  status = ZB_GET_APP_SIGNAL_STATUS(bufid);

	switch (sig) {
	case ZB_BDB_SIGNAL_DEVICE_REBOOT:
	/* fall-through */
	case ZB_BDB_SIGNAL_STEERING:
		if (status == RET_OK) {
			k_timer_start(&dev_ctx.alarm, K_NO_WAIT, K_HOURS(24));
		}
		break;
	default:
		break;
	}
}

void zigbee_fota_zcl_cb(zb_bufid_t bufid)
{
	zb_zcl_device_callback_param_t *device_cb_param =
		ZB_BUF_GET_PARAM(bufid, zb_zcl_device_callback_param_t);

	if (device_cb_param->device_cb_id != ZB_ZCL_OTA_UPGRADE_VALUE_CB_ID) {
		return;
	}

	device_cb_param->status = RET_OK;
	zb_zcl_ota_upgrade_value_param_t *ota_upgrade_value =
		&(device_cb_param->cb_param.ota_value_param);

	switch (ota_upgrade_value->upgrade_status) {
	case ZB_ZCL_OTA_UPGRADE_STATUS_START:
		LOG_INF("New OTA image available:");
		LOG_INF("\tManufacturer: 0x%04x",
			ota_upgrade_value->upgrade.start.manufacturer);
		LOG_INF("\tType: 0x%04x",
			ota_upgrade_value->upgrade.start.image_type);
		LOG_INF("\tVersion: 0x%08x",
			ota_upgrade_value->upgrade.start.file_version);

		/* Check if OTA client is in the middle of image
		 * download. If so, silently ignore the second
		 * QueryNextImageResponse packet from OTA server.
		 */
		if (zb_zcl_ota_upgrade_get_ota_status(
			device_cb_param->endpoint) !=
			ZB_ZCL_OTA_UPGRADE_IMAGE_STATUS_NORMAL) {

			ota_upgrade_value->upgrade_status =
				ZB_ZCL_OTA_UPGRADE_STATUS_BUSY;

		/* Check if we're not downgrading.
		 * If we do, let's politely say no since we do not
		 * support that.
		 */
		} else if (ota_upgrade_value->upgrade.start.file_version
			 > dev_ctx.ota_attr.file_version) {
			ota_dfu_reset();
			ota_upgrade_value->upgrade_status =
				ZB_ZCL_OTA_UPGRADE_STATUS_OK;
		} else {
			LOG_DBG("ZB_ZCL_OTA_UPGRADE_STATUS_ABORT");
			ota_upgrade_value->upgrade_status =
				ZB_ZCL_OTA_UPGRADE_STATUS_ABORT;
		}
		break;

	case ZB_ZCL_OTA_UPGRADE_STATUS_RECEIVE:
		/* Process image block. */
		ota_upgrade_value->upgrade_status =
			ota_process_chunk(ota_upgrade_value, bufid);
		break;

	case ZB_ZCL_OTA_UPGRADE_STATUS_CHECK:
		LOG_INF("New OTA image downloaded.");
		ota_upgrade_value->upgrade_status =
			ZB_ZCL_OTA_UPGRADE_STATUS_OK;
		break;

	case ZB_ZCL_OTA_UPGRADE_STATUS_APPLY:
		LOG_INF("Mark OTA image as ready to be installed.");
		ota_upgrade_value->upgrade_status =
			ZB_ZCL_OTA_UPGRADE_STATUS_OK;
		send_progress(ZIGBEE_FOTA_EVT_DL_COMPLETE_VAL);
		break;

	case ZB_ZCL_OTA_UPGRADE_STATUS_FINISH:
		LOG_INF("Zigbee DFU completed. Reboot the application.");
		/* It is time to upgrade FW.
		 * We use callback so the stack can have time to i.e.
		 * send response etc.
		 */
		ZB_SCHEDULE_APP_CALLBACK(send_evt, ZIGBEE_FOTA_EVT_FINISHED);
		if (dfu_target_reset() != 0) {
			LOG_ERR("Unable to reset DFU target");
		}
		break;

	case ZB_ZCL_OTA_UPGRADE_STATUS_ABORT:
		LOG_INF("Zigbee DFU Aborted");
		ota_upgrade_value->upgrade_status =
			ZB_ZCL_OTA_UPGRADE_STATUS_ABORT;
		zigbee_fota_abort();
		send_evt(ZIGBEE_FOTA_EVT_ERROR);
		break;

	default:
		break;
	}

	/* No need to free the buffer - stack handles that if needed. */
	return;
}
