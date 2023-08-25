/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "bridge_util.h"
#include "bridged_device_data_provider.h"
#include "matter_bridged_device.h"
#include <lib/support/CHIPMem.h>

#ifdef CONFIG_BRIDGE_HUMIDITY_SENSOR_BRIDGED_DEVICE
#include "humidity_sensor.h"
#ifdef CONFIG_BRIDGED_DEVICE_SIMULATED
#include "simulated_humidity_sensor_data_provider.h"
#endif
#endif

#ifdef CONFIG_BRIDGE_ONOFF_LIGHT_BRIDGED_DEVICE
#include "onoff_light.h"
#ifdef CONFIG_BRIDGED_DEVICE_SIMULATED
#include "simulated_onoff_light_data_provider.h"
#endif
#ifdef CONFIG_BRIDGED_DEVICE_BT
#include "ble_onoff_light_data_provider.h"
#endif
#endif

#ifdef CONFIG_BRIDGE_TEMPERATURE_SENSOR_BRIDGED_DEVICE
#include "temperature_sensor.h"
#ifdef CONFIG_BRIDGED_DEVICE_SIMULATED
#include "simulated_temperature_sensor_data_provider.h"
#endif
#endif

#ifdef CONFIG_BRIDGED_DEVICE_BT
#if defined(CONFIG_BRIDGE_HUMIDITY_SENSOR_BRIDGED_DEVICE) && defined(CONFIG_BRIDGE_TEMPERATURE_SENSOR_BRIDGED_DEVICE)
#include "ble_environmental_data_provider.h"
#endif
#endif

namespace BridgeFactory
{
using UpdateAttributeCallback = BridgedDeviceDataProvider::UpdateAttributeCallback;
using DeviceType = MatterBridgedDevice::DeviceType;
using BridgedDeviceFactory = DeviceFactory<MatterBridgedDevice, const char *>;

#ifdef CONFIG_BRIDGED_DEVICE_SIMULATED
using SimulatedDataProviderFactory = DeviceFactory<BridgedDeviceDataProvider, UpdateAttributeCallback>;
#endif

#ifdef CONFIG_BRIDGED_DEVICE_BT
using BleDataProviderFactory = DeviceFactory<BridgedDeviceDataProvider, UpdateAttributeCallback>;
#endif

auto checkLabel = [](const char *nodeLabel) {
	/* If node label is provided it must fit the maximum defined length */
	if (!nodeLabel || (nodeLabel && (strlen(nodeLabel) < MatterBridgedDevice::kNodeLabelSize))) {
		return true;
	}
	return false;
};

inline BridgedDeviceFactory &GetBridgedDeviceFactory()
{
	static BridgedDeviceFactory sBridgedDeviceFactory{
#ifdef CONFIG_BRIDGE_HUMIDITY_SENSOR_BRIDGED_DEVICE
		{ DeviceType::HumiditySensor,
		  [](const char *nodeLabel) -> MatterBridgedDevice * {
			  if (!checkLabel(nodeLabel)) {
				  return nullptr;
			  }
			  return chip::Platform::New<HumiditySensorDevice>(nodeLabel);
		  } },
#endif
#ifdef CONFIG_BRIDGE_ONOFF_LIGHT_BRIDGED_DEVICE
		{ DeviceType::OnOffLight,
		  [](const char *nodeLabel) -> MatterBridgedDevice * {
			  if (!checkLabel(nodeLabel)) {
				  return nullptr;
			  }
			  return chip::Platform::New<OnOffLightDevice>(nodeLabel);
		  } },
#endif
#ifdef CONFIG_BRIDGE_TEMPERATURE_SENSOR_BRIDGED_DEVICE
		{ MatterBridgedDevice::DeviceType::TemperatureSensor,
		  [](const char *nodeLabel) -> MatterBridgedDevice * {
			  if (!checkLabel(nodeLabel)) {
				  return nullptr;
			  }
			  return chip::Platform::New<TemperatureSensorDevice>(nodeLabel);
		  } },
#endif
	};
	return sBridgedDeviceFactory;
}

#ifdef CONFIG_BRIDGED_DEVICE_SIMULATED
inline SimulatedDataProviderFactory &GetSimulatedDataProviderFactory()
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
#endif

#ifdef CONFIG_BRIDGED_DEVICE_BT
inline BleDataProviderFactory &GetBleDataProviderFactory()
{
	static BleDataProviderFactory sDeviceDataProvider
	{
#ifdef CONFIG_BRIDGE_ONOFF_LIGHT_BRIDGED_DEVICE
		{ DeviceType::OnOffLight,
		  [](UpdateAttributeCallback clb) { return chip::Platform::New<BleOnOffLightDataProvider>(clb); } },
#endif
#if defined(CONFIG_BRIDGE_TEMPERATURE_SENSOR_BRIDGED_DEVICE) && defined(CONFIG_BRIDGE_HUMIDITY_SENSOR_BRIDGED_DEVICE)
			{ DeviceType::EnvironmentalSensor, [](UpdateAttributeCallback clb) {
				 return chip::Platform::New<BleEnvironmentalDataProvider>(clb);
			 } },
#endif
	};
	return sDeviceDataProvider;
}
#endif

} // namespace BridgeFactory
