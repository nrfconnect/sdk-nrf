/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "app_task.h"

#ifdef CONFIG_AWS_IOT_INTEGRATION
#include "aws_iot_integration.h"
#endif

#include "app/matter_init.h"
#include "app/task_executor.h"

#if defined(CONFIG_PWM)
#include "pwm/pwm_device.h"
#endif

#ifdef CONFIG_CHIP_OTA_REQUESTOR
#include "dfu/ota/ota_util.h"
#endif

#include <app-common/zap-generated/attributes/Accessors.h>
#include <app/DeferredAttributePersistenceProvider.h>
#include <app/clusters/identify-server/identify-server.h>
#include <app/server/OnboardingCodesUtil.h>
#include <app/server/Server.h>

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

using namespace ::chip;
using namespace ::chip::app;
using namespace ::chip::DeviceLayer;

namespace
{
constexpr EndpointId kLightEndpointId = 1;
constexpr uint8_t kDefaultMinLevel = 0;
constexpr uint8_t kDefaultMaxLevel = 254;
constexpr uint16_t kTriggerEffectTimeout = 5000;
constexpr uint16_t kTriggerEffectFinishTimeout = 1000;

k_timer sTriggerEffectTimer;

Identify sIdentify = { kLightEndpointId, AppTask::IdentifyStartHandler, AppTask::IdentifyStopHandler,
		       Clusters::Identify::IdentifyTypeEnum::kVisibleIndicator, AppTask::TriggerIdentifyEffectHandler };

bool sIsTriggerEffectActive = false;

#if defined(CONFIG_PWM)
const struct pwm_dt_spec sLightPwmDevice = PWM_DT_SPEC_GET(DT_ALIAS(pwm_led1));
#endif

// Define a custom attribute persister which makes actual write of the CurrentLevel attribute value
// to the non-volatile storage only when it has remained constant for 5 seconds. This is to reduce
// the flash wearout when the attribute changes frequently as a result of MoveToLevel command.
// DeferredAttribute object describes a deferred attribute, but also holds a buffer with a value to
// be written, so it must live so long as the DeferredAttributePersistenceProvider object.
DeferredAttribute gCurrentLevelPersister(ConcreteAttributePath(kLightEndpointId, Clusters::LevelControl::Id,
							       Clusters::LevelControl::Attributes::CurrentLevel::Id));
DeferredAttributePersistenceProvider gDeferredAttributePersister(Server::GetInstance().GetDefaultAttributePersister(),
								 Span<DeferredAttribute>(&gCurrentLevelPersister, 1),
								 System::Clock::Milliseconds32(5000));

#define APPLICATION_BUTTON_MASK DK_BTN2_MSK
} /* namespace */

void AppTask::IdentifyStartHandler(Identify *)
{
	Nrf::PostTask(
		[] { Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED2).Blink(Nrf::LedConsts::kIdentifyBlinkRate_ms); });
}

void AppTask::IdentifyStopHandler(Identify *)
{
	Nrf::PostTask([] {
		Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED2).Set(false);

#if defined(CONFIG_PWM)
		Instance().mPWMDevice.ApplyLevel();
#endif
	});
}

void AppTask::TriggerEffectTimerTimeoutCallback(k_timer *timer)
{
	LOG_INF("Identify effect completed");

	sIsTriggerEffectActive = false;

	Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED2).Set(false);

#if defined(CONFIG_PWM)
	Instance().mPWMDevice.ApplyLevel();
#endif
}

void AppTask::TriggerIdentifyEffectHandler(Identify *identify)
{
	switch (identify->mCurrentEffectIdentifier) {
	/* Just handle all effects in the same way. */
	case Clusters::Identify::EffectIdentifierEnum::kBlink:
	case Clusters::Identify::EffectIdentifierEnum::kBreathe:
	case Clusters::Identify::EffectIdentifierEnum::kOkay:
	case Clusters::Identify::EffectIdentifierEnum::kChannelChange:
		LOG_INF("Identify effect identifier changed to %d",
			static_cast<uint8_t>(identify->mCurrentEffectIdentifier));

		sIsTriggerEffectActive = false;

		k_timer_stop(&sTriggerEffectTimer);
		k_timer_start(&sTriggerEffectTimer, K_MSEC(kTriggerEffectTimeout), K_NO_WAIT);

#if defined(CONFIG_PWM)
		Instance().mPWMDevice.SuppressOutput();
#endif
		Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED2).Blink(Nrf::LedConsts::kIdentifyBlinkRate_ms);

		break;
	case Clusters::Identify::EffectIdentifierEnum::kFinishEffect:
		LOG_INF("Identify effect finish triggered");
		k_timer_stop(&sTriggerEffectTimer);
		k_timer_start(&sTriggerEffectTimer, K_MSEC(kTriggerEffectFinishTimeout), K_NO_WAIT);
		break;
	case Clusters::Identify::EffectIdentifierEnum::kStopEffect:
		if (sIsTriggerEffectActive) {
			sIsTriggerEffectActive = false;

			k_timer_stop(&sTriggerEffectTimer);

			Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED2).Set(false);

#if defined(CONFIG_PWM)
			Instance().mPWMDevice.ApplyLevel();
#endif
		}
		break;
	default:
		LOG_ERR("Received invalid effect identifier.");
		break;
	}
}

void AppTask::LightingActionEventHandler(const LightingEvent &event)
{
#if defined(CONFIG_PWM)
	Nrf::PWMDevice::Action_t action = Nrf::PWMDevice::INVALID_ACTION;
	int32_t actor = 0;
	if (event.Actor == LightingActor::Button) {
		action = Instance().mPWMDevice.IsTurnedOn() ? Nrf::PWMDevice::OFF_ACTION : Nrf::PWMDevice::ON_ACTION;
		actor = static_cast<int32_t>(event.Actor);
	}

	if (action == Nrf::PWMDevice::INVALID_ACTION || !Instance().mPWMDevice.InitiateAction(action, actor, NULL)) {
		LOG_INF("An action could not be initiated.");
	}
#else
	Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED2).Set(!Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED2).GetState());
#endif
}

void AppTask::ButtonEventHandler(Nrf::ButtonState state, Nrf::ButtonMask hasChanged)
{
	if ((APPLICATION_BUTTON_MASK & hasChanged) & state) {
		Nrf::PostTask([] {
			LightingEvent event;
			event.Actor = LightingActor::Button;
			LightingActionEventHandler(event);
		});
	}
}

#ifdef CONFIG_AWS_IOT_INTEGRATION
bool AppTask::AWSIntegrationCallback(struct aws_iot_integration_cb_data *data)
{
	LOG_INF("Attribute change requested from AWS IoT: %d", data->value);

	Protocols::InteractionModel::Status status;

	VerifyOrDie(data->error == 0);

	if (data->attribute_id == ATTRIBUTE_ID_ONOFF) {
		/* write the new on/off value */
		status = Clusters::OnOff::Attributes::OnOff::Set(kLightEndpointId, data->value);
		if (status != Protocols::InteractionModel::Status::Success) {
			LOG_ERR("Updating on/off cluster failed: %x", to_underlying(status));
			return false;
		}
	} else if (data->attribute_id == ATTRIBUTE_ID_LEVEL_CONTROL) {
		/* write the current level */
		status = Clusters::LevelControl::Attributes::CurrentLevel::Set(kLightEndpointId, data->value);

		if (status != Protocols::InteractionModel::Status::Success) {
			LOG_ERR("Updating level cluster failed: %x", to_underlying(status));
			return false;
		}
	}

	return true;
}
#endif /* CONFIG_AWS_IOT_INTEGRATION */

#if defined(CONFIG_PWM)
void AppTask::ActionInitiated(Nrf::PWMDevice::Action_t action, int32_t actor)
{
	if (action == Nrf::PWMDevice::ON_ACTION) {
		LOG_INF("Turn On Action has been initiated");
	} else if (action == Nrf::PWMDevice::OFF_ACTION) {
		LOG_INF("Turn Off Action has been initiated");
	} else if (action == Nrf::PWMDevice::LEVEL_ACTION) {
		LOG_INF("Level Action has been initiated");
	}
}

void AppTask::ActionCompleted(Nrf::PWMDevice::Action_t action, int32_t actor)
{
	if (action == Nrf::PWMDevice::ON_ACTION) {
		LOG_INF("Turn On Action has been completed");
	} else if (action == Nrf::PWMDevice::OFF_ACTION) {
		LOG_INF("Turn Off Action has been completed");
	} else if (action == Nrf::PWMDevice::LEVEL_ACTION) {
		LOG_INF("Level Action has been completed");
	}

	if (actor == static_cast<int32_t>(LightingActor::Button)) {
		Instance().UpdateClusterState();
	}
}
#endif /* CONFIG_PWM */

void AppTask::UpdateClusterState()
{
	SystemLayer().ScheduleLambda([this] {
#if defined(CONFIG_PWM)
		/* write the new on/off value */
		Protocols::InteractionModel::Status status =
			Clusters::OnOff::Attributes::OnOff::Set(kLightEndpointId, mPWMDevice.IsTurnedOn());
#else
		Protocols::InteractionModel::Status status = Clusters::OnOff::Attributes::OnOff::Set(
			kLightEndpointId, Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED2).GetState());
#endif
		if (status != Protocols::InteractionModel::Status::Success) {
			LOG_ERR("Updating on/off cluster failed: %x", to_underlying(status));
		}

#if defined(CONFIG_PWM)
		/* write the current level */
		status = Clusters::LevelControl::Attributes::CurrentLevel::Set(kLightEndpointId, mPWMDevice.GetLevel());
#else
		/* write the current level */
		if (Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED2).GetState()) {
			status = Clusters::LevelControl::Attributes::CurrentLevel::Set(kLightEndpointId, 100);
		} else {
			status = Clusters::LevelControl::Attributes::CurrentLevel::Set(kLightEndpointId, 0);
		}
#endif

		if (status != Protocols::InteractionModel::Status::Success) {
			LOG_ERR("Updating level cluster failed: %x", to_underlying(status));
		}
	});
}

void AppTask::InitPWMDDevice()
{
#if defined(CONFIG_PWM)
	/* Initialize lighting device (PWM) */
	uint8_t minLightLevel = kDefaultMinLevel;
	Clusters::LevelControl::Attributes::MinLevel::Get(kLightEndpointId, &minLightLevel);

	uint8_t maxLightLevel = kDefaultMaxLevel;
	Clusters::LevelControl::Attributes::MaxLevel::Get(kLightEndpointId, &maxLightLevel);

	Clusters::LevelControl::Attributes::CurrentLevel::TypeInfo::Type currentLevel;
	Clusters::LevelControl::Attributes::CurrentLevel::Get(kLightEndpointId, currentLevel);

	int ret =
		mPWMDevice.Init(&sLightPwmDevice, minLightLevel, maxLightLevel, currentLevel.ValueOr(kDefaultMaxLevel));
	if (ret != 0) {
		LOG_ERR("Failed to initialize PWD device.");
	}

	mPWMDevice.SetCallbacks(ActionInitiated, ActionCompleted);
#endif
}

CHIP_ERROR AppTask::Init()
{
	/* Initialize Matter stack */
	ReturnErrorOnFailure(Nrf::Matter::PrepareServer(Nrf::Matter::InitData{ .mPostServerInitClbk = [] {
		app::SetAttributePersistenceProvider(&gDeferredAttributePersister);
		return CHIP_NO_ERROR;
	} }));

	if (!Nrf::GetBoard().Init(ButtonEventHandler)) {
		LOG_ERR("User interface initialization failed.");
		return CHIP_ERROR_INCORRECT_STATE;
	}

	/* Register Matter event handler that controls the connectivity status LED based on the captured Matter network
	 * state. */
	ReturnErrorOnFailure(Nrf::Matter::RegisterEventHandler(Nrf::Board::DefaultMatterEventHandler, 0));

#ifdef CONFIG_AWS_IOT_INTEGRATION
	int retAws = aws_iot_integration_register_callback(AWSIntegrationCallback);
	if (retAws) {
		LOG_ERR("aws_iot_integration_register_callback() failed");
		return chip::System::MapErrorZephyr(retAws);
	}
#endif

	/* Initialize trigger effect timer */
	k_timer_init(&sTriggerEffectTimer, &AppTask::TriggerEffectTimerTimeoutCallback, nullptr);

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
