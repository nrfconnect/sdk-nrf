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
} /* namespace */

SimulatedBridgedDeviceFactory::BridgedDeviceFactory &SimulatedBridgedDeviceFactory::GetBridgedDeviceFactory()
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
	};
	return sBridgedDeviceFactory;
}

SimulatedBridgedDeviceFactory::SimulatedDataProviderFactory &SimulatedBridgedDeviceFactory::GetDataProviderFactory()
{
	static SimulatedDataProviderFactory sDeviceDataProvider{
#ifdef CONFIG_BRIDGE_HUMIDITY_SENSOR_BRIDGED_DEVICE
		{ DeviceType::HumiditySensor,
		  [](UpdateAttributeCallback clb) {
			  return chip::Platform::New<SimulatedHumiditySensorDataProvider>(clb);
		  } },
#endif
#ifdef CONFIG_BRIDGE_ONOFF_LIGHT_BRIDGED_DEVICE
		{ DeviceType::OnOffLight,
		  [](UpdateAttributeCallback clb) {
			  return chip::Platform::New<SimulatedOnOffLightDataProvider>(clb);
		  } },
#endif
#ifdef CONFIG_BRIDGE_TEMPERATURE_SENSOR_BRIDGED_DEVICE
		{ DeviceType::TemperatureSensor,
		  [](UpdateAttributeCallback clb) {
			  return chip::Platform::New<SimulatedTemperatureSensorDataProvider>(clb);
		  } },
#endif
	};
	return sDeviceDataProvider;
}

CHIP_ERROR SimulatedBridgedDeviceFactory::CreateDevice(int deviceType, const char *nodeLabel,
						       chip::Optional<uint8_t> index,
						       chip::Optional<uint16_t> endpointId)
{
	CHIP_ERROR err;

	BridgedDeviceDataProvider *provider = GetDataProviderFactory().Create(
		static_cast<MatterBridgedDevice::DeviceType>(deviceType), BridgeManager::HandleUpdate);

	VerifyOrReturnError(provider != nullptr, CHIP_ERROR_INVALID_ARGUMENT, LOG_ERR("No valid data provider!"));

	auto *newBridgedDevice =
		GetBridgedDeviceFactory().Create(static_cast<MatterBridgedDevice::DeviceType>(deviceType), nodeLabel);

	MatterBridgedDevice *newBridgedDevices[] = { newBridgedDevice };

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
		err = BridgeManager::Instance().AddBridgedDevices(
			newBridgedDevices, provider, ARRAY_SIZE(newBridgedDevices), deviceIndex, endpointIds);
	} else {
		err = BridgeManager::Instance().AddBridgedDevices(newBridgedDevices, provider,
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

	return CHIP_NO_ERROR;
}
