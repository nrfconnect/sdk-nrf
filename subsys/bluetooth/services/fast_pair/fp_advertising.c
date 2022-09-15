/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <zephyr/net/buf.h>
#include <zephyr/random/rand32.h>
#include <zephyr/bluetooth/bluetooth.h>

#include <bluetooth/services/fast_pair.h>
#include "fp_battery.h"
#include "fp_common.h"
#include "fp_crypto.h"
#include "fp_registration_data.h"
#include "fp_storage.h"

#define LEN_BITS				4
#define TYPE_BITS				4
#define FIELD_LEN_TYPE_SIZE			sizeof(uint8_t)
#define ENCODE_FIELD_LEN_TYPE(len, type)	(((len) << TYPE_BITS) | (type))

#define BATTERY_LEVEL_BITS			7
#define BATTERY_VALUE_SIZE			sizeof(uint8_t)
#define ENCODE_FIELD_BATTERY_STATUS_LEVEL(charging, level) \
						(((charging) << BATTERY_LEVEL_BITS) | (level))

enum fp_field_type {
	FP_FIELD_TYPE_SHOW_UI_INDICATION	 = 0b0000,
	FP_FIELD_TYPE_SALT			 = 0b0001,
	FP_FIELD_TYPE_HIDE_UI_INDICATION	 = 0b0010,
	FP_FIELD_TYPE_SHOW_BATTERY_UI_INDICATION = 0b0011,
	FP_FIELD_TYPE_HIDE_BATTERY_UI_INDICATION = 0b0100,
};

static const uint16_t fast_pair_uuid = FP_SERVICE_UUID;
static const uint8_t version_and_flags;
static const uint8_t empty_account_key_list;

static size_t bt_fast_pair_adv_data_size_non_discoverable(size_t account_key_cnt,
							  struct bt_fast_pair_adv_info fp_adv_info)
{
	size_t res = 0;

	res += sizeof(version_and_flags);

	if (account_key_cnt == 0) {
		res += sizeof(empty_account_key_list);
	} else {
		uint8_t salt;

		res += FIELD_LEN_TYPE_SIZE;
		res += fp_crypto_account_key_filter_size(account_key_cnt);

		res += FIELD_LEN_TYPE_SIZE;
		res += sizeof(salt);
	}

	if (fp_adv_info.adv_battery_mode != BT_FAST_PAIR_ADV_BATTERY_MODE_NONE) {
		res += FP_CRYPTO_BATTERY_INFO_LEN;
	}

	return res;
}

static size_t bt_fast_pair_adv_data_size_discoverable(void)
{
	return FP_REG_DATA_MODEL_ID_LEN;
}

size_t bt_fast_pair_adv_data_size(struct bt_fast_pair_adv_info fp_adv_info)
{
	int account_key_cnt = fp_storage_account_key_count();
	size_t res = 0;

	if ((fp_adv_info.adv_mode >= BT_FAST_PAIR_ADV_MODE_COUNT) || (fp_adv_info.adv_mode < 0)) {
		return 0;
	}

	if ((fp_adv_info.adv_battery_mode >= BT_FAST_PAIR_ADV_BATTERY_MODE_COUNT) ||
	    (fp_adv_info.adv_battery_mode < 0)) {
		return 0;
	}

	if (account_key_cnt < 0) {
		return 0;
	}

	res += sizeof(fast_pair_uuid);

	if (fp_adv_info.adv_mode == BT_FAST_PAIR_ADV_MODE_DISCOVERABLE) {
		res += bt_fast_pair_adv_data_size_discoverable();
	} else {
		res += bt_fast_pair_adv_data_size_non_discoverable(account_key_cnt, fp_adv_info);
	}

	return res;
}

static void fp_adv_data_fill_battery_info(uint8_t *battery_info,
					  enum fp_field_type battery_data_type)
{
	struct bt_fast_pair_battery_data battery_data = fp_battery_get_battery_data();
	size_t battery_data_size = FP_CRYPTO_BATTERY_INFO_LEN - FIELD_LEN_TYPE_SIZE;
	size_t pos = 0;

	BUILD_ASSERT(sizeof(uint8_t) == FIELD_LEN_TYPE_SIZE);
	__ASSERT_NO_MSG(battery_data_size <= BIT_MASK(LEN_BITS));
	battery_info[pos] = ENCODE_FIELD_LEN_TYPE(battery_data_size, battery_data_type);
	pos++;

	BUILD_ASSERT(sizeof(uint8_t) == BATTERY_VALUE_SIZE);

	__ASSERT_NO_MSG(battery_data.left_bud.level <= BIT_MASK(BATTERY_LEVEL_BITS));
	battery_info[pos] = ENCODE_FIELD_BATTERY_STATUS_LEVEL(battery_data.left_bud.charging,
							      battery_data.left_bud.level);
	pos++;

	__ASSERT_NO_MSG(battery_data.right_bud.level <= BIT_MASK(BATTERY_LEVEL_BITS));
	battery_info[pos] = ENCODE_FIELD_BATTERY_STATUS_LEVEL(battery_data.right_bud.charging,
							      battery_data.right_bud.level);
	pos++;

	__ASSERT_NO_MSG(battery_data.bud_case.level <= BIT_MASK(BATTERY_LEVEL_BITS));
	battery_info[pos] = ENCODE_FIELD_BATTERY_STATUS_LEVEL(battery_data.bud_case.charging,
							      battery_data.bud_case.level);
}

static int fp_adv_data_fill_non_discoverable(struct net_buf_simple *buf, size_t account_key_cnt,
					     enum fp_field_type ak_filter_type,
					     struct bt_fast_pair_adv_info fp_adv_info)
{
	uint8_t battery_info[FP_CRYPTO_BATTERY_INFO_LEN];

	net_buf_simple_add_u8(buf, version_and_flags);

	if (fp_adv_info.adv_battery_mode != BT_FAST_PAIR_ADV_BATTERY_MODE_NONE) {
		enum fp_field_type battery_data_type;

		if (fp_adv_info.adv_battery_mode == BT_FAST_PAIR_ADV_BATTERY_MODE_SHOW_UI_IND) {
			battery_data_type = FP_FIELD_TYPE_SHOW_BATTERY_UI_INDICATION;
		} else {
			battery_data_type = FP_FIELD_TYPE_HIDE_BATTERY_UI_INDICATION;
		}

		fp_adv_data_fill_battery_info(battery_info, battery_data_type);
	}

	if (account_key_cnt == 0) {
		net_buf_simple_add_u8(buf, empty_account_key_list);
	} else {
		struct fp_account_key ak[CONFIG_BT_FAST_PAIR_STORAGE_ACCOUNT_KEY_MAX];
		size_t ak_filter_size = fp_crypto_account_key_filter_size(account_key_cnt);
		size_t account_key_get_cnt = account_key_cnt;
		uint8_t salt;
		int err;

		err = sys_csrand_get(&salt, sizeof(salt));
		if (err) {
			return err;
		}

		err = fp_storage_account_keys_get(ak, &account_key_get_cnt);
		if (err) {
			return err;
		}

		if (account_key_get_cnt != account_key_cnt) {
			return -ENODATA;
		}

		BUILD_ASSERT(sizeof(uint8_t) == FIELD_LEN_TYPE_SIZE);

		__ASSERT_NO_MSG(ak_filter_size <= BIT_MASK(LEN_BITS));
		net_buf_simple_add_u8(buf, ENCODE_FIELD_LEN_TYPE(ak_filter_size, ak_filter_type));

		if (fp_adv_info.adv_battery_mode != BT_FAST_PAIR_ADV_BATTERY_MODE_NONE) {
			err = fp_crypto_account_key_filter(net_buf_simple_add(buf, ak_filter_size),
							   ak, account_key_cnt, salt, battery_info);
		} else {
			err = fp_crypto_account_key_filter(net_buf_simple_add(buf, ak_filter_size),
							   ak, account_key_cnt, salt, NULL);
		}
		if (err) {
			return err;
		}

		net_buf_simple_add_u8(buf, ENCODE_FIELD_LEN_TYPE(sizeof(salt), FP_FIELD_TYPE_SALT));
		net_buf_simple_add_u8(buf, salt);
	}

	if (fp_adv_info.adv_battery_mode != BT_FAST_PAIR_ADV_BATTERY_MODE_NONE) {
		net_buf_simple_add_mem(buf, battery_info, sizeof(battery_info));
	}

	return 0;
}

static int fp_adv_data_fill_discoverable(struct net_buf_simple *buf)
{
	return fp_reg_data_get_model_id(net_buf_simple_add(buf, FP_REG_DATA_MODEL_ID_LEN),
					FP_REG_DATA_MODEL_ID_LEN);
}

int bt_fast_pair_adv_data_fill(struct bt_data *bt_adv_data, uint8_t *buf, size_t buf_size,
			       struct bt_fast_pair_adv_info fp_adv_info)
{
	struct net_buf_simple nb;
	int account_key_cnt = fp_storage_account_key_count();
	size_t adv_data_len = bt_fast_pair_adv_data_size(fp_adv_info);
	enum fp_field_type ak_filter_type;
	int err;

	if ((fp_adv_info.adv_mode >= BT_FAST_PAIR_ADV_MODE_COUNT) || (fp_adv_info.adv_mode < 0)) {
		return -EINVAL;
	}

	if ((fp_adv_info.adv_battery_mode >= BT_FAST_PAIR_ADV_BATTERY_MODE_COUNT) ||
	    (fp_adv_info.adv_battery_mode < 0)) {
		return -EINVAL;
	}

	if ((adv_data_len == 0) || (account_key_cnt < 0)) {
		return -ENODATA;
	}

	if (adv_data_len > buf_size) {
		return -EINVAL;
	}

	net_buf_simple_init_with_data(&nb, buf, buf_size);
	net_buf_simple_reset(&nb);

	net_buf_simple_add_le16(&nb, fast_pair_uuid);

	if (fp_adv_info.adv_mode == BT_FAST_PAIR_ADV_MODE_DISCOVERABLE) {
		err = fp_adv_data_fill_discoverable(&nb);
	} else {
		if (fp_adv_info.adv_mode == BT_FAST_PAIR_ADV_MODE_NOT_DISCOVERABLE_SHOW_UI_IND) {
			ak_filter_type = FP_FIELD_TYPE_SHOW_UI_INDICATION;
		} else {
			ak_filter_type = FP_FIELD_TYPE_HIDE_UI_INDICATION;
		}
		err = fp_adv_data_fill_non_discoverable(&nb, account_key_cnt, ak_filter_type,
							fp_adv_info);
	}

	if (!err) {
		bt_adv_data->type = BT_DATA_SVC_DATA16;
		bt_adv_data->data_len = adv_data_len;
		bt_adv_data->data = buf;
	}

	return err;
}
