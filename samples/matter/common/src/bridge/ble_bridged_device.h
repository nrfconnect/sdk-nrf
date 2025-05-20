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

namespace Nrf
{

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
	BLEBridgedDeviceProvider(UpdateAttributeCallback updateCallback, InvokeCommandCallback commandCallback)
		: BridgedDeviceDataProvider(updateCallback, commandCallback)
	{
	}
	~BLEBridgedDeviceProvider() { BLEConnectivityManager::Instance().RemoveBLEProvider(GetBtAddress()); }

	virtual const bt_uuid *GetServiceUuid() = 0;
	virtual int ParseDiscoveredData(bt_gatt_dm *discoveredData) = 0;

	BLEBridgedDevice &GetBLEBridgedDevice() { return mDevice; }
	void SetConnectionObject(bt_conn *conn) { mDevice.mConn = conn; }
	bt_conn *GetConnectionObject() { return mDevice.mConn; }
	void RemoveConnectionObject() { mDevice.mConn = nullptr; }

	/**
	 * @brief Initialize BLE bridged device
	 *
	 * Sets the user provided initialization parameters of the BLE bridged device.
	 * The user provided connection callback is triggered as a final step of the connection establishment procedure.
	 * The context data can be used to perform next steps aiming to eventually bridge the BLE device with the Matter
	 * counterpart after the connection was successful. Note that it is user's responsibility to take care about the
	 * context data lifetime.
	 *
	 * @param address BT address of the device
	 * @param callback connection callback
	 * @param context connection callback context data
	 */
	void InitializeBridgedDevice(bt_addr_le_t address, BLEConnectivityManager::DeviceConnectedCallback callback,
				     void *context)
	{
		mDevice.mAddr = address;
		mDevice.mFirstConnectionCallback = callback;
		mDevice.mFirstConnectionCallbackContext = context;
		mDevice.mProvider = this;
	}

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
	 * @brief Confirm that the @ref mFirstConnectionCallback has been already called.
	 *
	 * This method informs the bridged device that the @ref mFirstConnectionCallback has been called
	 * and connection has been established successfully.
	 *
	 */
	void ConfirmInitialConnection() { mDevice.mInitiallyConnected = true; }

	bt_addr_le_t GetBtAddress() { return mDevice.mAddr; }

	/**
	 * @brief Get a number of failed recovery attempts for this provider.
	 */
	uint16_t GetFailedRecoveryAttempts() { return mFailedRecoveryAttempts; }

	/**
	 * @brief Inform provider that recovery attempt for it failed.
	 *
	 * This method increments the number of failed recovery attempts.
	 *
	 */
	void NotifyFailedRecovery()
	{
		if (mFailedRecoveryAttempts < UINT16_MAX) {
			mFailedRecoveryAttempts++;
		}
	}

	/**
	 * @brief Inform provider that recovery attempt for it succeeded.
	 *
	 * This method resets the number of failed recovery attempts.
	 *
	 */
	void NotifySuccessfulRecovery() { mFailedRecoveryAttempts = 0; }

protected:
	BLEBridgedDevice mDevice = { 0 };
	uint16_t mFailedRecoveryAttempts = 0;
};

} /* namespace Nrf */
