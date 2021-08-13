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

#include <net/lwm2m_client_utils.h>
#include <net/lwm2m_client_utils_fota.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(lwm2m_firmware, CONFIG_LWM2M_CLIENT_UTILS_LOG_LEVEL);

#define BYTE_PROGRESS_STEP (1024 * 10)
#define REBOOT_DELAY       K_SECONDS(1)

static uint8_t firmware_buf[CONFIG_LWM2M_COAP_BLOCK_SIZE];

#ifdef CONFIG_DFU_TARGET_MCUBOOT
static uint8_t mcuboot_buf[CONFIG_APP_MCUBOOT_FLASH_BUF_SZ] __aligned(4);
#endif

#define MAX_INSTANCE_COUNT CONFIG_LWM2M_FIRMWARE_INSTANCE_COUNT

static int image_type;

static struct k_work_delayable reboot_work;

void client_acknowledge(void);

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

	ret = dfu_target_done(true);
	if (ret != 0) {
		switch (image_type) {
		case DFU_TARGET_IMAGE_TYPE_MODEM_DELTA:
		case DFU_TARGET_IMAGE_TYPE_FULL_MODEM:
			LOG_ERR("Failed to upgrade modem firmware.");
		break;
		case DFU_TARGET_IMAGE_TYPE_MCUBOOT:
			LOG_ERR("Failed to upgrade application firmware.");
		break;
		}

		goto cleanup;
	}

	LOG_INF("Rebooting device");

	k_work_schedule(&reboot_work, REBOOT_DELAY);

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

		switch (image_type) {
		case DFU_TARGET_IMAGE_TYPE_MODEM_DELTA:
		case DFU_TARGET_IMAGE_TYPE_FULL_MODEM:
			if (obj_inst_id != CONFIG_LWM2M_CLIENT_UTILS_FIRMWARE_INSTANCE_MODEM) {
				LOG_WRN("Unsupported DFU image type");
				ret = -ENOMSG;
				goto cleanup;
			}
		break;
		case DFU_TARGET_IMAGE_TYPE_MCUBOOT:
			if (obj_inst_id != CONFIG_LWM2M_CLIENT_UTILS_FIRMWARE_INSTANCE_APP) {
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


		ret = dfu_target_init(image_type, total_size, dfu_target_cb);
		if (ret < 0) {
			LOG_ERR("Failed to init DFU target, err: %d", ret);
			goto cleanup;
		}

		LOG_INF("%s firmware download started.",
			image_type == DFU_TARGET_IMAGE_TYPE_MODEM_DELTA ?
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

	if (!last_block) {
		/* Keep going */
		return 0;
	} else {
		LOG_INF("Firmware downloaded, %d bytes in total",
			bytes_downloaded);
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

int lwm2m_init_firmware(void)
{
	int ret;
	char path[6 + sizeof(int)]; /* '/5/%d/0' where %d denotes the obj instance id, int */

	k_work_init_delayable(&reboot_work, reboot_work_handler);

	lwm2m_firmware_set_update_cb(firmware_update_cb);

	/* setup data buffer for block-wise transfer */
	for (int i = 0; i < MAX_INSTANCE_COUNT; i++) {
		ret = snprintf(path, sizeof(path), "/5/%d/0", i);

		if (ret < 0 || ret > sizeof(path)) {
			__ASSERT(false, "invalid lwm2m fw update path, err code %d", ret);
			return -1;
		}

		lwm2m_engine_register_pre_write_callback(path, firmware_get_buf);
	}

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

