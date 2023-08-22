/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "le_audio.h"

#include <zephyr/zbus/zbus.h>
#include <zephyr/bluetooth/conn.h>

#include "nrf5340_audio_common.h"
#include "bt_mgmt.h"
#include "bt_rend.h"
#include "bt_content_ctrl.h"
#include "broadcast_internal.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(le_audio, CONFIG_BLE_LOG_LEVEL);

static enum le_audio_device_type device_type;

/**
 * @brief	Zbus listener to receive events from bt_mgmt
 *
 * @note	Will in most cases be called from BT_RX context,
 *		so there should not be too much processing done here
 */
static void bt_mgmt_evt_handler(const struct zbus_channel *chan)
{
	int ret;
	const struct bt_mgmt_msg *msg;

	msg = zbus_chan_const_msg(chan);
	uint8_t event = msg->event;

	switch (event) {
	case BT_MGMT_EXT_ADV_READY:
		LOG_INF("Ext adv ready");
		if (IS_ENABLED(CONFIG_TRANSPORT_BIS)) {
			ret = broadcast_source_start(msg->ext_adv);
			if (ret) {
				LOG_ERR("Failed to start broadcaster: %d", ret);
			}
		}

		break;

	case BT_MGMT_CONNECTED:
		LOG_INF("Connected");

		break;

	case BT_MGMT_SECURITY_CHANGED:
		LOG_INF("Security changed");

		if (CONFIG_AUDIO_DEV == GATEWAY) {
			/*le_audio_conn_set(msg->conn);*/
		}

		ret = bt_rend_discover(msg->conn);
		if (ret) {
			LOG_WRN("Failed to discover rendering services");
		}

		ret = bt_content_ctrl_discover(msg->conn);
		if (ret == -EALREADY) {
			LOG_DBG("Discovery in progress or already done");
		} else if (ret) {
			LOG_ERR("Failed to start discovery of content control: %d", ret);
		}

		break;

	case BT_MGMT_PA_SYNC_OBJECT_READY:
		LOG_INF("PA sync object ready");
		/* ret = le_audio_pa_sync_set(msg->pa_sync, msg->broadcast_id);
		 * if (ret) {
		 *	LOG_WRN("Failed to set PA sync");
		 * }
		 */

		break;

	case BT_MGMT_DISCONNECTED:
		LOG_INF("Disconnected");
		/*le_audio_conn_disconnected(msg->conn);*/

		ret = bt_content_ctrl_conn_disconnected(msg->conn);
		if (ret) {
			LOG_ERR("bt_content_ctrl_disconnected failed with %d", ret);
		}

		break;

	default:
		LOG_WRN("Unexpected/unhandled bt_mgmt event: %d", event);

		break;
	}
}

ZBUS_LISTENER_DEFINE(bt_mgmt_evt_listen, bt_mgmt_evt_handler);

int le_audio_stop(enum audio_roles role)
{
	int ret;

	if (role == BROADCAST_SOURCE) {
		ret = broadcast_source_stop();
		if (ret) {
			LOG_WRN("Failed to stop broadcaster: %d", ret);
			return ret;
		}
	}

	return 0;
}

int le_audio_start(enum audio_roles role)
{
	int ret;

	if (role == BROADCAST_SOURCE) {
		ret = broadcast_source_start(NULL);
		if (ret) {
			LOG_WRN("Failed to start broadcaster: %d", ret);
			return ret;
		}
	}

	return 0;
}

int le_audio_send(struct encoded_audio enc_audio, enum audio_roles role)
{
	int ret;

	if (role & BROADCAST_SOURCE) {

		ret = broadcast_source_send(enc_audio);
		if (ret) {
			LOG_WRN("Failed to send data to broadcaster: %d", ret);
			return ret;
		}
	}

	return 0;
}

int le_audio_enable(enum le_audio_device_type type, le_audio_receive_cb recv_cb)
{
	int ret;

	device_type = type;

	switch (device_type) {
	case LE_AUDIO_RECEIVER:
		LOG_ERR("Not yet supported");
		return -ENOTSUP;

	case LE_AUDIO_HEADSET:
		LOG_ERR("Not yet supported");
		return -ENOTSUP;

	case LE_AUDIO_MICROPHONE:
		LOG_ERR("Not yet supported");
		return -ENOTSUP;

	case LE_AUDIO_SENDER:
		LOG_ERR("Not yet supported");
		return -ENOTSUP;

	case LE_AUDIO_BROADCASTER:
		ARG_UNUSED(recv_cb);

		size_t ext_adv_size = 0;
		size_t per_adv_size = 0;
		const struct bt_data *ext_adv = NULL;
		const struct bt_data *per_adv = NULL;

		ret = broadcast_source_enable();
		if (ret) {
			LOG_WRN("Failed to enable broadcaster");
			return ret;
		}

		broadcast_source_adv_get(&ext_adv, &ext_adv_size, &per_adv, &per_adv_size);

		ret = bt_mgmt_adv_start(ext_adv, ext_adv_size, per_adv, per_adv_size, false);
		if (ret) {
			LOG_WRN("Failed to start advertiser");
			return ret;
		}

		break;

	case LE_AUDIO_GATEWAY:
		LOG_ERR("Not yet supported");
		return -ENOTSUP;

	default:
		LOG_WRN("Unknown device type: %d", device_type);
		return -EINVAL;
	}

	return 0;
}

int le_audio_disable(void)
{
	int ret;

	if (!device_type) {
		LOG_ERR("No device type enabled");
		return -ENXIO;
	}

	switch (device_type) {
	case LE_AUDIO_RECEIVER:
		LOG_ERR("Not yet supported");
		return -ENOTSUP;

	case LE_AUDIO_HEADSET:
		LOG_ERR("Not yet supported");
		return -ENOTSUP;

	case LE_AUDIO_MICROPHONE:
		LOG_ERR("Not yet supported");
		return -ENOTSUP;

	case LE_AUDIO_SENDER:
		LOG_ERR("Not yet supported");
		return -ENOTSUP;

	case LE_AUDIO_BROADCASTER:
		ret = broadcast_source_disable();
		if (ret) {
			LOG_ERR("Failed to disable broadcaster");
		}

		break;
	case LE_AUDIO_GATEWAY:
		LOG_ERR("Not yet supported");
		return -ENOTSUP;

	default:
		LOG_WRN("Unknown device type: %d", device_type);
		return -EINVAL;
	}

	return 0;
}
