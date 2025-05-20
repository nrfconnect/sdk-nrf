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

	/* Values of EventTrigger in the range 0xFFFF_FFFF_0000_0000 through 0xFFFF_FFFF_FFFF_FFFF are reserved for
	   testing use by manufactures */

	/* System */
	FactoryReset = 0xFFFF'FFFF'0000'0000,
	Reboot = 0xFFFF'FFFF'1000'0000,

#ifdef CONFIG_NCS_SAMPLE_MATTER_WATCHDOG_EVENT_TRIGGERS
	/* Watchdog */
	BlockMatterThread = 0xFFFF'FFFF'2000'0000,
	BlockMainThread = 0xFFFF'FFFF'3000'0000,
#endif

#ifdef CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_TEST
#ifdef CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_END_USER_LOGS
	DiagnosticLogsUserData = 0xFFFF'FFFF'4000'0000,
#endif /* CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_END_USER_LOGS */
#ifdef CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_NETWORK_LOGS
	DiagnosticLogsNetworkData = 0xFFFF'FFFF'5000'0000,
#endif /* CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_NETWORK_LOGS */
#ifdef CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_CRASH_LOGS
	DiagnosticLogsCrash = 0xFFFF'FFFF'6000'0000,
#endif /* CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_CRASH_LOGS */
#endif /* CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_TEST */
};

enum ValueMasks : TestEventTrigger::TriggerValueMask {
	/* System */
	FactoryResetDelayMs = 0xFFFF,
	RebootDelayMs = 0xFFFF,

#ifdef CONFIG_NCS_SAMPLE_MATTER_WATCHDOG_EVENT_TRIGGERS
	/* Watchdog */
	BlockingTimeMs = 0xFFFF,
#endif

#ifdef CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_TEST
	NumberOfBytes = 0x7FF,
#endif
};

/**
 * @brief Register all the default event triggers.
 *
 * @return CHIP_ERROR that refers to TestEventTrigger::RegisterTestEventTrigger method.
 */
CHIP_ERROR Register();

} // namespace Nrf::Matter::DefaultTestEventTriggers
