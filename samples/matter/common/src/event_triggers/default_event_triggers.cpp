/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <app/server/Server.h>
#include <platform/nrfconnect/Reboot.h>

#include "app/task_executor.h"
#include "default_event_triggers.h"

#include "event_triggers.h"
#include <zephyr/logging/log.h>
#include <zephyr/sys/reboot.h>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

using namespace Nrf::Matter;
using namespace chip;

namespace
{
/* Timer to delay some operations to allow send back ACK to Matter Controller. */
k_timer sDelayTimer;

/* Some operations may interrupt communication with the Matter controller so we need to postpone the action because it
 * must receive the acknowledgement from the device.  */
enum class DelayedAction {
	FactoryReset,
#ifdef CONFIG_NCS_SAMPLE_MATTER_WATCHDOG_EVENT_TRIGGERS
	BlockMainThread,
#endif
};
struct DelayedCtx {
	DelayedAction action;
	TestEventTrigger::TriggerValue value;
};

void DelayTimerCallback(k_timer *timer)
{
	/* Schedule an action in the Main thread to allow releasing Delayed context memory (it cannot be done within the
	 * Timer IRQ)*/
	void *context = k_timer_user_data_get(timer);

	Nrf::PostTask([context] {
		Platform::UniquePtr<DelayedCtx> ctx(reinterpret_cast<DelayedCtx *>(context));
		if (!ctx) {
			return;
		}

		switch (ctx->action) {
		case DelayedAction::FactoryReset:
			Server::GetInstance().ScheduleFactoryReset();
			break;
#ifdef CONFIG_NCS_SAMPLE_MATTER_WATCHDOG_EVENT_TRIGGERS
		case DelayedAction::BlockMainThread: {
			TestEventTrigger::TriggerValue value = ctx->value;
			Nrf::PostTask([value] {
				int64_t startTime = k_uptime_get();

				LOG_INF("Blocking Main thread for %d seconds", value / 1000);

				while (startTime + value > k_uptime_get()) {
					;
				}

				LOG_INF("Blocking Main thread finished");
			});
			break;
		}
#endif
		default:
			break;
		}
	});
}

CHIP_ERROR FactoryResetCallback(TestEventTrigger::TriggerValue delayMs)
{
	LOG_DBG("Called Factory Reset event trigger with delay %d ms", delayMs);

	Platform::UniquePtr<DelayedCtx> delayedContext(Platform::New<DelayedCtx>());
	if (!delayedContext) {
		return CHIP_ERROR_NO_MEMORY;
	}

	delayedContext->action = DelayedAction::FactoryReset;
	delayedContext->value = delayMs;

	k_timer_start(&sDelayTimer, K_MSEC(delayMs), K_NO_WAIT);
	k_timer_user_data_set(&sDelayTimer, reinterpret_cast<void *>(delayedContext.get()));

	delayedContext.release();

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

#ifdef CONFIG_NCS_SAMPLE_MATTER_WATCHDOG_EVENT_TRIGGERS
CHIP_ERROR BlockMainThreadCallback(TestEventTrigger::TriggerValue blockingTimeS)
{
	Platform::UniquePtr<DelayedCtx> delayedContext(Platform::New<DelayedCtx>());
	if (!delayedContext) {
		return CHIP_ERROR_NO_MEMORY;
	}

	/* Wait for 50 ms to allow finishing communication with Matter Controller */
	constexpr static uint32_t delayTimeMs = 50;

	delayedContext->action = DelayedAction::BlockMainThread;
	delayedContext->value = blockingTimeS * 1000;

	k_timer_start(&sDelayTimer, K_MSEC(delayTimeMs), K_NO_WAIT);
	k_timer_user_data_set(&sDelayTimer, reinterpret_cast<void *>(delayedContext.get()));

	delayedContext.release();

	return CHIP_NO_ERROR;
}

CHIP_ERROR BlockMatterThreadCallback(TestEventTrigger::TriggerValue blockingTimeS)
{
	chip::DeviceLayer::SystemLayer().ScheduleLambda([blockingTimeS] {
		int64_t startTime = k_uptime_get();

		LOG_INF("Blocking Matter thread for %d seconds", blockingTimeS);

		while (startTime + (blockingTimeS * 1000) > k_uptime_get()) {
			;
		}

		LOG_INF("Blocking Matter thread finished");
	});

	return CHIP_NO_ERROR;
}
#endif

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

#ifdef CONFIG_NCS_SAMPLE_MATTER_WATCHDOG_EVENT_TRIGGERS
	/* Register Watchdog events */
	ReturnErrorOnFailure(Nrf::Matter::TestEventTrigger::Instance().RegisterTestEventTrigger(
		Ids::BlockMainThread,
		TestEventTrigger::EventTrigger{ ValueMasks::BlockingTimeMs, BlockMainThreadCallback }));
	ReturnErrorOnFailure(Nrf::Matter::TestEventTrigger::Instance().RegisterTestEventTrigger(
		Ids::BlockMatterThread,
		TestEventTrigger::EventTrigger{ ValueMasks::BlockingTimeMs, BlockMatterThreadCallback }));
#endif

	/* Register OTA test events handler */
	static chip::OTATestEventTriggerHandler otaTestEventTrigger;
	ReturnErrorOnFailure(
		Nrf::Matter::TestEventTrigger::Instance().RegisterTestEventTriggerHandler(&otaTestEventTrigger));

	k_timer_init(&sDelayTimer, &DelayTimerCallback, nullptr);

	return CHIP_NO_ERROR;
}

} // namespace Nrf::Matter::DefaultTestEventTriggers
