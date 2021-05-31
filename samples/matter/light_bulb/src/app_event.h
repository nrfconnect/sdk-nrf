/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include <cstdint>

struct AppEvent {
	enum LightEventType : uint8_t { On, Off, Toggle, Level };

	enum FunctionEventType : uint8_t { FunctionPress = Level + 1, FunctionRelease, FunctionTimer };

	enum OtherEventType : uint8_t { StartThread = FunctionTimer + 1, StartBleAdvertising, PublishLightBulbService };

	AppEvent() = default;

	AppEvent(LightEventType type, uint8_t value, bool chipInitiated)
		: Type(type), LightEvent{ value, chipInitiated }
	{
	}

	AppEvent(FunctionEventType type) : Type(type) {}

	AppEvent(OtherEventType type) : Type(type) {}

	uint8_t Type;

	union {
		struct {
			/* value indicating light brightness in the scope (0-255) */
			uint8_t Value;
			/* was the event triggered by CHIP Data Model layer */
			bool ChipInitiated;
		} LightEvent;
	};
};
