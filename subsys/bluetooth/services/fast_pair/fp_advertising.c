/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <sys/byteorder.h>
#include <bluetooth/bluetooth.h>

#include <bluetooth/services/fast_pair.h>
#include "fp_registration_data.h"
#include "fp_common.h"


size_t bt_fast_pair_adv_data_size(bool fp_discoverable)
{
	size_t res;

	if (fp_discoverable) {
		const uint16_t fast_pair_uuid = BT_FAST_PAIR_SERVICE_UUID;

		res = (sizeof(fast_pair_uuid) + FP_MODEL_ID_LEN);
	} else {
		/* Not supported yet. */
		__ASSERT_NO_MSG(false);
		res = 0;
	}

	return res;
}

static int fp_adv_data_fill_non_discoverable(struct bt_data *bt_adv_data,
					     uint8_t *buf, size_t buf_size)
{
	/* Not supported yet. */
	return -ENOTSUP;
}

static int fp_adv_data_fill_discoverable(struct bt_data *bt_adv_data,
					 uint8_t *buf, size_t buf_size)
{
	size_t adv_data_len = bt_fast_pair_adv_data_size(true);

	if (adv_data_len > buf_size) {
		return -EINVAL;
	}

	const uint16_t fast_pair_uuid = BT_FAST_PAIR_SERVICE_UUID;

	sys_put_le16(fast_pair_uuid, buf);
	int err = fp_get_model_id(buf + sizeof(fast_pair_uuid), FP_MODEL_ID_LEN);

	if (!err) {
		bt_adv_data->type = BT_DATA_SVC_DATA16;
		bt_adv_data->data_len = adv_data_len;
		bt_adv_data->data = buf;
	}

	return err;
}

int bt_fast_pair_adv_data_fill(struct bt_data *bt_adv_data, uint8_t *buf, size_t buf_size,
			       bool fp_discoverable)
{
	if (fp_discoverable) {
		return fp_adv_data_fill_discoverable(bt_adv_data, buf, buf_size);
	} else {
		return fp_adv_data_fill_non_discoverable(bt_adv_data, buf, buf_size);
	}

	return 0;
}
