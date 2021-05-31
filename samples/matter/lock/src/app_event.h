/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include <cstdint>

struct AppEvent {
	enum LockEventType : uint8_t { Lock, Unlock, Toggle, CompleteLockAction };

	enum FunctionEventType : uint8_t { FunctionPress = CompleteLockAction + 1, FunctionRelease, FunctionTimer };

	enum OtherEventType : uint8_t { StartThread = FunctionTimer + 1, StartBleAdvertising };

	AppEvent() = default;
	AppEvent(LockEventType type, bool chipInitiated) : Type(type), LockEvent{ chipInitiated } {}
	explicit AppEvent(FunctionEventType type) : Type(type) {}
	explicit AppEvent(OtherEventType type) : Type(type) {}

	uint8_t Type;

	union {
		struct {
			/* was the event triggered by CHIP Data Model layer */
			bool ChipInitiated;
		} LockEvent;
	};
};
