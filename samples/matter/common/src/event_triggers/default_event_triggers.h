/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "event_triggers.h"
#include <app/clusters/ota-requestor/OTATestEventTriggerHandler.h>

#include <cstddef>
#include <cstdint>

namespace Nrf::Matter::DefaultTestEventTriggers
{
enum Ids : TestEventTrigger::EventTriggerId {

	/* System */
	FactoryReset = 0xF000'0000'0000'0000,
	Reboot = 0xF000'0001'0000'0000,
};

enum ValueMasks : TestEventTrigger::TriggerValueMask {
	/* System */
	FactoryResetDelayMs = 0xFFFF,
	RebootDelayMs = 0xFFFF,
};

/**
 * @brief Register all the default event triggers.
 *
 * @return CHIP_ERROR that refers to TestEventTrigger::RegisterTestEventTrigger method.
 */
CHIP_ERROR Register();

} // namespace Nrf::Matter::DefaultTestEventTriggers
