/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include <cstdint>

class LEDWidget;

struct AppEvent {
	enum FunctionEventType : uint8_t {
		FunctionPress,
		FunctionRelease,
		FunctionTimer,
		MeasurementsTimer,
		IdentifyTimer,
	};

	enum UpdateLedStateEventType : uint8_t { UpdateLedState = IdentifyTimer + 1 };

	enum OtherEventType : uint8_t {
#ifdef CONFIG_MCUMGR_SMP_BT
		StartSMPAdvertising = UpdateLedState + 1
#endif
	};

	AppEvent() = default;

	explicit AppEvent(FunctionEventType type) : Type(type) {}

	AppEvent(UpdateLedStateEventType type, LEDWidget *ledWidget) : Type(type), UpdateLedStateEvent{ ledWidget } {}

	explicit AppEvent(OtherEventType type) : Type(type) {}

	uint8_t Type;

	struct {
		LEDWidget *LedWidget;
	} UpdateLedStateEvent;
};
