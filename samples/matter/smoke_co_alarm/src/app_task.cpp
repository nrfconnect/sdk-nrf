/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "app_task.h"

#include "app/matter_init.h"
#include "app/task_executor.h"
#include "board/board.h"
#include "lib/core/CHIPError.h"
#include "lib/support/CodeUtils.h"
#include "clusters/identify.h"

#include <app-common/zap-generated/attributes/Accessors.h>
#include <app/clusters/smoke-co-alarm-server/SmokeCOTestEventTriggerHandler.h>
#include <app/clusters/smoke-co-alarm-server/smoke-co-alarm-server.h>
#include <setup_payload/OnboardingCodesUtil.h>

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

using namespace ::chip;
using namespace ::chip::app;
using namespace ::chip::DeviceLayer;

namespace
{

constexpr uint32_t kExpressedStateLedEvenBlinkMs = 300;
constexpr uint32_t kExpressedStateLedOddBlink1Ms = 300;
constexpr uint32_t kExpressedStateLedOddBlink2Ms = 700;
constexpr uint32_t kSelfTestDurationMs = 5000;
constexpr uint32_t kSelfTestLedUpdateTimeMs = 200;

static std::array<Clusters::SmokeCoAlarm::ExpressedStateEnum, SmokeCoAlarmServer::kPriorityOrderLength>
	sPriorityOrder = { Clusters::SmokeCoAlarm::ExpressedStateEnum::kSmokeAlarm,
			   Clusters::SmokeCoAlarm::ExpressedStateEnum::kCOAlarm,
			   Clusters::SmokeCoAlarm::ExpressedStateEnum::kHardwareFault,
			   Clusters::SmokeCoAlarm::ExpressedStateEnum::kTesting,
			   Clusters::SmokeCoAlarm::ExpressedStateEnum::kEndOfService,
			   Clusters::SmokeCoAlarm::ExpressedStateEnum::kBatteryAlert,
			   Clusters::SmokeCoAlarm::ExpressedStateEnum::kInterconnectSmoke,
			   Clusters::SmokeCoAlarm::ExpressedStateEnum::kInterconnectCO };

constexpr chip::EndpointId kWiredPowerSourceEndpointId = 0;
constexpr chip::EndpointId kBatteryPowerSourceEndpointId = 1;

Nrf::Matter::IdentifyCluster sIdentifyCluster(AppTask::kSmokeCoAlarmEndpointId);

#ifdef CONFIG_CHIP_ICD_UAT_SUPPORT
#define UAT_BUTTON_MASK DK_BTN3_MSK
#endif
} /* namespace */

void AppTask::ButtonEventHandler(Nrf::ButtonState state, Nrf::ButtonMask hasChanged)
{
#ifdef CONFIG_CHIP_ICD_UAT_SUPPORT
	if ((UAT_BUTTON_MASK & state & hasChanged)) {
		LOG_INF("ICD UserActiveMode has been triggered.");
		Server::GetInstance().GetICDManager().OnNetworkActivity();
	}
#endif
}

void AppTask::UpdatedExpressedLedState()
{
	Clusters::SmokeCoAlarm::ExpressedStateEnum state;
	SmokeCoAlarmServer::Instance().GetExpressedState(AppTask::kSmokeCoAlarmEndpointId, state);

	if (mCurrentState == state) {
		return;
	}
	mCurrentState = state;

	k_timer_stop(&mSelfTestLedTimer);

	switch (state) {
	case Clusters::SmokeCoAlarm::ExpressedStateEnum::kNormal:
		Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED2).Set(false);
		Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED3).Set(false);
		Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED4).Set(false);
		break;
	case Clusters::SmokeCoAlarm::ExpressedStateEnum::kSmokeAlarm:
		Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED2).Blink(kExpressedStateLedEvenBlinkMs);
		Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED3).Set(false);
		Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED4).Set(false);
		break;
	case Clusters::SmokeCoAlarm::ExpressedStateEnum::kCOAlarm:
		Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED2).Set(false);
		Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED3).Blink(kExpressedStateLedEvenBlinkMs);
		Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED4).Set(false);
		break;
	case Clusters::SmokeCoAlarm::ExpressedStateEnum::kBatteryAlert:
		Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED2).Set(false);
		Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED3).Set(false);
		Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED4).Blink(kExpressedStateLedEvenBlinkMs);
		break;
	case Clusters::SmokeCoAlarm::ExpressedStateEnum::kTesting:
		Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED2).Set(false);
		Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED3).Set(false);
		Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED4).Set(false);

		k_timer_start(&mSelfTestLedTimer, K_MSEC(kSelfTestLedUpdateTimeMs), K_NO_WAIT);
		Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED2).Set(true);

		break;
	case Clusters::SmokeCoAlarm::ExpressedStateEnum::kHardwareFault:
		Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED2).Set(false);
		Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED3).Set(false);
		Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED4).Set(false);

		Nrf::GetBoard()
			.GetLED(Nrf::DeviceLeds::LED2)
			.Blink(kExpressedStateLedOddBlink1Ms, kExpressedStateLedOddBlink2Ms);
		Nrf::GetBoard()
			.GetLED(Nrf::DeviceLeds::LED3)
			.Blink(kExpressedStateLedOddBlink1Ms, kExpressedStateLedOddBlink2Ms);
		Nrf::GetBoard()
			.GetLED(Nrf::DeviceLeds::LED4)
			.Blink(kExpressedStateLedOddBlink1Ms, kExpressedStateLedOddBlink2Ms);
		break;
	case Clusters::SmokeCoAlarm::ExpressedStateEnum::kEndOfService:
		Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED2).Set(false);
		Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED3).Set(false);
		Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED4).Set(false);

		Nrf::GetBoard()
			.GetLED(Nrf::DeviceLeds::LED2)
			.Blink(kExpressedStateLedOddBlink2Ms, kExpressedStateLedOddBlink1Ms);
		Nrf::GetBoard()
			.GetLED(Nrf::DeviceLeds::LED3)
			.Blink(kExpressedStateLedOddBlink2Ms, kExpressedStateLedOddBlink1Ms);
		Nrf::GetBoard()
			.GetLED(Nrf::DeviceLeds::LED4)
			.Blink(kExpressedStateLedOddBlink2Ms, kExpressedStateLedOddBlink1Ms);
		break;
	case Clusters::SmokeCoAlarm::ExpressedStateEnum::kInterconnectSmoke:
	case Clusters::SmokeCoAlarm::ExpressedStateEnum::kInterconnectCO:
	case Clusters::SmokeCoAlarm::ExpressedStateEnum::kUnknownEnumValue:
	default:
		break;
	}
}

bool HandleSmokeCOTestEventTrigger(uint64_t eventTrigger)
{
	SmokeCOTrigger trigger = static_cast<SmokeCOTrigger>(eventTrigger);

	switch (trigger) {
	case SmokeCOTrigger::kForceSmokeCritical:
		LOG_INF("Triggered smoke alarm with critical severity");
		VerifyOrReturnValue(SmokeCoAlarmServer::Instance().SetSmokeState(
					    AppTask::kSmokeCoAlarmEndpointId, Clusters::SmokeCoAlarm::AlarmStateEnum::kCritical),
				    false);
		SmokeCoAlarmServer::Instance().SetExpressedStateByPriority(AppTask::kSmokeCoAlarmEndpointId, sPriorityOrder);
		break;
	case SmokeCOTrigger::kForceCOCritical:
		LOG_INF("Triggered CO alarm with critical severity");
		VerifyOrReturnValue(SmokeCoAlarmServer::Instance().SetCOState(
					    AppTask::kSmokeCoAlarmEndpointId, Clusters::SmokeCoAlarm::AlarmStateEnum::kCritical),
				    false);
		SmokeCoAlarmServer::Instance().SetExpressedStateByPriority(AppTask::kSmokeCoAlarmEndpointId, sPriorityOrder);
		break;
	case SmokeCOTrigger::kForceMalfunction:
		LOG_INF("Triggered hardware fault alert");
		VerifyOrReturnValue(SmokeCoAlarmServer::Instance().SetHardwareFaultAlert(AppTask::kSmokeCoAlarmEndpointId, true),
				    false);
		SmokeCoAlarmServer::Instance().SetExpressedStateByPriority(AppTask::kSmokeCoAlarmEndpointId, sPriorityOrder);
		break;
	case SmokeCOTrigger::kForceLowBatteryCritical:
		LOG_INF("Triggered low battery alert with critical severity");
		VerifyOrReturnValue(SmokeCoAlarmServer::Instance().SetBatteryAlert(
					    AppTask::kSmokeCoAlarmEndpointId, Clusters::SmokeCoAlarm::AlarmStateEnum::kCritical),
				    false);
		SmokeCoAlarmServer::Instance().SetExpressedStateByPriority(AppTask::kSmokeCoAlarmEndpointId, sPriorityOrder);
		break;
	case SmokeCOTrigger::kForceEndOfLife:
		LOG_INF("Triggered end of service alert");
		VerifyOrReturnValue(SmokeCoAlarmServer::Instance().SetEndOfServiceAlert(
					    AppTask::kSmokeCoAlarmEndpointId,
					    Clusters::SmokeCoAlarm::EndOfServiceEnum::kExpired),
				    false);
		SmokeCoAlarmServer::Instance().SetExpressedStateByPriority(AppTask::kSmokeCoAlarmEndpointId, sPriorityOrder);
		break;
	case SmokeCOTrigger::kClearSmoke:
		LOG_INF("Triggered end of smoke alarm action");
		VerifyOrReturnValue(SmokeCoAlarmServer::Instance().SetSmokeState(
					    AppTask::kSmokeCoAlarmEndpointId, Clusters::SmokeCoAlarm::AlarmStateEnum::kNormal),
				    false);
		SmokeCoAlarmServer::Instance().SetExpressedStateByPriority(AppTask::kSmokeCoAlarmEndpointId, sPriorityOrder);
		break;
	case SmokeCOTrigger::kClearCO:
		LOG_INF("Triggered end of CO alarm action");
		VerifyOrReturnValue(SmokeCoAlarmServer::Instance().SetCOState(
					    AppTask::kSmokeCoAlarmEndpointId, Clusters::SmokeCoAlarm::AlarmStateEnum::kNormal),
				    false);
		SmokeCoAlarmServer::Instance().SetExpressedStateByPriority(AppTask::kSmokeCoAlarmEndpointId, sPriorityOrder);
		break;
	case SmokeCOTrigger::kClearMalfunction:
		LOG_INF("Triggered end of hardware fault alert action");
		VerifyOrReturnValue(
			SmokeCoAlarmServer::Instance().SetHardwareFaultAlert(AppTask::kSmokeCoAlarmEndpointId, false), false);
		SmokeCoAlarmServer::Instance().SetExpressedStateByPriority(AppTask::kSmokeCoAlarmEndpointId, sPriorityOrder);
		break;
	case SmokeCOTrigger::kClearEndOfLife:
		LOG_INF("Triggered end of end of service alert action");
		VerifyOrReturnValue(SmokeCoAlarmServer::Instance().SetEndOfServiceAlert(
					    AppTask::kSmokeCoAlarmEndpointId, Clusters::SmokeCoAlarm::EndOfServiceEnum::kNormal),
				    false);
		SmokeCoAlarmServer::Instance().SetExpressedStateByPriority(AppTask::kSmokeCoAlarmEndpointId, sPriorityOrder);
		break;
	case SmokeCOTrigger::kClearBatteryLevelLow:
		LOG_INF("Triggered end of low battery level alert action");
		VerifyOrReturnValue(SmokeCoAlarmServer::Instance().SetBatteryAlert(
					    AppTask::kSmokeCoAlarmEndpointId, Clusters::SmokeCoAlarm::AlarmStateEnum::kNormal),
				    false);
		SmokeCoAlarmServer::Instance().SetExpressedStateByPriority(AppTask::kSmokeCoAlarmEndpointId, sPriorityOrder);
		break;
	case SmokeCOTrigger::kForceSmokeWarning:
		LOG_WRN("Triggered unsupported smoke alarm with warning severity");
		return false;
	case SmokeCOTrigger::kForceSmokeInterconnect:
		LOG_WRN("Triggered unsupported smoke interconnect alarm");
		return false;
	case SmokeCOTrigger::kForceCOWarning:
		LOG_WRN("Triggered unsupported CO alarm with warning severity");
		return false;
	case SmokeCOTrigger::kForceCOInterconnect:
		LOG_WRN("Triggered unsupported CO interconnect alarm");
		return false;
	case SmokeCOTrigger::kForceSmokeContaminationHigh:
		LOG_WRN("Triggered unsupported high smoke contamination alarm");
		return false;
	case SmokeCOTrigger::kForceSmokeContaminationLow:
		LOG_WRN("Triggered unsupported low smoke contamination alarm");
		return false;
	case SmokeCOTrigger::kForceSmokeSensitivityHigh:
		LOG_WRN("Triggered unsupported high smoke sensitivity alarm");
		return false;
	case SmokeCOTrigger::kForceSmokeSensitivityLow:
		LOG_WRN("Triggered unsupported low smoke sensitivity alarm");
		return false;
	case SmokeCOTrigger::kForceLowBatteryWarning:
		LOG_WRN("Triggered unsupported low battery alert with warning severity");
		return false;
	case SmokeCOTrigger::kForceSilence:
		LOG_WRN("Triggered unsupported alarm mute action");
		return false;
	case SmokeCOTrigger::kClearSmokeInterconnect:
		LOG_WRN("Triggered unsupported end of smoke interconnect alarm action");
		return false;
	case SmokeCOTrigger::kClearCOInterconnect:
		LOG_WRN("Triggered unsupported end of CO interconnect alarm action");
		return false;
	case SmokeCOTrigger::kClearSilence:
		LOG_WRN("Triggered unsupported unmute action");
		return false;
	case SmokeCOTrigger::kClearContamination:
		LOG_WRN("Triggered unsupported end of smoke contamination alert action");
		return false;
	case SmokeCOTrigger::kClearSensitivity:
		LOG_WRN("Triggered unsupported end of smoke sensitivity alert action");
		return false;
	default:
		return false;
	}

	AppTask::Instance().UpdatedExpressedLedState();

	return true;
}

void AppTask::SelfTestTimerTimeoutCallback(k_timer *timer)
{
	Nrf::PostTask([] { EndSelfTestEventHandler(); });
}

void AppTask::EndSelfTestEventHandler()
{
	LOG_INF("Alarm self test action end");
	SmokeCoAlarmServer::Instance().SetTestInProgress(kSmokeCoAlarmEndpointId, false);
	SmokeCoAlarmServer::Instance().SetExpressedStateByPriority(kSmokeCoAlarmEndpointId, sPriorityOrder);
	Instance().UpdatedExpressedLedState();
}

void AppTask::SelfTestLedTimerTimeoutCallback(k_timer *timer)
{
	Nrf::PostTask([] { SelfTestLedTimerEventHandler(); });
}

void AppTask::SelfTestLedTimerEventHandler()
{
	if (Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED2).GetState()) {
		Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED2).Set(false);
		Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED3).Set(true);
	} else if (Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED3).GetState()) {
		Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED3).Set(false);
		Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED4).Set(true);
	} else if (Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED4).GetState()) {
		Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED4).Set(false);
		Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED2).Set(true);
	}

	k_timer_start(&Instance().mSelfTestLedTimer, K_MSEC(kSelfTestLedUpdateTimeMs), K_NO_WAIT);
}

#ifdef CONFIG_NCS_SAMPLE_MATTER_TEST_EVENT_TRIGGERS
CHIP_ERROR AppTask::PowerSourceOnEventCallback(Nrf::Matter::TestEventTrigger::TriggerValue endpointId)
{
	if (endpointId == kWiredPowerSourceEndpointId || endpointId == kBatteryPowerSourceEndpointId) {
		LOG_INF("Event Trigger: Power Source on endpoint id %d activated.", endpointId);
	} else {
		LOG_ERR("Event Trigger: Power Source On triggered on invalid endpoint id %d", endpointId);
		return CHIP_ERROR_INVALID_ARGUMENT;
	}

	/* Wired power source is treated as primary one. */
	if (endpointId == kWiredPowerSourceEndpointId) {
		Clusters::PowerSource::Attributes::Status::Set(kWiredPowerSourceEndpointId,
							       Clusters::PowerSource::PowerSourceStatusEnum::kActive);

		Clusters::PowerSource::PowerSourceStatusEnum status;
		Clusters::PowerSource::Attributes::Status::Get(kBatteryPowerSourceEndpointId, &status);

		/* If battery power source was active, change its state to standby. */
		if (status == Clusters::PowerSource::PowerSourceStatusEnum::kActive) {
			Clusters::PowerSource::Attributes::Status::Set(
				kBatteryPowerSourceEndpointId, Clusters::PowerSource::PowerSourceStatusEnum::kStandby);
		}
	} else {
		Clusters::PowerSource::PowerSourceStatusEnum status;
		Clusters::PowerSource::Attributes::Status::Get(kWiredPowerSourceEndpointId, &status);

		/* If wired power source is active, change put requested power source into standby state to standby. */
		if (status == Clusters::PowerSource::PowerSourceStatusEnum::kActive) {
			Clusters::PowerSource::Attributes::Status::Set(
				kBatteryPowerSourceEndpointId, Clusters::PowerSource::PowerSourceStatusEnum::kStandby);
		} else {
			Clusters::PowerSource::Attributes::Status::Set(
				kBatteryPowerSourceEndpointId, Clusters::PowerSource::PowerSourceStatusEnum::kActive);
		}
	}

	return CHIP_NO_ERROR;
}

CHIP_ERROR AppTask::PowerSourceOffEventCallback(Nrf::Matter::TestEventTrigger::TriggerValue endpointId)
{
	if (endpointId == kWiredPowerSourceEndpointId || endpointId == kBatteryPowerSourceEndpointId) {
		LOG_INF("Event Trigger: Power Source on endpoint id %d deactivated.", endpointId);
	} else {
		LOG_ERR("Event Trigger: Power Source Off triggered on invalid endpoint id %d", endpointId);
		return CHIP_ERROR_INVALID_ARGUMENT;
	}

	/* Wired power source is treated as primary one. */
	if (endpointId == kWiredPowerSourceEndpointId) {
		Clusters::PowerSource::Attributes::Status::Set(
			kWiredPowerSourceEndpointId, Clusters::PowerSource::PowerSourceStatusEnum::kUnavailable);

		Clusters::PowerSource::PowerSourceStatusEnum status;
		Clusters::PowerSource::Attributes::Status::Get(kBatteryPowerSourceEndpointId, &status);

		/* If battery power source was standby, change its state to active. */
		if (status == Clusters::PowerSource::PowerSourceStatusEnum::kStandby) {
			Clusters::PowerSource::Attributes::Status::Set(
				kBatteryPowerSourceEndpointId, Clusters::PowerSource::PowerSourceStatusEnum::kActive);
		}
	} else {
		Clusters::PowerSource::Attributes::Status::Set(
			kBatteryPowerSourceEndpointId, Clusters::PowerSource::PowerSourceStatusEnum::kUnavailable);
	}

	return CHIP_NO_ERROR;
}
#endif

CHIP_ERROR AppTask::Init()
{
	/* Initialize Matter stack */
	ReturnErrorOnFailure(Nrf::Matter::PrepareServer());

	if (!Nrf::GetBoard().Init(ButtonEventHandler)) {
		LOG_ERR("User interface initialization failed.");
		return CHIP_ERROR_INCORRECT_STATE;
	}

	k_timer_init(&mSelfTestLedTimer, &SelfTestLedTimerTimeoutCallback, nullptr);
	k_timer_init(&mSelfTestTimer, &SelfTestTimerTimeoutCallback, nullptr);

	/* Register Matter event handler that controls the connectivity status LED based on the captured Matter network
	 * state. */
	ReturnErrorOnFailure(Nrf::Matter::RegisterEventHandler(Nrf::Board::DefaultMatterEventHandler, 0));

	/* Register Smoke CO Alarm test event triggers */
#ifdef CONFIG_NCS_SAMPLE_MATTER_TEST_EVENT_TRIGGERS
	/* Register Smoke CO alarm test events handler */
	static chip::SmokeCOTestEventTriggerHandler sSmokeCoAlarmTestEventTrigger;
	ReturnErrorOnFailure(Nrf::Matter::TestEventTrigger::Instance().RegisterTestEventTriggerHandler(
		&sSmokeCoAlarmTestEventTrigger));

	/* Register test event handler to change the power source. */
	ReturnErrorOnFailure(Nrf::Matter::TestEventTrigger::Instance().RegisterTestEventTrigger(
		kPowerSourceOnEventTriggerId,
		Nrf::Matter::TestEventTrigger::EventTrigger{ 0xFFFF, PowerSourceOnEventCallback }));
	ReturnErrorOnFailure(Nrf::Matter::TestEventTrigger::Instance().RegisterTestEventTrigger(
		kPowerSourceOffEventTriggerId,
		Nrf::Matter::TestEventTrigger::EventTrigger{ 0xFFFF, PowerSourceOffEventCallback }));
#endif

	ReturnErrorOnFailure(sIdentifyCluster.Init());

	return Nrf::Matter::StartServer();
}

CHIP_ERROR AppTask::StartApp()
{
	ReturnErrorOnFailure(Init());

	while (true) {
		Nrf::DispatchNextTask();
	}

	return CHIP_NO_ERROR;
}

void AppTask::SelfTestHandler()
{
	LOG_INF("Triggered alarm self test action");
	k_timer_start(&Instance().mSelfTestTimer, K_MSEC(kSelfTestDurationMs), K_NO_WAIT);
	UpdatedExpressedLedState();
}
