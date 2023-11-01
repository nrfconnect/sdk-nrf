/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <stdio.h>
#include <zephyr/drivers/flash.h>
#include <nrf_modem_delta_dfu.h>
#include <nrf_errno.h>
#include <zephyr/logging/log.h>
#include <dfu/dfu_target.h>

LOG_MODULE_REGISTER(dfu_target_modem_delta, CONFIG_DFU_TARGET_LOG_LEVEL);

#define MODEM_MAGIC 0x7544656d

struct modem_delta_header {
	uint16_t pad1;
	uint16_t pad2;
	uint32_t magic;
};

static dfu_target_callback_t callback;

#define SLEEP_TIME 1
static int delete_banked_modem_delta_fw(void)
{
	int err;
	int offset;
	int timeout = CONFIG_DFU_TARGET_MODEM_TIMEOUT;

	LOG_INF("Deleting firmware image, this can take several minutes");
	err = nrf_modem_delta_dfu_erase();
	if (err != 0 && err != NRF_MODEM_DELTA_DFU_ERASE_PENDING) {
		LOG_ERR("Failed to delete backup, error %d", err);
		return -EFAULT;
	}
	callback(DFU_TARGET_EVT_ERASE_PENDING);

	while (true) {
		err = nrf_modem_delta_dfu_offset(&offset);
		if (err != 0) {
			if (timeout < 0) {
				callback(DFU_TARGET_EVT_TIMEOUT);
				timeout = CONFIG_DFU_TARGET_MODEM_TIMEOUT;
			}
			if (err != NRF_MODEM_DELTA_DFU_ERASE_PENDING &&
			    err != NRF_MODEM_DELTA_DFU_INVALID_DATA) {
				LOG_ERR("Error during erase, error %d", err);
				break;
			}
			k_sleep(K_SECONDS(SLEEP_TIME));
			timeout -= SLEEP_TIME;
		} else {
			callback(DFU_TARGET_EVT_ERASE_DONE);
			LOG_INF("Modem FW delete complete");
			break;
		}
	}

	return err;
}

bool dfu_target_modem_delta_identify(const void *const buf)
{
	return ((const struct modem_delta_header *)buf)->magic == MODEM_MAGIC;
}

int dfu_target_modem_delta_init(size_t file_size, int img_num, dfu_target_callback_t cb)
{
	int err;
	int offset;
	size_t scratch_space;
	struct nrf_modem_delta_dfu_uuid version;
	char version_string[NRF_MODEM_DELTA_DFU_UUID_LEN+1];

	callback = cb;

	/* Retrieve and print modem firmware UUID */
	err = nrf_modem_delta_dfu_uuid(&version);
	if (err != 0) {
		LOG_ERR("Firmware version request failed, error %d", err);
		return -EFAULT;
	}

	snprintf(version_string, sizeof(version_string), "%.*s",
		 sizeof(version.data), version.data);

	LOG_INF("Modem firmware version: %s", version_string);

	/* Check if scratch area is big enough for downloaded image */
	err = nrf_modem_delta_dfu_area(&scratch_space);
	if (err != 0) {
		LOG_ERR("Failed to retrieve size of modem DFU area, error %d", err);
		return -EFAULT;
	}

	if (file_size > scratch_space) {
		LOG_ERR("Requested file too big to fit in flash %d > %d",
			file_size, scratch_space);
		return -EFBIG;
	}

	/* Check offset and erase firmware if necessary */
	err = nrf_modem_delta_dfu_offset(&offset);
	if (err != 0 && err != NRF_MODEM_DELTA_DFU_ERASE_PENDING) {
		LOG_ERR("Failed to retrieve offset in scratch area, error %d", err);
		return -EFAULT;
	}

	if (offset == NRF_MODEM_DELTA_DFU_OFFSET_DIRTY ||
	    err == NRF_MODEM_DELTA_DFU_ERASE_PENDING) {
		err = delete_banked_modem_delta_fw();
		return err;
	}

	return 0;
}

int dfu_target_modem_delta_offset_get(size_t *out)
{
	int err;

	err = nrf_modem_delta_dfu_offset(out);
	if (err != 0) {
		LOG_ERR("Failed to retrieve offset in scratch area, error %d", err);
		return -EFAULT;
	}

	return 0;
}

int dfu_target_modem_delta_write(const void *const buf, size_t len)
{
	int err;

	err = nrf_modem_delta_dfu_write_init();
	if (err != 0 && err != -NRF_EALREADY) {
		LOG_ERR("Failed to ready modem for firmware update receival, error %d", err);
		return -EFAULT;
	}

	err = nrf_modem_delta_dfu_write(buf, len);
	if (err < 0) {
		LOG_ERR("Write failed, modem library error %d", err);
		return -EFAULT;
	} else if (err > 0) {
		LOG_ERR("Write failed, modem error %d", err);
		switch (err) {
		case NRF_MODEM_DELTA_DFU_INVALID_UUID:
			return -EINVAL;
		case NRF_MODEM_DELTA_DFU_INVALID_FILE_OFFSET:
		case NRF_MODEM_DELTA_DFU_AREA_NOT_BLANK:
			delete_banked_modem_delta_fw();
			err = dfu_target_modem_delta_write(buf, len);
			if (err != 0) {
				return -EINVAL;
			} else {
				return 0;
			}
		default:
			return -EFAULT;
		}
	}

	return 0;
}

int dfu_target_modem_delta_done(bool successful)
{
	int err;

	ARG_UNUSED(successful);

	err = nrf_modem_delta_dfu_write_done();
	if (err != 0) {
		LOG_ERR("Failed to stop MFU and release resources, error %d", err);
		return -EFAULT;
	}

	return 0;
}

int dfu_target_modem_delta_schedule_update(int img_num)
{
	int err;

	ARG_UNUSED(img_num);

	err = nrf_modem_delta_dfu_update();

	if (err != 0) {
		LOG_ERR("Modem firmware upgrade scheduling failed, error %d", err);
		return -EFAULT;
	}
	LOG_INF("Scheduling modem firmware upgrade at next boot");

	return err;
}

int dfu_target_modem_delta_reset(void)
{
	return nrf_modem_delta_dfu_erase();
}
