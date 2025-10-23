/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "app_task.h"

#ifdef CONFIG_BRIDGED_DEVICE_BT
#include "ble_bridged_device_factory.h"
#else
#include "simulated_bridged_device_factory.h"
#endif /* CONFIG_BRIDGED_DEVICE_BT */

#include "app/matter_init.h"
#include "app/task_executor.h"
#include "board/board.h"
#include "bridge_manager.h"
#include "bridge_storage_manager.h"

#ifdef CONFIG_BRIDGED_DEVICE_BT
#include "ble_connectivity_manager.h"
#endif

#include <app-common/zap-generated/attributes/Accessors.h>
#include <app-common/zap-generated/ids/Attributes.h>
#include <app-common/zap-generated/ids/Clusters.h>
#include <app/clusters/identify-server/identify-server.h>
#include <setup_payload/OnboardingCodesUtil.h>

#ifdef CONFIG_BRIDGED_DEVICE_BT
#include <bluetooth/services/lbs.h>
#endif /* CONFIG_BRIDGED_DEVICE_BT */
#include <zephyr/logging/log.h>

#ifdef CONFIG_BRIDGE_SMART_PLUG_SUPPORT
#define APPLICATION_BUTTON_MASK DK_BTN2_MSK
#endif /* CONFIG_BRIDGE_SMART_PLUG_SUPPORT */

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

using namespace ::chip;
using namespace ::chip::app;
using namespace ::chip::DeviceLayer;

namespace
{

constexpr EndpointId kBridgeEndpointId = 1;
constexpr uint16_t kTriggerEffectTimeout = 5000;
constexpr uint16_t kTriggerEffectFinishTimeout = 1000;

k_timer sTriggerEffectTimer;
bool sIsTriggerEffectActive;

Identify sIdentify = { kBridgeEndpointId, AppTask::IdentifyStartHandler, AppTask::IdentifyStopHandler,
		       Clusters::Identify::IdentifyTypeEnum::kVisibleIndicator, AppTask::TriggerIdentifyEffectHandler };

#ifdef CONFIG_BRIDGED_DEVICE_BT
const bt_uuid *sUuidLbs = BT_UUID_LBS;
const bt_uuid *sUuidEs = BT_UUID_ESS;
const bt_uuid *sUuidServices[] = { sUuidLbs, sUuidEs };
constexpr uint8_t kUuidServicesNumber = ARRAY_SIZE(sUuidServices);
/**
 * @brief Blink rates for indication the BLE Connectivity Manager state.
 *
 */
constexpr uint32_t kPairingBlinkRate{ 100 };
constexpr uint32_t kScanningBlinkRate_ms{ 300 };
constexpr uint32_t kLostBlinkRate_ms{ 1000 };
#ifndef CONFIG_BRIDGE_SMART_PLUG_SUPPORT
void BLEStateChangeCallback(Nrf::BLEConnectivityManager::State state)
{
	switch (state) {
	case Nrf::BLEConnectivityManager::State::Connected:
		Nrf::PostTask([] { Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED2).Set(true); });
		break;
	case Nrf::BLEConnectivityManager::State::Scanning:
		Nrf::PostTask([] {
			Nrf::GetBoard()
				.GetLED(Nrf::DeviceLeds::LED2)
				.Blink(kScanningBlinkRate_ms, kScanningBlinkRate_ms);
		});
		break;
	case Nrf::BLEConnectivityManager::State::LostDevice:
		Nrf::PostTask([] {
			Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED2).Blink(kLostBlinkRate_ms, kLostBlinkRate_ms);
		});
		break;
	case Nrf::BLEConnectivityManager::State::Pairing:
		Nrf::PostTask([] {
			Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED2).Blink(kPairingBlinkRate, kPairingBlinkRate);
		});
		break;
	default:
		Nrf::PostTask([] { Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED2).Set(false); });
	}
}
#endif /* CONFIG_BRIDGE_SMART_PLUG_SUPPORT */

#endif /* CONFIG_BRIDGED_DEVICE_BT */

#ifndef CONFIG_CHIP_FACTORY_RESET_ERASE_SETTINGS
void AppFactoryResetHandler(const ChipDeviceEvent *event, intptr_t /* unused */)
{
	switch (event->Type) {
	case DeviceEventType::kFactoryReset:
		Nrf::BridgeStorageManager::Instance().FactoryReset();
		break;
	default:
		break;
	}
}
#endif

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
	});
}

void AppTask::TriggerEffectTimerTimeoutCallback(k_timer *timer)
{
	LOG_INF("Identify effect completed");

	sIsTriggerEffectActive = false;

	Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED2).Set(false);

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
		}
		break;
	default:
		LOG_ERR("Received invalid effect identifier.");
		break;
	}
}

CHIP_ERROR AppTask::RestoreBridgedDevices()
{
	uint8_t count;
	uint8_t indexes[Nrf::BridgeManager::kMaxBridgedDevices] = { 0 };
	size_t indexesCount = 0;

	if (!Nrf::BridgeStorageManager::Instance().LoadBridgedDevicesCount(count)) {
		LOG_INF("No bridged devices to load from the storage.");
		return CHIP_NO_ERROR;
	}

	if (!Nrf::BridgeStorageManager::Instance().LoadBridgedDevicesIndexes(
		    indexes, Nrf::BridgeManager::kMaxBridgedDevices, indexesCount)) {
		return CHIP_NO_ERROR;
	}

	/* Load all devices based on the read count number. */
	for (size_t i = 0; i < indexesCount; i++) {
		Nrf::BridgeStorageManager::BridgedDevice device;
#ifdef CONFIG_BRIDGED_DEVICE_BT
		bt_addr_le_t btAddr;
		device.mUserData = reinterpret_cast<uint8_t *>(&btAddr);
		device.mUserDataSize = sizeof(btAddr);
#endif

		if (!Nrf::BridgeStorageManager::Instance().LoadBridgedDevice(device, indexes[i])) {
			return CHIP_ERROR_NOT_FOUND;
		}

		LOG_INF("Loaded bridged device on endpoint id %d from the storage", device.mEndpointId);

#ifdef CONFIG_BRIDGED_DEVICE_BT
		BleBridgedDeviceFactory::CreateDevice(device.mDeviceType, btAddr, device.mUniqueID, device.mNodeLabel,
						      indexes[i], device.mEndpointId);
#else
		SimulatedBridgedDeviceFactory::CreateDevice(device.mDeviceType, device.mUniqueID, device.mNodeLabel,
							    chip::Optional<uint8_t>(indexes[i]),
							    chip::Optional<uint16_t>(device.mEndpointId));
#endif
	}
	return CHIP_NO_ERROR;
}
#ifdef CONFIG_BRIDGE_SMART_PLUG_SUPPORT
void AppTask::SmartplugOnOffEventHandler()
{
	SystemLayer().ScheduleLambda([] {
		bool newState = !Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED2).GetState();
		Protocols::InteractionModel::Status status =
			Clusters::OnOff::Attributes::OnOff::Set(kSmartplugEndpointId, newState);
		if (status != Protocols::InteractionModel::Status::Success) {
			LOG_ERR("Updating on/off cluster failed: %x", to_underlying(status));
		}
	});
}

void AppTask::ButtonEventHandler(Nrf::ButtonState state, Nrf::ButtonMask hasChanged)
{
	if ((APPLICATION_BUTTON_MASK & hasChanged) & state) {
		Nrf::PostTask([] { SmartplugOnOffEventHandler(); });
	}
}
#endif /* CONFIG_BRIDGE_SMART_PLUG_SUPPORT */

CHIP_ERROR AppTask::Init()
{
	/* Initialize Matter stack */
	ReturnErrorOnFailure(Nrf::Matter::PrepareServer(Nrf::Matter::InitData{ .mPostServerInitClbk = [] {
#ifdef CONFIG_BRIDGED_DEVICE_BT
#ifndef CONFIG_BRIDGE_SMART_PLUG_SUPPORT
		Nrf::BLEConnectivityManager::Instance().RegisterStateCallback(BLEStateChangeCallback);
#endif /* CONFIG_BRIDGE_SMART_PLUG_SUPPORT */
		/* Initialize BLE Connectivity Manager before the Bridge Manager, as it must be ready to recover
		 * devices loaded from persistent storage during the bridge init. */
		CHIP_ERROR bleInitError =
			Nrf::BLEConnectivityManager::Instance().Init(sUuidServices, kUuidServicesNumber);
		if (bleInitError != CHIP_NO_ERROR) {
			LOG_ERR("BLEConnectivityManager initialization failed");
			return bleInitError;
		}
#endif

		/* Initialize bridge manager */
		CHIP_ERROR bridgeMgrInitError = Nrf::BridgeManager::Instance().Init(RestoreBridgedDevices);
		if (bridgeMgrInitError != CHIP_NO_ERROR) {
			LOG_ERR("BridgeManager initialization failed");
			return bridgeMgrInitError;
		}
		return CHIP_NO_ERROR;
	} }));
#ifdef CONFIG_BRIDGE_SMART_PLUG_SUPPORT
	/* onoff plug part add buttonEventHandler*/
	if (!Nrf::GetBoard().Init(ButtonEventHandler)) {
		LOG_ERR("User interface initialization failed.");
		return CHIP_ERROR_INCORRECT_STATE;
	}
#else
	if (!Nrf::GetBoard().Init()) {
		LOG_ERR("User interface initialization failed.");
		return CHIP_ERROR_INCORRECT_STATE;
	}
#endif
	/* Register Matter event handler that controls the connectivity status LED based on the captured Matter
	 * network state. */
	ReturnErrorOnFailure(Nrf::Matter::RegisterEventHandler(Nrf::Board::DefaultMatterEventHandler, 0));

#ifndef CONFIG_CHIP_FACTORY_RESET_ERASE_SETTINGS
	/* Register factory reset event handler.
	 * With this configuration we have to manually clean up the storage,
	 * as whole settings partition won't be erased.
	 * */
	ReturnErrorOnFailure(Nrf::Matter::RegisterEventHandler(AppFactoryResetHandler, 0));
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
