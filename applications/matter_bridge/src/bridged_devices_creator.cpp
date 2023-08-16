/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "bridged_devices_creator.h"
#include "bridge_manager.h"
#include "bridge_storage_manager.h"
#include "bridged_device_factory.h"

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

namespace
{
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

/* Store additional information that are not present for every generic MatterBridgedDevice. */
#ifdef CONFIG_BRIDGED_DEVICE_BT
	BLEBridgedDeviceProvider *bleProvider = static_cast<BLEBridgedDeviceProvider *>(provider);

	bt_addr_le_t addr = bleProvider->GetBtAddress();

	if (!BridgeStorageManager::Instance().StoreBtAddress(addr, index)) {
		LOG_ERR("Failed to store bridged device's Bluetooth address");
		return CHIP_ERROR_INTERNAL;
	}
#endif

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

CHIP_ERROR AddMatterDevice(int deviceType, const char *nodeLabel, BridgedDeviceDataProvider *provider,
			   chip::Optional<uint8_t> index, chip::Optional<uint16_t> endpointId)
{
	VerifyOrReturnError(provider != nullptr, CHIP_ERROR_INVALID_ARGUMENT, LOG_ERR("No valid data provider!"));

	CHIP_ERROR err;

	if (deviceType == MatterBridgedDevice::DeviceType::EnvironmentalSensor) {
		/* Handle special case for environmental sensor */
		auto *temperatureBridgedDevice = BridgeFactory::GetBridgedDeviceFactory().Create(
			MatterBridgedDevice::DeviceType::TemperatureSensor, nodeLabel);

		if (!temperatureBridgedDevice) {
			LOG_ERR("Cannot allocate Matter device of given type");
			return CHIP_ERROR_NO_MEMORY;
		}

		auto *humidityBridgedDevice = BridgeFactory::GetBridgedDeviceFactory().Create(
			MatterBridgedDevice::DeviceType::HumiditySensor, nodeLabel);

		if (!humidityBridgedDevice) {
			chip::Platform::Delete(temperatureBridgedDevice);
			LOG_ERR("Cannot allocate Matter device of given type");
			return CHIP_ERROR_NO_MEMORY;
		}

		MatterBridgedDevice *newBridgedDevices[] = { temperatureBridgedDevice, humidityBridgedDevice };
		uint8_t deviceIndexes[] = { 0, 0 };
		err = BridgeManager::Instance().AddBridgedDevices(newBridgedDevices, provider,
								  ARRAY_SIZE(newBridgedDevices), deviceIndexes);

		if (err == CHIP_NO_ERROR) {
			for (uint8_t i = 0; i < ARRAY_SIZE(newBridgedDevices); i++) {
				err = StoreDevice(newBridgedDevices[i], provider, deviceIndexes[i]);
				if (err != CHIP_NO_ERROR) {
					break;
				}
			}
		}
	} else {
		auto *newBridgedDevice = BridgeFactory::GetBridgedDeviceFactory().Create(
			static_cast<MatterBridgedDevice::DeviceType>(deviceType), nodeLabel);

		MatterBridgedDevice *newBridgedDevices[] = { newBridgedDevice };

		if (!newBridgedDevice) {
			LOG_ERR("Cannot allocate Matter device of given type");
			return CHIP_ERROR_NO_MEMORY;
		}

		uint8_t deviceIndex[] = { 0 };
		uint16_t endpointIds[] = { 0 };

		/* The current implementation assumes that index and endpoint id values are given only in case of
		 * recovering a device. In such scenario the devices (endpoints) are recovered one by one, so it's not
		 * an option for environmental sensor. */
		if (index.HasValue() && endpointId.HasValue()) {
			endpointIds[0] = endpointId.Value();
			deviceIndex[0] = index.Value();
			err = BridgeManager::Instance().AddBridgedDevices(
				newBridgedDevices, provider, ARRAY_SIZE(newBridgedDevices), deviceIndex, endpointIds);
		} else {
			err = BridgeManager::Instance().AddBridgedDevices(newBridgedDevices, provider,
									  ARRAY_SIZE(newBridgedDevices), deviceIndex);
		}

		if (err == CHIP_NO_ERROR) {
			err = StoreDevice(newBridgedDevice, provider, deviceIndex[0]);
		}
	}

	if (err != CHIP_NO_ERROR) {
		LOG_ERR("Adding Matter bridged device failed: %s", ErrorStr(err));
	}
	return err;
}

#ifdef CONFIG_BRIDGED_DEVICE_SIMULATED
BridgedDeviceDataProvider *CreateSimulatedProvider(int deviceType)
{
	return BridgeFactory::GetSimulatedDataProviderFactory().Create(
		static_cast<MatterBridgedDevice::DeviceType>(deviceType), BridgeManager::HandleUpdate);
}
#endif /* CONFIG_BRIDGED_DEVICE_SIMULATED */

#ifdef CONFIG_BRIDGED_DEVICE_BT

struct BluetoothConnectionContext {
	int deviceType;
	char nodeLabel[MatterBridgedDevice::kNodeLabelSize] = { 0 };
	BLEBridgedDeviceProvider *provider;
	bt_addr_le_t address;
	chip::Optional<uint8_t> index;
	chip::Optional<uint16_t> endpointId;
};

void BluetoothDeviceConnected(bt_gatt_dm *discoveredData, bool discoverySucceeded, void *context)
{
	if (!context) {
		return;
	}

	BluetoothConnectionContext *ctx = reinterpret_cast<BluetoothConnectionContext *>(context);

	VerifyOrExit(discoveredData && discoverySucceeded, );
	VerifyOrExit(ctx->provider->ParseDiscoveredData(discoveredData) == 0, );

	/* Schedule adding device to main thread to avoid overflowing the BT stack. */
	VerifyOrExit(chip::DeviceLayer::PlatformMgr().ScheduleWork(
			     [](intptr_t context) {
				     BluetoothConnectionContext *ctx =
					     reinterpret_cast<BluetoothConnectionContext *>(context);
				     if (CHIP_NO_ERROR != AddMatterDevice(ctx->deviceType, ctx->nodeLabel,
									  ctx->provider, ctx->index, ctx->endpointId)) {
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

BridgedDeviceDataProvider *CreateBleProvider(int deviceType)
{
	return BridgeFactory::GetBleDataProviderFactory().Create(
		static_cast<MatterBridgedDevice::DeviceType>(deviceType), BridgeManager::HandleUpdate);
}
#endif /* CONFIG_BRIDGED_DEVICE_BT */
} /* namespace */

#ifdef CONFIG_BRIDGED_DEVICE_BT
CHIP_ERROR BridgedDeviceCreator::CreateDevice(int deviceType, const char *nodeLabel, bt_addr_le_t btAddress,
					      chip::Optional<uint8_t> index, chip::Optional<uint16_t> endpointId)
{
	CHIP_ERROR err;

	/* Check if there is already existing provider for given address.
	 * In case it is, the provider was either connected or recovered before. All what needs to be done is creating
	 * new Matter endpoint. In case there isn't, the provider has to be created.
	 */
	BLEBridgedDeviceProvider *provider = BLEConnectivityManager::Instance().FindBLEProvider(btAddress);
	if (provider) {
		return AddMatterDevice(deviceType, nodeLabel, provider, index, endpointId);
	}

	/* The EnvironmentalSensor is a dummy device type, so it is not saved in the persistent storage. For now the
	 * only BLE service handling temperature and humidity device types is environmental sensor, so it's necessary to
	 * perform mapping. */
	if (deviceType == MatterBridgedDevice::DeviceType::TemperatureSensor ||
	    deviceType == MatterBridgedDevice::DeviceType::HumiditySensor) {
		provider = static_cast<BLEBridgedDeviceProvider *>(
			CreateBleProvider(MatterBridgedDevice::DeviceType::EnvironmentalSensor));
	} else {
		provider = static_cast<BLEBridgedDeviceProvider *>(CreateBleProvider(deviceType));
	}

	if (!provider) {
		return CHIP_ERROR_NO_MEMORY;
	}

	err = BLEConnectivityManager::Instance().AddBLEProvider(provider);
	VerifyOrExit(err == CHIP_NO_ERROR, );

	/* There are two cases for creating a new device:
	 * - connecting new device for the first time - the index and endpoint id are not relevant, the new Bluetooth LE
	 * connection has to be created and the operation confirmed by connected callback.
	 * - recovering a device that was connected in the past after reboot - the index and endpoint id are read from
	 * storage, the device has to be re-connected without confirming its existence by callback.
	 */
	if (index.HasValue() && endpointId.HasValue()) {
		provider->InitializeBridgedDevice(btAddress, nullptr, nullptr);
		BLEConnectivityManager::Instance().Recover(provider);
		err = AddMatterDevice(deviceType, nodeLabel, provider, index, endpointId);
	} else {
		/* The device object can be created once the Bluetooth LE connection will be established. */
		BluetoothConnectionContext *context = chip::Platform::New<BluetoothConnectionContext>();

		VerifyOrExit(context, err = CHIP_ERROR_NO_MEMORY);

		chip::Platform::UniquePtr<BluetoothConnectionContext> contextPtr(context);
		contextPtr->deviceType = deviceType;

		if (nodeLabel) {
			strcpy(contextPtr->nodeLabel, nodeLabel);
		}

		contextPtr->provider = provider;
		contextPtr->index = index;
		contextPtr->endpointId = endpointId;
		contextPtr->address = btAddress;

		provider->InitializeBridgedDevice(btAddress, BluetoothDeviceConnected, contextPtr.get());
		err = BLEConnectivityManager::Instance().Connect(provider);

		if (err == CHIP_NO_ERROR) {
			contextPtr.release();
		}
	}

exit:

	if (err != CHIP_NO_ERROR) {
		BLEConnectivityManager::Instance().RemoveBLEProvider(btAddress);
		chip::Platform::Delete(provider);
	}

	return err;
}
#else
CHIP_ERROR BridgedDeviceCreator::CreateDevice(int deviceType, const char *nodeLabel, chip::Optional<uint8_t> index,
					      chip::Optional<uint16_t> endpointId)
{
#if defined(CONFIG_BRIDGED_DEVICE_SIMULATED)
	BridgedDeviceDataProvider *provider = CreateSimulatedProvider(deviceType);
	/* The device is simulated, so it can be added immediately. */
	return AddMatterDevice(deviceType, nodeLabel, provider, index, endpointId);
#else
	return CHIP_ERROR_NOT_IMPLEMENTED;
#endif /* CONFIG_BRIDGED_DEVICE_SIMULATED */
}
#endif /* CONFIG_BRIDGED_DEVICE_BT */

CHIP_ERROR BridgedDeviceCreator::RemoveDevice(int endpointId)
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

#ifdef CONFIG_BRIDGED_DEVICE_BT
	if (!BridgeStorageManager::Instance().RemoveBtAddress(index)) {
		LOG_ERR("Failed to remove bridged device Bluetooth address.");
		return CHIP_ERROR_INTERNAL;
	}
#endif

	return CHIP_NO_ERROR;
}
