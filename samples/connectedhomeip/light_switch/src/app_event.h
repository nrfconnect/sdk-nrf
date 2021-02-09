/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include <inet/IPAddress.h>

#include <cstdint>

struct AppEvent {
	using IPAddress = chip::Inet::IPAddress;

	enum SimpleEventType : uint8_t { FactoryReset, StartDiscovery, StopDiscovery, ToggleLight };
	enum LightLevelEventType : uint8_t { SetLevel = ToggleLight + 1 };
	enum LightFoundEventType : uint8_t { LightFound = SetLevel + 1 };

	AppEvent() = default;
	explicit AppEvent(SimpleEventType type) : Type(type) {}
	AppEvent(LightLevelEventType type, uint8_t level) : Type(type), LightLevelEvent{ level } {}
	AppEvent(LightFoundEventType type, const IPAddress &addr) : Type(type), LightFoundEvent{ addr } {}

	uint8_t Type;

	union {
		struct {
			uint8_t Level;
		} LightLevelEvent;
		struct {
			IPAddress PeerAddress;
		} LightFoundEvent;
	};
};
