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

#include <app-common/zap-generated/ids/Attributes.h>
#include <app-common/zap-generated/ids/Clusters.h>
#include <app/server/OnboardingCodesUtil.h>

#ifdef CONFIG_BRIDGED_DEVICE_BT
#include <bluetooth/services/lbs.h>
#endif /* CONFIG_BRIDGED_DEVICE_BT */
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
		uint16_t endpointId;
		char label[Nrf::MatterBridgedDevice::kNodeLabelSize] = { 0 };
		size_t labelSize;
		uint16_t deviceType;

		if (!Nrf::BridgeStorageManager::Instance().LoadBridgedDeviceEndpointId(endpointId, indexes[i])) {
			return CHIP_ERROR_NOT_FOUND;
		}

		/* Ignore an error, as node label is optional, so it may not be found. */
		Nrf::BridgeStorageManager::Instance().LoadBridgedDeviceNodeLabel(label, sizeof(label), labelSize,
										 indexes[i]);

		if (!Nrf::BridgeStorageManager::Instance().LoadBridgedDeviceType(deviceType, indexes[i])) {
			return CHIP_ERROR_NOT_FOUND;
		}

		LOG_INF("Loaded bridged device on endpoint id %d from the storage", endpointId);

#ifdef CONFIG_BRIDGED_DEVICE_BT
		bt_addr_le_t addr;

		if (!Nrf::BridgeStorageManager::Instance().LoadBtAddress(addr, indexes[i])) {
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
	ReturnErrorOnFailure(Nrf::Matter::PrepareServer(Nrf::Matter::InitData{ .mPostServerInitClbk = [] {
#ifdef CONFIG_BRIDGED_DEVICE_BT
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

	if (!Nrf::GetBoard().Init()) {
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
