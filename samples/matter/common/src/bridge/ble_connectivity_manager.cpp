/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "ble_connectivity_manager.h"

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
	const bt_addr_le_t *addr = bt_conn_get_dst(conn);
	char str_addr[BT_ADDR_LE_STR_LEN];
	int err;
	BLEBridgedDevice *device = nullptr;

	bt_addr_le_to_str(addr, str_addr, sizeof(str_addr));

	/* Find the created device instance based on address. */
	for (int i = 0; i < Instance().mCreatedDevicesCounter; i++) {
		if (memcmp(&Instance().mCreatedDevices[i].mAddr, addr,
			   sizeof(Instance().mCreatedDevices[i].mAddr)) == 0) {
			device = &Instance().mCreatedDevices[i];
			break;
		}
	}

	if (!device) {
		return;
	}

	LOG_INF("Connected: %s", str_addr);

	/* TODO: Add security validation. */

	/* Start GATT discovery for the device's service UUID. */
	err = bt_gatt_dm_start(conn, device->mServiceUuid, &discovery_cb, device);
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

	/* TODO: Implement connection re-establishment procedure. */
}

void BLEConnectivityManager::DiscoveryCompletedHandler(bt_gatt_dm *dm, void *context)
{
	LOG_INF("The GATT discovery completed");

	BLEBridgedDevice *device = reinterpret_cast<BLEBridgedDevice *>(context);
	bool discoveryResult = false;
	const bt_gatt_dm_attr *gatt_service_attr = bt_gatt_dm_service_get(dm);
	const bt_gatt_service_val *gatt_service = bt_gatt_dm_attr_service_val(gatt_service_attr);

	VerifyOrExit(device, );
	bt_gatt_dm_data_print(dm);
	VerifyOrExit(bt_uuid_cmp(gatt_service->uuid, device->mServiceUuid) == 0, );
	device->mConn = bt_gatt_dm_conn_get(dm);

	discoveryResult = true;
exit:

	device->mConnectedCallback(device, dm, discoveryResult, device->mConnectedCallbackContext);
}

void BLEConnectivityManager::DiscoveryNotFound(bt_conn *conn, void *context)
{
	LOG_ERR("GATT service could not be found during the discovery");

	BLEBridgedDevice *device = reinterpret_cast<BLEBridgedDevice *>(context);

	device->mConnectedCallback(device, nullptr, false, device->mConnectedCallbackContext);
}

void BLEConnectivityManager::DiscoveryError(bt_conn *conn, int err, void *context)
{
	LOG_ERR("The GATT discovery procedure failed with %d", err);

	BLEBridgedDevice *device = reinterpret_cast<BLEBridgedDevice *>(context);

	device->mConnectedCallback(device, nullptr, false, device->mConnectedCallbackContext);
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

CHIP_ERROR BLEConnectivityManager::Scan(ScanDoneCallback callback, void *context)
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

	k_timer_start(&mScanTimer, K_MSEC(kScanTimeoutMs), K_NO_WAIT);
	mScanActive = true;

	return CHIP_NO_ERROR;
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
	Instance().mScanDoneCallback(Instance().mScannedDevices,
						      Instance().mScannedDevicesCounter,
						      Instance().mScanDoneCallbackContext);
}

CHIP_ERROR BLEConnectivityManager::Connect(uint8_t index, BLEBridgedDevice::DeviceConnectedCallback callback,
					   void *context, bt_uuid *serviceUuid)
{
	/* Check if device selected to connect is present on the scanned devices list. */
	if (mScannedDevicesCounter <= index) {
		return CHIP_ERROR_INVALID_ARGUMENT;
	}

	/* Verify whether there is a free space for a new device. */
	if (mCreatedDevicesCounter >= kMaxCreatedDevices) {
		return CHIP_ERROR_NO_MEMORY;
	}

	if (!callback || !serviceUuid) {
		return CHIP_ERROR_INVALID_ARGUMENT;
	}

	StopScan();

	bt_conn *conn;

	int err = bt_conn_le_create(&mScannedDevices[index].mAddr, create_param, &mScannedDevices[index].mConnParam,
				    &conn);

	if (err) {
		LOG_ERR("Creating connection failed (err %d)", err);
		return System::MapErrorZephyr(err);
	}

	mCreatedDevices[mCreatedDevicesCounter].mAddr = mScannedDevices[index].mAddr;
	mCreatedDevices[mCreatedDevicesCounter].mConnectedCallback = callback;
	mCreatedDevices[mCreatedDevicesCounter].mConnectedCallbackContext = context;
	mCreatedDevices[mCreatedDevicesCounter].mServiceUuid = serviceUuid;
	mCreatedDevicesCounter++;

	return CHIP_NO_ERROR;
}

BLEBridgedDevice *BLEConnectivityManager::FindBLEBridgedDevice(bt_conn *conn)
{
	bt_conn_info info_in, info_local;

	VerifyOrExit(conn, );
	VerifyOrExit(bt_conn_get_info(conn, &info_in) == 0, );

	/* Find BLE device that matches given connection id. */
	for (int i = 0; i < mCreatedDevicesCounter; i++) {
		if (bt_conn_get_info(mCreatedDevices[i].mConn, &info_local) == 0) {
			if (info_local.id == info_in.id) {
				return &mCreatedDevices[i];
			}
		}
	}

exit:

	return nullptr;
}
