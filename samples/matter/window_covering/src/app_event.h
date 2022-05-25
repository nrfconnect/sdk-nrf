/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include <cstdint>

class LEDWidget;

struct AppEvent {
	using EventHandler = void (*)(AppEvent *);

	enum class Type : uint8_t {
		None,
		Button,
		Timer,
		UpdateLedState,
#ifdef CONFIG_MCUMGR_SMP_BT
		StartSMPAdvertising
#endif
	};

	Type Type{ Type::None };

	union {
		struct {
			uint8_t PinNo;
			uint8_t Action;
		} ButtonEvent;
		struct {
			void *Context;
		} TimerEvent;
		struct {
			LEDWidget *LedWidget;
		} UpdateLedStateEvent;
	};

	EventHandler Handler;
};
