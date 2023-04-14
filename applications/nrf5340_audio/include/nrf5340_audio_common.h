/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _NRF5340_AUDIO_COMMON_H_
#define _NRF5340_AUDIO_COMMON_H_

#include <nrfx_timer.h>

#define AUDIO_SYNC_TIMER_I2S_FRAME_START_EVT_CAPTURE_CHANNEL 0
#define AUDIO_SYNC_TIMER_CURR_TIME_CAPTURE_CHANNEL 1

extern const nrfx_timer_t audio_sync_timer_instance;

/***** Messages for Z-Bus ******/

enum button_action {
	BUTTON_PRESS,
	BUTTON_ACTION_NUM,
};

struct button_msg {
	uint32_t button_pin;
	enum button_action button_action;
};

enum le_audio_evt_type {
	LE_AUDIO_EVT_CONFIG_RECEIVED,
	LE_AUDIO_EVT_PRES_DELAY_SET,
	LE_AUDIO_EVT_STREAMING,
	LE_AUDIO_EVT_NOT_STREAMING,
	LE_AUDIO_EVT_NUM_EVTS
};

struct le_audio_msg {
	enum le_audio_evt_type event;
};

#endif /* _NRF5340_AUDIO_COMMON_H_ */
