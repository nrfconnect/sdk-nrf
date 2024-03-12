/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <app/server/Server.h>
#include <platform/nrfconnect/Reboot.h>

#include "default_event_triggers.h"

#include "event_triggers.h"
#include <zephyr/logging/log.h>
#include <zephyr/sys/reboot.h>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

using namespace Nrf::Matter;
using namespace chip;

namespace
{
k_timer sFactoryResetDelayTimer;

void FactoryResetDelayTimerTimeoutCallback(k_timer *timer)
{
	Server::GetInstance().ScheduleFactoryReset();
}

CHIP_ERROR FactoryResetCallback(TestEventTrigger::TriggerValue delayMs)
{
	LOG_DBG("Called Factory Reset event trigger with delay %d ms", delayMs);

	k_timer_init(&sFactoryResetDelayTimer, &FactoryResetDelayTimerTimeoutCallback, nullptr);
	k_timer_start(&sFactoryResetDelayTimer, K_MSEC(delayMs), K_NO_WAIT);

	return CHIP_NO_ERROR;
}

CHIP_ERROR RebootCallback(TestEventTrigger::TriggerValue delayMs)
{
	LOG_DBG("Called Reboot event trigger with delay %d ms", delayMs);

	return chip::DeviceLayer::SystemLayer().StartTimer(
		chip::System::Clock::Milliseconds32(delayMs),
		[](chip::System::Layer *, void * /* context */) {
			chip::DeviceLayer::PlatformMgr().HandleServerShuttingDown();
			k_msleep(CHIP_DEVICE_CONFIG_SERVER_SHUTDOWN_ACTIONS_SLEEP_MS);
			sys_reboot(SYS_REBOOT_WARM);
		},
		nullptr /* context */);

	return CHIP_NO_ERROR;
}

} // namespace

namespace Nrf::Matter::DefaultTestEventTriggers
{

CHIP_ERROR Register()
{
	/* Register System events */
	ReturnErrorOnFailure(Nrf::Matter::TestEventTrigger::Instance().RegisterTestEventTrigger(
		Ids::FactoryReset,
		TestEventTrigger::EventTrigger{ ValueMasks::FactoryResetDelayMs, FactoryResetCallback }));
	ReturnErrorOnFailure(Nrf::Matter::TestEventTrigger::Instance().RegisterTestEventTrigger(
		Ids::Reboot, TestEventTrigger::EventTrigger{ ValueMasks::RebootDelayMs, RebootCallback }));

	/* Register OTA test events handler */
	static chip::OTATestEventTriggerHandler otaTestEventTrigger;
	ReturnErrorOnFailure(
		Nrf::Matter::TestEventTrigger::Instance().RegisterTestEventTriggerHandler(&otaTestEventTrigger));

	return CHIP_NO_ERROR;
}

} // namespace Nrf::Matter::DefaultTestEventTriggers
