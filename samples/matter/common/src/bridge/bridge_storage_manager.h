/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "matter_bridged_device.h"
#include "persistent_storage/persistent_storage.h"

#ifdef CONFIG_BRIDGED_DEVICE_BT
#include <zephyr/bluetooth/addr.h>
#endif

namespace Nrf
{

/*
 * The class implements the following key-values storage structure:
 *
 * /br/
 *		/brd_cnt/ /<uint8_t>/
 *		/brd_ids/ /<uint8_t[brd_cnt]>/
 * 		/brd/
 *			/0/ /<BridgedDevice>/
 *			/1/ /<BridgedDevice>/
 *			.
 *			.
 *			/n/ /<BridgedDevice>/
 *		/ver/ <uint8_t>
 */
class BridgeStorageManager {
public:
	static inline constexpr auto kMaxUserDataSize = 128u;

	struct BridgedDevice {
		uint16_t mEndpointId;
		uint16_t mDeviceType;
		size_t mNodeLabelLength;
		char mNodeLabel[MatterBridgedDevice::kNodeLabelSize] = { 0 };
		size_t mUserDataSize = 0;
		uint8_t *mUserData = nullptr;
	};

	static constexpr auto kMaxIndexLength = 3;

	BridgeStorageManager()
		: mBridge("br", strlen("br")), mBridgedDevicesCount("brd_cnt", strlen("brd_cnt"), &mBridge),
		  mBridgedDevicesIndexes("brd_ids", strlen("brd_ids"), &mBridge),
		  mBridgedDevice("brd", strlen("brd"), &mBridge), mVersion("ver", strlen("ver"), &mBridge),
		  mBridgedDeviceEndpointId("eid", strlen("eid"), &mBridgedDevice),
		  mBridgedDeviceNodeLabel("label", strlen("label"), &mBridgedDevice),
		  mBridgedDeviceType("type", strlen("type"), &mBridgedDevice)
#ifdef CONFIG_BRIDGED_DEVICE_BT
		  ,
		  mBt("bt", strlen("bt"), &mBridgedDevice), mBtAddress("addr", strlen("addr"), &mBt)
#endif
	{
	}

	static BridgeStorageManager &Instance()
	{
		static BridgeStorageManager sInstance;
		return sInstance;
	}

	/**
	 * @brief Initialize BridgeStorageManager module.
	 *
	 * @return true if module has been initialized successfully
	 * @return false an error occurred
	 */
	bool Init();

	/**
	 * @brief Store bridged devices count into settings
	 *
	 * @param count count value to be stored
	 * @return true if key has been written successfully
	 * @return false an error occurred
	 */
	bool StoreBridgedDevicesCount(uint8_t count);

	/**
	 * @brief Load bridged devices count from settings
	 *
	 * @param count reference to the count object to be filled with loaded data
	 * @return true if key has been loaded successfully
	 * @return false an error occurred
	 */
	bool LoadBridgedDevicesCount(uint8_t &count);

	/**
	 * @brief Store bridged devices indexes into settings
	 *
	 * @param indexes address of array containing indexes to be stored
	 * @param count size of indexes array to be stored
	 * @return true if key has been written successfully
	 * @return false an error occurred
	 */
	bool StoreBridgedDevicesIndexes(uint8_t *indexes, uint8_t count);

	/**
	 * @brief Load bridged devices indexes from settings
	 *
	 * @param indexes address of indexes array to be filled with loaded data
	 * @param maxCount maximum size that can be used for indexes array
	 * @param count reference to the count object to be filled with size of actually loaded data
	 * @return true if key has been loaded successfully
	 * @return false an error occurred
	 */
	bool LoadBridgedDevicesIndexes(uint8_t *indexes, uint8_t maxCount, size_t &count);

	/**
	 * @brief Load bridged device from settings
	 *
	 * If the caller wants to load the optional user data, the mUserData must be set to the valid buffer to store
	 * data and mUserDataSize must be set to this data size. On success, the method overrides mUserDataSize with
	 * the data size actually obtained from the storage.
	 *
	 * @param device instance of bridged device object to be filled with loaded data.
	 * @param index index describing specific bridged device
	 * @return true if key has been loaded successfully
	 * @return false an error occurred
	 */
	bool LoadBridgedDevice(BridgedDevice &device, uint8_t index);

	/**
	 * @brief Store bridged device into settings. Helper method allowing to store endpoint id, node label and device
	 * type of specific bridged device using a single call.
	 *
	 * @param device instance of bridged device object to be stored
	 * @param index index describing specific bridged device
	 * @return true if key has been written successfully
	 * @return false an error occurred
	 */
	bool StoreBridgedDevice(BridgedDevice &device, uint8_t index);

	/**
	 * @brief Remove bridged device entry from settings
	 *
	 * @param bridgedDeviceIndex index describing specific bridged device to be removed
	 * @return true if key entry has been removed successfully
	 * @return false an error occurred
	 */
	bool RemoveBridgedDevice(uint8_t bridgedDeviceIndex);

private:
	/**
	 * @brief Provides backward compatibility between non-compatible data scheme versions.
	 *
	 * It checks the used data scheme version based on version key value.
	 * If the version key is present it means that the new keys structure is used (only version
	 * equal to 1 is for now valid and means post nRF Connect SDK 2.6.0).
	 * Otherwise, the deprecated structure is used, so the migration has to be done. In such a case, method loads
	 * the data using key names from an old scheme, removes the old data and stores again using the new key names.
	 *
	 * @return true if migration was successful
	 * @return false an error occurred
	 */
	bool MigrateData();

	/* The below methods are deprecated and used only for the migration purposes between the older scheme versions.
	 */

	/**
	 * @brief Load bridged device endpoint id from settings
	 *
	 * @param endpointId reference to the endpoint id object to be filled with loaded data
	 * @param bridgedDeviceIndex index describing specific bridged device using given endpoint id
	 * @return true if key has been loaded successfully
	 * @return false an error occurred
	 */
	bool LoadBridgedDeviceEndpointId(uint16_t &endpointId, uint8_t bridgedDeviceIndex);

	/**
	 * @brief Remove bridged device's endpoint id entry from settings
	 *
	 * @param bridgedDeviceIndex index describing specific bridged device's endpoint id to be removed
	 * @return true if key entry has been removed successfully
	 * @return false an error occurred
	 */
	bool RemoveBridgedDeviceEndpointId(uint8_t bridgedDeviceIndex);

	/**
	 * @brief Load bridged device node label from settings
	 *
	 * @param label address of char array to be filled with loaded data
	 * @param labelMaxLength maximum size that can be used for label array
	 * @param labelLength reference to the labelLength object to be filled with size of actually loaded data
	 * @param bridgedDeviceIndex index describing specific bridged device using given node label
	 * @return true if key has been loaded successfully
	 * @return false an error occurred
	 */
	bool LoadBridgedDeviceNodeLabel(char *label, size_t labelMaxLength, size_t &labelLength,
					uint8_t bridgedDeviceIndex);

	/**
	 * @brief Remove bridged device's node label entry from settings
	 *
	 * @param bridgedDeviceIndex index describing specific bridged device's node label to be removed
	 * @return true if key entry has been removed successfully
	 * @return false an error occurred
	 */
	bool RemoveBridgedDeviceNodeLabel(uint8_t bridgedDeviceIndex);

	/**
	 * @brief Load bridged device Matter device type from settings
	 *
	 * @param deviceType reference to the device type object to be filled with loaded data
	 * @param bridgedDeviceIndex index describing specific bridged device using given device type
	 * @return true if key has been loaded successfully
	 * @return false an error occurred
	 */
	bool LoadBridgedDeviceType(uint16_t &deviceType, uint8_t bridgedDeviceIndex);

	/**
	 * @brief Remove bridged device's Matter device type entry from settings
	 *
	 * @param bridgedDeviceIndex index describing specific bridged device's device type to be removed
	 * @return true if key entry has been removed successfully
	 * @return false an error occurred
	 */
	bool RemoveBridgedDeviceType(uint8_t bridgedDeviceIndex);

#ifdef CONFIG_BRIDGED_DEVICE_BT
	/**
	 * @brief Load bridged device Bluetooth LE address from settings
	 *
	 * @param addr reference to the address object to be filled with loaded data
	 * @param bridgedDeviceIndex index describing specific bridged device using given address
	 * @return true if key has been loaded successfully
	 * @return false an error occurred
	 */
	bool LoadBtAddress(bt_addr_le_t &addr, uint8_t bridgedDeviceIndex);

	/**
	 * @brief Remove bridged device's Bluetooth LE address entry from settings
	 *
	 * @param bridgedDeviceIndex index describing specific bridged device's bluetooth address to be removed
	 * @return true if key entry has been removed successfully
	 * @return false an error occurred
	 */
	bool RemoveBtAddress(uint8_t bridgedDeviceIndex);
#endif

	static constexpr auto kMaxBufferSize = sizeof(MatterBridgedDevice);

	Nrf::PersistentStorageNode mBridge;
	Nrf::PersistentStorageNode mBridgedDevicesCount;
	Nrf::PersistentStorageNode mBridgedDevicesIndexes;
	Nrf::PersistentStorageNode mBridgedDevice;
	Nrf::PersistentStorageNode mVersion;

	/* The below fields are deprecated and used only for the migration purposes between the older scheme versions.
	 */
	Nrf::PersistentStorageNode mBridgedDeviceEndpointId;
	Nrf::PersistentStorageNode mBridgedDeviceNodeLabel;
	Nrf::PersistentStorageNode mBridgedDeviceType;
#ifdef CONFIG_BRIDGED_DEVICE_BT
	Nrf::PersistentStorageNode mBt;
	Nrf::PersistentStorageNode mBtAddress;
#endif
};

} /* namespace Nrf */
