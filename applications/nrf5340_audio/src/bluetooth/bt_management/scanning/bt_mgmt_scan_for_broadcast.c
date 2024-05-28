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
#include <hci_core.h>

#include "bt_mgmt.h"
#include "macros_common.h"
#include "zbus_common.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(bt_mgmt_scan);

/* Any value above 0xFFFFFF is invalid, so one can use 0xFFFFFFFF to denote
 * an invalid broadcast ID.
 */
#define INVALID_BROADCAST_ID		  0xFFFFFFFF
#define PA_SYNC_SKIP			  2
/* Similar to retries for connections */
#define PA_SYNC_INTERVAL_TO_TIMEOUT_RATIO 20

ZBUS_CHAN_DECLARE(bt_mgmt_chan);

struct bt_le_scan_cb scan_callback;
static bool scan_cb_registered;
static bool sync_cb_registered;
static char const *srch_name;
static uint32_t srch_brdcast_id = BRDCAST_ID_NOT_USED;
static struct bt_le_per_adv_sync *pa_sync;

struct broadcast_source {
	char name[BLE_SEARCH_NAME_MAX_LEN];
	uint32_t id;
};

static struct broadcast_source brcast_src_info;

static void scan_restart_worker(struct k_work *work)
{
	int ret;

	ret = bt_le_scan_stop();
	if (ret && ret != -EALREADY) {
		LOG_WRN("Stop scan failed: %d", ret);
	}

	/* Delete pending PA sync before restarting scan */
	ret = bt_mgmt_pa_sync_delete(pa_sync);
	if (ret) {
		LOG_WRN("Failed to delete pending PA sync: %d", ret);
	}

	ret = bt_mgmt_scan_start(0, 0, BT_MGMT_SCAN_TYPE_BROADCAST, NULL, BRDCAST_ID_NOT_USED);
	if (ret) {
		LOG_WRN("Failed to restart scanning for broadcast: %d", ret);
	}
}

K_WORK_DEFINE(scan_restart_work, scan_restart_worker);

static void pa_sync_timeout(struct k_timer *timer)
{
	LOG_WRN("PA sync create timed out, restarting scanning");

	k_work_submit(&scan_restart_work);
}

K_TIMER_DEFINE(pa_sync_timer, pa_sync_timeout, NULL);

static uint16_t interval_to_sync_timeout(uint16_t interval)
{
	uint32_t interval_ms;
	uint32_t timeout;

	/* Add retries and convert to unit in 10's of ms */
	interval_ms = BT_GAP_PER_ADV_INTERVAL_TO_MS(interval);
	timeout = (interval_ms * PA_SYNC_INTERVAL_TO_TIMEOUT_RATIO) / 10;

	/* Enforce restraints */
	timeout = CLAMP(timeout, BT_GAP_PER_ADV_MIN_TIMEOUT, BT_GAP_PER_ADV_MAX_TIMEOUT);

	return (uint16_t)timeout;
}

static void periodic_adv_sync(const struct bt_le_scan_recv_info *info,
			      struct broadcast_source source)
{
	int ret;
	struct bt_le_per_adv_sync_param param;

	bt_le_scan_cb_unregister(&scan_callback);
	scan_cb_registered = false;

	bt_addr_le_copy(&param.addr, info->addr);
	param.options = 0;
	param.sid = info->sid;
	param.skip = PA_SYNC_SKIP;
	param.timeout = interval_to_sync_timeout(info->interval);

	/* Set timeout to same value as PA sync timeout in ms */
	k_timer_start(&pa_sync_timer, K_MSEC(param.timeout * 10), K_NO_WAIT);

	ret = bt_le_per_adv_sync_create(&param, &pa_sync);
	if (ret) {
		LOG_ERR("Could not sync to PA: %d", ret);
		ret = bt_mgmt_pa_sync_delete(pa_sync);
		if (ret) {
			LOG_ERR("Could not delete PA sync: %d", ret);
		}
		return;
	}
	brcast_src_info = source;
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

	source->id = sys_get_le24(data->data + BT_UUID_SIZE_16);

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
	struct broadcast_source source = {.id = INVALID_BROADCAST_ID};

	/* We are only interested in non-connectable periodic advertisers */
	if ((info->adv_props & BT_GAP_ADV_PROP_CONNECTABLE) || info->interval == 0) {
		return;
	}

	bt_data_parse(ad, scan_check_broadcast_source, (void *)&source);

	if (source.id != INVALID_BROADCAST_ID) {
		if (srch_brdcast_id < BRDCAST_ID_NOT_USED) {
			/* Valid srch_brdcast_id supplied */
			if (source.id != srch_brdcast_id) {
				/* Broadcaster does not match src_brdcast_id */
				return;
			}

		} else if (strncmp(source.name, srch_name, BLE_SEARCH_NAME_MAX_LEN) != 0) {
			/* Broadcaster does not match src_name */
			return;
		}

		LOG_DBG("Broadcast source %s found, id: 0x%06x", source.name, source.id);
		periodic_adv_sync(info, source);
	}
}

static void pa_synced_cb(struct bt_le_per_adv_sync *sync,
			 struct bt_le_per_adv_sync_synced_info *info)
{
	int ret;
	struct bt_mgmt_msg msg;

	if (sync != pa_sync) {
		LOG_WRN("Synced to unknown source");
		return;
	}

	char addr_str[BT_ADDR_LE_STR_LEN];
	(void)bt_addr_le_to_str(&sync->addr, addr_str, BT_ADDR_LE_STR_LEN);
	LOG_INF("PA synced to name: %s, id: 0x%06x, addr: %s", brcast_src_info.name,
		brcast_src_info.id, addr_str);

	k_timer_stop(&pa_sync_timer);

	ret = bt_le_scan_stop();
	if (ret && ret != -EALREADY) {
		LOG_WRN("Stop scan failed: %d", ret);
	}

	msg.event = BT_MGMT_PA_SYNCED;
	msg.pa_sync = sync;
	msg.broadcast_id = brcast_src_info.id;

	ret = zbus_chan_pub(&bt_mgmt_chan, &msg, K_NO_WAIT);
	ERR_CHK(ret);
}

static void pa_sync_terminated_cb(struct bt_le_per_adv_sync *sync,
				  const struct bt_le_per_adv_sync_term_info *info)
{
	int ret;
	struct bt_mgmt_msg msg;

	LOG_DBG("Periodic advertising sync lost");

	msg.event = BT_MGMT_PA_SYNC_LOST;
	msg.pa_sync = sync;
	msg.pa_sync_term_reason = info->reason;

	ret = zbus_chan_pub(&bt_mgmt_chan, &msg, K_NO_WAIT);
	ERR_CHK(ret);
}

static struct bt_le_per_adv_sync_cb sync_callbacks = {
	.synced = pa_synced_cb,
	.term = pa_sync_terminated_cb,
};

int bt_mgmt_scan_for_broadcast_start(struct bt_le_scan_param *scan_param, char const *const name,
				     uint32_t brdcast_id)
{
	int ret;

	if (!sync_cb_registered) {
		bt_le_per_adv_sync_cb_register(&sync_callbacks);
		sync_cb_registered = true;
	}

	if (!scan_cb_registered) {
		scan_callback.recv = scan_recv_cb;
		bt_le_scan_cb_register(&scan_callback);
		scan_cb_registered = true;
	} else {
		if (name == srch_name) {
			return -EALREADY;
		}
		/* Might already be scanning, stop current scan to update param in case it has
		 * changed.
		 */
		ret = bt_le_scan_stop();
		if (ret && ret != -EALREADY) {
			LOG_ERR("Failed to stop scan: %d", ret);
			return ret;
		}
	}

	srch_name = name;
	if (brdcast_id != BRDCAST_ID_NOT_USED) {
		srch_brdcast_id = brdcast_id;
	}

	ret = bt_le_scan_start(scan_param, NULL);
	if (ret) {
		return ret;
	}

	return 0;
}
