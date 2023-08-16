/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "ble_connectivity_manager.h"
#include "ble_bridged_device.h"

#include <bluetooth/gatt_dm.h>
#include <bluetooth/scan.h>
#include <bluetooth/services/lbs.h>

#include <platform/CHIPDeviceLayer.h>

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

using namespace chip;

BT_SCAN_CB_INIT(scan_cb, BLEConnectivityManager::FilterMatch, NULL, NULL, NULL);

BT_CONN_CB_DEFINE(conn_callbacks) = { .connected = BLEConnectivityManager::ConnectionHandler,
				      .disconnected = BLEConnectivityManager::DisconnectionHandler };

static const struct bt_gatt_dm_cb discovery_cb = { .completed = BLEConnectivityManager::DiscoveryCompletedHandler,
						   .service_not_found = BLEConnectivityManager::DiscoveryNotFound,
						   .error_found = BLEConnectivityManager::DiscoveryError };

static struct bt_conn_le_create_param *create_param = BT_CONN_LE_CREATE_CONN;

void BLEConnectivityManager::FilterMatch(bt_scan_device_info *device_info, bt_scan_filter_match *filter_match,
					 bool connectable)
{
	auto scannedDevices = Instance().mScannedDevices;
	auto scannedDevicesCounter = Instance().mScannedDevicesCounter;
	/* Limit the number of devices that can be scanned. */
	if (scannedDevicesCounter >= kMaxScannedDevices) {
		return;
	}

	/* Verify the device address to make sure that the scan result will be handled only once for every device. */
	for (int i = 0; i < scannedDevicesCounter; i++) {
		if (memcmp(device_info->recv_info->addr, &scannedDevices[i].mAddr, sizeof(scannedDevices[i].mAddr)) ==
		    0) {
			return;
		}
	}

	memcpy(&scannedDevices[scannedDevicesCounter].mAddr, device_info->recv_info->addr,
	       sizeof(scannedDevices[scannedDevicesCounter].mAddr));
	memcpy(&scannedDevices[scannedDevicesCounter].mConnParam, device_info->conn_param,
	       sizeof(scannedDevices[scannedDevicesCounter].mConnParam));

	/* TODO: obtain more information from advertising data to help user distinguish the devices (e.g. BT name). */

	Instance().mScannedDevicesCounter++;
}

void BLEConnectivityManager::ConnectionHandler(bt_conn *conn, uint8_t conn_err)
{
	const bt_addr_le_t *dstAddr = bt_conn_get_dst(conn);
	char str_addr[BT_ADDR_LE_STR_LEN];
	int err;
	BLEBridgedDeviceProvider *provider = nullptr;

	bt_addr_le_to_str(dstAddr, str_addr, sizeof(str_addr));

	/* Find the created device instance based on address. */
	for (int i = 0; i < kMaxConnectedDevices; i++) {
		if (Instance().mConnectedProviders[i] == nullptr) {
			continue;
		}

		bt_addr_le_t addr = Instance().mConnectedProviders[i]->GetBtAddress();
		if (memcmp(&addr, dstAddr, sizeof(addr)) == 0) {
			provider = Instance().mConnectedProviders[i];
			break;
		}
	}

	/* The device with given address was not found.  */
	if (!provider) {
		return;
	}

	/* TODO: Add security validation. */

	if (conn_err && Instance().mRecovery.mRecoveryInProgress) {
		Instance().mRecovery.PutProvider(provider);
		return;
	}

	LOG_INF("Connected: %s", str_addr);

	/* Start GATT discovery for the device's service UUID. */
	err = bt_gatt_dm_start(conn, provider->GetServiceUuid(), &discovery_cb, provider);
	if (err) {
		LOG_ERR("Could not start the discovery procedure, error "
			"code: %d",
			err);
	}
}

void BLEConnectivityManager::DisconnectionHandler(bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];
	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Disconnected: %s (reason %u)", addr, reason);

	/* Try to find provider matching the connection */
	BLEBridgedDeviceProvider *provider = Instance().FindBLEProvider(*bt_conn_get_dst(conn));

	bt_conn_unref(conn);

	if (provider) {
		/* Verify whether the device should be recovered. */
		if (reason == BT_HCI_ERR_CONN_TIMEOUT) {
			Instance().mRecovery.NotifyProviderToRecover(provider);
			VerifyOrReturn(CHIP_NO_ERROR == provider->NotifyReachableStatusChange(false),
				       LOG_WRN("The device has not been notified about the status change."));
		}
	}
}

void BLEConnectivityManager::DiscoveryCompletedHandler(bt_gatt_dm *dm, void *context)
{
	LOG_INF("The GATT discovery completed");
	BLEBridgedDeviceProvider *provider = reinterpret_cast<BLEBridgedDeviceProvider *>(context);
	bool discoveryResult = false;
	const bt_gatt_dm_attr *gatt_service_attr = bt_gatt_dm_service_get(dm);
	const bt_gatt_service_val *gatt_service = bt_gatt_dm_attr_service_val(gatt_service_attr);

	VerifyOrExit(provider, );
	bt_gatt_dm_data_print(dm);
	VerifyOrExit(bt_uuid_cmp(gatt_service->uuid, provider->GetServiceUuid()) == 0, );

	discoveryResult = true;
exit:

	if (!Instance().mRecovery.mRecoveryInProgress) {
		provider->GetBLEBridgedDevice().mFirstConnectionCallback(
			dm, discoveryResult, provider->GetBLEBridgedDevice().mFirstConnectionCallbackContext);
	}

	if (provider) {
		VerifyOrReturn(CHIP_NO_ERROR == provider->NotifyReachableStatusChange(true),
			       LOG_WRN("The device has not been notified about the status change."));
	}

	Instance().mRecovery.mRecoveryInProgress = false;

	if (Instance().mRecovery.IsNeeded()) {
		Instance().mRecovery.StartTimer();
	}

	bt_gatt_dm_data_release(dm);
}

void BLEConnectivityManager::DiscoveryNotFound(bt_conn *conn, void *context)
{
	LOG_ERR("GATT service could not be found during the discovery");

	BLEBridgedDeviceProvider *provider = reinterpret_cast<BLEBridgedDeviceProvider *>(context);

	provider->GetBLEBridgedDevice().mFirstConnectionCallback(
		nullptr, false, provider->GetBLEBridgedDevice().mFirstConnectionCallbackContext);
}

void BLEConnectivityManager::DiscoveryError(bt_conn *conn, int err, void *context)
{
	LOG_ERR("The GATT discovery procedure failed with %d", err);

	BLEBridgedDeviceProvider *provider = reinterpret_cast<BLEBridgedDeviceProvider *>(context);

	provider->GetBLEBridgedDevice().mFirstConnectionCallback(
		nullptr, false, provider->GetBLEBridgedDevice().mFirstConnectionCallbackContext);
}

CHIP_ERROR BLEConnectivityManager::Init(bt_uuid **serviceUuids, uint8_t serviceUuidsCount)
{
	if (!serviceUuids || serviceUuidsCount > kMaxServiceUuids) {
		return CHIP_ERROR_INVALID_ARGUMENT;
	}

	memcpy(mServicesUuid, serviceUuids, serviceUuidsCount * sizeof(serviceUuids));
	mServicesUuidCount = serviceUuidsCount;

	k_timer_init(&mScanTimer, BLEConnectivityManager::ScanTimeoutCallback, nullptr);
	k_timer_user_data_set(&mScanTimer, this);

	bt_scan_init_param scan_init = {
		.connect_if_match = 0,
	};

	bt_scan_init(&scan_init);
	bt_scan_cb_register(&scan_cb);

	int err;

	for (uint8_t i = 0; i < mServicesUuidCount; i++) {
		err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_UUID, mServicesUuid[i]);

		if (err) {
			LOG_ERR("Failed to set scanning filter");
			return System::MapErrorZephyr(err);
		}
	}

	err = bt_scan_filter_enable(BT_SCAN_UUID_FILTER, false);
	if (err) {
		LOG_ERR("Filters cannot be turned on");
		return System::MapErrorZephyr(err);
	}

	return CHIP_NO_ERROR;
}

CHIP_ERROR BLEConnectivityManager::Scan(ScanDoneCallback callback, void *context, uint32_t scanTimeoutMs)
{
	if (mScanActive) {
		LOG_ERR("Scan is already in progress");
		return CHIP_ERROR_INCORRECT_STATE;
	}

	if (!callback) {
		return CHIP_ERROR_INVALID_ARGUMENT;
	}

	mScanDoneCallback = callback;
	mScanDoneCallbackContext = context;
	mScannedDevicesCounter = 0;

	int err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
	if (err) {
		LOG_ERR("Scanning failed to start (err %d)", err);
		return System::MapErrorZephyr(err);
	}

	k_timer_start(&mScanTimer, K_MSEC(scanTimeoutMs), K_NO_WAIT);
	mScanActive = true;

	return CHIP_NO_ERROR;
}

void BLEConnectivityManager::ReScanCallback(ScannedDevice *devices, uint8_t count, void *context)
{
	LOG_DBG("Lost devices no. %d", Instance().mRecovery.GetCurrentAmount());
	LOG_DBG("Found devices no. %d", Instance().mScannedDevicesCounter);

	if (Instance().mScannedDevicesCounter != 0 && !Instance().mRecovery.mRecoveryInProgress) {
		BLEBridgedDeviceProvider *deviceLost = Instance().mRecovery.GetProvider();
		if (deviceLost) {
			for (uint8_t idx = 0; idx < Instance().mScannedDevicesCounter; idx++) {
				auto &deviceScanned = Instance().mScannedDevices[idx];
				bt_addr_le_t address = deviceLost->GetBtAddress();

				if (memcmp(&deviceScanned.mAddr, &address, sizeof(address)) == 0) {
					LOG_DBG("Found the lost device");

					Instance().mRecovery.mIndexToRecover = idx;
					Instance().mRecovery.mCurrentProvider = deviceLost;
					Instance().mRecovery.mRecoveryInProgress = true;

					DeviceLayer::PlatformMgr().ScheduleWork(
						[](intptr_t context) {
							Instance().Reconnect(
								reinterpret_cast<BLEBridgedDeviceProvider *>(context));
						},
						reinterpret_cast<intptr_t>(deviceLost));
					break;
				}
			}
			if (!Instance().mRecovery.mRecoveryInProgress) {
				Instance().mRecovery.NotifyProviderToRecover(deviceLost);
			}
		}
	} else if (Instance().mRecovery.IsNeeded()) {
		Instance().mRecovery.StartTimer();
	}
}

CHIP_ERROR BLEConnectivityManager::StopScan()
{
	if (!mScanActive) {
		return CHIP_ERROR_INCORRECT_STATE;
	}

	int err = bt_scan_stop();
	if (err) {
		LOG_ERR("Scanning failed to stop (err %d)", err);
		return System::MapErrorZephyr(err);
	}

	mScanActive = false;

	return CHIP_NO_ERROR;
}

void BLEConnectivityManager::ScanTimeoutCallback(k_timer *timer)
{
	DeviceLayer::PlatformMgr().ScheduleWork(ScanTimeoutHandle, reinterpret_cast<intptr_t>(timer));
}

void BLEConnectivityManager::ScanTimeoutHandle(intptr_t context)
{
	Instance().StopScan();
	Instance().mScanDoneCallback(Instance().mScannedDevices, Instance().mScannedDevicesCounter,
				     Instance().mScanDoneCallbackContext);
}

CHIP_ERROR BLEConnectivityManager::Reconnect(BLEBridgedDeviceProvider *provider)
{
	if (!provider) {
		return CHIP_ERROR_INVALID_ARGUMENT;
	}

	StopScan();

	bt_conn *conn;

	int err = bt_conn_le_create(&mScannedDevices[Instance().mRecovery.mIndexToRecover].mAddr, create_param,
				    &mScannedDevices[Instance().mRecovery.mIndexToRecover].mConnParam, &conn);

	if (err) {
		LOG_ERR("Creating reconnection failed (err %d)", err);
		return System::MapErrorZephyr(err);
	} else {
		provider->SetConnectionObject(conn);
	}

	return CHIP_NO_ERROR;
}

CHIP_ERROR BLEConnectivityManager::Connect(BLEBridgedDeviceProvider *provider)
{
	if (!provider) {
		return CHIP_ERROR_INVALID_ARGUMENT;
	}

	StopScan();

	bt_conn *conn;
	bt_le_conn_param *connParams = GetScannedDeviceConnParams(provider->GetBLEBridgedDevice().mAddr);

	if (!connParams) {
		LOG_ERR("Failed to get conn params");
		return CHIP_ERROR_INTERNAL;
	}

	int err = bt_conn_le_create(&provider->GetBLEBridgedDevice().mAddr, create_param, connParams, &conn);

	if (err) {
		LOG_ERR("Creating connection failed (err %d)", err);
		return System::MapErrorZephyr(err);
	} else {
		provider->SetConnectionObject(conn);
	}

	return CHIP_NO_ERROR;
}

CHIP_ERROR BLEConnectivityManager::AddBLEProvider(BLEBridgedDeviceProvider *provider)
{
	if (!provider) {
		return CHIP_ERROR_INVALID_ARGUMENT;
	}

	if (mConnectedProvidersCounter >= kMaxConnectedDevices) {
		return CHIP_ERROR_NO_MEMORY;
	}

	/* Find first free slot to store new providers' address. */
	for (auto i = 0; i < kMaxConnectedDevices; i++) {
		if (mConnectedProviders[i] == nullptr) {
			mConnectedProviders[i] = provider;
			mConnectedProvidersCounter++;
			return CHIP_NO_ERROR;
		}
	}

	return CHIP_ERROR_NOT_FOUND;
}

CHIP_ERROR BLEConnectivityManager::RemoveBLEProvider(bt_addr_le_t address)
{
	BLEBridgedDeviceProvider *provider = nullptr;

	/* Find provider's address on the list and remove it. */
	for (auto i = 0; i < kMaxConnectedDevices; i++) {
		if (mConnectedProviders[i] == nullptr) {
			continue;
		}

		bt_addr_le_t addr = mConnectedProviders[i]->GetBtAddress();
		if (memcmp(&addr, &address, sizeof(address)) == 0) {
			provider = mConnectedProviders[i];
			mConnectedProviders[i] = nullptr;
			mConnectedProvidersCounter--;
			break;
		}
	}

	if (!provider) {
		return CHIP_ERROR_NOT_FOUND;
	}

	if (!provider->GetBLEBridgedDevice().mConn) {
		return CHIP_ERROR_INTERNAL;
	}

	/* Release the connection immediately in case of disconnection failed, as callback will not be called. */
	if (bt_conn_disconnect(provider->GetBLEBridgedDevice().mConn, BT_HCI_ERR_REMOTE_USER_TERM_CONN) != 0) {
		bt_conn_unref(provider->GetBLEBridgedDevice().mConn);
	}

	return CHIP_NO_ERROR;
}

BLEBridgedDeviceProvider *BLEConnectivityManager::FindBLEProvider(bt_addr_le_t address)
{
	/* Find BLE provider that matches given address. */
	for (int i = 0; i < kMaxConnectedDevices; i++) {
		if (!mConnectedProviders[i]) {
			continue;
		}

		bt_addr_le_t addr = mConnectedProviders[i]->GetBtAddress();

		if (memcmp(&addr, &address, sizeof(addr)) == 0) {
			return mConnectedProviders[i];
		}
	}

	return nullptr;
}

CHIP_ERROR BLEConnectivityManager::GetScannedDeviceAddress(bt_addr_le_t *address, uint8_t index)
{
	if (address == nullptr || index >= mScannedDevicesCounter) {
		return CHIP_ERROR_INVALID_ARGUMENT;
	}

	memcpy(address, &mScannedDevices[index].mAddr, sizeof(mScannedDevices[index].mAddr));

	return CHIP_NO_ERROR;
}

bt_le_conn_param *BLEConnectivityManager::GetScannedDeviceConnParams(bt_addr_le_t address)
{
	for (auto i = 0; i < mScannedDevicesCounter; i++) {
		if (memcmp(&mScannedDevices[i].mAddr, &address, sizeof(address)) == 0) {
			return &mScannedDevices[i].mConnParam;
		}
	}

	return nullptr;
}

BLEConnectivityManager::Recovery::Recovery()
{
	ring_buf_init(&mRingBuf, sizeof(mProvidersToRecover), reinterpret_cast<uint8_t *>(mProvidersToRecover));
	k_timer_init(&mRecoveryTimer, TimerTimeoutCallback, nullptr);
	k_timer_user_data_set(&mRecoveryTimer, this);
}

void BLEConnectivityManager::Recovery::NotifyProviderToRecover(BLEBridgedDeviceProvider *provider)
{
	if (provider) {
		PutProvider(provider);
		StartTimer();
	}
}

void BLEConnectivityManager::Recovery::TimerTimeoutCallback(k_timer *timer)
{
	LOG_DBG("Re-scanning BLE connections...");
	DeviceLayer::PlatformMgr().ScheduleWork(
		[](intptr_t) { Instance().Scan(ReScanCallback, nullptr, kRecoveryScanTimeoutMs); }, 0);
}

BLEBridgedDeviceProvider *BLEConnectivityManager::Recovery::GetProvider()
{
	uint32_t deviceAddr;
	int ret = ring_buf_get(&mRingBuf, reinterpret_cast<uint8_t *>(&deviceAddr), sizeof(deviceAddr));
	if (ret == sizeof(deviceAddr)) {
		return reinterpret_cast<BLEBridgedDeviceProvider *>(deviceAddr);
	}

	return nullptr;
}

bool BLEConnectivityManager::Recovery::PutProvider(BLEBridgedDeviceProvider *provider)
{
	int ret = ring_buf_put(&mRingBuf, reinterpret_cast<uint8_t *>(&provider), sizeof(provider));
	return ret == sizeof(provider);
}
