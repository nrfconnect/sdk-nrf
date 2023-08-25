/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "bridge_util.h"
#include "bridged_device_data_provider.h"
#include "matter_bridged_device.h"

class BridgeManager {
public:
	void Init();
	/**
	 * @brief Add devices which are supposed to be bridged to the Bridge Manager
	 *
	 * @param devices Matter devices to be bridged (the maximum size of this array may depend on the
	 * application and can be memory constrained)
	 * @param dataProvider data provider which is going to be bridged with Matter devices
	 * @param deviceListSize number of Matter devices which are going to be bridged with data provider
	 * @return CHIP_NO_ERROR in case of success, specific CHIP_ERROR code otherwise
	 */
	CHIP_ERROR AddBridgedDevices(MatterBridgedDevice *devices[], BridgedDeviceDataProvider *dataProvider,
				     uint8_t deviceListSize);
	CHIP_ERROR RemoveBridgedDevice(uint16_t endpoint);
	static CHIP_ERROR HandleRead(uint16_t index, chip::ClusterId clusterId,
				     const EmberAfAttributeMetadata *attributeMetadata, uint8_t *buffer,
				     uint16_t maxReadLength);
	static CHIP_ERROR HandleWrite(uint16_t index, chip::ClusterId clusterId,
				      const EmberAfAttributeMetadata *attributeMetadata, uint8_t *buffer);
	static void HandleUpdate(BridgedDeviceDataProvider &dataProvider, chip::ClusterId clusterId,
				 chip::AttributeId attributeId, void *data, size_t dataSize);

	static BridgeManager &Instance()
	{
		static BridgeManager sInstance;
		return sInstance;
	}

private:
	struct BridgedDevicePair {
		BridgedDevicePair() : mDevice(nullptr), mProvider(nullptr) {}
		BridgedDevicePair(MatterBridgedDevice *device, BridgedDeviceDataProvider *dataProvider)
			: mDevice(device), mProvider(dataProvider)
		{
		}

		~BridgedDevicePair()
		{
			chip::Platform::Delete(mDevice);
			chip::Platform::Delete(mProvider);
			mDevice = nullptr;
			mProvider = nullptr;
		}

		/* Disable copy semantics and implement move semantics. */
		BridgedDevicePair(const BridgedDevicePair &other) = delete;
		BridgedDevicePair &operator=(const BridgedDevicePair &other) = delete;

		BridgedDevicePair(BridgedDevicePair &&other)
		{
			mDevice = other.mDevice;
			mProvider = other.mProvider;

			other.mDevice = nullptr;
			other.mProvider = nullptr;
		}

		BridgedDevicePair &operator=(BridgedDevicePair &&other)
		{
			if (this != &other) {
				this->~BridgedDevicePair();
				mDevice = other.mDevice;
				mProvider = other.mProvider;
				other.mDevice = nullptr;
				other.mProvider = nullptr;
			}
			return *this;
		}

		operator bool() const { return mDevice || mProvider; }
		bool operator==(const BridgedDevicePair &other)
		{
			return (mDevice == other.mDevice) || (mProvider == other.mProvider);
		}

		MatterBridgedDevice *mDevice;
		BridgedDeviceDataProvider *mProvider;
	};

	static constexpr uint8_t kMaxBridgedDevices = CHIP_DEVICE_CONFIG_DYNAMIC_ENDPOINT_COUNT;
	static constexpr uint8_t kMaxDataProviders = CONFIG_BRIDGE_MAX_BRIDGED_DEVICES_NUMBER;

	using DeviceMap = FiniteMap<BridgedDevicePair, kMaxBridgedDevices>;

	bool AddSingleDevice(MatterBridgedDevice *device, BridgedDeviceDataProvider *dataProvider);
	CHIP_ERROR SafelyRemoveDevice(uint8_t index);

	DeviceMap mDevicesMap;
	uint16_t mNumberOfProviders{ 0 };

	chip::EndpointId mFirstDynamicEndpointId;
	chip::EndpointId mCurrentDynamicEndpointId;
};
