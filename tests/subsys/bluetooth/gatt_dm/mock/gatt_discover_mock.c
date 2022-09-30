/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <stdbool.h>
#include <inttypes.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/sys/util.h>


/* Settings of the discover mock */
static struct bt_discover_mock {
	const struct bt_gatt_attr *attr;
	size_t len;
	struct bt_conn *conn;
	struct bt_gatt_discover_params *params;
	struct k_work_delayable work;
} discover_mock_data;

static void bt_gatt_discover_work(struct k_work *work);

void bt_gatt_discover_mock_setup(const struct bt_gatt_attr *attr, size_t len)
{
	k_work_init_delayable(&discover_mock_data.work, bt_gatt_discover_work);
	discover_mock_data.attr = attr;
	discover_mock_data.len  = len;
}

static bool bt_gatt_primary_check(const struct bt_gatt_attr *attr_cur,
				  const struct bt_uuid *uuid)
{
	if ((!bt_uuid_cmp(BT_UUID_GATT_PRIMARY, attr_cur->uuid) ||
	     !bt_uuid_cmp(BT_UUID_GATT_SECONDARY, attr_cur->uuid))) {
		if (!uuid || !bt_uuid_cmp(uuid, ((struct bt_gatt_service_val *)
					  attr_cur->user_data)->uuid)) {
			return true;
		}
	}
	return false;
}

static void bt_gatt_discover_work(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct bt_discover_mock *mock_data =
		CONTAINER_OF(dwork, struct bt_discover_mock, work);
	const struct bt_gatt_attr *const attr_end =
		discover_mock_data.attr + discover_mock_data.len;
	const struct bt_gatt_attr *attr_cur;

	printk("Running simulated discovery:"
	       " %d, range: <%"PRIu16", %"PRIu16">\n",
	       (int)mock_data->params->type,
	       mock_data->params->start_handle,
	       mock_data->params->end_handle);

	zassert_true(mock_data->params->start_handle <= discover_mock_data.len,
		"Unexpected start handle: %u", mock_data->params->start_handle);

	for (attr_cur = discover_mock_data.attr;
	     attr_cur < attr_end;
	     ++attr_cur) {
		if (attr_cur->handle > mock_data->params->end_handle) {
			/* Too far */
			break;
		}
		if (attr_cur->handle < mock_data->params->start_handle) {
			continue;
		}

		/* In handle range, what we have to process anyway? */
		switch (mock_data->params->type) {
		case BT_GATT_DISCOVER_PRIMARY:
		case BT_GATT_DISCOVER_SECONDARY:
			if (bt_gatt_primary_check(attr_cur,
						  mock_data->params->uuid)) {
				break;
			}
			continue; /* Skip */
		case BT_GATT_DISCOVER_INCLUDE:
			if (!bt_uuid_cmp(BT_UUID_GATT_INCLUDE,
					 attr_cur->uuid)) {
				break;
			}
			continue; /* Skip */
		case BT_GATT_DISCOVER_CHARACTERISTIC:
			if (!bt_uuid_cmp(BT_UUID_GATT_CHRC,
					 attr_cur->uuid)) {
				break;
			}
			continue; /* Skip */
		case BT_GATT_DISCOVER_ATTRIBUTE:
			break;
		default:
			zassert_unreachable(
				"Invalid discovery type: %u",
				mock_data->params->type);
		}

		mock_data->params->start_handle = attr_cur->handle;
		if (BT_GATT_ITER_STOP ==
			mock_data->params->func(mock_data->conn,
			attr_cur,
			mock_data->params)) {
			printk("Discovery function stopped by user callback\n");
			return;
		}
	}

	if (attr_cur >= attr_end) {
		mock_data->params->start_handle = UINT16_MAX;
	} else {
		mock_data->params->start_handle = attr_cur->handle;
	}
	/* Send NULL to mark processing end */
	(void)mock_data->params->func(mock_data->conn, NULL, mock_data->params);
}

/* Mocked version of the bt_gatt_discover */
/* Call the bt_gatt_discover_mock_setup function first */
int bt_gatt_discover(struct bt_conn *conn,
		     struct bt_gatt_discover_params *params)
{
	printk("Running %s mock\n", __func__);
	discover_mock_data.conn = conn;
	discover_mock_data.params = params;

	k_work_schedule(&discover_mock_data.work, K_MSEC(5));
	return 0;
}
