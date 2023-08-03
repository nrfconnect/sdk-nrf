/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "bt_mgmt_scan_for_broadcast_internal.h"

#include <zephyr/zbus/zbus.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/sys/byteorder.h>

#include "bt_mgmt.h"
#include "macros_common.h"
#include "nrf5340_audio_common.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(bt_mgmt_scan);

/* Any value above 0xFFFFFF is invalid, so one can use 0xFFFFFFFF to denote
 * an invalid broadcast ID.
 */
#define INVALID_BROADCAST_ID 0xFFFFFFFF
#define PA_SYNC_SKIP	     5
/* Similar to retries for connections */
#define SYNC_RETRY_COUNT     6

ZBUS_CHAN_DECLARE(bt_mgmt_chan);

struct bt_le_scan_cb scan_callback;
static bool cb_registered;
static char const *srch_name;

struct broadcast_source {
	char name[BLE_SEARCH_NAME_MAX_LEN];
	uint32_t broadcast_id;
};

static uint16_t interval_to_sync_timeout(uint16_t interval)
{
	uint16_t timeout;

	/* Ensure that the following calculation does not overflow silently */
	__ASSERT(SYNC_RETRY_COUNT < 10, "SYNC_RETRY_COUNT shall be less than 10");

	/* Add retries and convert to unit in 10s of ms */
	timeout = ((uint32_t)interval * SYNC_RETRY_COUNT) / 10;

	/* Enforce restraints */
	timeout = CLAMP(timeout, BT_GAP_PER_ADV_MIN_TIMEOUT, BT_GAP_PER_ADV_MAX_TIMEOUT);

	return timeout;
}

static void periodic_adv_sync(const struct bt_le_scan_recv_info *info, uint32_t broadcast_id)
{
	int ret;
	struct bt_le_per_adv_sync_param param;
	struct bt_le_per_adv_sync *pa_sync;

	struct bt_mgmt_msg msg;

	bt_le_scan_cb_unregister(&scan_callback);
	cb_registered = false;

	ret = bt_le_scan_stop();
	if (ret) {
		LOG_WRN("Stop scan failed: %d", ret);
	}

	bt_addr_le_copy(&param.addr, info->addr);
	param.options = 0;
	param.sid = info->sid;
	param.skip = PA_SYNC_SKIP;
	param.timeout = interval_to_sync_timeout(info->interval);

	ret = bt_le_per_adv_sync_create(&param, &pa_sync);
	if (ret) {
		LOG_ERR("Could not sync to PA: %d", ret);
		return;
	}

	msg.event = BT_MGMT_PA_SYNC_OBJECT_READY;
	msg.broadcast_id = broadcast_id;
	msg.pa_sync = pa_sync;

	ret = zbus_chan_pub(&bt_mgmt_chan, &msg, K_NO_WAIT);
	ERR_CHK(ret);
}

/**
 * @brief	Check and parse advertising data for broadcast name and ID.
 *
 * @param[in]	data		Advertising data to check and parse.
 * @param[out]	user_data	Will contain pointer to broadcast_source struct to be populated.
 *
 * @retval	true	Continue to parse LTVs.
 * @retval	false	Stop parsing LTVs.
 */
static bool scan_check_broadcast_source(struct bt_data *data, void *user_data)
{
	struct broadcast_source *source = (struct broadcast_source *)user_data;
	struct bt_uuid_16 adv_uuid;

	if (data->type == BT_DATA_BROADCAST_NAME && data->data_len) {
		/* Ensure that broadcast name is at least one character shorter than the value of
		 * BLE_SEARCH_NAME_MAX_LEN
		 */
		if (data->data_len < BLE_SEARCH_NAME_MAX_LEN) {
			memcpy(source->name, data->data, data->data_len);
			source->name[data->data_len] = '\0';
		}

		return true;
	}

	if (data->type != BT_DATA_SVC_DATA16) {
		return true;
	}

	if (data->data_len < BT_UUID_SIZE_16 + BT_AUDIO_BROADCAST_ID_SIZE) {
		return true;
	}

	if (!bt_uuid_create(&adv_uuid.uuid, data->data, BT_UUID_SIZE_16)) {
		return false;
	}

	if (bt_uuid_cmp(&adv_uuid.uuid, BT_UUID_BROADCAST_AUDIO)) {
		return true;
	}

	source->broadcast_id = sys_get_le24(data->data + BT_UUID_SIZE_16);

	return true;
}

/**
 * @brief	Callback handler for scan receive when scanning for broadcasters.
 *
 * @param[in]	info	Advertiser packet and scan response information.
 * @param[in]	ad	Received advertising data.
 */
static void scan_recv_cb(const struct bt_le_scan_recv_info *info, struct net_buf_simple *ad)
{
	struct broadcast_source source = {.broadcast_id = INVALID_BROADCAST_ID};

	/* We are only interested in non-connectable periodic advertisers */
	if ((info->adv_props & BT_GAP_ADV_PROP_CONNECTABLE) || info->interval == 0) {
		return;
	}

	bt_data_parse(ad, scan_check_broadcast_source, (void *)&source);

	if (source.broadcast_id != INVALID_BROADCAST_ID) {
		if (strncmp(source.name, srch_name, BLE_SEARCH_NAME_MAX_LEN) == 0) {
			LOG_INF("Broadcast source %s found, id: 0x%06x", source.name,
				source.broadcast_id);
			periodic_adv_sync(info, source.broadcast_id);
		}
	}
}

int bt_mgmt_scan_for_broadcast_start(struct bt_le_scan_param *scan_param, char const *const name)
{
	int ret;

	srch_name = name;

	if (!cb_registered) {
		scan_callback.recv = scan_recv_cb;
		bt_le_scan_cb_register(&scan_callback);
		cb_registered = true;
	} else {
		/* Already scanning, stop current scan to update param in case it has changed */
		ret = bt_le_scan_stop();
		if (ret) {
			LOG_ERR("Failed to stop scan: %d", ret);
			return ret;
		}
	}

	ret = bt_le_scan_start(scan_param, NULL);
	if (ret && ret != -EALREADY) {
		return ret;
	}

	return 0;
}
