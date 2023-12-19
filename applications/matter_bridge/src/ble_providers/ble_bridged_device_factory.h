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

#if defined(CONFIG_BRIDGE_ONOFF_LIGHT_BRIDGED_DEVICE) && (defined(CONFIG_BRIDGE_GENERIC_SWITCH_BRIDGED_DEVICE) || defined(CONFIG_BRIDGE_ONOFF_LIGHT_SWITCH_BRIDGED_DEVICE))
#include "ble_lbs_data_provider.h"

#ifdef CONFIG_BRIDGE_ONOFF_LIGHT_BRIDGED_DEVICE
#include "onoff_light.h"
#endif

#ifdef CONFIG_BRIDGE_GENERIC_SWITCH_BRIDGED_DEVICE
#include "generic_switch.h"
#endif

#ifdef CONFIG_BRIDGE_ONOFF_LIGHT_SWITCH_BRIDGED_DEVICE
#include "onoff_light_switch.h"
#endif

#endif

#ifdef CONFIG_BRIDGE_HUMIDITY_SENSOR_BRIDGED_DEVICE
#include "humidity_sensor.h"
#endif

#ifdef CONFIG_BRIDGE_TEMPERATURE_SENSOR_BRIDGED_DEVICE
#include "temperature_sensor.h"
#endif

#if defined(CONFIG_BRIDGE_HUMIDITY_SENSOR_BRIDGED_DEVICE) && defined(CONFIG_BRIDGE_TEMPERATURE_SENSOR_BRIDGED_DEVICE)
#include "ble_environmental_data_provider.h"
#endif

namespace BleBridgedDeviceFactory
{

/* The values were assigned based on BT_UUID_16(uuid)->val of a BT services. */
enum ServiceUuid : uint16_t { LedButtonService = 0xbcd1, EnvironmentalSensorService = 0x181a };

using UpdateAttributeCallback = BridgedDeviceDataProvider::UpdateAttributeCallback;
using InvokeCommandCallback = BridgedDeviceDataProvider::InvokeCommandCallback;
using DeviceType = uint16_t;
using BridgedDeviceFactory = DeviceFactory<MatterBridgedDevice, DeviceType, const char *>;
using BleDataProviderFactory = DeviceFactory<BridgedDeviceDataProvider, ServiceUuid, UpdateAttributeCallback, InvokeCommandCallback>;

BridgedDeviceFactory &GetBridgedDeviceFactory();
BleDataProviderFactory &GetDataProviderFactory();

/**
 * @brief Create a bridged device using a specific device type, index and endpoint ID.
 *
 * @param deviceType the Matter device type of a bridged device to be created
 * @param btAddress the Bluetooth LE address of a device to be bridged with created Matter device
 * @param nodeLabel node label of a Matter device to be created
 * @param index index that will be assigned to the created device
 * @param endpointId endpoint id that will be assigned to the created device
 * @return CHIP_NO_ERROR on success
 * @return other error code on failure
 */
CHIP_ERROR CreateDevice(int deviceType, bt_addr_le_t btAddress, const char *nodeLabel, uint8_t index,
			uint16_t endpointId);

/**
 * @brief Create a bridged device using a specific Bluetooth LE service and leaving index and endpoint ID selection to
 * the default algorithm.
 *
 * @param uuid the Bluetooth LE service UUID of a bridged device provider that will be paired with bridged device
 * @param btAddress the Bluetooth LE address of a device to be bridged with created Matter device
 * @param nodeLabel node label of a Matter device to be created
 * @param request address of connection request object for handling additional security information requiered by the connection.
 *				  Can be nullptr, if connection does not use security.
 * @return CHIP_NO_ERROR on success
 * @return other error code on failure
 */
CHIP_ERROR CreateDevice(uint16_t uuid, bt_addr_le_t btAddress, const char *nodeLabel, BLEConnectivityManager::ConnectionSecurityRequest * request = nullptr);

/**
 * @brief Remove bridged device.
 *
 * @param endpointId value of endpoint id specifying the bridged device to be removed
 * @return CHIP_NO_ERROR on success
 * @return other error code on failure
 */
CHIP_ERROR RemoveDevice(int endpointId);

/**
 * @brief Get helper string containing Bluetooth LE service name
 *
 * @param uuid the Bluetooth LE service UUID
 */
const char *GetUuidString(uint16_t uuid);

} /* namespace BleBridgedDeviceFactory */
