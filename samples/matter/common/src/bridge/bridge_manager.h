/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "bridge_util.h"
#include "bridged_device_data_provider.h"
#include "matter_bridged_device.h"
#include "binding/binding_handler.h"

class BridgeManager {
public:
	static constexpr uint8_t kMaxBridgedDevices = CHIP_DEVICE_CONFIG_DYNAMIC_ENDPOINT_COUNT;
	static constexpr uint8_t kMaxBridgedDevicesPerProvider = CONFIG_BRIDGE_MAX_BRIDGED_DEVICES_NUMBER_PER_PROVIDER;
	static constexpr chip::EndpointId kAggregatorEndpointId = CONFIG_BRIDGE_AGGREGATOR_ENDPOINT_ID;

	using LoadStoredBridgedDevicesCallback = CHIP_ERROR (*)();

	/**
	 * @brief Initialize BridgeManager instance.
	 *
	 * @param loadStoredBridgedDevicesCb callback to method capable of loading and adding bridged devices stored in
	 * persistent storage
	 * @return CHIP_NO_ERROR on success
	 * @return other error code on failure
	 */
	CHIP_ERROR Init(LoadStoredBridgedDevicesCallback loadStoredBridgedDevicesCb);

	/**
	 * @brief Add devices which are supposed to be bridged to the Bridge Manager
	 *
	 * @param devices Matter devices to be bridged (the maximum size of this array may depend on the
	 * application and can be memory constrained)
	 * @param dataProvider data provider which is going to be bridged with Matter devices
	 * @param deviceListSize number of Matter devices which are going to be bridged with data provider
	 * @param devicesPairIndexes array of the indexes that will be filled with pairs' indexes
	 * assigned by the bridge
	 * @return CHIP_NO_ERROR on success
	 * @return other error code on failure
	 */
	CHIP_ERROR AddBridgedDevices(MatterBridgedDevice *devices[], BridgedDeviceDataProvider *dataProvider,
				     uint8_t deviceListSize, uint8_t devicesPairIndexes[]);

	/**
	 * @brief Add devices which are supposed to be bridged to the Bridge Manager using specific index and endpoint
	 * id.
	 *
	 * @param devices Matter devices to be bridged (the maximum size of this array may depend on the
	 * application and can be memory constrained)
	 * @param dataProvider data provider which is going to be bridged with Matter devices
	 * @param deviceListSize number of Matter devices which are going to be bridged with data provider
	 * @param devicesPairIndexes array of the indexes that contain index value required to be
	 * assigned and that will be filled with pairs' indexes finally assigned by the bridge
	 * @param endpointIds values of endpoint ids required to be assigned
	 * @return CHIP_NO_ERROR on success
	 * @return other error code on failure
	 */
	CHIP_ERROR AddBridgedDevices(MatterBridgedDevice *devices[], BridgedDeviceDataProvider *dataProvider,
				     uint8_t deviceListSize, uint8_t devicesPairIndexes[], uint16_t endpointIds[]);

	/**
	 * @brief Remove bridged device.
	 *
	 * @param endpoint value of endpoint id specifying the bridged device to be removed
	 * @param devicesPairIndex reference to the index that will be filled with pair's index obtained by the
	 * bridge
	 * @return CHIP_NO_ERROR on success
	 * @return other error code on failure
	 */
	CHIP_ERROR RemoveBridgedDevice(uint16_t endpoint, uint8_t &devicesPairIndex);

	/**
	 * @brief Get bridged devices indexes.
	 *
	 * @param indexes array address to be filled with bridged devices indexes
	 * @param maxSize maximum size that can be used for indexes array
	 * @param count reference to the count object to be filled with size of actually saved data
	 * @return CHIP_NO_ERROR on success
	 * @return other error code on failure
	 */
	CHIP_ERROR GetDevicesIndexes(uint8_t *indexes, uint8_t maxSize, uint8_t &count)
	{
		if (!indexes) {
			return CHIP_ERROR_INVALID_ARGUMENT;
		}

		if (maxSize < mDevicesIndexesCounter) {
			return CHIP_ERROR_BUFFER_TOO_SMALL;
		}

		memcpy(indexes, mDevicesIndexes, mDevicesIndexesCounter);
		count = mDevicesIndexesCounter;
		return CHIP_NO_ERROR;
	}

	/**
	 * @brief Get the data provider stored on the specified endpoint.
	 *
	 * @param endpoint endpoint on which the bridged device is stored
	 * @param[out] deviceType a type of the bridged device
	 * @return pointer to the data provider bridged with the device on the specified endpoint
	 */
	BridgedDeviceDataProvider *GetProvider(chip::EndpointId endpoint, uint16_t &deviceType);

	static CHIP_ERROR HandleRead(uint16_t index, chip::ClusterId clusterId,
				     const EmberAfAttributeMetadata *attributeMetadata, uint8_t *buffer,
				     uint16_t maxReadLength);
	static CHIP_ERROR HandleWrite(uint16_t index, chip::ClusterId clusterId,
				      const EmberAfAttributeMetadata *attributeMetadata, uint8_t *buffer);
	static void HandleUpdate(BridgedDeviceDataProvider &dataProvider, chip::ClusterId clusterId,
				 chip::AttributeId attributeId, void *data, size_t dataSize);
	static void HandleCommand(BridgedDeviceDataProvider &dataProvider, chip::ClusterId clusterId,
				  chip::CommandId commandId, BindingHandler::InvokeCommand invokeCommand);

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

	static constexpr uint8_t kMaxDataProviders = CONFIG_BRIDGE_MAX_BRIDGED_DEVICES_NUMBER;

	using DeviceMap = FiniteMap<BridgedDevicePair, kMaxBridgedDevices>;

	/**
	 * @brief Add pair of single bridged device and its data provider using optional index and endpoint id.
	 * The method takes care of releasing the memory allocated for the data provider and bridged device objects
	 * in case of adding process failure.
	 *
	 * @param device address of valid bridged device object
	 * @param dataProvider address of valid data provider object
	 * @param devicesPairIndex index object that will be filled with pair's index
	 * assigned by the bridge
	 * @param endpointId value of endpoint id required to be assigned
	 * @return CHIP_NO_ERROR on success
	 * @return other error code on failure
	 */
	CHIP_ERROR AddSingleDevice(MatterBridgedDevice *device, BridgedDeviceDataProvider *dataProvider,
				   chip::Optional<uint8_t> &devicesPairIndex, uint16_t endpointId);
	CHIP_ERROR SafelyRemoveDevice(uint8_t index);

	/**
	 * @brief Add pair of bridged devices and their data provider using optional index and endpoint id. This is a
	 * wrapper method invoked by public AddBridgedDevices methods that maps integer indexes to optionals are assigns
	 * output index values.
	 *
	 * @param device address of valid bridged device object
	 * @param dataProvider address of valid data provider object
	 * @param devicesPairIndexes array of the index objects that will be filled with pairs' indexes
	 * assigned by the bridge
	 * @param endpointIds values of endpoint ids required to be assigned
	 * @param indexes array of pointers to the optional index objects that shall have a valid value set if the value
	 * is meant to be used to index assignment, or shall not have a value set if the default index assignment should
	 * be used.
	 * @return CHIP_NO_ERROR on success
	 * @return other error code on failure
	 */
	CHIP_ERROR AddBridgedDevices(MatterBridgedDevice *devices[], BridgedDeviceDataProvider *dataProvider,
				     uint8_t deviceListSize, uint8_t devicesPairIndexes[], uint16_t endpointIds[],
				     chip::Optional<uint8_t> indexes[]);

	/**
	 * @brief Add pair of bridged devices and their data provider using optional index and endpoint id. The method
	 * creates a map entry, matches the bridged device object with the data provider object and creates Matter
	 * dynamic endpoint.
	 *
	 * @param device address of valid bridged device object
	 * @param dataProvider address of valid data provider object
	 * @param devicesPairIndex array of optional index objects that shall have a valid value set if
	 * the value is meant to be used to index assignment, or shall not have a value set if the default index
	 * assignment should be used.
	 * @param endpointIds values of endpoint ids required to be assigned
	 * @return CHIP_NO_ERROR on success
	 * @return other error code on failure
	 */
	CHIP_ERROR AddDevices(MatterBridgedDevice *devices[], BridgedDeviceDataProvider *dataProvider,
			      uint8_t deviceListSize, chip::Optional<uint8_t> devicesPairIndexes[],
			      uint16_t endpointIds[]);

	/**
	 * @brief Create Matter dynamic endpoint.
	 *
	 * @param index index in Matter Data Model's (ember) array to store the endpoint
	 * @param endpointId value of endpoint id to be created
	 * @return CHIP_NO_ERROR on success
	 * @return other error code on failure
	 */
	CHIP_ERROR CreateEndpoint(uint8_t index, uint16_t endpointId);

	DeviceMap mDevicesMap;
	uint16_t mNumberOfProviders{ 0 };
	uint8_t mDevicesIndexes[BridgeManager::kMaxBridgedDevices] = { 0 };
	uint8_t mDevicesIndexesCounter;

	chip::EndpointId mFirstDynamicEndpointId;
	chip::EndpointId mCurrentDynamicEndpointId;
	bool mIsInitialized = false;
};
