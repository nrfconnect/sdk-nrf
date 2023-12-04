/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "ble_bridged_device_factory.h"
#include "bridge_manager.h"
#include "bridge_storage_manager.h"

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

namespace
{
/* Maps single Matter device type to corresponding BLE service. */
CHIP_ERROR MatterDeviceTypeToBleService(MatterBridgedDevice::DeviceType deviceType,
					BleBridgedDeviceFactory::ServiceUuid &serviceUUid)
{
	switch (deviceType) {
	case MatterBridgedDevice::DeviceType::OnOffLight:
	case MatterBridgedDevice::DeviceType::GenericSwitch:
		serviceUUid = BleBridgedDeviceFactory::ServiceUuid::LedButtonService;
		break;
	case MatterBridgedDevice::DeviceType::TemperatureSensor:
	case MatterBridgedDevice::DeviceType::HumiditySensor:
		serviceUUid = BleBridgedDeviceFactory::ServiceUuid::EnvironmentalSensorService;
		break;
	default:
		return CHIP_ERROR_INVALID_ARGUMENT;
	}
	return CHIP_NO_ERROR;
}

/* Maps single BLE service to one or multiple Matter device types. */
CHIP_ERROR BleServiceToMatterDeviceType(BleBridgedDeviceFactory::ServiceUuid serviceUUid,
					MatterBridgedDevice::DeviceType deviceType[], uint8_t maxCount, uint8_t &count)
{
	switch (serviceUUid) {
	case BleBridgedDeviceFactory::ServiceUuid::LedButtonService: {
		if (maxCount == 0) {
			return CHIP_ERROR_BUFFER_TOO_SMALL;
		}
		deviceType[0] = MatterBridgedDevice::DeviceType::OnOffLight;
		deviceType[1] = MatterBridgedDevice::DeviceType::GenericSwitch;
		count = 2;
	} break;
	case BleBridgedDeviceFactory::ServiceUuid::EnvironmentalSensorService: {
		if (maxCount < 2) {
			return CHIP_ERROR_BUFFER_TOO_SMALL;
		}
		deviceType[0] = MatterBridgedDevice::DeviceType::TemperatureSensor;
		deviceType[1] = MatterBridgedDevice::DeviceType::HumiditySensor;
		count = 2;
	} break;
	default:
		return CHIP_ERROR_INVALID_ARGUMENT;
	}
	return CHIP_NO_ERROR;
}

CHIP_ERROR StoreDevice(MatterBridgedDevice *device, BridgedDeviceDataProvider *provider, uint8_t index)
{
	uint16_t endpointId;
	uint8_t count = 0;
	uint8_t indexes[BridgeManager::kMaxBridgedDevices] = { 0 };
	bool deviceRefresh = false;

	/* Check if a device is already present in the storage. */
	if (BridgeStorageManager::Instance().LoadBridgedDeviceEndpointId(endpointId, index)) {
		deviceRefresh = true;
	}

	if (!BridgeStorageManager::Instance().StoreBridgedDevice(device, index)) {
		LOG_ERR("Failed to store bridged device");
		return CHIP_ERROR_INTERNAL;
	}

	BLEBridgedDeviceProvider *bleProvider = static_cast<BLEBridgedDeviceProvider *>(provider);

	bt_addr_le_t addr = bleProvider->GetBtAddress();

	if (!BridgeStorageManager::Instance().StoreBtAddress(addr, index)) {
		LOG_ERR("Failed to store bridged device's Bluetooth address");
		return CHIP_ERROR_INTERNAL;
	}

	/* If a device was not present in the storage before, put new index on the end of list and increment the count
	 * number of stored devices. */
	if (!deviceRefresh) {
		CHIP_ERROR err = BridgeManager::Instance().GetDevicesIndexes(indexes, sizeof(indexes), count);
		if (CHIP_NO_ERROR != err) {
			LOG_ERR("Failed to get bridged devices indexes");
			return err;
		}

		if (!BridgeStorageManager::Instance().StoreBridgedDevicesIndexes(indexes, count)) {
			LOG_ERR("Failed to store bridged devices indexes.");
			return CHIP_ERROR_INTERNAL;
		}

		if (!BridgeStorageManager::Instance().StoreBridgedDevicesCount(count)) {
			LOG_ERR("Failed to store bridged devices count.");
			return CHIP_ERROR_INTERNAL;
		}
	}

	return CHIP_NO_ERROR;
}

CHIP_ERROR AddMatterDevices(MatterBridgedDevice::DeviceType deviceTypes[], uint8_t count, const char *nodeLabel,
			    BridgedDeviceDataProvider *provider, uint8_t indexes[] = nullptr,
			    uint16_t endpointIds[] = nullptr)
{
	VerifyOrReturnError(provider != nullptr, CHIP_ERROR_INVALID_ARGUMENT, LOG_ERR("No valid data provider!"));
	VerifyOrReturnError(count <= BridgeManager::kMaxBridgedDevicesPerProvider, CHIP_ERROR_BUFFER_TOO_SMALL,
			    LOG_ERR("Trying to add too many endpoints for single provider device."));

	CHIP_ERROR err;

	MatterBridgedDevice *newBridgedDevices[BridgeManager::kMaxBridgedDevicesPerProvider];
	uint8_t deviceIndexes[BridgeManager::kMaxBridgedDevicesPerProvider];
	bool forceIndexesAndEndpoints = false;

	if (indexes && endpointIds) {
		forceIndexesAndEndpoints = true;
	}

	for (uint8_t i = 0; i < count; i++) {
		newBridgedDevices[i] =
			BleBridgedDeviceFactory::GetBridgedDeviceFactory().Create(deviceTypes[i], nodeLabel);

		if (!newBridgedDevices[i]) {
			LOG_ERR("Cannot allocate Matter device of given type");
			return CHIP_ERROR_NO_MEMORY;
		}

		if (forceIndexesAndEndpoints) {
			deviceIndexes[i] = indexes[i];
		}
	}

	if (forceIndexesAndEndpoints) {
		err = BridgeManager::Instance().AddBridgedDevices(newBridgedDevices, provider, count, deviceIndexes,
								  endpointIds);
	} else {
		err = BridgeManager::Instance().AddBridgedDevices(newBridgedDevices, provider, count, deviceIndexes);
	}

	if (err == CHIP_NO_ERROR) {
		for (uint8_t i = 0; i < count; i++) {
			err = StoreDevice(newBridgedDevices[i], provider, deviceIndexes[i]);
			if (err != CHIP_NO_ERROR) {
				break;
			}
		}
	}

	if (err != CHIP_NO_ERROR) {
		LOG_ERR("Adding Matter bridged device failed: %s", ErrorStr(err));
	} else {
		for (auto i = 0; i < count; i++) {
			LOG_INF("Created 0x%x device type on the endpoint %d", newBridgedDevices[i]->GetDeviceType(),
				newBridgedDevices[i]->GetEndpointId());
		}
	}
	return err;
}

struct BluetoothConnectionContext {
	MatterBridgedDevice::DeviceType deviceTypes[BridgeManager::kMaxBridgedDevicesPerProvider];
	uint8_t count;
	char nodeLabel[MatterBridgedDevice::kNodeLabelSize] = { 0 };
	BLEBridgedDeviceProvider *provider;
	bt_addr_le_t address;
};

void BluetoothDeviceConnected(bool discoverySucceeded, void *context)
{
	if (!context) {
		return;
	}

	BluetoothConnectionContext *ctx = reinterpret_cast<BluetoothConnectionContext *>(context);

	VerifyOrExit(discoverySucceeded, );

	/* Schedule adding device to main thread to avoid overflowing the BT stack. */
	VerifyOrExit(chip::DeviceLayer::PlatformMgr().ScheduleWork(
			     [](intptr_t context) {
				     BluetoothConnectionContext *ctx =
					     reinterpret_cast<BluetoothConnectionContext *>(context);
				     chip::Optional<uint8_t> indexes[BridgeManager::kMaxBridgedDevicesPerProvider];
				     chip::Optional<uint16_t> endpointIds[BridgeManager::kMaxBridgedDevicesPerProvider];
				     if (CHIP_NO_ERROR != AddMatterDevices(ctx->deviceTypes, ctx->count, ctx->nodeLabel,
									   ctx->provider)) {
					     BLEConnectivityManager::Instance().RemoveBLEProvider(ctx->address);
					     chip::Platform::Delete(ctx->provider);
				     }
				     chip::Platform::Delete(ctx);
			     },
			     reinterpret_cast<intptr_t>(ctx)) == CHIP_NO_ERROR, );
	return;

exit:
	BLEConnectivityManager::Instance().RemoveBLEProvider(ctx->address);
	chip::Platform::Delete(ctx);
}
} // namespace

BleBridgedDeviceFactory::BridgedDeviceFactory &BleBridgedDeviceFactory::GetBridgedDeviceFactory()
{
	auto checkLabel = [](const char *nodeLabel) {
		/* If node label is provided it must fit the maximum defined length */
		if (!nodeLabel || (nodeLabel && (strlen(nodeLabel) < MatterBridgedDevice::kNodeLabelSize))) {
			return true;
		}
		return false;
	};

	static BridgedDeviceFactory sBridgedDeviceFactory{
#ifdef CONFIG_BRIDGE_HUMIDITY_SENSOR_BRIDGED_DEVICE
		{ DeviceType::HumiditySensor,
		  [checkLabel](const char *nodeLabel) -> MatterBridgedDevice * {
			  if (!checkLabel(nodeLabel)) {
				  return nullptr;
			  }
			  return chip::Platform::New<HumiditySensorDevice>(nodeLabel);
		  } },
#endif
#ifdef CONFIG_BRIDGE_ONOFF_LIGHT_BRIDGED_DEVICE
		{ DeviceType::OnOffLight,
		  [checkLabel](const char *nodeLabel) -> MatterBridgedDevice * {
			  if (!checkLabel(nodeLabel)) {
				  return nullptr;
			  }
			  return chip::Platform::New<OnOffLightDevice>(nodeLabel);
		  } },
#endif
#ifdef CONFIG_BRIDGE_TEMPERATURE_SENSOR_BRIDGED_DEVICE
		{ MatterBridgedDevice::DeviceType::TemperatureSensor,
		  [checkLabel](const char *nodeLabel) -> MatterBridgedDevice * {
			  if (!checkLabel(nodeLabel)) {
				  return nullptr;
			  }
			  return chip::Platform::New<TemperatureSensorDevice>(nodeLabel);
		  } },
#endif
#ifdef CONFIG_BRIDGE_GENERIC_SWITCH_BRIDGED_DEVICE
		{ MatterBridgedDevice::DeviceType::GenericSwitch,
		  [checkLabel](const char *nodeLabel) -> MatterBridgedDevice * {
			  if (!checkLabel(nodeLabel)) {
				  return nullptr;
			  }
			  return chip::Platform::New<GenericSwitchDevice>(nodeLabel);
		  } },
#endif
	};
	return sBridgedDeviceFactory;
}

BleBridgedDeviceFactory::BleDataProviderFactory &BleBridgedDeviceFactory::GetDataProviderFactory()
{
	static BleDataProviderFactory sDeviceDataProvider
	{
#if defined(CONFIG_BRIDGE_ONOFF_LIGHT_BRIDGED_DEVICE) && defined(CONFIG_BRIDGE_GENERIC_SWITCH_BRIDGED_DEVICE)
		{ ServiceUuid::LedButtonService,
		  [](UpdateAttributeCallback clb) { return chip::Platform::New<BleLBSDataProvider>(clb); } },
#endif
#if defined(CONFIG_BRIDGE_TEMPERATURE_SENSOR_BRIDGED_DEVICE) && defined(CONFIG_BRIDGE_HUMIDITY_SENSOR_BRIDGED_DEVICE)
			{ ServiceUuid::EnvironmentalSensorService, [](UpdateAttributeCallback clb) {
				 return chip::Platform::New<BleEnvironmentalDataProvider>(clb);
			 } },
#endif
	};
	return sDeviceDataProvider;
}

CHIP_ERROR BleBridgedDeviceFactory::CreateDevice(int deviceType, bt_addr_le_t btAddress, const char *nodeLabel,
						 uint8_t index, uint16_t endpointId)
{
	CHIP_ERROR err;

	/* Check if there is already existing provider for given address.
	 * In case it is, the provider was either connected or recovered before. All what needs to be done is creating
	 * new Matter endpoint. In case there isn't, the provider has to be created.
	 */
	BLEBridgedDeviceProvider *provider = BLEConnectivityManager::Instance().FindBLEProvider(btAddress);
	if (provider) {
		return AddMatterDevices(reinterpret_cast<MatterBridgedDevice::DeviceType *>(&deviceType), 1, nodeLabel,
					provider, &index, &endpointId);
	}

	ServiceUuid providerType;
	err = MatterDeviceTypeToBleService(static_cast<MatterBridgedDevice::DeviceType>(deviceType), providerType);
	VerifyOrExit(err == CHIP_NO_ERROR, );

	provider = static_cast<BLEBridgedDeviceProvider *>(
		BleBridgedDeviceFactory::GetDataProviderFactory().Create(providerType, BridgeManager::HandleUpdate));
	if (!provider) {
		return CHIP_ERROR_NO_MEMORY;
	}

	err = BLEConnectivityManager::Instance().AddBLEProvider(provider);
	VerifyOrExit(err == CHIP_NO_ERROR, );

	/* Confirm that the first connection was done and this will be only the device recovery. */
	provider->ConfirmInitialConnection();
	provider->InitializeBridgedDevice(btAddress, nullptr, nullptr);
	BLEConnectivityManager::Instance().Recover(provider);
	err = AddMatterDevices(reinterpret_cast<MatterBridgedDevice::DeviceType *>(&deviceType), 1, nodeLabel, provider,
			       &index, &endpointId);
exit:

	if (err != CHIP_NO_ERROR) {
		BLEConnectivityManager::Instance().RemoveBLEProvider(btAddress);
		chip::Platform::Delete(provider);
	}

	return err;
}

CHIP_ERROR BleBridgedDeviceFactory::CreateDevice(uint16_t uuid, bt_addr_le_t btAddress, const char *nodeLabel)
{
	BLEBridgedDeviceProvider *provider =
		static_cast<BLEBridgedDeviceProvider *>(BleBridgedDeviceFactory::GetDataProviderFactory().Create(
			static_cast<ServiceUuid>(uuid), BridgeManager::HandleUpdate));
	if (!provider) {
		return CHIP_ERROR_NO_MEMORY;
	}

	MatterBridgedDevice::DeviceType deviceTypes[BridgeManager::kMaxBridgedDevicesPerProvider];
	uint8_t count;

	/* The device object can be created once the Bluetooth LE connection will be established. */
	BluetoothConnectionContext *context = chip::Platform::New<BluetoothConnectionContext>();
	chip::Platform::UniquePtr<BluetoothConnectionContext> contextPtr(context);

	CHIP_ERROR err = BleServiceToMatterDeviceType(static_cast<ServiceUuid>(uuid), deviceTypes,
						      BridgeManager::kMaxBridgedDevicesPerProvider, count);
	VerifyOrExit(err == CHIP_NO_ERROR, );

	err = BLEConnectivityManager::Instance().AddBLEProvider(provider);
	VerifyOrExit(err == CHIP_NO_ERROR, );
	VerifyOrExit(context, err = CHIP_ERROR_NO_MEMORY);

	contextPtr->count = count;
	for (uint8_t i = 0; i < count; i++) {
		contextPtr->deviceTypes[i] = deviceTypes[i];
	}

	if (nodeLabel) {
		strcpy(contextPtr->nodeLabel, nodeLabel);
	}

	contextPtr->provider = provider;
	contextPtr->address = btAddress;

	provider->InitializeBridgedDevice(btAddress, BluetoothDeviceConnected, contextPtr.get());
	err = BLEConnectivityManager::Instance().Connect(provider);

	if (err == CHIP_NO_ERROR) {
		contextPtr.release();
	}

exit:

	if (err != CHIP_NO_ERROR) {
		BLEConnectivityManager::Instance().RemoveBLEProvider(btAddress);
		chip::Platform::Delete(provider);
	}

	return err;
}

CHIP_ERROR BleBridgedDeviceFactory::RemoveDevice(int endpointId)
{
	uint8_t index;
	uint8_t count = 0;
	uint8_t indexes[BridgeManager::kMaxBridgedDevices] = { 0 };

	CHIP_ERROR err = BridgeManager::Instance().RemoveBridgedDevice(endpointId, index);

	if (CHIP_NO_ERROR != err) {
		LOG_ERR("Failed to remove bridged device");
		return err;
	}

	err = BridgeManager::Instance().GetDevicesIndexes(indexes, sizeof(indexes), count);
	if (CHIP_NO_ERROR != err) {
		LOG_ERR("Failed to get bridged devices indexes");
		return err;
	}

	/* Update the current indexes list. */
	if (!BridgeStorageManager::Instance().StoreBridgedDevicesIndexes(indexes, count)) {
		LOG_ERR("Failed to store bridged devices indexes.");
		return CHIP_ERROR_INTERNAL;
	}

	if (!BridgeStorageManager::Instance().StoreBridgedDevicesCount(count)) {
		LOG_ERR("Failed to store bridged devices count.");
		return CHIP_ERROR_INTERNAL;
	}

	if (!BridgeStorageManager::Instance().RemoveBridgedDeviceEndpointId(index)) {
		LOG_ERR("Failed to remove bridged device endpoint id.");
		return CHIP_ERROR_INTERNAL;
	}

	/* Ignore error, as node label may not be present in the storage. */
	BridgeStorageManager::Instance().RemoveBridgedDeviceNodeLabel(index);

	if (!BridgeStorageManager::Instance().RemoveBridgedDeviceType(index)) {
		LOG_ERR("Failed to remove bridged device type.");
		return CHIP_ERROR_INTERNAL;
	}

	if (!BridgeStorageManager::Instance().RemoveBtAddress(index)) {
		LOG_ERR("Failed to remove bridged device Bluetooth address.");
		return CHIP_ERROR_INTERNAL;
	}

	return CHIP_NO_ERROR;
}

const char *BleBridgedDeviceFactory::GetUuidString(uint16_t uuid)
{
	switch (uuid) {
	case ServiceUuid::LedButtonService:
		return "Led Button Service";
	case ServiceUuid::EnvironmentalSensorService:
		return "Environmental Sensing Service";
	default:
		return "Unknown";
	}
}
