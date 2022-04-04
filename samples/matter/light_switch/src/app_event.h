/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include <inet/IPAddress.h>

#include <cstdint>

#include "led_widget.h"

struct AppEvent;
typedef void (*EventHandler)(AppEvent *);

struct AppEvent {
	constexpr static uint8_t kButtonPushEvent = 1;
	constexpr static uint8_t kButtonReleaseEvent = 0;

	enum AppEventTypes : uint8_t {
		kEventType_StartBLEAdvertising,
		kEventType_Button,
		kEventType_Timer,
		kEventType_UpdateLedState,
		kEventType_IdentifyStart,
		kEventType_IdentifyStop,
#ifdef CONFIG_MCUMGR_SMP_BT
		kEventType_StartSMPAdvertising,
#endif
	};

	uint8_t Type;

	union {
		struct {
			uint8_t PinNo;
			uint8_t Action;
		} ButtonEvent;
		struct {
			uint8_t TimerType;
			void *Context;
		} TimerEvent;
		struct {
			LEDWidget *LedWidget;
		} UpdateLedStateEvent;
	};

	EventHandler Handler;
};
