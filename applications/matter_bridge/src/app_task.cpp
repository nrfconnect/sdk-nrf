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
#include "bridge/bridge_manager.h"
#include "bridge/bridge_storage_manager.h"

#ifdef CONFIG_BRIDGED_DEVICE_BT
#include "bridge/ble_connectivity_manager.h"
#endif

#ifdef CONFIG_CHIP_OTA_REQUESTOR
#include "dfu/ota/ota_util.h"
#endif /* CONFIG_BRIDGED_DEVICE_BT */

#include <app-common/zap-generated/attributes/Accessors.h>
#include <app-common/zap-generated/ids/Attributes.h>
#include <app-common/zap-generated/ids/Clusters.h>
#include <app/server/OnboardingCodesUtil.h>

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

#ifdef CONFIG_BRIDGED_DEVICE_BT
static const bt_uuid *sUuidLbs = BT_UUID_LBS;
static const bt_uuid *sUuidEs = BT_UUID_ESS;
static const bt_uuid *sUuidServices[] = { sUuidLbs, sUuidEs };
static constexpr uint8_t kUuidServicesNumber = ARRAY_SIZE(sUuidServices);
/**
 * @brief Blink rates for indication the BLE Connectivity Manager state.
 *
 */
constexpr static uint32_t kPairingBlinkRate{ 100 };
constexpr static uint32_t kScanningBlinkRate_ms{ 300 };
constexpr static uint32_t kLostBlinkRate_ms{ 1000 };
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

} /* namespace */

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
		BleBridgedDeviceFactory::CreateDevice(device.mDeviceType, btAddr, device.mNodeLabel, indexes[i],
						      device.mEndpointId);
#else
		SimulatedBridgedDeviceFactory::CreateDevice(device.mDeviceType, device.mNodeLabel,
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
