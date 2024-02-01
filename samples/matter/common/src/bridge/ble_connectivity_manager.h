/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "bridged_device_data_provider.h"

#include <bluetooth/gatt_dm.h>
#include <bluetooth/scan.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/sys/slist.h>

namespace Nrf
{

/* Forward declarations. */
struct BLEBridgedDevice;
struct BLEBridgedDeviceProvider;

class BLEConnectivityManager {
public:
	static constexpr uint16_t kScanTimeoutMs = 10000;
	static constexpr uint16_t kMaxScannedDevices = 16;
	/* One BT connection is reserved for the Matter service purposes. */
	static constexpr uint16_t kMaxConnectedDevices = CONFIG_BT_MAX_CONN - 1;
	static constexpr uint8_t kMaxServiceUuids = CONFIG_BT_SCAN_UUID_CNT;

	struct ScannedDevice {
		bt_addr_le_t mAddr;
		bt_le_conn_param mConnParam;
		uint16_t mUuid;
	};

	struct ScanResult {
		ScannedDevice *mDevices = nullptr;
		uint8_t mCount = 0;
	};

private:
	class Recovery {
		friend class BLEConnectivityManager;

		/* Recovery intervals in seconds. */
		constexpr static auto kRecoveryIntervalSec = 1;
		constexpr static auto kRecoveryMaxIntervalSec = CONFIG_BRIDGE_BT_RECOVERY_MAX_INTERVAL;

		constexpr static auto kRecoveryScanTimeoutMs = CONFIG_BRIDGE_BT_RECOVERY_SCAN_TIMEOUT_MS;

		struct ListItem : public sys_snode_t {
			BLEBridgedDeviceProvider *mProvider = nullptr;
		};

	public:
		Recovery();
		~Recovery() { CancelTimer(); }
		void NotifyProviderToRecover(BLEBridgedDeviceProvider *provider);

	private:
		BLEBridgedDeviceProvider *GetProvider(sys_slist_t *list);
		bool PutProvider(BLEBridgedDeviceProvider *provider, sys_slist_t *list);
		bool IsNeeded() { return !sys_slist_is_empty(&mListToRecover); }
		void StartTimer();
		void CancelTimer() { k_timer_stop(&mRecoveryTimer); }
		void RemoveRecovered(BLEBridgedDeviceProvider *provider);
		uint16_t GetFailedRecoveryAttempts();

		static void TimerTimeoutCallback(k_timer *timer);

		sys_slist_t mListToRecover;
		sys_slist_t mListToReconnect;
		k_timer mRecoveryTimer;
	};

	struct DiscoveryHandlerCtx {
		BLEBridgedDeviceProvider *mProvider;
		bt_gatt_dm *mDiscoveryData;
	};

public:
	using DeviceConnectedCallback = CHIP_ERROR (*)(bool success, void *context);
	using ScanDoneCallback = void (*)(ScanResult &result, void *context);
	using ConnectionSecurityRequestCallback = void (*)(void *context);

	struct ConnectionSecurityRequest {
		ConnectionSecurityRequestCallback mCallback;
		void *mContext;
	};

	/**
	 * @brief Initialize BLEConnectivityManager instance.
	 *
	 * @param serviceUuids the address of array containing all Bluetooth service UUIDs to be handled by manager
	 * @param serviceUuidsCount the number of services on the serviceUuids list
	 * @return CHIP_NO_ERROR on success
	 * @return other error code on failure
	 */
	CHIP_ERROR Init(bt_uuid **serviceUuids, uint8_t serviceUuidsCount);

	/**
	 * @brief Start scanning for Bluetooth LE peripheral devices advertising service UUIDs passed in @ref Init
	 * method.
	 *
	 * @param callback callback with results to be called once scan is done
	 * @param context context that will be passed as an argument once calling @ref callback
	 * @param scanTimeoutMs optional timeout on scan operation in ms (by default 10 s)
	 * @return CHIP_NO_ERROR on success
	 * @return other error code on failure
	 */
	CHIP_ERROR Scan(ScanDoneCallback callback, void *context, uint32_t scanTimeoutMs = kScanTimeoutMs);

	/**
	 * @brief Stop scanning operation.
	 *
	 * @return CHIP_NO_ERROR on success
	 * @return other error code on failure
	 */
	CHIP_ERROR StopScan();

	/**
	 * @brief Create connection to the Bluetooth LE device, managed by BLEBridgedDeviceProvider object. It is
	 * necessary to first add the provider to the manager's list using @ref AddBLEProvider method.
	 *
	 * @param provider address of a valid provider object
	 * @param request address of connection request object for handling additional security information requiered by
	 *the connection. Can be nullptr, if connection does not use security.
	 * @return CHIP_NO_ERROR on success
	 * @return other error code on failure
	 */
	CHIP_ERROR Connect(BLEBridgedDeviceProvider *provider, ConnectionSecurityRequest *request = nullptr);

	/**
	 * @brief Create connection to the first Bluetooth LE device on the @ref mProvidersToRecover recovery list.
	 *
	 * @return CHIP_NO_ERROR on success
	 * @return other error code on failure
	 */
	CHIP_ERROR Reconnect(BLEBridgedDeviceProvider *provider);

	/**
	 * @brief Get BLE provider that uses the specified connection object.
	 *
	 * @param address Bluetooth LE address used by the provider.
	 * @return address of provider on success
	 * @return nullptr on failure
	 */
	BLEBridgedDeviceProvider *FindBLEProvider(bt_addr_le_t address);

	/**
	 * @brief Add the BLE provider's address to the manager's list.
	 *
	 * @param provider address of a valid provider object to be added
	 * @return CHIP_NO_ERROR on success
	 * @return other error code on failure
	 */
	CHIP_ERROR AddBLEProvider(BLEBridgedDeviceProvider *provider);

	/**
	 * @brief Remove the BLE provider from the manager's list.
	 *
	 * @param address Bluetooth LE address used by the provider.
	 * @return CHIP_NO_ERROR on success
	 * @return other error code on failure
	 */
	CHIP_ERROR RemoveBLEProvider(bt_addr_le_t address);

	/**
	 * @brief Gets Bluetooth LE address of a device that was scanned before.
	 *
	 * @param address Bluetooth LE address to store the obtained address
	 * @param index index of Bluetooth LE device on the scanned devices list
	 * @return CHIP_NO_ERROR on success
	 * @return other error code if device is not present or argument is invalid
	 */
	CHIP_ERROR GetScannedDeviceAddress(bt_addr_le_t *address, uint8_t index);

	/**
	 * @brief Gets Bluetooth LE service UUID of a device that was scanned before.
	 *
	 * @param uuid Bluetooth LE service UUID to store the obtained UUID
	 * @param index index of Bluetooth LE device on the scanned devices list
	 * @return CHIP_NO_ERROR on success
	 * @return other error code if device is not present or argument is invalid
	 */
	CHIP_ERROR GetScannedDeviceUuid(uint16_t &uuid, uint8_t index);

	/**
	 * @brief Recover connection with the specified BLE provider. It is
	 * necessary to first add the provider to the manager's list using @ref AddBLEProvider method.
	 *
	 * @param provider address of a valid provider object to be recovered.
	 */
	void Recover(BLEBridgedDeviceProvider *provider) { mRecovery.NotifyProviderToRecover(provider); }

	CHIP_ERROR PrepareFilterForUuid();
	CHIP_ERROR PrepareFilterForAddress(bt_addr_le_t *addr);

	/* Public static callbacks for Bluetooth LE connection handling. */
	static void FilterMatch(bt_scan_device_info *device_info, bt_scan_filter_match *filter_match, bool connectable);
	static void ScanTimeoutCallback(k_timer *timer);
	static void ScanTimeoutHandle(intptr_t context);
	static void ConnectionHandler(bt_conn *conn, uint8_t conn_err);
	static void DisconnectionHandler(bt_conn *conn, uint8_t reason);
	static void DiscoveryCompletedHandler(bt_gatt_dm *dm, void *context);
	static void DiscoveryNotFound(bt_conn *conn, void *context);
	static void DiscoveryError(bt_conn *conn, int err, void *context);
	static int StartGattDiscovery(bt_conn *conn, BLEBridgedDeviceProvider *provider);

#ifdef CONFIG_BT_SMP
	static void SecurityChangedHandler(struct bt_conn *conn, bt_security_t level, enum bt_security_err err);
	static void AuthenticationCancel(struct bt_conn *conn);
	static void PasskeyEntry(struct bt_conn *conn);
	static void PairingComplete(struct bt_conn *conn, bool bonded);
	static void PairingFailed(struct bt_conn *conn, enum bt_security_err reason);

	/**
	 * @brief Set authentication pincode to confirm the specific Bluetooth LE connection.
	 *
	 * @param addr Bluetooth LE address of a device used in the connection
	 * @param pincode authentication pincode that fits in range of 0 - 999999
	 */
	void SetPincode(bt_addr_le_t addr, unsigned int pincode);
#endif /* CONFIG_BT_SMP */

	static BLEConnectivityManager &Instance()
	{
		static BLEConnectivityManager sInstance;
		return sInstance;
	}
	static void ReScanCallback(ScanResult &result, void *context);

private:
	bt_le_conn_param *GetScannedDeviceConnParams(bt_addr_le_t address);

	bool mScanActive;
	k_timer mScanTimer;
	uint8_t mScannedDevicesCounter;
	uint8_t mConnectedProvidersCounter;
	ScannedDevice mScannedDevices[kMaxScannedDevices];
	ScanResult mScanResult;
	BLEBridgedDeviceProvider *mConnectedProviders[kMaxConnectedDevices];
	bt_uuid *mServicesUuid[kMaxServiceUuids];
	uint8_t mServicesUuidCount;
	ScanDoneCallback mScanDoneCallback;
	void *mScanDoneCallbackContext;
#ifdef CONFIG_BT_SMP
	ConnectionSecurityRequest mConnectionSecurityRequest;
#endif /* CONFIG_BT_SMP */
	Recovery mRecovery;
};

} /* namespace Nrf */
