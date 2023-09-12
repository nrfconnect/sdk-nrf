/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "ble_connectivity_manager.h"
#include "bridged_device_data_provider.h"

#include <bluetooth/gatt_dm.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>

struct BLEBridgedDeviceProvider;

struct BLEBridgedDevice {
	bt_addr_le_t mAddr;
	BLEConnectivityManager::DeviceConnectedCallback mFirstConnectionCallback;
	void *mFirstConnectionCallbackContext;
	bool mInitiallyConnected = false; /* Indicates whether the first connection has been established and the device
					     has been successfully added to the Bridge. */
	bt_conn *mConn;
	BLEBridgedDeviceProvider *mProvider;
};

class BLEBridgedDeviceProvider : public BridgedDeviceDataProvider {
public:
	explicit BLEBridgedDeviceProvider(UpdateAttributeCallback callback) : BridgedDeviceDataProvider(callback) {}
	~BLEBridgedDeviceProvider() { BLEConnectivityManager::Instance().RemoveBLEProvider(GetBtAddress()); }

	virtual bt_uuid *GetServiceUuid() = 0;
	virtual int ParseDiscoveredData(bt_gatt_dm *discoveredData) = 0;

	void InitializeBridgedDevice(bt_addr_le_t address, BLEConnectivityManager::DeviceConnectedCallback callback,
				     void *context)
	{
		mDevice.mAddr = address;
		mDevice.mFirstConnectionCallback = callback;
		mDevice.mFirstConnectionCallbackContext = context;
		mDevice.mProvider = this;
	}

	BLEBridgedDevice &GetBLEBridgedDevice() { return mDevice; }
	void SetConnectionObject(bt_conn *conn) { mDevice.mConn = conn; }
	void RemoveConnectionObject() { mDevice.mConn = nullptr; }

	/**
	 * @brief Check if the bridged device has been initially connected.
	 *
	 * The bridged device should call the @ref mFirstConnectionCallback only when it is the first connection.
	 * This method returns indicates whether the callback has been called and the device is already initially
	 * connected.
	 *
	 * @return true if the bridged device has been initially connected.
	 * @return false if this is the first connection to the bridged device.
	 */
	bool IsInitiallyConnected() { return mDevice.mInitiallyConnected; }

	/**
	 * @brief Confirm that the @ref mFirstConnectionCallback has been already called .
	 *
	 * This method informs the bridged device that the @ref mFirstConnectionCallback has been called
	 * and connection has been established successfully.
	 *
	 */
	void ConfirmInitialConnection() { mDevice.mInitiallyConnected = true; }

	bt_addr_le_t GetBtAddress() { return mDevice.mAddr; }

protected:
	BLEBridgedDevice mDevice = { 0 };
};
