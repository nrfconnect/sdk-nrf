/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <app/server/Server.h>
#include <platform/nrfconnect/Reboot.h>

#include "app/group_data_provider.h"
#include "app/task_executor.h"
#include "default_event_triggers.h"
#include "event_triggers.h"

#include <zephyr/logging/log.h>
#include <zephyr/sys/reboot.h>

#ifdef CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_TEST
#include "diagnostic/diagnostic_logs_provider.h"
#endif

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

using namespace Nrf::Matter;
using namespace chip;

namespace
{
/* Timer to delay some operations to allow send back ACK to Matter Controller. */
k_timer sDelayTimer;

#ifdef CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_TEST
constexpr size_t kMaxTestingLogsSingleSize = 1024;
char sTempLogBuffer[kMaxTestingLogsSingleSize];
#endif

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
			GroupDataProviderImpl::Instance().WillBeFactoryReseted();
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

#ifdef CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_TEST
CHIP_ERROR DiagnosticLogsUserDataCallback(TestEventTrigger::TriggerValue bytesNumber)
{
	memset(sTempLogBuffer, 0x6E, sizeof(sTempLogBuffer));

	size_t logSize = static_cast<size_t>(bytesNumber);

	if (logSize > kMaxTestingLogsSingleSize) {
		return CHIP_ERROR_NO_MEMORY;
	}

	ChipLogProgress(Zcl, "Storing %zu User logs", logSize);
	if (logSize == 0) {
		DiagnosticLogProvider::GetInstance().ClearTestingBuffer(
			chip::app::Clusters::DiagnosticLogs::IntentEnum::kEndUserSupport);
	} else {
		VerifyOrReturnError(DiagnosticLogProvider::GetInstance().StoreTestingLog(
					    chip::app::Clusters::DiagnosticLogs::IntentEnum::kEndUserSupport,
					    sTempLogBuffer, logSize),
				    CHIP_ERROR_NO_MEMORY);
	}

	return CHIP_NO_ERROR;
}

CHIP_ERROR DiagnosticLogsNetworkDataCallback(TestEventTrigger::TriggerValue bytesNumber)
{
	memset(sTempLogBuffer, 0x75, sizeof(sTempLogBuffer));

	size_t logSize = static_cast<size_t>(bytesNumber);

	if (logSize > kMaxTestingLogsSingleSize) {
		return CHIP_ERROR_NO_MEMORY;
	}

	ChipLogProgress(Zcl, "Storing %zu Network logs", logSize);

	if (logSize == 0) {
		DiagnosticLogProvider::GetInstance().ClearTestingBuffer(
			chip::app::Clusters::DiagnosticLogs::IntentEnum::kNetworkDiag);
	} else {
		VerifyOrReturnError(DiagnosticLogProvider::GetInstance().StoreTestingLog(
					    chip::app::Clusters::DiagnosticLogs::IntentEnum::kNetworkDiag,
					    sTempLogBuffer, logSize),
				    CHIP_ERROR_NO_MEMORY);
	}

	return CHIP_NO_ERROR;
}

#ifdef CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_CRASH_LOGS
CHIP_ERROR DiagnosticLogsCrashCallback(TestEventTrigger::TriggerValue)
{
	/* Trigger the execute of the undefined instruction attempt */
	chip::DeviceLayer::SystemLayer().ScheduleLambda([] {
		uint8_t *undefined = nullptr;
		*undefined = 5;
	});

	return CHIP_NO_ERROR;
}
#endif /* CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_CRASH_LOGS */
#endif /* CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_TEST */

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

#ifdef CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_TEST
#ifdef CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_END_USER_LOGS
	ReturnErrorOnFailure(Nrf::Matter::TestEventTrigger::Instance().RegisterTestEventTrigger(
		Ids::DiagnosticLogsUserData,
		TestEventTrigger::EventTrigger{ ValueMasks::NumberOfBytes, DiagnosticLogsUserDataCallback }));
#endif /* CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_END_USER_LOGS */
#ifdef CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_NETWORK_LOGS
	ReturnErrorOnFailure(Nrf::Matter::TestEventTrigger::Instance().RegisterTestEventTrigger(
		Ids::DiagnosticLogsNetworkData,
		TestEventTrigger::EventTrigger{ ValueMasks::NumberOfBytes, DiagnosticLogsNetworkDataCallback }));
#endif /* CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_NETWORK_LOGS */
#ifdef CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_CRASH_LOGS
	ReturnErrorOnFailure(Nrf::Matter::TestEventTrigger::Instance().RegisterTestEventTrigger(
		Ids::DiagnosticLogsCrash, TestEventTrigger::EventTrigger{ 0, DiagnosticLogsCrashCallback }));
#endif /* CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_CRASH_LOGS */
#endif /* CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_TEST */

	/* Register OTA test events handler */
	static chip::OTATestEventTriggerHandler otaTestEventTrigger;
	ReturnErrorOnFailure(
		Nrf::Matter::TestEventTrigger::Instance().RegisterTestEventTriggerHandler(&otaTestEventTrigger));

	k_timer_init(&sDelayTimer, &DelayTimerCallback, nullptr);

	return CHIP_NO_ERROR;
}

} // namespace Nrf::Matter::DefaultTestEventTriggers
