/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "bridge_util.h"
#include "bridged_device.h"
#include "bridged_device_data_provider.h"

class BridgeManager {
public:
	void Init();
	CHIP_ERROR AddBridgedDevices(BridgedDevice *aDevice, BridgedDeviceDataProvider *aDataProvider);
	CHIP_ERROR RemoveBridgedDevice(uint16_t endpoint);
	static CHIP_ERROR HandleRead(uint16_t index, chip::ClusterId clusterId,
				     const EmberAfAttributeMetadata *attributeMetadata, uint8_t *buffer,
				     uint16_t maxReadLength);
	static CHIP_ERROR HandleWrite(uint16_t index, chip::ClusterId clusterId,
				      const EmberAfAttributeMetadata *attributeMetadata, uint8_t *buffer);
	static void HandleUpdate(BridgedDeviceDataProvider &dataProvider, chip::ClusterId clusterId,
				 chip::AttributeId attributeId, void *data, size_t dataSize);

private:
	static constexpr uint8_t kMaxBridgedDevices = CHIP_DEVICE_CONFIG_DYNAMIC_ENDPOINT_COUNT;
	static constexpr uint8_t kMaxDataProviders = CONFIG_BRIDGE_MAX_BRIDGED_DEVICES_NUMBER;

	using DevicePtr = chip::Platform::UniquePtr<BridgedDevice>;
	using ProviderPtr = chip::Platform::UniquePtr<BridgedDeviceDataProvider>;
	struct DevicePair {
		DevicePair() : mDevice(nullptr), mProvider(nullptr) {}
		DevicePair(DevicePtr device, ProviderPtr provider)
			: mDevice(std::move(device)), mProvider(std::move(provider))
		{
		}
		DevicePair &operator=(DevicePair &&other)
		{
			mDevice = std::move(other.mDevice);
			mProvider = std::move(other.mProvider);
			return *this;
		}
		operator bool() const { return mDevice && mProvider; }
		DevicePair &operator=(const DevicePair &other) = delete;
		DevicePtr mDevice;
		ProviderPtr mProvider;
	};
	using DeviceMap = FiniteMap<DevicePair, kMaxBridgedDevices>;

	CHIP_ERROR AddDevices(BridgedDevice *aDevice, BridgedDeviceDataProvider *aDataProvider);

	DeviceMap mDevicesMap;
	uint16_t mNumberOfProviders{ 0 };

	chip::EndpointId mFirstDynamicEndpointId;
	chip::EndpointId mCurrentDynamicEndpointId;

	friend BridgeManager &GetBridgeManager();
	static BridgeManager sBridgeManager;
};

inline BridgeManager &GetBridgeManager()
{
	return BridgeManager::sBridgeManager;
}
