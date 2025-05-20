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

using namespace Nrf;

namespace
{
/* Maps single Matter device type to corresponding BLE service. */
CHIP_ERROR MatterDeviceTypeToBleService(MatterBridgedDevice::DeviceType deviceType,
					BleBridgedDeviceFactory::ServiceUuid &serviceUUid)
{
	switch (deviceType) {
	case MatterBridgedDevice::DeviceType::OnOffLight:
	case MatterBridgedDevice::DeviceType::GenericSwitch:
	case MatterBridgedDevice::DeviceType::OnOffLightSwitch:
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
#ifdef CONFIG_BRIDGE_ONOFF_LIGHT_SWITCH_BRIDGED_DEVICE
		deviceType[1] = MatterBridgedDevice::DeviceType::OnOffLightSwitch;
		count = 2;
#elif defined(CONFIG_BRIDGE_GENERIC_SWITCH_BRIDGED_DEVICE)
		deviceType[1] = MatterBridgedDevice::DeviceType::GenericSwitch;
		count = 2;
#else
		count = 1;
#endif
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
	uint8_t count = 0;
	uint8_t indexes[BridgeManager::kMaxBridgedDevices] = { 0 };
	bool deviceRefresh = false;
	BridgeStorageManager::BridgedDevice bridgedDevice;

	/* Check if a device is already present in the storage. */
	if (BridgeStorageManager::Instance().LoadBridgedDevice(bridgedDevice, index)) {
		deviceRefresh = true;
	}

	BLEBridgedDeviceProvider *bleProvider = static_cast<BLEBridgedDeviceProvider *>(provider);
	bt_addr_le_t addr = bleProvider->GetBtAddress();

	bridgedDevice.mEndpointId = device->GetEndpointId();
	bridgedDevice.mDeviceType = device->GetDeviceType();
	bridgedDevice.mUniqueIDLength = strlen(device->GetUniqueID());
	memcpy(bridgedDevice.mUniqueID, device->GetUniqueID(), bridgedDevice.mUniqueIDLength);
	bridgedDevice.mNodeLabelLength = strlen(device->GetNodeLabel());
	memcpy(bridgedDevice.mNodeLabel, device->GetNodeLabel(), bridgedDevice.mNodeLabelLength);

	/* Fill BT address information as a part of implementation specific user data. */
	bridgedDevice.mUserDataSize = sizeof(addr);
	bridgedDevice.mUserData = reinterpret_cast<uint8_t*>(&addr);

	if (!BridgeStorageManager::Instance().StoreBridgedDevice(bridgedDevice, index)) {
		LOG_ERR("Failed to store bridged device");
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

CHIP_ERROR AddMatterDevices(MatterBridgedDevice::DeviceType deviceTypes[], uint8_t count, const char *uniqueID,
			    const char *nodeLabel, BridgedDeviceDataProvider *provider, uint8_t indexes[] = nullptr,
			    uint16_t endpointIds[] = nullptr)
{
	VerifyOrReturnError(provider != nullptr, CHIP_ERROR_INVALID_ARGUMENT, LOG_ERR("No valid data provider!"));
	chip::Platform::UniquePtr<BridgedDeviceDataProvider> providerPtr(provider);
	VerifyOrReturnError(count <= BridgeManager::kMaxBridgedDevicesPerProvider, CHIP_ERROR_BUFFER_TOO_SMALL,
			    LOG_ERR("Trying to add too many endpoints for single provider device."));

	CHIP_ERROR err = CHIP_NO_ERROR;

	MatterBridgedDevice *newBridgedDevices[BridgeManager::kMaxBridgedDevicesPerProvider];
	uint8_t deviceIndexes[BridgeManager::kMaxBridgedDevicesPerProvider];
	bool forceIndexesAndEndpoints = false;

	if (indexes && endpointIds) {
		forceIndexesAndEndpoints = true;
	}

	uint8_t addedDevicesCount = 0;
	for (; addedDevicesCount < count; addedDevicesCount++) {
		newBridgedDevices[addedDevicesCount] = BleBridgedDeviceFactory::GetBridgedDeviceFactory().Create(
			deviceTypes[addedDevicesCount], uniqueID, nodeLabel);

		if (!newBridgedDevices[addedDevicesCount]) {
			LOG_ERR("Cannot allocate Matter device of given type");
			err = CHIP_ERROR_NO_MEMORY;
			break;
		}

		if (forceIndexesAndEndpoints) {
			deviceIndexes[addedDevicesCount] = indexes[addedDevicesCount];
		}
	}

	/* Not all requested devices were created successfully, delete all previously created objects and return. */
	if (err != CHIP_NO_ERROR) {
		for (uint8_t i = 0; i < addedDevicesCount; i++) {
			chip::Platform::Delete(newBridgedDevices[i]);
		}
		return err;
	}

	if (forceIndexesAndEndpoints) {
		err = BridgeManager::Instance().AddBridgedDevices(newBridgedDevices, providerPtr.get(), count,
								  deviceIndexes, endpointIds);
	} else {
		err = BridgeManager::Instance().AddBridgedDevices(newBridgedDevices, providerPtr.get(), count,
								  deviceIndexes);
	}

	if (err == CHIP_NO_ERROR) {
		for (uint8_t i = 0; i < count; i++) {
			err = StoreDevice(newBridgedDevices[i], providerPtr.get(), deviceIndexes[i]);
			if (err != CHIP_NO_ERROR) {
				break;
			}
		}
	}

	/* The ownership was transferred unconditionally to the BridgeManager, release the pointer. */
	providerPtr.release();

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
	char uniqueID[MatterBridgedDevice::kUniqueIDSize] = { 0 };
	char nodeLabel[MatterBridgedDevice::kNodeLabelSize] = { 0 };
	BLEBridgedDeviceProvider *provider;
	bt_addr_le_t address;
};

CHIP_ERROR BluetoothDeviceConnected(bool success, void *context)
{
	if (!context) {
		return CHIP_ERROR_INVALID_ARGUMENT;
	}

	BluetoothConnectionContext *ctx = reinterpret_cast<BluetoothConnectionContext *>(context);

	if (!success) {
		chip::Platform::Delete(ctx->provider);
		chip::Platform::Delete(ctx);
		return CHIP_ERROR_INTERNAL;
	}

	/* Discovery was successful, try to bridge connected BLE device with the Matter counterpart. */
	chip::Optional<uint8_t> indexes[BridgeManager::kMaxBridgedDevicesPerProvider];
	chip::Optional<uint16_t> endpointIds[BridgeManager::kMaxBridgedDevicesPerProvider];
	/* AddMatterDevices takes the ownership of the passed provider object and will
	   delete it in case the BridgeManager fails to accept this object. */
	CHIP_ERROR err = AddMatterDevices(ctx->deviceTypes, ctx->count, ctx->uniqueID, ctx->nodeLabel, ctx->provider);
	chip::Platform::Delete(ctx);

	return err;
}
} // namespace

BleBridgedDeviceFactory::BridgedDeviceFactory &BleBridgedDeviceFactory::GetBridgedDeviceFactory()
{
	auto checkUniqueID = [](const char *uniqueID) {
		/* If node uniqueID is provided it must fit the maximum defined length */
		if (!uniqueID || (uniqueID && (strlen(uniqueID) < Nrf::MatterBridgedDevice::kUniqueIDSize))) {
			return true;
		}
		return false;
	};

	auto checkLabel = [](const char *nodeLabel) {
		/* If node label is provided it must fit the maximum defined length */
		if (!nodeLabel || (nodeLabel && (strlen(nodeLabel) < MatterBridgedDevice::kNodeLabelSize))) {
			return true;
		}
		return false;
	};

	static BridgedDeviceFactory sBridgedDeviceFactory{
#ifdef CONFIG_BRIDGE_HUMIDITY_SENSOR_BRIDGED_DEVICE
		{ MatterBridgedDevice::DeviceType::HumiditySensor,
		  [checkUniqueID, checkLabel](const char *uniqueID,
					      const char *nodeLabel) -> Nrf::MatterBridgedDevice * {
			  if (!checkUniqueID(uniqueID) || !checkLabel(nodeLabel)) {
				  return nullptr;
			  }
			  return chip::Platform::New<HumiditySensorDevice>(uniqueID, nodeLabel);
		  } },
#endif
#ifdef CONFIG_BRIDGE_ONOFF_LIGHT_BRIDGED_DEVICE
		{ MatterBridgedDevice::DeviceType::OnOffLight,
		  [checkUniqueID, checkLabel](const char *uniqueID,
					      const char *nodeLabel) -> Nrf::MatterBridgedDevice * {
			  if (!checkUniqueID(uniqueID) || !checkLabel(nodeLabel)) {
				  return nullptr;
			  }
			  return chip::Platform::New<OnOffLightDevice>(uniqueID, nodeLabel);
		  } },
#endif
#ifdef CONFIG_BRIDGE_TEMPERATURE_SENSOR_BRIDGED_DEVICE
		{ MatterBridgedDevice::DeviceType::TemperatureSensor,
		  [checkUniqueID, checkLabel](const char *uniqueID,
					      const char *nodeLabel) -> Nrf::MatterBridgedDevice * {
			  if (!checkUniqueID(uniqueID) || !checkLabel(nodeLabel)) {
				  return nullptr;
			  }
			  return chip::Platform::New<TemperatureSensorDevice>(uniqueID, nodeLabel);
		  } },
#endif
#ifdef CONFIG_BRIDGE_GENERIC_SWITCH_BRIDGED_DEVICE
		{ MatterBridgedDevice::DeviceType::GenericSwitch,
		  [checkUniqueID, checkLabel](const char *uniqueID,
					      const char *nodeLabel) -> Nrf::MatterBridgedDevice * {
			  if (!checkUniqueID(uniqueID) || !checkLabel(nodeLabel)) {
				  return nullptr;
			  }
			  return chip::Platform::New<GenericSwitchDevice>(uniqueID, nodeLabel);
		  } },
#endif
#ifdef CONFIG_BRIDGE_ONOFF_LIGHT_SWITCH_BRIDGED_DEVICE
		{ MatterBridgedDevice::DeviceType::OnOffLightSwitch,
		  [checkUniqueID, checkLabel](const char *uniqueID,
					      const char *nodeLabel) -> Nrf::MatterBridgedDevice * {
			  if (!checkUniqueID(uniqueID) || !checkLabel(nodeLabel)) {
				  return nullptr;
			  }
			  return chip::Platform::New<OnOffLightSwitchDevice>(uniqueID, nodeLabel);
		  } },
#endif
	};
	return sBridgedDeviceFactory;
}

BleBridgedDeviceFactory::BleDataProviderFactory &BleBridgedDeviceFactory::GetDataProviderFactory()
{
	static BleDataProviderFactory sDeviceDataProvider
	{
#if defined(CONFIG_BRIDGE_ONOFF_LIGHT_BRIDGED_DEVICE) && (defined(CONFIG_BRIDGE_GENERIC_SWITCH_BRIDGED_DEVICE) ||      \
							  defined(CONFIG_BRIDGE_ONOFF_LIGHT_SWITCH_BRIDGED_DEVICE))
		{ ServiceUuid::LedButtonService,
		  [](UpdateAttributeCallback updateClb, InvokeCommandCallback commandClb) {
			  return chip::Platform::New<BleLBSDataProvider>(updateClb, commandClb);
		  } },
#endif
#if defined(CONFIG_BRIDGE_TEMPERATURE_SENSOR_BRIDGED_DEVICE) && defined(CONFIG_BRIDGE_HUMIDITY_SENSOR_BRIDGED_DEVICE)
			{ ServiceUuid::EnvironmentalSensorService,
			  [](UpdateAttributeCallback updateClb, InvokeCommandCallback commandClb) {
				  return chip::Platform::New<BleEnvironmentalDataProvider>(updateClb, commandClb);
			  } },
#endif
	};
	return sDeviceDataProvider;
}

CHIP_ERROR BleBridgedDeviceFactory::CreateDevice(int deviceType, bt_addr_le_t btAddress, const char *uniqueID,
						 const char *nodeLabel, uint8_t index, uint16_t endpointId)
{
	CHIP_ERROR err;

	/* Check if there is already existing provider for given address.
	 * In case it is, the provider was either connected or recovered before. All what needs to be done is creating
	 * new Matter endpoint. In case there isn't, the provider has to be created.
	 */
	BLEBridgedDeviceProvider *provider = BLEConnectivityManager::Instance().FindBLEProvider(btAddress);
	if (provider) {
		return AddMatterDevices(reinterpret_cast<MatterBridgedDevice::DeviceType *>(&deviceType), 1, uniqueID,
					nodeLabel, provider, &index, &endpointId);
	}

	ServiceUuid providerType;
	err = MatterDeviceTypeToBleService(static_cast<MatterBridgedDevice::DeviceType>(deviceType), providerType);
	VerifyOrExit(err == CHIP_NO_ERROR, );

	provider = static_cast<BLEBridgedDeviceProvider *>(BleBridgedDeviceFactory::GetDataProviderFactory().Create(
		providerType, BridgeManager::HandleUpdate, BridgeManager::HandleCommand));
	if (!provider) {
		return CHIP_ERROR_NO_MEMORY;
	}

	err = BLEConnectivityManager::Instance().AddBLEProvider(provider);
	VerifyOrExit(err == CHIP_NO_ERROR, );

	/* Confirm that the first connection was done and this will be only the device recovery. */
	provider->ConfirmInitialConnection();
	provider->InitializeBridgedDevice(btAddress, nullptr, nullptr);
	err = AddMatterDevices(reinterpret_cast<MatterBridgedDevice::DeviceType *>(&deviceType), 1, uniqueID, nodeLabel,
			       provider, &index, &endpointId);

	if (err != CHIP_NO_ERROR) {
		return err;
	}

	BLEConnectivityManager::Instance().Recover(provider);

exit:
	if (err != CHIP_NO_ERROR) {
		chip::Platform::Delete(provider);
	}

	return err;
}

CHIP_ERROR BleBridgedDeviceFactory::CreateDevice(uint16_t uuid, bt_addr_le_t btAddress, const char *uniqueID,
						 const char *nodeLabel,
						 BLEConnectivityManager::ConnectionSecurityRequest *request)
{
	/* Check if there is already existing provider for given address.
	 * In case it is, the provider was either connected or recovered before.
	 */
	BLEBridgedDeviceProvider *provider = BLEConnectivityManager::Instance().FindBLEProvider(btAddress);
	if (provider) {
		return CHIP_ERROR_INCORRECT_STATE;
	}

	provider = static_cast<BLEBridgedDeviceProvider *>(BleBridgedDeviceFactory::GetDataProviderFactory().Create(
		static_cast<ServiceUuid>(uuid), BridgeManager::HandleUpdate, BridgeManager::HandleCommand));
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

	if (uniqueID) {
		strcpy(contextPtr->uniqueID, uniqueID);
	}

	if (nodeLabel) {
		strcpy(contextPtr->nodeLabel, nodeLabel);
	}

	contextPtr->provider = provider;
	contextPtr->address = btAddress;

	provider->InitializeBridgedDevice(btAddress, BluetoothDeviceConnected, contextPtr.get());
	err = BLEConnectivityManager::Instance().Connect(provider, request);

	if (err == CHIP_NO_ERROR) {
		contextPtr.release();
	}

exit:
	if (err != CHIP_NO_ERROR) {
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

	if (!BridgeStorageManager::Instance().RemoveBridgedDevice(index)) {
		LOG_ERR("Failed to remove bridged device from the storage.");
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
