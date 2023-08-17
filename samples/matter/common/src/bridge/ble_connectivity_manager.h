/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "ble_bridged_device.h"
#include "bridged_device_data_provider.h"

#include <bluetooth/scan.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/ring_buffer.h>

class BLEConnectivityManager {
public:
	static constexpr uint16_t kScanTimeoutMs = 10000;
	static constexpr uint16_t kMaxScannedDevices = 16;
	/* One BT connection is reserved for the Matter service purposes. */
	static constexpr uint16_t kMaxCreatedDevices = CONFIG_BT_MAX_CONN - 1;
	static constexpr uint8_t kMaxServiceUuids = CONFIG_BT_SCAN_UUID_CNT;

private:
	class Recovery {
		friend class BLEConnectivityManager;
		constexpr static auto kRecoveryIntervalMs = CONFIG_BRIDGE_BT_RECOVERY_INTERVAL_MS;
		constexpr static auto kRecoveryScanTimeoutMs = CONFIG_BRIDGE_BT_RECOVERY_SCAN_TIMEOUT_MS;

	public:
		Recovery();
		~Recovery() { CancelTimer(); }
		void NotifyLostDevice(BLEBridgedDevice *device);

	private:
		BLEBridgedDevice *GetDevice();
		bool PutDevice(BLEBridgedDevice *device);
		bool IsNeeded() { return !ring_buf_is_empty(&mRingBuf); }
		void StartTimer() { k_timer_start(&mRecoveryTimer, K_MSEC(kRecoveryIntervalMs), K_NO_WAIT); }
		void CancelTimer() { k_timer_stop(&mRecoveryTimer); }
		size_t GetCurrentAmount() { return ring_buf_size_get(&mRingBuf) / sizeof(BLEBridgedDevice *); }

		static void TimerTimeoutCallback(k_timer *timer);

		BLEBridgedDevice *mDevicesToRecover[BLEConnectivityManager::kMaxCreatedDevices];
		BLEBridgedDevice *mCurrentDevice = nullptr;
		bool mRecoveryInProgress = false;
		ring_buf mRingBuf;
		uint8_t mIndexToRecover;
		k_timer mRecoveryTimer;
	};

public:
	struct ScannedDevice {
		bt_addr_le_t mAddr;
		bt_le_conn_param mConnParam;
	};

	using ScanDoneCallback = void (*)(ScannedDevice *devices, uint8_t count, void *context);

	CHIP_ERROR Init(bt_uuid **serviceUuids, uint8_t serviceUuidsCount);
	CHIP_ERROR Scan(ScanDoneCallback callback, void *context, uint32_t scanTimeoutMs = kScanTimeoutMs);
	CHIP_ERROR StopScan();
	CHIP_ERROR Connect(uint8_t index, BLEBridgedDevice::DeviceConnectedCallback callback, void *context,
			   bt_uuid *serviceUuid);
	CHIP_ERROR Reconnect();
	BLEBridgedDevice *FindBLEBridgedDevice(bt_conn *conn);

	static void FilterMatch(bt_scan_device_info *device_info, bt_scan_filter_match *filter_match, bool connectable);
	static void ScanTimeoutCallback(k_timer *timer);
	static void ScanTimeoutHandle(intptr_t context);
	static void ConnectionHandler(bt_conn *conn, uint8_t conn_err);
	static void DisconnectionHandler(bt_conn *conn, uint8_t reason);
	static void DiscoveryCompletedHandler(bt_gatt_dm *dm, void *context);
	static void DiscoveryNotFound(bt_conn *conn, void *context);
	static void DiscoveryError(bt_conn *conn, int err, void *context);

	static BLEConnectivityManager &Instance()
	{
		static BLEConnectivityManager sInstance;
		return sInstance;
	}
	static void ReScanCallback(ScannedDevice *devices, uint8_t count, void *context);

private:
	bool mScanActive;
	k_timer mScanTimer;
	uint8_t mScannedDevicesCounter;
	uint8_t mCreatedDevicesCounter;
	ScannedDevice mScannedDevices[kMaxScannedDevices];
	BLEBridgedDevice mCreatedDevices[kMaxCreatedDevices];
	bt_uuid *mServicesUuid[kMaxServiceUuids];
	uint8_t mServicesUuidCount;
	ScanDoneCallback mScanDoneCallback;
	void *mScanDoneCallbackContext;
	Recovery mRecovery;
};
