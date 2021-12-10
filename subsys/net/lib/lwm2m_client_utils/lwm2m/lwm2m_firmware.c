/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <nrf_modem.h>
#include <drivers/flash.h>
#include <dfu/dfu_target.h>
#include <dfu/dfu_target_mcuboot.h>
#include <dfu/mcuboot.h>
#include <logging/log_ctrl.h>
#include <net/lwm2m.h>
#include <modem/nrf_modem_lib.h>
#include <sys/reboot.h>
#include <net/fota_download.h>
#include <net/lwm2m_client_utils.h>
#include <net/lwm2m_client_utils_fota.h>
/* Firmware update needs access to internal functions as well */
#include <lwm2m_engine.h>

#if defined(CONFIG_DFU_TARGET_FULL_MODEM)
#include <dfu/dfu_target_full_modem.h>
#include <nrf_modem_full_dfu.h>
#include <dfu/fmfu_fdev.h>
#include <string.h>
#endif

#include <logging/log.h>
LOG_MODULE_REGISTER(lwm2m_firmware, CONFIG_LWM2M_CLIENT_UTILS_LOG_LEVEL);

#define BYTE_PROGRESS_STEP (1024 * 10)
#define REBOOT_DELAY       K_SECONDS(1)

static uint8_t firmware_buf[CONFIG_LWM2M_COAP_BLOCK_SIZE];

#if defined(CONFIG_DFU_TARGET_FULL_MODEM)
#define EXT_FLASH_DEVICE DT_LABEL(DT_INST(0, jedec_spi_nor))
static uint8_t fmfu_buf[1024];
static const struct device *flash_dev;
static struct k_work full_modem_update_work;
#endif

#ifdef CONFIG_DFU_TARGET_MCUBOOT
static uint8_t mcuboot_buf[CONFIG_APP_MCUBOOT_FLASH_BUF_SZ] __aligned(4);
#endif

static int image_type = DFU_TARGET_IMAGE_TYPE_ANY;
static char *fota_path;
static char *fota_host;
static int current_update_instance = -1;
static int fota_sec_tag;
#define MAX_INSTANCE_COUNT CONFIG_LWM2M_FIRMWARE_INSTANCE_COUNT

static struct k_work_delayable reboot_work;
static struct k_work download_work;

void client_acknowledge(void);

#if defined(CONFIG_DFU_TARGET_FULL_MODEM)
static void apply_fmfu_from_ext_flash(struct k_work *work)
{
	int ret;

	LOG_INF("Applying full modem firmware update from external flash\n");

	ret = nrf_modem_lib_shutdown();
	if (ret != 0) {
		LOG_ERR("nrf_modem_lib_shutdown() failed: %d\n", ret);
		return;
	}

	ret = nrf_modem_lib_init(FULL_DFU_MODE);
	if (ret != 0) {
		LOG_ERR("nrf_modem_lib_init(FULL_DFU_MODE) failed: %d\n", ret);
		return;
	}

	ret = fmfu_fdev_load(fmfu_buf, sizeof(fmfu_buf), flash_dev, 0);
	if (ret != 0) {
		LOG_ERR("fmfu_fdev_load failed: %d\n", ret);
		return;
	}
	LOG_INF("Modem firmware update completed\n");
	LOG_INF("Rebooting device");

	k_work_schedule(&reboot_work, REBOOT_DELAY);
}

static int configure_full_modem_update(void)
{
	int ret = 0;

	if (flash_dev == NULL) {
		flash_dev = device_get_binding(EXT_FLASH_DEVICE);
	}
	if (flash_dev == NULL) {
		LOG_ERR("Failed to get flash device: %s\n", EXT_FLASH_DEVICE);
	}

	const struct dfu_target_full_modem_params params = {
		.buf = fmfu_buf,
		.len = sizeof(fmfu_buf),
		.dev = &(struct dfu_target_fmfu_fdev){ .dev = flash_dev,
							.offset = 0,
							.size = 0 }
	};

	ret = dfu_target_full_modem_cfg(&params);
	if (ret != 0 && ret != -EALREADY) {
		LOG_ERR("dfu_target_full_modem_cfg failed: %d\n", ret);
	} else {
		ret = 0;
	}

	return ret;
}
#endif

static void reboot_work_handler(struct k_work *work)
{
	LOG_PANIC();
	sys_reboot(SYS_REBOOT_COLD);
}

static int firmware_update_cb(uint16_t obj_inst_id, uint8_t *args,
			    uint16_t args_len)
{
	ARG_UNUSED(args);
	ARG_UNUSED(args_len);

	int ret = 0;
	LOG_DBG("Executing firmware update");
	LOG_INF("Firmware image type %d", image_type);

	/* Bump update counter so it can be verified on the next reboot */
	struct update_counter update_counter;

	ret = fota_update_counter_read(obj_inst_id, &update_counter);
	if (ret) {
		LOG_ERR("Failed read update counter");
		goto cleanup;
	}
	LOG_INF("Update Counter: current %d, update %d",
		update_counter.current, update_counter.update);
	ret = fota_update_counter_update(obj_inst_id, COUNTER_UPDATE,
					 update_counter.current + 1);
	if (ret) {
		LOG_ERR("Failed to update the update counter: %d", ret);
		goto cleanup;
	}

#if defined(CONFIG_DFU_TARGET_FULL_MODEM)
	if (image_type == DFU_TARGET_IMAGE_TYPE_FULL_MODEM) {
		k_work_submit(&full_modem_update_work);
	} else
#endif
	{
		LOG_INF("Rebooting device");
		k_work_schedule(&reboot_work, REBOOT_DELAY);
	}

	return 0;

cleanup:
	return ret;
}

static void *firmware_get_buf(uint16_t obj_inst_id, uint16_t res_id,
			      uint16_t res_inst_id, size_t *data_len)
{
	*data_len = sizeof(firmware_buf);
	return firmware_buf;
}

static void dfu_target_cb(enum dfu_target_evt_id evt)
{
	ARG_UNUSED(evt);
}

static int firmware_block_received_cb(uint16_t obj_inst_id,
				      uint16_t res_id, uint16_t res_inst_id,
				      uint8_t *data, uint16_t data_len,
				      bool last_block, size_t total_size)
{
	static uint8_t percent_downloaded;
	static uint32_t bytes_downloaded;
	uint8_t curent_percent;
	uint32_t current_bytes;
	size_t offset;
	size_t skip = 0;
	int ret;

	/* Resets the DFU target */
	if (!total_size) {
		ret = -ENODATA;
		goto cleanup;
	}

	if (!data_len) {
		LOG_ERR("Data len is zero, nothing to write.");
		return -EINVAL;
	}

	if (bytes_downloaded == 0) {
		client_acknowledge();

		image_type = dfu_target_img_type(data, data_len);
		LOG_INF("Image type %d", image_type);

		switch (image_type) {
		case DFU_TARGET_IMAGE_TYPE_MODEM_DELTA:
		case DFU_TARGET_IMAGE_TYPE_FULL_MODEM:
			if (obj_inst_id !=
			    CONFIG_LWM2M_CLIENT_UTILS_FIRMWARE_INSTANCE_MODEM) {
				LOG_WRN("Unsupported DFU image type");
				ret = -ENOMSG;
				goto cleanup;
			}
			break;
		case DFU_TARGET_IMAGE_TYPE_MCUBOOT:
			if (obj_inst_id !=
			    CONFIG_LWM2M_CLIENT_UTILS_FIRMWARE_INSTANCE_APP) {
				LOG_WRN("Unsupported DFU image type");
				ret = -ENOMSG;
				goto cleanup;
			}
			break;
		default:
			LOG_ERR("Invalid DFU target image type");
			ret = -ENOMSG;
			goto cleanup;
		}

#if defined(CONFIG_DFU_TARGET_FULL_MODEM)
		if (image_type == DFU_TARGET_IMAGE_TYPE_FULL_MODEM) {
			configure_full_modem_update();
		}
#endif

		ret = dfu_target_init(image_type, total_size, dfu_target_cb);
		if (ret < 0) {
			LOG_ERR("Failed to init DFU target, err: %d", ret);
			goto cleanup;
		}

		LOG_INF("%s firmware download started.",
			image_type == DFU_TARGET_IMAGE_TYPE_MODEM_DELTA ||
			image_type == DFU_TARGET_IMAGE_TYPE_FULL_MODEM ?
				"Modem" :
				"Application");
	}

	ret = dfu_target_offset_get(&offset);
	if (ret < 0) {
		LOG_ERR("Failed to obtain current offset, err: %d", ret);
		goto cleanup;
	}

	/* Display a % downloaded or byte progress, if no total size was
	 * provided (this can happen in PULL mode FOTA)
	 */
	if (total_size > 0) {
		curent_percent = bytes_downloaded * 100 / total_size;
		if (curent_percent > percent_downloaded) {
			percent_downloaded = curent_percent;
			LOG_INF("Downloaded %d%%", percent_downloaded);
		}
	} else {
		current_bytes = bytes_downloaded + data_len;
		if (current_bytes / BYTE_PROGRESS_STEP >
		    bytes_downloaded / BYTE_PROGRESS_STEP) {
			LOG_INF("Downloaded %d kB", current_bytes / 1024);
		}
	}

	if (bytes_downloaded < offset) {
		skip = MIN(data_len, offset - bytes_downloaded);

		LOG_INF("Skipping bytes %d-%d, already written.",
			bytes_downloaded, bytes_downloaded + skip);
	}

	bytes_downloaded += data_len;

	if (skip == data_len) {
		/* Nothing to do. */
		return 0;
	}

	ret = dfu_target_write(data + skip, data_len - skip);
	if (ret < 0) {
		LOG_ERR("dfu_target_write error, err %d", ret);
		goto cleanup;
	}

	if (last_block) {
		/* Last write to flash should be flush write */
		ret = dfu_target_done(true);
		if (ret < 0) {
			LOG_ERR("dfu_target_done error, err %d", ret);
			goto cleanup;
		}
		LOG_INF("Firmware downloaded, %d bytes in total",
			bytes_downloaded);
	} else {
		/* Keep going */
		return 0;
	}

	if (total_size && (bytes_downloaded != total_size)) {
		LOG_ERR("Early last block, downloaded %d, expecting %d",
			bytes_downloaded, total_size);
		ret = -EIO;
	}

cleanup:
	if (ret < 0) {
		if (dfu_target_reset() < 0) {
			LOG_ERR("Failed to reset DFU target");
		}
	}

	bytes_downloaded = 0;
	percent_downloaded = 0;

	return ret;
}

static void fota_download_callback(const struct fota_download_evt *evt)
{
	switch (evt->id) {
	/* These two cases return immediately */
	case FOTA_DOWNLOAD_EVT_PROGRESS:
		LOG_DBG("DL progress %d", evt->progress);
		return;
	default:
		return;

	/* Following cases mark end of FOTA download */
	case FOTA_DOWNLOAD_EVT_CANCELLED:
	case FOTA_DOWNLOAD_EVT_ERROR:
		LOG_ERR("FOTA_DOWNLOAD_EVT_ERROR");
		lwm2m_firmware_set_update_state(current_update_instance, STATE_IDLE);
		break;
	case FOTA_DOWNLOAD_EVT_FINISHED:
		image_type = fota_download_target();
		LOG_INF("FOTA download finished, target %d", image_type);
		lwm2m_firmware_set_update_state(current_update_instance, STATE_DOWNLOADED);
		break;
	}
	k_free(fota_host);
	current_update_instance = -1;
	fota_host = NULL;
	fota_path = NULL;
}

static void start_fota_download(struct k_work *work)
{
#if defined(CONFIG_DFU_TARGET_FULL_MODEM)
	/* We can't know if the download is full modem firmware
	 * before the downloader actually starts, so configure
	 * the dfu_target_full_modem here
	 */
	configure_full_modem_update();
#endif
	int ret = fota_download_start(fota_host, fota_path, fota_sec_tag, 0, 0);

	if (ret < 0) {
		LOG_ERR("fota_download_start() failed, return code %d", ret);
		lwm2m_firmware_set_update_state(current_update_instance, STATE_IDLE);
		k_free(fota_host);
		current_update_instance = -1;
		fota_host = NULL;
		fota_path = NULL;
	}
}

static int write_dl_uri(uint16_t obj_inst_id,
			uint16_t res_id, uint16_t res_inst_id,
			uint8_t *data, uint16_t data_len,
			bool last_block, size_t total_size)
{
	int ret;

	LOG_INF("write URI: %s", log_strdup((char *) data));
	ret = fota_download_init(fota_download_callback);
	if (ret != 0) {
		LOG_ERR("fota_download_init() returned %d", ret);
		return -EBUSY;
	}

	if (fota_host) {
		LOG_ERR("FOTA download already ongoing");
		return -EBUSY;
	}

	bool is_tls = strncmp((char *) data, "https://", 8) == 0 ||
		      strncmp((char *) data, "coaps://", 8) == 0;
	if (is_tls) {
		fota_sec_tag = CONFIG_LWM2M_CLIENT_UTILS_DOWNLOADER_SEC_TAG;
	} else {
		fota_sec_tag = -1;
	}

	/* Find the end of protocol marker https:// or coap:// */
	char *s = strstr((char *) data, "://");

	if (!s) {
		LOG_ERR("Host not found");
		return -EINVAL;
	}
	s += strlen("://");

	/* Find the end of host name, which is start of path */
	char  *e = strchr(s, '/');

	if (!e) {
		LOG_ERR("Path not found");
		return -EINVAL;
	}

	/* Path can point to a string, which is kept in LwM2M engine's memory */
	fota_path = e + 1; /* Skip the '/' from path */
	int len = e - (char *) data;

	/* For host, I need to allocate space, as I need to copy the substring */
	fota_host = k_malloc(len + 1);
	if (!fota_host) {
		LOG_ERR("Failed to allocate memory");
		return -ENOMEM;
	}
	strncpy(fota_host, (char *)data, len);
	fota_host[len] = 0;
	current_update_instance = obj_inst_id;

	k_work_submit(&download_work);
	lwm2m_firmware_set_update_state(current_update_instance, STATE_DOWNLOADING);
	return 0;
}

int lwm2m_init_firmware(void)
{
	k_work_init_delayable(&reboot_work, reboot_work_handler);
	k_work_init(&download_work, start_fota_download);
#if defined(CONFIG_DFU_TARGET_FULL_MODEM)
	k_work_init(&full_modem_update_work, apply_fmfu_from_ext_flash);
#endif
	lwm2m_firmware_set_update_cb(firmware_update_cb);

	lwm2m_engine_register_pre_write_callback(LWM2M_PATH(5,
						 LWM2M_CLIENT_UTILS_FIRMWARE_INSTANCE_MODEM, 0),
						 firmware_get_buf);
	lwm2m_engine_register_post_write_callback(LWM2M_PATH(5,
						  LWM2M_CLIENT_UTILS_FIRMWARE_INSTANCE_MODEM, 1),
						  write_dl_uri);

	lwm2m_engine_register_pre_write_callback(LWM2M_PATH(5,
						 LWM2M_CLIENT_UTILS_FIRMWARE_INSTANCE_APP, 0),
						 firmware_get_buf);
	lwm2m_engine_register_post_write_callback(LWM2M_PATH(5,
						  LWM2M_CLIENT_UTILS_FIRMWARE_INSTANCE_APP, 1),
						  write_dl_uri);

	lwm2m_firmware_set_write_cb(firmware_block_received_cb);
	return 0;
}

void lwm2m_verify_modem_fw_update(uint16_t obj_inst_id)
{
	int ret = nrf_modem_lib_get_init_ret();
	struct update_counter counter;

	/* Handle return values relating to modem firmware update */
	switch (ret) {
	case MODEM_DFU_RESULT_OK:
		LOG_INF("MODEM UPDATE OK. Will run new firmware");

		ret = fota_update_counter_read(obj_inst_id, &counter);
		if (ret != 0) {
			LOG_ERR("Failed read the update counter, err: %d", ret);
			break;
		}

		if (counter.update != -1) {
			ret = fota_update_counter_update(obj_inst_id, COUNTER_CURRENT,
							 counter.update);
			if (ret != 0) {
				LOG_ERR("Failed to update the update counter, err: %d",
					ret);
			}
		}

		break;

	case MODEM_DFU_RESULT_UUID_ERROR:
	case MODEM_DFU_RESULT_AUTH_ERROR:
		LOG_ERR("MODEM UPDATE ERROR %d. Will run old firmware", ret);
		break;

	case MODEM_DFU_RESULT_HARDWARE_ERROR:
	case MODEM_DFU_RESULT_INTERNAL_ERROR:
		LOG_ERR("MODEM UPDATE FATAL ERROR %d. Modem failiure", ret);
		break;

	default:
		return;
	}

	LOG_PANIC();
	sys_reboot(SYS_REBOOT_COLD);
}

int lwm2m_init_image(uint16_t obj_inst_id)
{
	int ret = 0;
	struct update_counter counter;
	bool image_ok;
	bool status;

	/* Update boot status and update counter */
	ret = fota_update_counter_read(obj_inst_id, &counter);
	if (ret) {
		LOG_ERR("Failed read update counter");
		return ret;
	}
	LOG_INF("Update Counter: current %d, update %d", counter.current, counter.update);

	image_ok = boot_is_img_confirmed();
	LOG_INF("Image is confirmed as OK - %s", image_ok ? "true" : "false");

	if (!image_ok) {
		ret = boot_write_img_confirmed();
		if (ret) {
			LOG_ERR("Couldn't mark this image as confirmed - return value %d", ret);
			return ret;
		}

		LOG_INF("Current image marked as OK");

		if (counter.update != -1) {
			ret = fota_update_counter_update(obj_inst_id, COUNTER_CURRENT,
							 counter.update);
			if (ret) {
				LOG_ERR("Failed to update the update "
					"counter: %d", ret);
				return ret;
			}

			ret = fota_update_counter_read(obj_inst_id, &counter);
			if (ret) {
				LOG_ERR("Failed to read update counter: %d",
					ret);
				return ret;
			}

			LOG_INF("Update Counter updated");
		}
	}

	char path[9+1]; /* '5/[0-65535]/5' -> max len 9 + terminating NULL */

	ret = snprintf(path, sizeof(path), "5/%" PRIu16 "/5", obj_inst_id);


	/* Check if a firmware update status needs to be reported */
	if (counter.update != -1 && counter.current == counter.update) {
		LOG_INF("Firmware updated successfully");
		lwm2m_engine_set_u8(path, RESULT_SUCCESS);
	} else if (counter.update > counter.current) {
		LOG_INF("Firmware failed to be updated");
		lwm2m_engine_set_u8(path, RESULT_UPDATE_FAILED);
	}

#ifdef CONFIG_DFU_TARGET_MCUBOOT
	/* Set the required buffer for MCUboot targets */
	ret = dfu_target_mcuboot_set_buf(mcuboot_buf, sizeof(mcuboot_buf));
	if (ret) {
		LOG_ERR("Failed to set MCUboot flash buffer %d", ret);
	}
#endif

	return ret;
}

