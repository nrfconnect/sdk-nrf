/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "matter_bridged_device.h"
#include "ps_storage/persistent_storage_util.h"

#ifdef CONFIG_BRIDGED_DEVICE_BT
#include <zephyr/bluetooth/addr.h>
#endif

/*
 * The class implements the following key-values storage structure:
 *
 * /br/
 *		/brd_cnt/ /<uint8_t>/
 *		/brd_ids/ /<uint8_t[brd_cnt]>/
 * 		/brd/
 *			/eid/
 *				/0/	/<uint16_t>/
 *				/1/	/<uint16_t>/
 *				.
 *				.
 *				/n/ /<uint16_t>/
 *			/label/
 *				/0/	/<char[]>/
 *				/1/	/<char[]>/
 *				.
 *				.
 *				/n/ /<char[]>/
 *			/type/
 *				/0/	/<uint16_t>/
 *				/1/	/<uint16_t>/
 *				.
 *				.
 *				/n/ /<uint16_t>/
 *			/bt/
 *				/addr/
 *					/0/	/<bt_addr_le_t>/
 *					/1/	/<bt_addr_le_t>/
 *					.
 *					.
 *					/n/	/<bt_addr_le_t>/
 */
class BridgeStorageManager {
public:
	static constexpr auto kMaxIndexLength = 3;

	BridgeStorageManager()
		: mBridge("br", strlen("br")), mBridgedDevicesCount("brd_cnt", strlen("brd_cnt"), &mBridge),
		  mBridgedDevicesIndexes("brd_ids", strlen("brd_ids"), &mBridge),
		  mBridgedDevice("brd", strlen("brd"), &mBridge),
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
	bool Init() { return PersistentStorage::Instance().Init(); }

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
	 * @brief Store bridged device endpoint id into settings
	 *
	 * @param endpointId endpoint id value to be stored
	 * @param bridgedDeviceIndex index describing specific bridged device using given endpoint id
	 * @return true if key has been written successfully
	 * @return false an error occurred
	 */
	bool StoreBridgedDeviceEndpointId(uint16_t endpointId, uint8_t bridgedDeviceIndex);

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
	 * @brief Store bridged device node label into settings
	 *
	 * @param label address of node label to be stored
	 * @param labelLength length of node label
	 * @param bridgedDeviceIndex index describing specific bridged device using given node label
	 * @return true if key has been written successfully
	 * @return false an error occurred
	 */
	bool StoreBridgedDeviceNodeLabel(const char *label, size_t labelLength, uint8_t bridgedDeviceIndex);

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
	 * @brief Store bridged device Matter device type into settings
	 *
	 * @param deviceType Matter device type to be stored
	 * @param bridgedDeviceIndex index describing specific bridged device using given device type
	 * @return true if key has been written successfully
	 * @return false an error occurred
	 */
	bool StoreBridgedDeviceType(uint16_t deviceType, uint8_t bridgedDeviceIndex);

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

	/**
	 * @brief Store bridged device into settings. Helper method allowing to store endpoint id, node label and device
	 * type of specific bridged device using a single call.
	 *
	 * @param device address of bridged device object to be stored
	 * @param index index describing specific bridged device
	 * @return true if key has been written successfully
	 * @return false an error occurred
	 */
	bool StoreBridgedDevice(const MatterBridgedDevice *device, uint8_t index);

#ifdef CONFIG_BRIDGED_DEVICE_BT

	/**
	 * @brief Store bridged device Bluetooth LE address into settings
	 *
	 * @param addr Bluetooth LE address to be stored
	 * @param bridgedDeviceIndex index describing specific bridged device using given address
	 * @return true if key has been written successfully
	 * @return false an error occurred
	 */
	bool StoreBtAddress(bt_addr_le_t addr, uint8_t bridgedDeviceIndex);

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

private:
	PersistentStorageNode mBridge;
	PersistentStorageNode mBridgedDevicesCount;
	PersistentStorageNode mBridgedDevicesIndexes;
	PersistentStorageNode mBridgedDevice;
	PersistentStorageNode mBridgedDeviceEndpointId;
	PersistentStorageNode mBridgedDeviceNodeLabel;
	PersistentStorageNode mBridgedDeviceType;
#ifdef CONFIG_BRIDGED_DEVICE_BT
	PersistentStorageNode mBt;
	PersistentStorageNode mBtAddress;
#endif
};
