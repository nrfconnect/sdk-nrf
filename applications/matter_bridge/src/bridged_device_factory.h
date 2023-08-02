/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "bridge_util.h"
#include "bridged_device.h"
#include "bridged_device_data_provider.h"
#include <lib/support/CHIPMem.h>

#ifdef CONFIG_BRIDGE_HUMIDITY_SENSOR_BRIDGED_DEVICE
#include "humidity_sensor.h"
#include "humidity_sensor_data_provider.h"
#endif

#ifdef CONFIG_BRIDGE_ONOFF_LIGHT_BRIDGED_DEVICE
#include "onoff_light.h"
#include "onoff_light_data_provider.h"
#endif

#ifdef CONFIG_BRIDGE_TEMPERATURE_SENSOR_BRIDGED_DEVICE
#include "temperature_sensor.h"
#include "temperature_sensor_data_provider.h"
#endif

namespace BridgeFactory
{

using UpdateAttributeCallback = BridgedDeviceDataProvider::UpdateAttributeCallback;
using DeviceType = BridgedDevice::DeviceType;
using BridgedDeviceFactory = DeviceFactory<BridgedDevice, const char *>;
using DataProviderFactory = DeviceFactory<BridgedDeviceDataProvider, UpdateAttributeCallback>;

auto checkLabel = [](const char *nodeLabel) {
	/* If node label is provided it must fit the maximum defined length */
	if (!nodeLabel || (nodeLabel && (strlen(nodeLabel) < BridgedDevice::kNodeLabelSize))) {
		return true;
	}
	return false;
};

inline BridgedDeviceFactory &GetBridgedDeviceFactory()
{
	static BridgedDeviceFactory sBridgedDeviceFactory{
#ifdef CONFIG_BRIDGE_HUMIDITY_SENSOR_BRIDGED_DEVICE
		{ DeviceType::HumiditySensor,
		  [](const char *nodeLabel) -> BridgedDevice * {
			  if (!checkLabel(nodeLabel)) {
				  return nullptr;
			  }
			  return chip::Platform::New<HumiditySensorDevice>(nodeLabel);
		  } },
#endif
#ifdef CONFIG_BRIDGE_ONOFF_LIGHT_BRIDGED_DEVICE
		{ DeviceType::OnOffLight,
		  [](const char *nodeLabel) -> BridgedDevice * {
			  if (!checkLabel(nodeLabel)) {
				  return nullptr;
			  }
			  return chip::Platform::New<OnOffLightDevice>(nodeLabel);
		  } },
#endif
#ifdef CONFIG_BRIDGE_TEMPERATURE_SENSOR_BRIDGED_DEVICE
		{ BridgedDevice::DeviceType::TemperatureSensor,
		  [](const char *nodeLabel) -> BridgedDevice * {
			  if (!checkLabel(nodeLabel)) {
				  return nullptr;
			  }
			  return chip::Platform::New<TemperatureSensorDevice>(nodeLabel);
		  } },
#endif
	};
	return sBridgedDeviceFactory;
}

inline DataProviderFactory &GetDataProviderFactory()
{
	static DataProviderFactory sDeviceDataProvider{
#ifdef CONFIG_BRIDGE_HUMIDITY_SENSOR_BRIDGED_DEVICE
		{ DeviceType::HumiditySensor,
		  [](UpdateAttributeCallback clb) { return chip::Platform::New<HumiditySensorDataProvider>(clb); } },
#endif
#ifdef CONFIG_BRIDGE_ONOFF_LIGHT_BRIDGED_DEVICE
		{ DeviceType::OnOffLight,
		  [](UpdateAttributeCallback clb) { return chip::Platform::New<OnOffLightDataProvider>(clb); } },
#endif
#ifdef CONFIG_BRIDGE_TEMPERATURE_SENSOR_BRIDGED_DEVICE
		{ DeviceType::TemperatureSensor,
		  [](UpdateAttributeCallback clb) { return chip::Platform::New<TemperatureSensorDataProvider>(clb); } },
#endif
	};
	return sDeviceDataProvider;
}

} // namespace BridgeFactory
