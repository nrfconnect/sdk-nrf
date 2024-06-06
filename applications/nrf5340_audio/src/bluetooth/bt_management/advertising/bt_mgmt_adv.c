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
#include "zbus_common.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_mgmt_adv, CONFIG_BT_MGMT_ADV_LOG_LEVEL);

ZBUS_CHAN_DECLARE(bt_mgmt_chan);

#ifndef CONFIG_BT_MAX_PAIRED
#define BONDS_QUEUE_SIZE 0
#else
#define BONDS_QUEUE_SIZE CONFIG_BT_MAX_PAIRED
#endif

static struct k_work adv_work;
static bool dir_adv_timed_out;
static struct bt_le_ext_adv *ext_adv[CONFIG_BT_EXT_ADV_MAX_ADV_SET];

static const struct bt_data *adv_local[CONFIG_BT_EXT_ADV_MAX_ADV_SET];
static size_t adv_local_size[CONFIG_BT_EXT_ADV_MAX_ADV_SET];
static const struct bt_data *per_adv_local[CONFIG_BT_EXT_ADV_MAX_ADV_SET];
static size_t per_adv_local_size[CONFIG_BT_EXT_ADV_MAX_ADV_SET];

/* Bonded address queue */
K_MSGQ_DEFINE(bonds_queue, sizeof(bt_addr_le_t), BONDS_QUEUE_SIZE, 4);
K_MSGQ_DEFINE(adv_queue, sizeof(uint8_t), CONFIG_BT_EXT_ADV_MAX_ADV_SET, 4);

static struct bt_le_adv_param ext_adv_param = {
	.id = BT_ID_DEFAULT,
	.sid = CONFIG_BLE_ACL_ADV_SID,
	.secondary_max_skip = 0,
	.options = BT_LE_ADV_OPT_EXT_ADV | BT_LE_ADV_OPT_USE_NAME,
	.interval_min = CONFIG_BLE_ACL_EXT_ADV_INT_MIN,
	.interval_max = CONFIG_BLE_ACL_EXT_ADV_INT_MAX,
	.peer = NULL,
};

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

static void filter_accept_list_add(const struct bt_bond_info *info, void *user_data)
{
	int ret;

	ret = bt_le_filter_accept_list_add(&info->addr);
	if (ret) {
		LOG_WRN("Could not add peer to Filter Accept List: %d", ret);
		return;
	}
}

/**
 * @brief Prints the address of the local device and the remote device.
 *
 * @note The address of the remote device is only printed if directed advertisement is active.
 */
static int addr_print(bt_addr_le_t const *const local_addr, bt_addr_le_t const *const dir_adv_addr)
{
	char local_addr_str[BT_ADDR_LE_STR_LEN] = {'\0'};
	char directed_to_addr_str[BT_ADDR_LE_STR_LEN] = {'\0'};

	if (local_addr == NULL) {
		return -EINVAL;
	}

	(void)bt_addr_le_to_str(local_addr, local_addr_str, BT_ADDR_LE_STR_LEN);
	LOG_INF("Local addr: %s", local_addr_str);

	if (dir_adv_addr != NULL) {
		(void)bt_addr_le_to_str(dir_adv_addr, directed_to_addr_str, BT_ADDR_LE_STR_LEN);
		LOG_INF("Adv directed to: %s.", directed_to_addr_str);
	}

	return 0;
}

#if defined(CONFIG_BT_PRIVACY)
static bool adv_rpa_expired_cb(struct bt_le_ext_adv *adv)
{
	int ret;
	struct bt_le_ext_adv_info ext_adv_info;

	LOG_INF("RPA (Resolvable Private Address) expired.");

	ret = bt_le_ext_adv_get_info(adv, &ext_adv_info);
	ERR_CHK_MSG(ret, "bt_le_ext_adv_get_info failed");

	ret = addr_print(ext_adv_info.addr, NULL);
	if (ret) {
		LOG_ERR("addr_print failed");
	}

	return true;
}
#endif /* CONFIG_BT_PRIVACY */

static const struct bt_le_ext_adv_cb adv_cb = {
#if defined(CONFIG_BT_PRIVACY)
	.rpa_expired = adv_rpa_expired_cb,
#endif /* CONFIG_BT_PRIVACY */
};

static int direct_adv_create(uint8_t ext_adv_index, bt_addr_le_t addr)
{
	int ret;
	struct bt_le_ext_adv_info ext_adv_info;

	ext_adv_param = *BT_LE_ADV_CONN_DIR(&addr);
	ext_adv_param.id = BT_ID_DEFAULT;
	ext_adv_param.options |= BT_LE_ADV_OPT_DIR_ADDR_RPA;

	/* Clear ADV data set before update to direct advertising */
	ret = bt_le_ext_adv_set_data(ext_adv[ext_adv_index], NULL, 0, NULL, 0);
	if (ret) {
		LOG_ERR("Failed to clear advertising data for set %d. Err: %d", ext_adv_index, ret);
		return ret;
	}

	ret = bt_le_ext_adv_update_param(ext_adv[ext_adv_index], &ext_adv_param);
	if (ret) {
		LOG_ERR("Failed to update ext_adv set %d to directed advertising. Err = %d",
			ext_adv_index, ret);
		return ret;
	}

	ret = bt_le_ext_adv_get_info(ext_adv[ext_adv_index], &ext_adv_info);
	if (ret) {
		return ret;
	}

	ret = addr_print(ext_adv_info.addr, &addr);
	if (ret) {
		return ret;
	}

	return 0;
}

static int extended_adv_create(uint8_t ext_adv_index)
{
	int ret;
	struct bt_le_ext_adv_info ext_adv_info;

	if (adv_local[ext_adv_index] == NULL) {
		LOG_ERR("Adv_local not set");
		return -ENXIO;
	}

	ret = bt_le_ext_adv_set_data(ext_adv[ext_adv_index], adv_local[ext_adv_index],
				     adv_local_size[ext_adv_index], NULL, 0);
	if (ret) {
		LOG_ERR("Failed to set advertising data: %d", ret);
		return ret;
	}

	if (per_adv_local[ext_adv_index] != NULL && IS_ENABLED(CONFIG_BT_PER_ADV)) {
		/* Set periodic advertising parameters */
		ret = bt_le_per_adv_set_param(ext_adv[ext_adv_index], LE_AUDIO_PERIODIC_ADV);
		if (ret) {
			LOG_ERR("Failed to set periodic advertising parameters: %d", ret);
			return ret;
		}

		ret = bt_le_per_adv_set_data(ext_adv[ext_adv_index], per_adv_local[ext_adv_index],
					     per_adv_local_size[ext_adv_index]);
		if (ret) {
			LOG_ERR("Failed to set periodic advertising data: %d", ret);
			return ret;
		}
	}

	ret = bt_le_ext_adv_get_info(ext_adv[ext_adv_index], &ext_adv_info);
	if (ret) {
		return ret;
	}

	ret = addr_print(ext_adv_info.addr, NULL);
	if (ret) {
		return ret;
	}

	return 0;
}

static void advertising_process(struct k_work *work)
{
	int ret;
	struct bt_mgmt_msg msg;
	uint8_t ext_adv_index;

	ret = k_msgq_get(&adv_queue, &ext_adv_index, K_NO_WAIT);
	if (ret) {
		LOG_ERR("No ext_adv_index found");
		return;
	}

	k_msgq_purge(&bonds_queue);

	if (IS_ENABLED(CONFIG_BT_BONDABLE)) {
		bt_foreach_bond(BT_ID_DEFAULT, bond_find, NULL);
		/* Populate Filter Accept List */
		if (IS_ENABLED(CONFIG_BT_FILTER_ACCEPT_LIST)) {
			ret = bt_le_filter_accept_list_clear();
			if (ret) {
				LOG_ERR("Failed to clear filter accept list");
				return;
			}

			bt_foreach_bond(BT_ID_DEFAULT, filter_accept_list_add, NULL);
		}
	}

	bt_addr_le_t addr;

	if (!k_msgq_get(&bonds_queue, &addr, K_NO_WAIT) && !dir_adv_timed_out) {
		ret = direct_adv_create(ext_adv_index, addr);
		if (ret) {
			LOG_WRN("Failed to create direct advertisement: %d", ret);
			return;
		}

		ret = bt_le_ext_adv_start(
			ext_adv[ext_adv_index],
			BT_LE_EXT_ADV_START_PARAM(BT_GAP_ADV_HIGH_DUTY_CYCLE_MAX_TIMEOUT, 0));
	} else {
		ret = extended_adv_create(ext_adv_index);
		if (ret) {
			LOG_WRN("Failed to create extended advertisement: %d", ret);
			return;
		}

		dir_adv_timed_out = false;
		ret = bt_le_ext_adv_start(ext_adv[ext_adv_index], BT_LE_EXT_ADV_START_DEFAULT);
	}

	if (ret) {
		LOG_ERR("Failed to start advertising set. Err: %d", ret);
		return;
	}

	if (per_adv_local[ext_adv_index] != NULL && IS_ENABLED(CONFIG_BT_PER_ADV)) {
		/* Enable Periodic Advertising */
		ret = bt_le_per_adv_start(ext_adv[ext_adv_index]);
		if (ret) {
			LOG_ERR("Failed to enable periodic advertising: %d", ret);
			return;
		}

		msg.event = BT_MGMT_EXT_ADV_WITH_PA_READY;
		msg.ext_adv = ext_adv[ext_adv_index];
		msg.index = ext_adv_index;

		ret = zbus_chan_pub(&bt_mgmt_chan, &msg, K_NO_WAIT);
		ERR_CHK(ret);
	}

	/* NOTE: The string below is used by the Nordic CI system */
	LOG_INF("Advertising successfully started");
}

void bt_mgmt_dir_adv_timed_out(uint8_t ext_adv_index)
{
	int ret;

	dir_adv_timed_out = true;

	LOG_DBG("Clearing ext_adv");

	ret = bt_le_ext_adv_delete(ext_adv[ext_adv_index]);
	if (ret) {
		LOG_ERR("Failed to clear ext_adv");
	}

	if (IS_ENABLED(CONFIG_BT_FILTER_ACCEPT_LIST)) {
		ret = bt_le_ext_adv_create(LE_AUDIO_EXTENDED_ADV_CONN_NAME_FILTER, &adv_cb,
					   &ext_adv[ext_adv_index]);
	} else {
		ret = bt_le_ext_adv_create(LE_AUDIO_EXTENDED_ADV_CONN_NAME, &adv_cb,
					   &ext_adv[ext_adv_index]);
	}

	if (ret) {
		LOG_ERR("Unable to create a connectable extended advertising set: %d", ret);
		return;
	}

	/* Restart normal advertising */
	ret = bt_mgmt_adv_start(ext_adv_index, NULL, 0, NULL, 0, true);
	if (ret) {
		LOG_ERR("Unable start advertising: %d", ret);
		return;
	}
}

int bt_mgmt_manufacturer_uuid_populate(struct net_buf_simple *uuid_buf, uint16_t company_id)
{
	if (net_buf_simple_tailroom(uuid_buf) >= BT_UUID_SIZE_16) {
		net_buf_simple_add_le16(uuid_buf, company_id);
	} else {
		return -ENOMEM;
	}

	return 0;
}

int bt_mgmt_adv_buffer_put(struct bt_data *const adv_buf, uint32_t *index, size_t adv_buf_vacant,
			   size_t data_len, uint8_t type, void *data)
{
	if (adv_buf == NULL || index == NULL || data_len == 0) {
		return -EINVAL;
	}

	/* Check that we have space for data */
	if (adv_buf_vacant <= *index) {
		return -ENOMEM;
	}

	adv_buf[*index].type = type;
	adv_buf[*index].data_len = data_len;
	adv_buf[*index].data = data;
	(*index)++;

	return 0;
}

int bt_mgmt_adv_start(uint8_t ext_adv_index, const struct bt_data *adv, size_t adv_size,
		      const struct bt_data *per_adv, size_t per_adv_size, bool connectable)
{
	int ret;

	/* Special case for restarting advertising */
	if (adv == NULL && adv_size == 0 && per_adv == NULL && per_adv_size == 0) {
		if (adv_local[ext_adv_index] == NULL) {
			LOG_ERR("No valid advertising data stored");
			return -ENOENT;
		}

		ret = k_msgq_put(&adv_queue, &ext_adv_index, K_NO_WAIT);
		if (ret) {
			LOG_ERR("No space in the queue for adv_index");
			return -ENOMEM;
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

	adv_local[ext_adv_index] = adv;
	adv_local_size[ext_adv_index] = adv_size;
	per_adv_local[ext_adv_index] = per_adv;
	per_adv_local_size[ext_adv_index] = per_adv_size;

	if (connectable) {
		ret = bt_le_ext_adv_create(LE_AUDIO_EXTENDED_ADV_CONN_NAME, &adv_cb,
					   &ext_adv[ext_adv_index]);
		if (ret) {
			LOG_ERR("Unable to create a connectable extended advertising set: %d", ret);
			return ret;
		}
	} else {
		ret = bt_le_ext_adv_create(&ext_adv_param, &adv_cb, &ext_adv[ext_adv_index]);
		if (ret) {
			LOG_ERR("Unable to create extended advertising set: %d", ret);
			return ret;
		}
	}

	ret = k_msgq_put(&adv_queue, &ext_adv_index, K_NO_WAIT);
	if (ret) {
		LOG_ERR("No space in the queue for adv_index");
		return -ENOMEM;
	}
	k_work_submit(&adv_work);

	return 0;
}

void bt_mgmt_adv_init(void)
{
	k_work_init(&adv_work, advertising_process);
}
