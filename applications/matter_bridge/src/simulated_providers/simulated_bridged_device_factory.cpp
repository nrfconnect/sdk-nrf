/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "simulated_bridged_device_factory.h"
#include "bridge_manager.h"
#include "bridge_storage_manager.h"

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

namespace
{
CHIP_ERROR StoreDevice(Nrf::MatterBridgedDevice *device, Nrf::BridgedDeviceDataProvider *provider, uint8_t index)
{
	uint8_t count = 0;
	uint8_t indexes[Nrf::BridgeManager::kMaxBridgedDevices] = { 0 };
	bool deviceRefresh = false;
	Nrf::BridgeStorageManager::BridgedDevice bridgedDevice;

	/* Check if a device is already present in the storage. */
	if (Nrf::BridgeStorageManager::Instance().LoadBridgedDevice(bridgedDevice, index)) {
		deviceRefresh = true;
	}

	bridgedDevice.mEndpointId = device->GetEndpointId();
	bridgedDevice.mDeviceType = device->GetDeviceType();
	bridgedDevice.mUniqueIDLength = strlen(device->GetUniqueID());
	memcpy(bridgedDevice.mUniqueID, device->GetUniqueID(), bridgedDevice.mUniqueIDLength);
	bridgedDevice.mNodeLabelLength = strlen(device->GetNodeLabel());
	memcpy(bridgedDevice.mNodeLabel, device->GetNodeLabel(), bridgedDevice.mNodeLabelLength);

	if (!Nrf::BridgeStorageManager::Instance().StoreBridgedDevice(bridgedDevice, index)) {
		LOG_ERR("Failed to store bridged device");
		return CHIP_ERROR_INTERNAL;
	}

	/* If a device was not present in the storage before, put new index on the end of list and increment the count
	 * number of stored devices. */
	if (!deviceRefresh) {
		CHIP_ERROR err = Nrf::BridgeManager::Instance().GetDevicesIndexes(indexes, sizeof(indexes), count);
		if (CHIP_NO_ERROR != err) {
			LOG_ERR("Failed to get bridged devices indexes");
			return err;
		}

		if (!Nrf::BridgeStorageManager::Instance().StoreBridgedDevicesIndexes(indexes, count)) {
			LOG_ERR("Failed to store bridged devices indexes.");
			return CHIP_ERROR_INTERNAL;
		}

		if (!Nrf::BridgeStorageManager::Instance().StoreBridgedDevicesCount(count)) {
			LOG_ERR("Failed to store bridged devices count.");
			return CHIP_ERROR_INTERNAL;
		}
	}

	return CHIP_NO_ERROR;
}
} /* namespace */

SimulatedBridgedDeviceFactory::BridgedDeviceFactory &SimulatedBridgedDeviceFactory::GetBridgedDeviceFactory()
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
		if (!nodeLabel || (nodeLabel && (strlen(nodeLabel) < Nrf::MatterBridgedDevice::kNodeLabelSize))) {
			return true;
		}
		return false;
	};

	static BridgedDeviceFactory sBridgedDeviceFactory{
#ifdef CONFIG_BRIDGE_ONOFF_LIGHT_BRIDGED_DEVICE
		{ Nrf::MatterBridgedDevice::DeviceType::OnOffLight,
		  [checkUniqueID, checkLabel](const char *uniqueID,
					      const char *nodeLabel) -> Nrf::MatterBridgedDevice * {
			  if (!checkUniqueID(uniqueID) || !checkLabel(nodeLabel)) {
				  return nullptr;
			  }
			  return chip::Platform::New<OnOffLightDevice>(uniqueID, nodeLabel);
		  } },
#endif
#ifdef CONFIG_BRIDGE_GENERIC_SWITCH_BRIDGED_DEVICE
		{ Nrf::MatterBridgedDevice::DeviceType::GenericSwitch,
		  [checkUniqueID, checkLabel](const char *uniqueID,
					      const char *nodeLabel) -> Nrf::MatterBridgedDevice * {
			  if (!checkUniqueID(uniqueID) || !checkLabel(nodeLabel)) {
				  return nullptr;
			  }
			  return chip::Platform::New<GenericSwitchDevice>(uniqueID, nodeLabel);
		  } },
#endif
#ifdef CONFIG_BRIDGE_ONOFF_LIGHT_SWITCH_BRIDGED_DEVICE
		{ Nrf::MatterBridgedDevice::DeviceType::OnOffLightSwitch,
		  [checkUniqueID, checkLabel](const char *uniqueID,
					      const char *nodeLabel) -> Nrf::MatterBridgedDevice * {
			  if (!checkUniqueID(uniqueID) || !checkLabel(nodeLabel)) {
				  return nullptr;
			  }
			  return chip::Platform::New<OnOffLightSwitchDevice>(uniqueID, nodeLabel);
		  } },
#endif
#ifdef CONFIG_BRIDGE_TEMPERATURE_SENSOR_BRIDGED_DEVICE
		{ Nrf::MatterBridgedDevice::DeviceType::TemperatureSensor,
		  [checkUniqueID, checkLabel](const char *uniqueID,
					      const char *nodeLabel) -> Nrf::MatterBridgedDevice * {
			  if (!checkUniqueID(uniqueID) || !checkLabel(nodeLabel)) {
				  return nullptr;
			  }
			  return chip::Platform::New<TemperatureSensorDevice>(uniqueID, nodeLabel);
		  } },
#endif
#ifdef CONFIG_BRIDGE_HUMIDITY_SENSOR_BRIDGED_DEVICE
		{ Nrf::MatterBridgedDevice::DeviceType::HumiditySensor,
		  [checkUniqueID, checkLabel](const char *uniqueID,
					      const char *nodeLabel) -> Nrf::MatterBridgedDevice * {
			  if (!checkUniqueID(uniqueID) || !checkLabel(nodeLabel)) {
				  return nullptr;
			  }
			  return chip::Platform::New<HumiditySensorDevice>(uniqueID, nodeLabel);
		  } },
#endif
	};
	return sBridgedDeviceFactory;
}

SimulatedBridgedDeviceFactory::SimulatedDataProviderFactory &SimulatedBridgedDeviceFactory::GetDataProviderFactory()
{
	static SimulatedDataProviderFactory sDeviceDataProvider{
#ifdef CONFIG_BRIDGE_ONOFF_LIGHT_BRIDGED_DEVICE
		{ Nrf::MatterBridgedDevice::DeviceType::OnOffLight,
		  [](UpdateAttributeCallback updateClb, InvokeCommandCallback commandClb) {
			  return chip::Platform::New<SimulatedOnOffLightDataProvider>(updateClb, commandClb);
		  } },
#endif
#ifdef CONFIG_BRIDGE_GENERIC_SWITCH_BRIDGED_DEVICE
		{ Nrf::MatterBridgedDevice::DeviceType::GenericSwitch,
		  [](UpdateAttributeCallback updateClb, InvokeCommandCallback commandClb) {
			  return chip::Platform::New<SimulatedGenericSwitchDataProvider>(updateClb, commandClb);
		  } },
#endif
#ifdef CONFIG_BRIDGE_ONOFF_LIGHT_SWITCH_BRIDGED_DEVICE
		{ Nrf::MatterBridgedDevice::DeviceType::OnOffLightSwitch,
		  [](UpdateAttributeCallback updateClb, InvokeCommandCallback commandClb) {
			  return chip::Platform::New<SimulatedOnOffLightSwitchDataProvider>(updateClb, commandClb);
		  } },
#endif
#ifdef CONFIG_BRIDGE_TEMPERATURE_SENSOR_BRIDGED_DEVICE
		{ Nrf::MatterBridgedDevice::DeviceType::TemperatureSensor,
		  [](UpdateAttributeCallback updateClb, InvokeCommandCallback commandClb) {
			  return chip::Platform::New<SimulatedTemperatureSensorDataProvider>(updateClb, commandClb);
		  } },
#endif
#ifdef CONFIG_BRIDGE_HUMIDITY_SENSOR_BRIDGED_DEVICE
		{ Nrf::MatterBridgedDevice::DeviceType::HumiditySensor,
		  [](UpdateAttributeCallback updateClb, InvokeCommandCallback commandClb) {
			  return chip::Platform::New<SimulatedHumiditySensorDataProvider>(updateClb, commandClb);
		  } },
#endif
	};
	return sDeviceDataProvider;
}

CHIP_ERROR SimulatedBridgedDeviceFactory::CreateDevice(int deviceType, const char *uniqueID, const char *nodeLabel,
						       chip::Optional<uint8_t> index,
						       chip::Optional<uint16_t> endpointId)
{
	CHIP_ERROR err;

	Nrf::BridgedDeviceDataProvider *provider = GetDataProviderFactory().Create(
		static_cast<Nrf::MatterBridgedDevice::DeviceType>(deviceType), Nrf::BridgeManager::HandleUpdate, Nrf::BridgeManager::HandleCommand);

	VerifyOrReturnError(provider != nullptr, CHIP_ERROR_INVALID_ARGUMENT, LOG_ERR("No valid data provider!"));

	auto *newBridgedDevice = GetBridgedDeviceFactory().Create(
		static_cast<Nrf::MatterBridgedDevice::DeviceType>(deviceType), uniqueID, nodeLabel);

	Nrf::MatterBridgedDevice *newBridgedDevices[] = { newBridgedDevice };

	if (!newBridgedDevice) {
		LOG_ERR("Cannot allocate Matter device of given type");
		return CHIP_ERROR_NO_MEMORY;
	}

	uint8_t deviceIndex[] = { 0 };
	uint16_t endpointIds[] = { 0 };

	/* The current implementation assumes that index and endpoint id values are given only in case of
	 * recovering a device. In such scenario the devices (endpoints) are recovered one by one. */
	if (index.HasValue() && endpointId.HasValue()) {
		endpointIds[0] = endpointId.Value();
		deviceIndex[0] = index.Value();
		err = Nrf::BridgeManager::Instance().AddBridgedDevices(
			newBridgedDevices, provider, ARRAY_SIZE(newBridgedDevices), deviceIndex, endpointIds);
	} else {
		err = Nrf::BridgeManager::Instance().AddBridgedDevices(newBridgedDevices, provider,
								  ARRAY_SIZE(newBridgedDevices), deviceIndex);
	}

	if (err == CHIP_NO_ERROR) {
		err = StoreDevice(newBridgedDevice, provider, deviceIndex[0]);
	}

	if (err != CHIP_NO_ERROR) {
		LOG_ERR("Adding Matter bridged device failed: %s", ErrorStr(err));
	} else {
		LOG_INF("Created %x device type on the endpoint %d", newBridgedDevice->GetDeviceType(),
			newBridgedDevice->GetEndpointId());
	}
	return err;
}

CHIP_ERROR SimulatedBridgedDeviceFactory::RemoveDevice(int endpointId)
{
	uint8_t index;
	uint8_t count = 0;
	uint8_t indexes[Nrf::BridgeManager::kMaxBridgedDevices] = { 0 };

	CHIP_ERROR err = Nrf::BridgeManager::Instance().RemoveBridgedDevice(endpointId, index);

	if (CHIP_NO_ERROR != err) {
		LOG_ERR("Failed to remove bridged device");
		return err;
	}

	err = Nrf::BridgeManager::Instance().GetDevicesIndexes(indexes, sizeof(indexes), count);
	if (CHIP_NO_ERROR != err) {
		LOG_ERR("Failed to get bridged devices indexes");
		return err;
	}

	/* Update the current indexes list. */
	if (!Nrf::BridgeStorageManager::Instance().StoreBridgedDevicesIndexes(indexes, count)) {
		LOG_ERR("Failed to store bridged devices indexes.");
		return CHIP_ERROR_INTERNAL;
	}

	if (!Nrf::BridgeStorageManager::Instance().StoreBridgedDevicesCount(count)) {
		LOG_ERR("Failed to store bridged devices count.");
		return CHIP_ERROR_INTERNAL;
	}

	if (!Nrf::BridgeStorageManager::Instance().RemoveBridgedDevice(index)) {
		LOG_ERR("Failed to remove bridged device from the storage.");
		return CHIP_ERROR_INTERNAL;
	}

	return CHIP_NO_ERROR;
}
