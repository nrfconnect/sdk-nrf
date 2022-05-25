/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/storage/flash_map.h>
#include <pm_config.h>

#include "fp_registration_data.h"

#define FP_PARTITION_ID		PM_BT_FAST_PAIR_ID
#define FP_PARTITION_SIZE	PM_BT_FAST_PAIR_SIZE

static const uint8_t fp_magic[] = {0xFA, 0x57, 0xFA, 0x57};

#define FP_MAGIC_SIZE		(sizeof(fp_magic))

#define FP_DATA_ALIGN		 4
#define FP_DATA_START		 0
#define FP_OFFSET(_prev_item_offset, _prev_item_len) \
	(_prev_item_offset + ROUND_UP(_prev_item_len, FP_DATA_ALIGN))

#define FP_MAGIC_HEADER_OFF	 FP_DATA_START
#define FP_MODEL_ID_OFF		 FP_OFFSET(FP_MAGIC_HEADER_OFF, FP_MAGIC_SIZE)
#define FP_ANTI_SPOOFING_KEY_OFF FP_OFFSET(FP_MODEL_ID_OFF, FP_REG_DATA_MODEL_ID_LEN)
#define FP_MAGIC_TRAILER_OFF	 FP_OFFSET(FP_ANTI_SPOOFING_KEY_OFF, \
					   FP_REG_DATA_ANTI_SPOOFING_PRIV_KEY_LEN)

BUILD_ASSERT(FP_OFFSET(FP_MAGIC_TRAILER_OFF, FP_MAGIC_SIZE) <= FP_PARTITION_SIZE,
	     "Fast Pair registration data partition is too small");

static bool data_valid;


int fp_reg_data_get_model_id(uint8_t *buf, size_t size)
{
	int err;
	const struct flash_area *fa;

	if (!data_valid) {
		return -ENODATA;
	}

	if (size < FP_REG_DATA_MODEL_ID_LEN) {
		return -EINVAL;
	}

	err = flash_area_open(FP_PARTITION_ID, &fa);
	if (!err) {
		err = flash_area_read(fa, FP_MODEL_ID_OFF, buf, FP_REG_DATA_MODEL_ID_LEN);
		flash_area_close(fa);
	}

	return err;
}

int fp_get_anti_spoofing_priv_key(uint8_t *buf, size_t size)
{
	int err;
	const struct flash_area *fa;

	if (!data_valid) {
		return -ENODATA;
	}

	if (size < FP_REG_DATA_ANTI_SPOOFING_PRIV_KEY_LEN) {
		return -EINVAL;
	}

	err = flash_area_open(FP_PARTITION_ID, &fa);
	if (!err) {
		err = flash_area_read(fa, FP_ANTI_SPOOFING_KEY_OFF, buf,
				      FP_REG_DATA_ANTI_SPOOFING_PRIV_KEY_LEN);
		flash_area_close(fa);
	}

	return err;
}

static int validate_fp_magic(const struct flash_area *fa, off_t off)
{
	int err;
	uint8_t read_buf[FP_MAGIC_SIZE];

	err = flash_area_read(fa, off, read_buf, FP_MAGIC_SIZE);
	if (err) {
		return err;
	}

	if (memcmp(read_buf, fp_magic, FP_MAGIC_SIZE)) {
		err = -EINVAL;
	}

	return err;
}

static int fp_registration_data_validate(const struct device *unused)
{
	ARG_UNUSED(unused);

	int err;
	const struct flash_area *fa = NULL;

	err = flash_area_open(FP_PARTITION_ID, &fa);
	if (!err) {
		err = validate_fp_magic(fa, FP_MAGIC_HEADER_OFF);
	}

	if (!err) {
		err = validate_fp_magic(fa, FP_MAGIC_TRAILER_OFF);
	}

	if (fa) {
		flash_area_close(fa);
	}

	__ASSERT(!err, "Invalid content of the Fast Pair partition");

	if (!err) {
		data_valid = true;
	}

	return err;
}

SYS_INIT(fp_registration_data_validate, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
