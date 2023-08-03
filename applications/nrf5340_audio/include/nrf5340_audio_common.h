/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _NRF5340_AUDIO_COMMON_H_
#define _NRF5340_AUDIO_COMMON_H_

#include <nrfx_timer.h>

#define AUDIO_SYNC_TIMER_I2S_FRAME_START_EVT_CAPTURE_CHANNEL 0
#define AUDIO_SYNC_TIMER_CURR_TIME_CAPTURE_CHANNEL	     1
#define ZBUS_READ_TIMEOUT_MS				     K_MSEC(100)

extern const nrfx_timer_t audio_sync_timer_instance;

/***** Messages for zbus ******/

enum button_action {
	BUTTON_PRESS = 1,
};

struct button_msg {
	uint32_t button_pin;
	enum button_action button_action;
};

enum le_audio_evt_type {
	LE_AUDIO_EVT_CONFIG_RECEIVED = 1,
	LE_AUDIO_EVT_PRES_DELAY_SET,
	LE_AUDIO_EVT_STREAMING,
	LE_AUDIO_EVT_NOT_STREAMING,
	LE_AUDIO_EVT_PA_SYNC_LOST,
	LE_AUDIO_EVT_NO_VALID_CFG,
};

struct le_audio_msg {
	enum le_audio_evt_type event;
	struct bt_conn *conn;
};

enum bt_mgmt_evt_type {
	BT_MGMT_CONNECTED = 1,
	BT_MGMT_DISCONNECTED,
	BT_MGMT_SECURITY_CHANGED,
	BT_MGMT_EXT_ADV_READY,
	BT_MGMT_PA_SYNC_OBJECT_READY,
};

struct bt_mgmt_msg {
	enum bt_mgmt_evt_type event;
	struct bt_conn *conn;
	struct bt_le_ext_adv *ext_adv;
	struct bt_le_per_adv_sync *pa_sync;
	uint32_t broadcast_id;
};

enum volume_evt_type {
	VOLUME_UP = 1,
	VOLUME_DOWN,
	VOLUME_SET,
	VOLUME_MUTE,
	VOLUME_UNMUTE,
};

struct volume_msg {
	enum volume_evt_type event;
	uint8_t volume;
};

enum content_control_evt_type {
	MEDIA_PLAY = 1,
	MEDIA_STOP,
};

struct content_control_msg {
	enum content_control_evt_type event;
};

#endif /* _NRF5340_AUDIO_COMMON_H_ */
