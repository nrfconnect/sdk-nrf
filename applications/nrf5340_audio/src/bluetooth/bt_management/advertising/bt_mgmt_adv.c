/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "bt_mgmt.h"

#include <zephyr/zbus/zbus.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>

#include "macros_common.h"
#include "nrf5340_audio_common.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_mgmt_adv, CONFIG_BT_MGMT_ADV_LOG_LEVEL);

ZBUS_CHAN_DECLARE(bt_mgmt_chan);

#ifndef CONFIG_BT_MAX_PAIRED
#define BONDS_QUEUE_SIZE 0
#else
#define BONDS_QUEUE_SIZE CONFIG_BT_MAX_PAIRED
#endif

static struct k_work adv_work;

static struct bt_le_ext_adv *ext_adv;

static const struct bt_data *adv_local;
static size_t adv_local_size;

static const struct bt_data *per_adv_local;
static size_t per_adv_local_size;

/* Bonded address queue */
K_MSGQ_DEFINE(bonds_queue, sizeof(bt_addr_le_t), BONDS_QUEUE_SIZE, 4);

static void bond_find(const struct bt_bond_info *info, void *user_data)
{
	int ret;
	struct bt_conn *conn;

	if (!IS_ENABLED(CONFIG_BT_BONDABLE)) {
		return;
	}

	/* Filter already connected peers. */
	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &info->addr);
	if (conn) {
		struct bt_conn_info conn_info;

		ret = bt_conn_get_info(conn, &conn_info);
		if (ret) {
			LOG_WRN("Could not get conn info");
			bt_conn_unref(conn);
			return;
		}

		if (conn_info.state == BT_CONN_STATE_CONNECTED) {
			LOG_WRN("Already connected");
			bt_conn_unref(conn);
			return;
		}

		bt_conn_unref(conn);
	}

	ret = k_msgq_put(&bonds_queue, (void *)&info->addr, K_NO_WAIT);
	if (ret) {
		LOG_WRN("No space in the queue for the bond");
	}
}

static int direct_adv_create(bt_addr_le_t addr)
{
	int ret;
	struct bt_le_adv_param adv_param;
	char addr_buf[BT_ADDR_LE_STR_LEN];

	adv_param = *BT_LE_ADV_CONN_DIR_LOW_DUTY(&addr);
	adv_param.id = BT_ID_DEFAULT;
	adv_param.options |= BT_LE_ADV_OPT_DIR_ADDR_RPA;

	/* Clear ADV data set before update to direct advertising */
	ret = bt_le_ext_adv_set_data(ext_adv, NULL, 0, NULL, 0);
	if (ret) {
		LOG_ERR("Failed to clear advertising data. Err: %d", ret);
		return ret;
	}

	ret = bt_le_ext_adv_update_param(ext_adv, &adv_param);
	if (ret) {
		LOG_ERR("Failed to update ext_adv to direct advertising. Err = %d", ret);
		return ret;
	}

	bt_addr_le_to_str(&addr, addr_buf, BT_ADDR_LE_STR_LEN);
	LOG_INF("Set direct advertising to %s", addr_buf);

	return 0;
}

static int extended_adv_create(void)
{
	int ret;

	if (adv_local == NULL) {
		LOG_ERR("Adv_local not set");
		return -ENXIO;
	}

	ret = bt_le_ext_adv_set_data(ext_adv, adv_local, adv_local_size, NULL, 0);
	if (ret) {
		LOG_ERR("Failed to set advertising data: %d", ret);
		return ret;
	}

	if (per_adv_local != NULL && IS_ENABLED(CONFIG_BT_PER_ADV)) {
		/* Set periodic advertising parameters */
		ret = bt_le_per_adv_set_param(ext_adv, LE_AUDIO_PERIODIC_ADV);
		if (ret) {
			LOG_ERR("Failed to set periodic advertising parameters: %d", ret);
			return ret;
		}

		ret = bt_le_per_adv_set_data(ext_adv, per_adv_local, per_adv_local_size);
		if (ret) {
			LOG_ERR("Failed to set periodic advertising data: %d", ret);
			return ret;
		}
	}

	return 0;
}

static void advertising_process(struct k_work *work)
{
	int ret;
	struct bt_mgmt_msg msg;

	k_msgq_purge(&bonds_queue);

	if (IS_ENABLED(CONFIG_BT_BONDABLE)) {
		bt_foreach_bond(BT_ID_DEFAULT, bond_find, NULL);
	}

	bt_addr_le_t addr;

	if (!k_msgq_get(&bonds_queue, &addr, K_NO_WAIT)) {
		ret = direct_adv_create(addr);
		if (ret) {
			LOG_WRN("Failed to create direct advertisement: %d", ret);
			return;
		}
	} else {
		ret = extended_adv_create();
		if (ret) {
			LOG_WRN("Failed to create extended advertisement: %d", ret);
			return;
		}
	}

	ret = bt_le_ext_adv_start(ext_adv, BT_LE_EXT_ADV_START_DEFAULT);
	if (ret) {
		LOG_ERR("Failed to start advertising set. Err: %d", ret);
		return;
	}

	if (per_adv_local != NULL && IS_ENABLED(CONFIG_BT_PER_ADV)) {
		/* Enable Periodic Advertising */
		ret = bt_le_per_adv_start(ext_adv);
		if (ret) {
			LOG_ERR("Failed to enable periodic advertising: %d", ret);
			return;
		}
	}

	/* Publish ext_adv */
	msg.event = BT_MGMT_EXT_ADV_READY;
	msg.ext_adv = ext_adv;

	ret = zbus_chan_pub(&bt_mgmt_chan, &msg, K_NO_WAIT);
	ERR_CHK(ret);

	/* NOTE: The string below is used by the Nordic CI system */
	LOG_INF("Advertising successfully started");
}

int bt_mgmt_adv_start(const struct bt_data *adv, size_t adv_size, const struct bt_data *per_adv,
		      size_t per_adv_size, bool connectable)
{
	int ret;

	/* Special case for restarting advertising */
	if (adv == NULL && adv_size == 0 && per_adv == NULL && per_adv_size == 0) {
		if (adv_local == NULL) {
			LOG_ERR("No valid advertising data stored");
			return -ENOENT;
		}

		k_work_submit(&adv_work);

		return 0;
	}

	if (adv == NULL) {
		LOG_ERR("No adv struct received");
		return -EINVAL;
	}

	if (adv_size == 0) {
		LOG_ERR("Invalid size of adv struct");
		return -EINVAL;
	}

	adv_local = adv;
	adv_local_size = adv_size;
	per_adv_local = per_adv;
	per_adv_local_size = per_adv_size;

	if (connectable) {
		ret = bt_le_ext_adv_create(LE_AUDIO_EXTENDED_ADV_CONN_NAME, NULL, &ext_adv);
		if (ret) {
			LOG_ERR("Unable to create a connectable extended advertising set: %d", ret);
			return ret;
		}
	} else {
		ret = bt_le_ext_adv_create(LE_AUDIO_EXTENDED_ADV_NAME, NULL, &ext_adv);
		if (ret) {
			LOG_ERR("Unable to create extended advertising set: %d", ret);
			return ret;
		}
	}

	k_work_submit(&adv_work);

	return 0;
}

void bt_mgmt_adv_init(void)
{
	k_work_init(&adv_work, advertising_process);
}
