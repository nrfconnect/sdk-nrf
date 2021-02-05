/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include <cstdint>

struct AppEvent {
	enum LockEventType : uint8_t { Lock, Unlock, Toggle, CompleteLockAction };

	enum OtherEventType : uint8_t { FactoryReset = CompleteLockAction + 1, StartThread, StartBleAdvertising };

	AppEvent() = default;
	AppEvent(LockEventType type, bool chipInitiated) : Type(type), LockEvent{ chipInitiated } {}
	explicit AppEvent(OtherEventType type) : Type(type) {}

	uint8_t Type;

	union {
		struct {
			/* was the event triggered by CHIP Data Model layer */
			bool ChipInitiated;
		} LockEvent;
	};
};
