/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "app_task.h"


#include "app/matter_init.h"
#include "app/task_executor.h"

#if defined(CONFIG_PWM)
#include "pwm/pwm_device.h"
#endif

#include <app-common/zap-generated/attributes/Accessors.h>
#include <app/DeferredAttributePersistenceProvider.h>
#include <app/clusters/identify-server/identify-server.h>
#include <app/server/Server.h>
#include <setup_payload/OnboardingCodesUtil.h>

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


	#if defined(CONFIG_PWM)
	const struct pwm_dt_spec sLightPwmDevice = PWM_DT_SPEC_GET(DT_ALIAS(pwm_led1));
	#endif
	#define APPLICATION_BUTTON_MASK DK_BTN2_MSK
	#ifdef CONFIG_CHIP_ICD_UAT_SUPPORT
	#define UAT_BUTTON_MASK DK_BTN3_MSK
	#endif
} /* namespace */

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
	#ifdef CONFIG_CHIP_ICD_UAT_SUPPORT
		if ((UAT_BUTTON_MASK & state & hasChanged)) {
			LOG_INF("ICD UserActiveMode has been triggered.");
			Server::GetInstance().GetICDManager().OnNetworkActivity();
		}
	#endif

	if ((APPLICATION_BUTTON_MASK & hasChanged) & state) {
		Nrf::PostTask([] {
			LightingEvent event;
			event.Actor = LightingActor::Button;
			LightingActionEventHandler(event);
		});
	}
}

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
