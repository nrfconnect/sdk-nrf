/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "app_task.h"
#include "task_executor.h"
#include "bridge_manager.h"
#include "bridge_storage_manager.h"
#include "fabric_table_delegate.h"

#ifdef CONFIG_BRIDGED_DEVICE_BT
#include "ble_bridged_device_factory.h"
#include "ble_connectivity_manager.h"
#include <bluetooth/services/lbs.h>
#else
#include "simulated_bridged_device_factory.h"
#endif /* CONFIG_BRIDGED_DEVICE_BT */
#include <platform/CHIPDeviceLayer.h>

#include <app-common/zap-generated/ids/Attributes.h>
#include <app-common/zap-generated/ids/Clusters.h>
#include <app/server/OnboardingCodesUtil.h>
#include <app/server/Server.h>
#include <credentials/DeviceAttestationCredsProvider.h>
#include <credentials/examples/DeviceAttestationCredsExample.h>
#include <lib/support/CHIPMem.h>
#include <lib/support/CodeUtils.h>
#include <system/SystemError.h>

#include <app/clusters/network-commissioning/network-commissioning.h>
#include <platform/nrfconnect/wifi/NrfWiFiDriver.h>

#ifdef CONFIG_CHIP_OTA_REQUESTOR
#include "ota_util.h"
#endif

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

using namespace ::chip;
using namespace ::chip::app;
using namespace ::chip::Credentials;
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

app::Clusters::NetworkCommissioning::Instance
	sWiFiCommissioningInstance(0, &(NetworkCommissioning::NrfWiFiDriver::Instance()));

CHIP_ERROR AppTask::Init()
{
	/* Initialize CHIP stack */
	LOG_INF("Init CHIP stack");

	CHIP_ERROR err = chip::Platform::MemoryInit();
	if (err != CHIP_NO_ERROR) {
		LOG_ERR("Platform::MemoryInit() failed");
		return err;
	}

	err = PlatformMgr().InitChipStack();
	if (err != CHIP_NO_ERROR) {
		LOG_ERR("PlatformMgr().InitChipStack() failed");
		return err;
	}

#if defined(CONFIG_CHIP_WIFI)
	sWiFiCommissioningInstance.Init();
#else
	return CHIP_ERROR_INTERNAL;
#endif /* CONFIG_CHIP_WIFI */

	if (!GetBoard().Init()) {
		LOG_ERR("User interface initialization failed.");
		return CHIP_ERROR_INCORRECT_STATE;
	}

#ifdef CONFIG_CHIP_OTA_REQUESTOR
	/* OTA image confirmation must be done before the factory data init. */
	OtaConfirmNewImage();
#endif

#ifdef CONFIG_MCUMGR_TRANSPORT_BT
	/* Initialize DFU over SMP */
	GetDFUOverSMP().Init();
	GetDFUOverSMP().ConfirmNewImage();
#endif

	/* Initialize CHIP server */
#if CONFIG_CHIP_FACTORY_DATA
	ReturnErrorOnFailure(mFactoryDataProvider.Init());
	SetDeviceInstanceInfoProvider(&mFactoryDataProvider);
	SetDeviceAttestationCredentialsProvider(&mFactoryDataProvider);
	SetCommissionableDataProvider(&mFactoryDataProvider);
#else
	SetDeviceInstanceInfoProvider(&DeviceInstanceInfoProviderMgrImpl());
	SetDeviceAttestationCredentialsProvider(Examples::GetExampleDACProvider());
#endif

	static chip::CommonCaseDeviceServerInitParams initParams;
	(void)initParams.InitializeStaticResourcesBeforeServerInit();

	ReturnErrorOnFailure(chip::Server::GetInstance().Init(initParams));
	ConfigurationMgr().LogDeviceConfig();
	PrintOnboardingCodes(chip::RendezvousInformationFlags(chip::RendezvousInformationFlag::kBLE));
	AppFabricTableDelegate::Init();

#ifdef CONFIG_BRIDGED_DEVICE_BT
	/* Initialize BLE Connectivity Manager before the Bridge Manager, as it must be ready to recover devices loaded
	 * from persistent storaged during the bridge init. */
	err = BLEConnectivityManager::Instance().Init(sUuidServices, kUuidServicesNumber);
	if (err != CHIP_NO_ERROR) {
		LOG_ERR("BLEConnectivityManager initialization failed");
		return err;
	}
#endif

	/* Initialize bridge manager */
	err = BridgeManager::Instance().Init(RestoreBridgedDevices);
	if (err != CHIP_NO_ERROR) {
		LOG_ERR("BridgeManager initialization failed");
		return err;
	}

	/*
	 * Add CHIP event handler and start CHIP thread.
	 * Note that all the initialization code should happen prior to this point to avoid data races
	 * between the main and the CHIP threads.
	 */
	PlatformMgr().AddEventHandler(ChipEventHandler, 0);

	err = PlatformMgr().StartEventLoopTask();
	if (err != CHIP_NO_ERROR) {
		LOG_ERR("PlatformMgr().StartEventLoopTask() failed");
		return err;
	}

	return CHIP_NO_ERROR;
}

CHIP_ERROR AppTask::StartApp()
{
	ReturnErrorOnFailure(Init());

	while (true) {
		TaskExecutor::DispatchNextTask();
	}

	return CHIP_NO_ERROR;
}

void AppTask::ChipEventHandler(const ChipDeviceEvent *event, intptr_t /* arg */)
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
		isNetworkProvisioned = ConnectivityMgr().IsWiFiStationProvisioned() && ConnectivityMgr().IsWiFiStationEnabled();
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
