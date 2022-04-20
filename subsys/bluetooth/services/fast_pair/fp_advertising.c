/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <net/buf.h>
#include <bluetooth/bluetooth.h>

#include <bluetooth/services/fast_pair.h>
#include "fp_registration_data.h"
#include "fp_common.h"

static const uint16_t fast_pair_uuid = BT_FAST_PAIR_SERVICE_UUID;

static const uint8_t flags;
static const uint8_t empty_account_key_list;


static size_t bt_fast_pair_adv_data_size_non_discoverable(size_t account_key_cnt)
{
	size_t res = 0;

	res += sizeof(flags);

	if (account_key_cnt == 0) {
		res += sizeof(empty_account_key_list);
	} else {
		/* Not supported yet. */
		__ASSERT_NO_MSG(false);
	}

	return res;
}

static size_t bt_fast_pair_adv_data_size_discoverable(void)
{
	return FP_MODEL_ID_LEN;
}

size_t bt_fast_pair_adv_data_size(bool fp_discoverable)
{
	size_t account_key_cnt = 0;
	size_t res = 0;

	res += sizeof(fast_pair_uuid);

	if (fp_discoverable) {
		res += bt_fast_pair_adv_data_size_discoverable();
	} else {
		res += bt_fast_pair_adv_data_size_non_discoverable(account_key_cnt);
	}

	return res;
}

static int fp_adv_data_fill_non_discoverable(struct net_buf_simple *buf, size_t account_key_cnt)
{
	net_buf_simple_add_u8(buf, flags);

	if (account_key_cnt == 0) {
		net_buf_simple_add_u8(buf, empty_account_key_list);
	} else {
		/* Not supported yet. */
		__ASSERT_NO_MSG(false);
	}

	return 0;
}

static int fp_adv_data_fill_discoverable(struct net_buf_simple *buf)
{
	return fp_get_model_id(net_buf_simple_add(buf, FP_MODEL_ID_LEN), FP_MODEL_ID_LEN);
}

int bt_fast_pair_adv_data_fill(struct bt_data *bt_adv_data, uint8_t *buf, size_t buf_size,
			       bool fp_discoverable)
{
	struct net_buf_simple nb;
	size_t account_key_cnt = 0;
	size_t adv_data_len = bt_fast_pair_adv_data_size(fp_discoverable);
	int err;

	if (adv_data_len > buf_size) {
		return -EINVAL;
	}

	net_buf_simple_init_with_data(&nb, buf, buf_size);
	net_buf_simple_reset(&nb);

	net_buf_simple_add_le16(&nb, fast_pair_uuid);

	if (fp_discoverable) {
		err = fp_adv_data_fill_discoverable(&nb);
	} else {
		err = fp_adv_data_fill_non_discoverable(&nb, account_key_cnt);
	}

	if (!err) {
		bt_adv_data->type = BT_DATA_SVC_DATA16;
		bt_adv_data->data_len = adv_data_len;
		bt_adv_data->data = buf;
	}

	return err;
}
