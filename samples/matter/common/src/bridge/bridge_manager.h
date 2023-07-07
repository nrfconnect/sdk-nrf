/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "bridged_device.h"
#include "bridged_device_data_provider.h"
#include <map>

class BridgeManager {
public:
	void Init();
	CHIP_ERROR AddBridgedDevice(BridgedDevice::DeviceType bridgedDeviceType, const char *nodeLabel);
	CHIP_ERROR RemoveBridgedDevice(uint16_t endpoint);
	static BridgedDevice *GetBridgedDevice(uint16_t endpoint, const EmberAfAttributeMetadata *attributeMetadata,
					       uint8_t *buffer);
	static CHIP_ERROR HandleRead(uint16_t endpoint, chip::ClusterId clusterId,
				     const EmberAfAttributeMetadata *attributeMetadata, uint8_t *buffer,
				     uint16_t maxReadLength);
	static CHIP_ERROR HandleWrite(uint16_t endpoint, chip::ClusterId clusterId,
				      const EmberAfAttributeMetadata *attributeMetadata, uint8_t *buffer);
	static void HandleUpdate(BridgedDeviceDataProvider &dataProvider, chip::ClusterId clusterId,
				 chip::AttributeId attributeId, void *data, size_t dataSize);

private:
	CHIP_ERROR AddDevice(BridgedDevice *device);

	static constexpr uint8_t kMaxBridgedDevices = CHIP_DEVICE_CONFIG_DYNAMIC_ENDPOINT_COUNT;
	static constexpr uint8_t kMaxDataProviders = CONFIG_BRIDGE_MAX_BRIDGED_DEVICES_NUMBER;

	/* TODO: Implement lightweight map to decrease flash usage. */
	std::map<BridgedDevice *, BridgedDeviceDataProvider *> mDevicesMap;
	BridgedDevice *mBridgedDevices[kMaxBridgedDevices];
	BridgedDeviceDataProvider *mDataProviders[kMaxDataProviders];

	chip::EndpointId mFirstDynamicEndpointId;
	chip::EndpointId mCurrentDynamicEndpointId;

	friend BridgeManager &GetBridgeManager();
	static BridgeManager sBridgeManager;
};

inline BridgeManager &GetBridgeManager()
{
	return BridgeManager::sBridgeManager;
}
