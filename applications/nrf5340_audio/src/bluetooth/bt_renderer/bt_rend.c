/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "bt_rend.h"

#include <zephyr/zbus/zbus.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/uuid.h>

#include "bt_rend_vol_internal.h"
#include "nrf5340_audio_common.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_rend, CONFIG_BT_REND_LOG_LEVEL);

ZBUS_CHAN_DEFINE(volume_chan, struct volume_msg, NULL, NULL, ZBUS_OBSERVERS_EMPTY,
		 ZBUS_MSG_INIT(0));

int bt_rend_volume_up(void)
{
	int ret;
	struct volume_msg msg;

	if (IS_ENABLED(CONFIG_BT_VCP_VOL_CTLR) || IS_ENABLED(CONFIG_BT_VCP_VOL_REND)) {
		ret = bt_rend_vol_up();
		return ret;
	}

	msg.event = VOLUME_UP;

	ret = zbus_chan_pub(&volume_chan, &msg, K_NO_WAIT);
	return ret;
}

int bt_rend_volume_down(void)
{
	int ret;
	struct volume_msg msg;

	if (IS_ENABLED(CONFIG_BT_VCP_VOL_CTLR) || IS_ENABLED(CONFIG_BT_VCP_VOL_REND)) {
		ret = bt_rend_vol_down();
		return ret;
	}

	msg.event = VOLUME_DOWN;

	ret = zbus_chan_pub(&volume_chan, &msg, K_NO_WAIT);
	return ret;
}

int bt_rend_volume_set(uint8_t volume, bool from_vcp)
{
	int ret;
	struct volume_msg msg;

	if ((IS_ENABLED(CONFIG_BT_VCP_VOL_CTLR) || IS_ENABLED(CONFIG_BT_VCP_VOL_REND)) &&
	    !from_vcp) {
		ret = bt_rend_vol_set(volume);
		return ret;
	}

	msg.event = VOLUME_SET;
	msg.volume = volume;

	ret = zbus_chan_pub(&volume_chan, &msg, K_NO_WAIT);
	return ret;
}

int bt_rend_volume_mute(bool from_vcp)
{
	int ret;
	struct volume_msg msg;

	if ((IS_ENABLED(CONFIG_BT_VCP_VOL_CTLR) || IS_ENABLED(CONFIG_BT_VCP_VOL_REND)) &&
	    !from_vcp) {
		ret = bt_rend_vol_mute();
		return ret;
	}

	msg.event = VOLUME_MUTE;

	ret = zbus_chan_pub(&volume_chan, &msg, K_NO_WAIT);
	return ret;
}

int bt_rend_volume_unmute(void)
{
	int ret;
	struct volume_msg msg;

	if (IS_ENABLED(CONFIG_BT_VCP_VOL_CTLR) || IS_ENABLED(CONFIG_BT_VCP_VOL_REND)) {
		ret = bt_rend_vol_unmute();
		return ret;
	}

	msg.event = VOLUME_UNMUTE;

	ret = zbus_chan_pub(&volume_chan, &msg, K_NO_WAIT);
	return ret;
}

int bt_rend_discover(struct bt_conn *conn)
{
	int ret;

	/* Only do a VCS discover if we are volume controller */
	if (IS_ENABLED(CONFIG_BT_VCP_VOL_CTLR)) {
		ret = bt_rend_vol_discover(conn);
		if (ret) {
			LOG_WRN("Failed to discover VCS: %d", ret);
			return ret;
		}
	}

	return 0;
}

int bt_rend_uuid_populate(struct net_buf_simple *uuid_buf)
{
	if (IS_ENABLED(CONFIG_BT_VCP_VOL_REND)) {
		if (net_buf_simple_tailroom(uuid_buf) >= BT_UUID_SIZE_16) {
			net_buf_simple_add_le16(uuid_buf, BT_UUID_VCS_VAL);
		} else {
			return -ENOMEM;
		}
	}

	return 0;
}

int bt_rend_init(void)
{
	int ret;

	if (IS_ENABLED(CONFIG_BT_VCP_VOL_CTLR)) {
		ret = bt_rend_vol_ctlr_init();

		if (ret) {
			LOG_WRN("Failed to initialize VCS controller: %d", ret);
			return ret;
		}
	}

	if (IS_ENABLED(CONFIG_BT_VCP_VOL_REND)) {
		ret = bt_rend_vol_rend_init();

		if (ret) {
			LOG_WRN("Failed to initialize VCS renderer: %d", ret);
			return ret;
		}
	}

	return 0;
}
