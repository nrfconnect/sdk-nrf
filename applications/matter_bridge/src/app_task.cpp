/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "app_task.h"

#include "bridge_manager.h"
#include "bridge_storage_manager.h"
#include "matter_init.h"
#include "task_executor.h"

#ifdef CONFIG_BRIDGED_DEVICE_BT
#include "ble_bridged_device_factory.h"
#include "ble_connectivity_manager.h"
#include <bluetooth/services/lbs.h>
#else
#include "simulated_bridged_device_factory.h"
#endif /* CONFIG_BRIDGED_DEVICE_BT */

#include <app-common/zap-generated/ids/Attributes.h>
#include <app-common/zap-generated/ids/Clusters.h>
#include <app/server/OnboardingCodesUtil.h>

#ifdef CONFIG_CHIP_OTA_REQUESTOR
#include "ota_util.h"
#endif

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

using namespace ::chip;
using namespace ::chip::app;
using namespace ::chip::DeviceLayer;

namespace
{
#ifdef CONFIG_BRIDGED_DEVICE_BT
static bt_uuid *sUuidLbs = BT_UUID_LBS;
static bt_uuid *sUuidEs = BT_UUID_ESS;
static bt_uuid *sUuidServices[] = { sUuidLbs, sUuidEs };
static constexpr uint8_t kUuidServicesNumber = ARRAY_SIZE(sUuidServices);
#endif /* CONFIG_BRIDGED_DEVICE_BT */

} /* namespace */

void AppTask::MatterEventHandler(const ChipDeviceEvent *event, intptr_t /* arg */)
{
	bool isNetworkProvisioned = false;

	switch (event->Type) {
	case DeviceEventType::kCHIPoBLEAdvertisingChange:
		if (ConnectivityMgr().NumBLEConnections() != 0) {
			GetBoard().UpdateDeviceState(DeviceState::DeviceConnectedBLE);
		}
		break;
#if defined(CONFIG_CHIP_WIFI)
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

CHIP_ERROR AppTask::RestoreBridgedDevices()
{
	uint8_t count;
	uint8_t indexes[BridgeManager::kMaxBridgedDevices] = { 0 };
	size_t indexesCount = 0;

	if (!BridgeStorageManager::Instance().LoadBridgedDevicesCount(count)) {
		LOG_INF("No bridged devices to load from the storage.");
		return CHIP_NO_ERROR;
	}

	if (!BridgeStorageManager::Instance().LoadBridgedDevicesIndexes(indexes, BridgeManager::kMaxBridgedDevices,
									indexesCount)) {
		return CHIP_NO_ERROR;
	}

	/* Load all devices based on the read count number. */
	for (size_t i = 0; i < indexesCount; i++) {
		uint16_t endpointId;
		char label[MatterBridgedDevice::kNodeLabelSize] = { 0 };
		size_t labelSize;
		uint16_t deviceType;

		if (!BridgeStorageManager::Instance().LoadBridgedDeviceEndpointId(endpointId, indexes[i])) {
			return CHIP_ERROR_NOT_FOUND;
		}

		/* Ignore an error, as node label is optional, so it may not be found. */
		BridgeStorageManager::Instance().LoadBridgedDeviceNodeLabel(label, sizeof(label), labelSize,
									    indexes[i]);

		if (!BridgeStorageManager::Instance().LoadBridgedDeviceType(deviceType, indexes[i])) {
			return CHIP_ERROR_NOT_FOUND;
		}

		LOG_INF("Loaded bridged device on endpoint id %d from the storage", endpointId);

#ifdef CONFIG_BRIDGED_DEVICE_BT
		bt_addr_le_t addr;

		if (!BridgeStorageManager::Instance().LoadBtAddress(addr, indexes[i])) {
			return CHIP_ERROR_NOT_FOUND;
		}

		BleBridgedDeviceFactory::CreateDevice(deviceType, addr, label, indexes[i], endpointId);
#else
		SimulatedBridgedDeviceFactory::CreateDevice(deviceType, label, chip::Optional<uint8_t>(indexes[i]),
							    chip::Optional<uint16_t>(endpointId));
#endif
	}
	return CHIP_NO_ERROR;
}

CHIP_ERROR AppTask::Init()
{
	/* Initialize Matter stack */
	ReturnErrorOnFailure(Nordic::Matter::PrepareServer(
		MatterEventHandler, Nordic::Matter::InitData{ .mPostServerInitClbk = [] {
#ifdef CONFIG_BRIDGED_DEVICE_BT
			/* Initialize BLE Connectivity Manager before the Bridge Manager, as it must be ready to recover
			 * devices loaded from persistent storage during the bridge init. */
			CHIP_ERROR bleInitError =
				BLEConnectivityManager::Instance().Init(sUuidServices, kUuidServicesNumber);
			if (bleInitError != CHIP_NO_ERROR) {
				LOG_ERR("BLEConnectivityManager initialization failed");
				return bleInitError;
			}
#endif

			/* Initialize bridge manager */
			CHIP_ERROR bridgeMgrInitError = BridgeManager::Instance().Init(RestoreBridgedDevices);
			if (bridgeMgrInitError != CHIP_NO_ERROR) {
				LOG_ERR("BridgeManager initialization failed");
				return bridgeMgrInitError;
			}
			return CHIP_NO_ERROR;
		} }));

	if (!GetBoard().Init()) {
		LOG_ERR("User interface initialization failed.");
		return CHIP_ERROR_INCORRECT_STATE;
	}

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
