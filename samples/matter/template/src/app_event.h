/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include <cstdint>

struct AppEvent {

	enum EventType : uint8_t { FunctionPress, FunctionRelease, FunctionTimer};

	AppEvent() = default;
	explicit AppEvent(EventType type) : Type(type) {}

	uint8_t Type;
};
