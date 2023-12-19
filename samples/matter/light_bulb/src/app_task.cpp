/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "app_task.h"

#include "matter_init.h"
#include "pwm_device.h"
#include "task_executor.h"

#include <app-common/zap-generated/attributes/Accessors.h>
#include <app/DeferredAttributePersistenceProvider.h>
#include <app/clusters/identify-server/identify-server.h>
#include <app/server/OnboardingCodesUtil.h>
#include <app/server/Server.h>

#ifdef CONFIG_CHIP_OTA_REQUESTOR
#include "ota_util.h"
#endif

#ifdef CONFIG_AWS_IOT_INTEGRATION
#include "aws_iot_integration.h"
#endif

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

const struct pwm_dt_spec sLightPwmDevice = PWM_DT_SPEC_GET(DT_ALIAS(pwm_led1));

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
	TaskExecutor::PostTask([] { GetBoard().GetLED(DeviceLeds::LED2).Blink(LedConsts::kIdentifyBlinkRate_ms); });
}

void AppTask::IdentifyStopHandler(Identify *)
{
	TaskExecutor::PostTask([] {
		GetBoard().GetLED(DeviceLeds::LED2).Set(false);
		Instance().mPWMDevice.ApplyLevel();
	});
}

void AppTask::TriggerEffectTimerTimeoutCallback(k_timer *timer)
{
	LOG_INF("Identify effect completed");

	sIsTriggerEffectActive = false;

	GetBoard().GetLED(DeviceLeds::LED2).Set(false);
	Instance().mPWMDevice.ApplyLevel();
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

		Instance().mPWMDevice.SuppressOutput();
		GetBoard().GetLED(DeviceLeds::LED2).Blink(LedConsts::kIdentifyBlinkRate_ms);

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

			GetBoard().GetLED(DeviceLeds::LED2).Set(false);
			Instance().mPWMDevice.ApplyLevel();
		}
		break;
	default:
		LOG_ERR("Received invalid effect identifier.");
		break;
	}
}

void AppTask::LightingActionEventHandler(const LightingEvent &event)
{
	PWMDevice::Action_t action = PWMDevice::INVALID_ACTION;
	int32_t actor = 0;
	if (event.Actor == LightingActor::Remote) {
		action = static_cast<PWMDevice::Action_t>(event.Action);
		actor = static_cast<int32_t>(event.Actor);
	} else if (event.Actor == LightingActor::Button) {
		action = Instance().mPWMDevice.IsTurnedOn() ? PWMDevice::OFF_ACTION : PWMDevice::ON_ACTION;
		actor = static_cast<int32_t>(event.Actor);
	}

	if (action == PWMDevice::INVALID_ACTION || !Instance().mPWMDevice.InitiateAction(action, actor, NULL)) {
		LOG_INF("An action could not be initiated.");
	}
}

void AppTask::ButtonEventHandler(ButtonState state, ButtonMask hasChanged)
{
	if ((APPLICATION_BUTTON_MASK & hasChanged) & state) {
		TaskExecutor::PostTask([] {
			LightingEvent event;
			event.Actor = LightingActor::Button;
			LightingActionEventHandler(event);
		});
	}
}

#ifdef CONFIG_AWS_IOT_INTEGRATION
bool AppTask::AWSIntegrationCallback(struct aws_iot_integration_cb_data *data)
{
	LOG_INF("Attribute cha1nge requested from AWS IoT: %d", data->value);

	EmberAfStatus status;

	VerifyOrDie(data->error == 0);

	if (data->attribute_id == ATTRIBUTE_ID_ONOFF) {
		/* write the new on/off value */
		status = Clusters::OnOff::Attributes::OnOff::Set(kLightEndpointId, data->value);
		if (status != EMBER_ZCL_STATUS_SUCCESS) {
			LOG_ERR("Updating on/off cluster failed: %x", status);
			return false;
		}
	} else if (data->attribute_id == ATTRIBUTE_ID_LEVEL_CONTROL) {
		/* write the current level */
		status = Clusters::LevelControl::Attributes::CurrentLevel::Set(kLightEndpointId, data->value);

		if (status != EMBER_ZCL_STATUS_SUCCESS) {
			LOG_ERR("Updating level cluster failed: %x", status);
			return false;
		}
	}

	return true;
}
#endif /* CONFIG_AWS_IOT_INTEGRATION */

void AppTask::MatterEventHandler(const ChipDeviceEvent *event, intptr_t /* arg */)
{
	bool isNetworkProvisioned = false;

	switch (event->Type) {
	case DeviceEventType::kCHIPoBLEAdvertisingChange:
#ifdef CONFIG_CHIP_NFC_COMMISSIONING
		if (event->CHIPoBLEAdvertisingChange.Result == kActivity_Started) {
			if (NFCMgr().IsTagEmulationStarted()) {
				LOG_INF("NFC Tag emulation is already started");
			} else {
				ShareQRCodeOverNFC(
					chip::RendezvousInformationFlags(chip::RendezvousInformationFlag::kBLE));
			}
		} else if (event->CHIPoBLEAdvertisingChange.Result == kActivity_Stopped) {
			NFCMgr().StopTagEmulation();
		}
#endif
		if (ConnectivityMgr().NumBLEConnections() != 0) {
			GetBoard().UpdateDeviceState(DeviceState::DeviceConnectedBLE);
		}
		break;
#if defined(CONFIG_NET_L2_OPENTHREAD)
	case DeviceEventType::kDnssdInitialized:
#if CONFIG_CHIP_OTA_REQUESTOR
		InitBasicOTARequestor();
#endif /* CONFIG_CHIP_OTA_REQUESTOR */
		break;
	case DeviceEventType::kThreadStateChange:
		isNetworkProvisioned = ConnectivityMgr().IsThreadProvisioned() && ConnectivityMgr().IsThreadEnabled();
#elif defined(CONFIG_CHIP_WIFI)
	case DeviceEventType::kWiFiConnectivityChange:
		isNetworkProvisioned =
			ConnectivityMgr().IsWiFiStationProvisioned() && ConnectivityMgr().IsWiFiStationEnabled();
#if CONFIG_CHIP_OTA_REQUESTOR
		if (event->WiFiConnectivityChange.Result == kConnectivity_Established) {
			InitBasicOTARequestor();
		}
#endif /* CONFIG_CHIP_OTA_REQUESTOR */
#endif
		if (isNetworkProvisioned) {
			GetBoard().UpdateDeviceState(DeviceState::DeviceProvisioned);
		} else {
			GetBoard().UpdateDeviceState(DeviceState::DeviceDisconnected);
		}
		break;
	default:
		break;
	}
}

void AppTask::ActionInitiated(PWMDevice::Action_t action, int32_t actor)
{
	if (action == PWMDevice::ON_ACTION) {
		LOG_INF("Turn On Action has been initiated");
	} else if (action == PWMDevice::OFF_ACTION) {
		LOG_INF("Turn Off Action has been initiated");
	} else if (action == PWMDevice::LEVEL_ACTION) {
		LOG_INF("Level Action has been initiated");
	}
}

void AppTask::ActionCompleted(PWMDevice::Action_t action, int32_t actor)
{
	if (action == PWMDevice::ON_ACTION) {
		LOG_INF("Turn On Action has been completed");
	} else if (action == PWMDevice::OFF_ACTION) {
		LOG_INF("Turn Off Action has been completed");
	} else if (action == PWMDevice::LEVEL_ACTION) {
		LOG_INF("Level Action has been completed");
	}

	if (actor == static_cast<int32_t>(LightingActor::Button)) {
		Instance().UpdateClusterState();
	}
}

void AppTask::UpdateClusterState()
{
	SystemLayer().ScheduleLambda([this] {
		/* write the new on/off value */
		EmberAfStatus status =
			Clusters::OnOff::Attributes::OnOff::Set(kLightEndpointId, mPWMDevice.IsTurnedOn());

		if (status != EMBER_ZCL_STATUS_SUCCESS) {
			LOG_ERR("Updating on/off cluster failed: %x", status);
		}

		/* write the current level */
		status = Clusters::LevelControl::Attributes::CurrentLevel::Set(kLightEndpointId, mPWMDevice.GetLevel());

		if (status != EMBER_ZCL_STATUS_SUCCESS) {
			LOG_ERR("Updating level cluster failed: %x", status);
		}
	});
}

CHIP_ERROR AppTask::Init()
{
	/* Initialize Matter stack */
	ReturnErrorOnFailure(
		Nordic::Matter::PrepareServer(MatterEventHandler, Nordic::Matter::InitData{ .mPostServerInitClbk = [] {
						   app::SetAttributePersistenceProvider(&gDeferredAttributePersister);
						   return CHIP_NO_ERROR;
					   } }));

	if (!GetBoard().Init(ButtonEventHandler)) {
		LOG_ERR("User interface initialization failed.");
		return CHIP_ERROR_INCORRECT_STATE;
	}

	int ret{};
#ifdef CONFIG_AWS_IOT_INTEGRATION
	ret = aws_iot_integration_register_callback(AWSIntegrationCallback);
	if (ret) {
		LOG_ERR("aws_iot_integration_register_callback() failed");
		return chip::System::MapErrorZephyr(ret);
	}
#endif

	/* Initialize trigger effect timer */
	k_timer_init(&sTriggerEffectTimer, &AppTask::TriggerEffectTimerTimeoutCallback, nullptr);

	/* Initialize lighting device (PWM) */
	uint8_t minLightLevel = kDefaultMinLevel;
	Clusters::LevelControl::Attributes::MinLevel::Get(kLightEndpointId, &minLightLevel);

	uint8_t maxLightLevel = kDefaultMaxLevel;
	Clusters::LevelControl::Attributes::MaxLevel::Get(kLightEndpointId, &maxLightLevel);

	ret = mPWMDevice.Init(&sLightPwmDevice, minLightLevel, maxLightLevel, maxLightLevel);
	if (ret != 0) {
		return chip::System::MapErrorZephyr(ret);
	}
	mPWMDevice.SetCallbacks(ActionInitiated, ActionCompleted);

	return Nordic::Matter::StartServer();
}

CHIP_ERROR AppTask::StartApp()
{
	ReturnErrorOnFailure(Init());

	while (true) {
		TaskExecutor::DispatchNextTask();
	}

	return CHIP_NO_ERROR;
}
