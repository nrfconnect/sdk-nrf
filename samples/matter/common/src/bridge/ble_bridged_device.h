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
	bt_conn *mConn;
	BLEBridgedDeviceProvider *mProvider;
};

class BLEBridgedDeviceProvider : public BridgedDeviceDataProvider {
public:
	explicit BLEBridgedDeviceProvider(UpdateAttributeCallback callback) : BridgedDeviceDataProvider(callback) {}
	~BLEBridgedDeviceProvider()
	{
		BLEConnectivityManager::Instance().RemoveBLEProvider(GetBtAddress());
	}

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

	bt_addr_le_t GetBtAddress() { return mDevice.mAddr; }

protected:
	BLEBridgedDevice mDevice = { 0 };
};
