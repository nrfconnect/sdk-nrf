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

#ifdef CONFIG_BRIDGE_ONOFF_LIGHT_BRIDGED_DEVICE
#include "onoff_light.h"
#include "simulated_onoff_light_data_provider.h"
#endif

#ifdef CONFIG_BRIDGE_GENERIC_SWITCH_BRIDGED_DEVICE
#include "generic_switch.h"
#include "simulated_generic_switch_data_provider.h"
#endif

#ifdef CONFIG_BRIDGE_ONOFF_LIGHT_SWITCH_BRIDGED_DEVICE
#include "onoff_light_switch.h"
#include "simulated_onoff_light_switch_data_provider.h"
#endif

#ifdef CONFIG_BRIDGE_TEMPERATURE_SENSOR_BRIDGED_DEVICE
#include "simulated_temperature_sensor_data_provider.h"
#include "temperature_sensor.h"
#endif

#ifdef CONFIG_BRIDGE_HUMIDITY_SENSOR_BRIDGED_DEVICE
#include "humidity_sensor.h"
#include "simulated_humidity_sensor_data_provider.h"
#endif

namespace SimulatedBridgedDeviceFactory
{
using UpdateAttributeCallback = Nrf::BridgedDeviceDataProvider::UpdateAttributeCallback;
using InvokeCommandCallback = Nrf::BridgedDeviceDataProvider::InvokeCommandCallback;
using DeviceType = uint16_t;
using BridgedDeviceFactory = Nrf::DeviceFactory<Nrf::MatterBridgedDevice, DeviceType, const char *>;
using SimulatedDataProviderFactory = Nrf::DeviceFactory<Nrf::BridgedDeviceDataProvider, DeviceType, UpdateAttributeCallback, InvokeCommandCallback>;

BridgedDeviceFactory &GetBridgedDeviceFactory();
SimulatedDataProviderFactory &GetDataProviderFactory();

/**
 * @brief Create a bridged device.
 *
 * @param deviceType the Matter device type of a bridged device to be created
 * @param nodeLabel node label of a Matter device to be created
 * @param index optional index object that shall have a valid value set if the value is meant
 * to be used to index assignment, or shall not have a value set if the default index assignment should be used.
 * @param endpointId optional endpoint id object that shall have a valid value set if the value is meant
 * to be used to endpoint id assignment, or shall not have a value set if the default endpoint id assignment should be
 * used.
 * @return CHIP_NO_ERROR on success
 * @return other error code on failure
 */
CHIP_ERROR CreateDevice(int deviceType, const char *nodeLabel,
			chip::Optional<uint8_t> index = chip::Optional<uint8_t>(),
			chip::Optional<uint16_t> endpointId = chip::Optional<uint16_t>());

/**
 * @brief Remove bridged device.
 *
 * @param endpointId value of endpoint id specifying the bridged device to be removed
 * @return CHIP_NO_ERROR on success
 * @return other error code on failure
 */
CHIP_ERROR RemoveDevice(int endpointId);

} /* namespace SimulatedBridgedDeviceFactory */
