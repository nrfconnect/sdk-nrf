/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include <cstdint>

#include "led_widget.h"

struct AppEvent {
	enum EventType : uint8_t { FunctionPress, FunctionRelease, FunctionTimer };

	enum UpdateLedStateEventType : uint8_t { UpdateLedState = FunctionTimer + 1 };

	AppEvent() = default;
	explicit AppEvent(EventType type) : Type(type) {}
	AppEvent(UpdateLedStateEventType type, LEDWidget *ledWidget) : Type(type), UpdateLedStateEvent{ ledWidget } {}

	uint8_t Type;

	struct {
		LEDWidget *LedWidget;
	} UpdateLedStateEvent;
};
