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

#include "fp_crypto.h"
#include "fp_registration_data.h"

#define FP_PARTITION_ID		PM_BT_FAST_PAIR_ID
#define FP_PARTITION_SIZE	PM_BT_FAST_PAIR_SIZE

static const uint8_t fp_magic[] = {0xFA, 0x57, 0xFA, 0x57};

#define FP_MAGIC_SIZE		(sizeof(fp_magic))

#define FP_DATA_ALIGN		 4
#define FP_DATA_START		 0
#define FP_OFFSET(_prev_item_offset, _prev_item_len) \
	((_prev_item_offset) + ROUND_UP(_prev_item_len, FP_DATA_ALIGN))

#define FP_MAGIC_OFF		 FP_DATA_START
#define FP_MODEL_ID_OFF		 FP_OFFSET(FP_MAGIC_OFF, FP_MAGIC_SIZE)
#define FP_ANTI_SPOOFING_KEY_OFF FP_OFFSET(FP_MODEL_ID_OFF, FP_REG_DATA_MODEL_ID_LEN)
#define FP_HASH_OFF		 FP_OFFSET(FP_ANTI_SPOOFING_KEY_OFF, \
					   FP_REG_DATA_ANTI_SPOOFING_PRIV_KEY_LEN)
#define FP_END_OFF		 FP_OFFSET(FP_HASH_OFF, FP_CRYPTO_SHA256_HASH_LEN)

#define FP_DATA_SIZE		 (FP_END_OFF - FP_DATA_START)
#define FP_OFFSET_TO_DATA_IDX(_offset) ((_offset) - FP_DATA_START)

BUILD_ASSERT(FP_END_OFF <= FP_PARTITION_SIZE, "Fast Pair registration data partition is too small");

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

static int validate_fp_magic(const uint8_t *magic_data)
{
	if (memcmp(magic_data, fp_magic, FP_MAGIC_SIZE)) {
		return -EINVAL;
	}

	return 0;
}

static int validate_fp_hash(const uint8_t *prov_data, const uint8_t *hash)
{
	int err;
	uint8_t hash_calc[FP_CRYPTO_SHA256_HASH_LEN];

	err = fp_crypto_sha256(hash_calc, prov_data, FP_DATA_SIZE - FP_CRYPTO_SHA256_HASH_LEN);
	if (err) {
		return err;
	}

	if (memcmp(hash, hash_calc, FP_CRYPTO_SHA256_HASH_LEN)) {
		err = -EINVAL;
	}

	return err;
}

static int fp_registration_data_validate(void)
{

	int err;
	uint8_t data[FP_DATA_SIZE];
	const struct flash_area *fa = NULL;

	err = flash_area_open(FP_PARTITION_ID, &fa);
	if (!err) {
		err = flash_area_read(fa, FP_DATA_START, data, FP_DATA_SIZE);
	}

	if (!err) {
		err = validate_fp_magic(&data[FP_OFFSET_TO_DATA_IDX(FP_MAGIC_OFF)]);
	}

	if (!err) {
		err = validate_fp_hash(data, &data[FP_OFFSET_TO_DATA_IDX(FP_HASH_OFF)]);
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
