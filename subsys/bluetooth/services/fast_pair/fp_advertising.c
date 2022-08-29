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
#include "fp_common.h"
#include "fp_crypto.h"
#include "fp_registration_data.h"
#include "fp_storage.h"

#define LEN_BITS				4
#define TYPE_BITS				4
#define FIELD_LEN_TYPE_SIZE			sizeof(uint8_t)
#define ENCODE_FIELD_LEN_TYPE(len, type)	(((len) << TYPE_BITS) | (type))

enum fp_field_type {
	FP_FIELD_TYPE_SHOW_UI_INDICATION = 0b0000,
	FP_FIELD_TYPE_SALT		 = 0b0001,
	FP_FIELD_TYPE_HIDE_UI_INDICATION = 0b0010,
};

static const uint16_t fast_pair_uuid = FP_SERVICE_UUID;
static const uint8_t version_and_flags;
static const uint8_t empty_account_key_list;

static size_t bt_fast_pair_adv_data_size_non_discoverable(size_t account_key_cnt)
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

	return res;
}

static size_t bt_fast_pair_adv_data_size_discoverable(void)
{
	return FP_REG_DATA_MODEL_ID_LEN;
}

size_t bt_fast_pair_adv_data_size(enum bt_fast_pair_adv_mode fp_adv_mode)
{
	int account_key_cnt = fp_storage_account_key_count();
	size_t res = 0;

	if ((fp_adv_mode >= BT_FAST_PAIR_ADV_MODE_COUNT) || (fp_adv_mode < 0)) {
		return 0;
	}

	if (account_key_cnt < 0) {
		return 0;
	}

	res += sizeof(fast_pair_uuid);

	if (fp_adv_mode == BT_FAST_PAIR_ADV_MODE_DISCOVERABLE) {
		res += bt_fast_pair_adv_data_size_discoverable();
	} else {
		res += bt_fast_pair_adv_data_size_non_discoverable(account_key_cnt);
	}

	return res;
}

static int fp_adv_data_fill_non_discoverable(struct net_buf_simple *buf, size_t account_key_cnt,
					     enum fp_field_type ak_filter_type)
{
	net_buf_simple_add_u8(buf, version_and_flags);

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

		err = fp_crypto_account_key_filter(net_buf_simple_add(buf, ak_filter_size), ak,
						   account_key_cnt, salt, NULL);
		if (err) {
			return err;
		}

		net_buf_simple_add_u8(buf, ENCODE_FIELD_LEN_TYPE(sizeof(salt), FP_FIELD_TYPE_SALT));
		net_buf_simple_add_u8(buf, salt);
	}

	return 0;
}

static int fp_adv_data_fill_discoverable(struct net_buf_simple *buf)
{
	return fp_reg_data_get_model_id(net_buf_simple_add(buf, FP_REG_DATA_MODEL_ID_LEN),
					FP_REG_DATA_MODEL_ID_LEN);
}

int bt_fast_pair_adv_data_fill(struct bt_data *bt_adv_data, uint8_t *buf, size_t buf_size,
			       enum bt_fast_pair_adv_mode fp_adv_mode)
{
	struct net_buf_simple nb;
	int account_key_cnt = fp_storage_account_key_count();
	size_t adv_data_len = bt_fast_pair_adv_data_size(fp_adv_mode);
	enum fp_field_type ak_filter_type;
	int err;

	if ((adv_data_len == 0) || (account_key_cnt < 0)) {
		return -ENODATA;
	}

	if (adv_data_len > buf_size) {
		return -EINVAL;
	}

	if ((fp_adv_mode >= BT_FAST_PAIR_ADV_MODE_COUNT) || (fp_adv_mode < 0)) {
		return -EINVAL;
	}

	net_buf_simple_init_with_data(&nb, buf, buf_size);
	net_buf_simple_reset(&nb);

	net_buf_simple_add_le16(&nb, fast_pair_uuid);

	if (fp_adv_mode == BT_FAST_PAIR_ADV_MODE_DISCOVERABLE) {
		err = fp_adv_data_fill_discoverable(&nb);
	} else {
		if (fp_adv_mode == BT_FAST_PAIR_ADV_MODE_NOT_DISCOVERABLE_SHOW_UI_IND) {
			ak_filter_type = FP_FIELD_TYPE_SHOW_UI_INDICATION;
		} else {
			ak_filter_type = FP_FIELD_TYPE_HIDE_UI_INDICATION;
		}
		err = fp_adv_data_fill_non_discoverable(&nb, account_key_cnt, ak_filter_type);
	}

	if (!err) {
		bt_adv_data->type = BT_DATA_SVC_DATA16;
		bt_adv_data->data_len = adv_data_len;
		bt_adv_data->data = buf;
	}

	return err;
}
