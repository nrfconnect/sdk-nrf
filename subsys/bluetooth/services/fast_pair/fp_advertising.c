/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <zephyr/net_buf.h>
#include <zephyr/random/random.h>
#include <zephyr/bluetooth/bluetooth.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(fast_pair, CONFIG_BT_FAST_PAIR_LOG_LEVEL);

#include <bluetooth/services/fast_pair/fast_pair.h>
#include <bluetooth/services/fast_pair/uuid.h>
#include "fp_battery.h"
#include "fp_common.h"
#include "fp_crypto.h"
#include "fp_registration_data.h"
#include "fp_storage_ak.h"

#define LEN_BITS				4
#define TYPE_BITS				4
#define FIELD_LEN_TYPE_SIZE			sizeof(uint8_t)
#define ENCODE_FIELD_LEN_TYPE(len, type)	(((len) << TYPE_BITS) | (type))

#define BATTERY_LEVEL_BITS			7
#define FIELD_BATTERY_STATUS_LEVEL_SIZE		sizeof(uint8_t)
#define ENCODE_FIELD_BATTERY_STATUS_LEVEL(charging, level) \
	(((charging ? 1 : 0) << BATTERY_LEVEL_BITS) | (level))

enum fp_field_type {
	FP_FIELD_TYPE_SHOW_PAIRING_UI_INDICATION = 0b0000,
	FP_FIELD_TYPE_SALT			 = 0b0001,
	FP_FIELD_TYPE_HIDE_PAIRING_UI_INDICATION = 0b0010,
	FP_FIELD_TYPE_SHOW_BATTERY_UI_INDICATION = 0b0011,
	FP_FIELD_TYPE_HIDE_BATTERY_UI_INDICATION = 0b0100,
};

static const uint16_t fast_pair_uuid = BT_FAST_PAIR_UUID_FPS_VAL;
static const uint8_t version_and_flags;
static const uint8_t empty_account_key_list;

static int check_adv_config(struct bt_fast_pair_adv_config fp_adv_config)
{
	if ((fp_adv_config.mode >= BT_FAST_PAIR_ADV_MODE_COUNT) || (fp_adv_config.mode < 0)) {
		return -EINVAL;
	}

	if (fp_adv_config.mode == BT_FAST_PAIR_ADV_MODE_NOT_DISC) {
		if ((fp_adv_config.not_disc.type >= BT_FAST_PAIR_NOT_DISC_ADV_TYPE_COUNT) ||
		    (fp_adv_config.not_disc.type < 0)) {
			return -EINVAL;
		}

		if (!IS_ENABLED(CONFIG_BT_FAST_PAIR_SUBSEQUENT_PAIRING) &&
		    fp_adv_config.not_disc.type == BT_FAST_PAIR_NOT_DISC_ADV_TYPE_SHOW_UI_IND) {
			LOG_ERR("Cannot use not discoverable advertising with UI indications "
				"without support for the subsequent pairing feature. Enable the "
				"CONFIG_BT_FAST_PAIR_SUBSEQUENT_PAIRING Kconfig to use this mode");

			return -EINVAL;
		}

		if (!IS_ENABLED(CONFIG_BT_FAST_PAIR_BN) &&
		    (fp_adv_config.not_disc.battery_mode != BT_FAST_PAIR_ADV_BATTERY_MODE_NONE)) {
			LOG_ERR("Cannot use not discoverable advertising with battery data "
				"without support for the Battery Notification extension. Enable "
				"the CONFIG_BT_FAST_PAIR_BN Kconfig to use this mode");

			return -EINVAL;
		}

		if ((fp_adv_config.not_disc.battery_mode >= BT_FAST_PAIR_ADV_BATTERY_MODE_COUNT) ||
		    (fp_adv_config.not_disc.battery_mode < 0)) {
			return -EINVAL;
		}
	}

	return 0;
}

static size_t bt_fast_pair_adv_data_size_non_discoverable(size_t account_key_cnt,
						enum bt_fast_pair_adv_battery_mode adv_battery_mode)
{
	size_t res = 0;

	res += sizeof(version_and_flags);

	if (account_key_cnt == 0) {
		res += sizeof(empty_account_key_list);
	} else {
		uint16_t salt;

		res += FIELD_LEN_TYPE_SIZE;
		res += fp_crypto_account_key_filter_size(account_key_cnt);

		res += FIELD_LEN_TYPE_SIZE;
		res += sizeof(salt);
	}

	if ((adv_battery_mode != BT_FAST_PAIR_ADV_BATTERY_MODE_NONE) && (account_key_cnt != 0)) {
		res += FP_CRYPTO_BATTERY_INFO_LEN;
	}

	return res;
}

static size_t bt_fast_pair_adv_data_size_discoverable(void)
{
	return FP_REG_DATA_MODEL_ID_LEN;
}

size_t bt_fast_pair_adv_data_size(struct bt_fast_pair_adv_config fp_adv_config)
{
	/* It is assumed that this function executes in the cooperative thread context. */
	__ASSERT_NO_MSG(!k_is_preempt_thread());
	__ASSERT_NO_MSG(!k_is_in_isr());

	if (!bt_fast_pair_is_ready()) {
		return 0;
	}

	int account_key_cnt = fp_storage_ak_count();
	size_t res = 0;
	int err;

	err = check_adv_config(fp_adv_config);
	if (err) {
		return 0;
	}

	if (account_key_cnt < 0) {
		return 0;
	}

	res += sizeof(fast_pair_uuid);

	if (fp_adv_config.mode == BT_FAST_PAIR_ADV_MODE_DISC) {
		res += bt_fast_pair_adv_data_size_discoverable();
	} else {
		res += bt_fast_pair_adv_data_size_non_discoverable(account_key_cnt,
							fp_adv_config.not_disc.battery_mode);
	}

	return res;
}

static void fp_adv_data_fill_battery_info(uint8_t *battery_info,
					  enum fp_field_type battery_data_type)
{
	struct net_buf_simple nb;
	struct bt_fast_pair_battery_data battery_data;
	static const size_t battery_data_size = FP_CRYPTO_BATTERY_INFO_LEN - FIELD_LEN_TYPE_SIZE;

	if (!IS_ENABLED(CONFIG_BT_FAST_PAIR_BN)) {
		__ASSERT_NO_MSG(false);
		return;
	}

	net_buf_simple_init_with_data(&nb, battery_info, FP_CRYPTO_BATTERY_INFO_LEN);
	net_buf_simple_reset(&nb);

	BUILD_ASSERT(sizeof(uint8_t) == FIELD_LEN_TYPE_SIZE);
	BUILD_ASSERT(battery_data_size <= BIT_MASK(LEN_BITS));
	BUILD_ASSERT(battery_data_size == BT_FAST_PAIR_BATTERY_COMP_COUNT);
	net_buf_simple_add_u8(&nb, ENCODE_FIELD_LEN_TYPE(battery_data_size, battery_data_type));

	BUILD_ASSERT(sizeof(uint8_t) == FIELD_BATTERY_STATUS_LEVEL_SIZE);

	battery_data = fp_battery_get_battery_data();
	for (size_t i = 0; i < battery_data_size; i++) {
		__ASSERT_NO_MSG(battery_data.batteries[i].level <=
				BIT_MASK(BATTERY_LEVEL_BITS));
		net_buf_simple_add_u8(&nb, ENCODE_FIELD_BATTERY_STATUS_LEVEL(
							battery_data.batteries[i].charging,
							battery_data.batteries[i].level));
	}
}

static int fp_adv_data_fill_non_discoverable(struct net_buf_simple *buf, size_t account_key_cnt,
					     enum fp_field_type ak_filter_type,
					     enum bt_fast_pair_adv_battery_mode adv_battery_mode)
{
	uint8_t battery_info[FP_CRYPTO_BATTERY_INFO_LEN];
	bool add_battery_info = ((adv_battery_mode != BT_FAST_PAIR_ADV_BATTERY_MODE_NONE) &&
				 (account_key_cnt != 0));

	net_buf_simple_add_u8(buf, version_and_flags);

	if (add_battery_info) {
		enum fp_field_type battery_data_type;

		if (adv_battery_mode == BT_FAST_PAIR_ADV_BATTERY_MODE_SHOW_UI_IND) {
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
		uint16_t salt;
		int err;

		err = sys_csrand_get(&salt, sizeof(salt));
		if (err) {
			return err;
		}

		err = fp_storage_ak_get(ak, &account_key_get_cnt);
		if (err) {
			return err;
		}

		if (account_key_get_cnt != account_key_cnt) {
			return -ENODATA;
		}

		BUILD_ASSERT(sizeof(uint8_t) == FIELD_LEN_TYPE_SIZE);

		__ASSERT_NO_MSG(ak_filter_size <= BIT_MASK(LEN_BITS));
		net_buf_simple_add_u8(buf, ENCODE_FIELD_LEN_TYPE(ak_filter_size, ak_filter_type));

		err = fp_crypto_account_key_filter(net_buf_simple_add(buf, ak_filter_size), ak,
						   account_key_cnt, salt,
						   add_battery_info ? battery_info : NULL);
		if (err) {
			return err;
		}

		net_buf_simple_add_u8(buf, ENCODE_FIELD_LEN_TYPE(sizeof(salt), FP_FIELD_TYPE_SALT));
		net_buf_simple_add_be16(buf, salt);
	}

	if (add_battery_info) {
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
			       struct bt_fast_pair_adv_config fp_adv_config)
{
	/* It is assumed that this function executes in the cooperative thread context. */
	__ASSERT_NO_MSG(!k_is_preempt_thread());
	__ASSERT_NO_MSG(!k_is_in_isr());

	if (!bt_fast_pair_is_ready()) {
		LOG_ERR("Fast Pair not enabled");
		return -EACCES;
	}

	struct net_buf_simple nb;
	int account_key_cnt = fp_storage_ak_count();
	size_t adv_data_len = bt_fast_pair_adv_data_size(fp_adv_config);
	enum fp_field_type ak_filter_type;
	int err;

	err = check_adv_config(fp_adv_config);
	if (err) {
		return err;
	}

	if ((adv_data_len == 0) || (account_key_cnt < 0)) {
		return -ENODATA;
	}

	if (adv_data_len > buf_size) {
		return -EINVAL;
	}

	if ((fp_adv_config.mode == BT_FAST_PAIR_ADV_MODE_NOT_DISC) &&
	    (fp_adv_config.not_disc.battery_mode != BT_FAST_PAIR_ADV_BATTERY_MODE_NONE) &&
	    (account_key_cnt == 0)) {
		LOG_INF("Battery data can't be included when Account Key List is empty");
	}

	net_buf_simple_init_with_data(&nb, buf, buf_size);
	net_buf_simple_reset(&nb);

	net_buf_simple_add_le16(&nb, fast_pair_uuid);

	if (fp_adv_config.mode == BT_FAST_PAIR_ADV_MODE_DISC) {
		err = fp_adv_data_fill_discoverable(&nb);
	} else {
		if (fp_adv_config.not_disc.type == BT_FAST_PAIR_NOT_DISC_ADV_TYPE_SHOW_UI_IND) {
			ak_filter_type = FP_FIELD_TYPE_SHOW_PAIRING_UI_INDICATION;
		} else {
			ak_filter_type = FP_FIELD_TYPE_HIDE_PAIRING_UI_INDICATION;
		}
		err = fp_adv_data_fill_non_discoverable(&nb, account_key_cnt, ak_filter_type,
							fp_adv_config.not_disc.battery_mode);
	}

	if (!err) {
		bt_adv_data->type = BT_DATA_SVC_DATA16;
		bt_adv_data->data_len = adv_data_len;
		bt_adv_data->data = buf;
	}

	return err;
}
