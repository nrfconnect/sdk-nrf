/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "ble_connectivity_manager.h"
#include "ble_bridged_device.h"

#include <bluetooth/gatt_dm.h>
#include <bluetooth/scan.h>

#include <platform/CHIPDeviceLayer.h>

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

using namespace chip;

BT_SCAN_CB_INIT(scan_cb, Nrf::BLEConnectivityManager::FilterMatch, NULL, NULL, NULL);

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = Nrf::BLEConnectivityManager::ConnectionHandler,
	.disconnected = Nrf::BLEConnectivityManager::DisconnectionHandler,
#ifdef CONFIG_BT_SMP
	.security_changed = Nrf::BLEConnectivityManager::SecurityChangedHandler,
#endif /* CONFIG_BT_SMP */
};

#ifdef CONFIG_BT_SMP
static const bt_conn_auth_cb auth_callbacks = {
	.passkey_entry = Nrf::BLEConnectivityManager::PasskeyEntry,
	.cancel = Nrf::BLEConnectivityManager::AuthenticationCancel,
};

static bt_conn_auth_info_cb conn_auth_info_callbacks = { .pairing_complete =
								 Nrf::BLEConnectivityManager::PairingComplete,
							 .pairing_failed = Nrf::BLEConnectivityManager::PairingFailed };
#endif /* CONFIG_BT_SMP */

static const struct bt_gatt_dm_cb discovery_cb = { .completed = Nrf::BLEConnectivityManager::DiscoveryCompletedHandler,
						   .service_not_found = Nrf::BLEConnectivityManager::DiscoveryNotFound,
						   .error_found = Nrf::BLEConnectivityManager::DiscoveryError };

static struct bt_conn_le_create_param *create_param = BT_CONN_LE_CREATE_CONN;

namespace Nrf
{

void BLEConnectivityManager::FilterMatch(bt_scan_device_info *device_info, bt_scan_filter_match *filter_match,
					 bool connectable)
{
	if (!filter_match) {
		LOG_ERR("Filter match error");
		return;
	}

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

	scannedDevices[scannedDevicesCounter].mAddr = *device_info->recv_info->addr;
	scannedDevices[scannedDevicesCounter].mConnParam = *device_info->conn_param;

	if (!filter_match->uuid.match) {
		return;
	}

	scannedDevices[scannedDevicesCounter].mUuid = BT_UUID_16(filter_match->uuid.uuid[0])->val;
	Instance().mScannedDevicesCounter++;
}

int BLEConnectivityManager::StartGattDiscovery(bt_conn *conn, BLEBridgedDeviceProvider *provider)
{
	/* Start GATT discovery for the device's service UUID. */
	int err = bt_gatt_dm_start(conn, provider->GetServiceUuid(), &discovery_cb, provider);
	if (err) {
		LOG_ERR("Could not start the discovery procedure, error "
			"code: %d",
			err);
	}
	return err;
}

void BLEConnectivityManager::ConnectionHandler(bt_conn *conn, uint8_t conn_err)
{
	const bt_addr_le_t *dstAddr = bt_conn_get_dst(conn);
	BLEBridgedDeviceProvider *provider = nullptr;
	int err = 0;
	bool securityFailed = false;

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

	/* The device with the given address was not found. */
	if (!provider) {
		return;
	}

	/* If there was an error during the initial connection, we should notify the application */
	bool firstConnFailed = (conn_err && !provider->IsInitiallyConnected());
	VerifyOrExit(!firstConnFailed, err = conn_err);

	/* The device was successfully recovered. */
	if (conn_err == 0 && provider->IsInitiallyConnected()) {
		Instance().mRecovery.RemoveRecovered(provider);
		provider->NotifySuccessfulRecovery();
	}

	if (!sys_slist_is_empty(&Instance().mRecovery.mListToReconnect)) {
		/* There is another provider to re-connect, schedule this operation. */
		BLEBridgedDeviceProvider *providerToRecover =
			Instance().mRecovery.GetProvider(&Instance().mRecovery.mListToReconnect);
		DeviceLayer::PlatformMgr().ScheduleWork(
			[](intptr_t context) {
				Instance().Reconnect(reinterpret_cast<BLEBridgedDeviceProvider *>(context));
			},
			reinterpret_cast<intptr_t>(providerToRecover));
	} else if (!sys_slist_is_empty(&Instance().mRecovery.mListToRecover)) {
		/* There are pending providers to recover and no more scanned ones, schedule next scan operation. */
		Instance().mRecovery.StartTimer();
	}

	char addrStr[BT_ADDR_LE_STR_LEN];
	bt_addr_le_to_str(dstAddr, addrStr, sizeof(addrStr));
	LOG_INF("Connected: %s", addrStr);

#if defined(CONFIG_BT_SMP)
	err = bt_conn_set_security(conn, static_cast<bt_security_t>(CONFIG_BRIDGE_BT_MINIMUM_SECURITY_LEVEL));
	VerifyOrExit(err == 0, securityFailed = true);

	/* Start GATT discovery only if this specific device was successfully connected before. Otherwise, it will be
	 * called after a successful pairing. */
	if (provider->IsInitiallyConnected()) {
		err = StartGattDiscovery(conn, provider);
		VerifyOrExit(err == 0, );
	}
#else
	err = StartGattDiscovery(conn, provider);
	VerifyOrExit(err == 0, );
#endif

	return;

exit:
	if (securityFailed) {
		LOG_ERR("Failed to set the connection security (%d)", err);
	} else {
		LOG_ERR("The connection failed (%d)", err);
	}
	/* Trigger the connection callback to inform the application that the connection procedure failed. */
	provider->GetBLEBridgedDevice().mFirstConnectionCallback(
		false, provider->GetBLEBridgedDevice().mFirstConnectionCallbackContext);
}

void BLEConnectivityManager::DisconnectionHandler(bt_conn *conn, uint8_t reason)
{
	char addrStr[BT_ADDR_LE_STR_LEN];
	bt_addr_le_to_str(bt_conn_get_dst(conn), addrStr, sizeof(addrStr));
	LOG_INF("Disconnected: %s (reason %u)", addrStr, reason);

	/* Try to find provider matching the connection */
	BLEBridgedDeviceProvider *provider = Instance().FindBLEProvider(*bt_conn_get_dst(conn));

	if (provider) {
		bt_conn_unref(provider->GetBLEBridgedDevice().mConn);
		provider->SetConnectionObject(nullptr);

		/* Verify whether the device should be recovered. */
		if (reason == BT_HCI_ERR_CONN_TIMEOUT) {
			provider->RemoveConnectionObject();
			Instance().mRecovery.NotifyProviderToRecover(provider);
			VerifyOrReturn(CHIP_NO_ERROR == provider->NotifyReachableStatusChange(false),
				       LOG_WRN("The device has not been notified about the status change."));
		}
	}
}

#if defined(CONFIG_BT_SMP)
void BLEConnectivityManager::SecurityChangedHandler(struct bt_conn *conn, bt_security_t level, enum bt_security_err err)
{
	if (err) {
		LOG_ERR("Security failed: level %d err %d", level, err);
	} else {
		LOG_INF("Security changed: level %d", level);
	}
}

void BLEConnectivityManager::AuthenticationCancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_ERR("Pairing cancelled: %s\n", addr);

	Instance().RemoveBLEProvider(*bt_conn_get_dst(conn));
}

void BLEConnectivityManager::PasskeyEntry(struct bt_conn *conn)
{
	/* Invoke callback that will forward information to user that pincode has to be provided. */
	if (Instance().mConnectionSecurityRequest.mCallback) {
		Instance().mConnectionSecurityRequest.mCallback(Instance().mConnectionSecurityRequest.mContext);
	}
}

void BLEConnectivityManager::PairingComplete(struct bt_conn *conn, bool bonded)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Pairing completed: %s, bonded: %d\n", addr, bonded);

	BLEBridgedDeviceProvider *provider = nullptr;

	/* Find the created device instance based on address. */
	for (int i = 0; i < kMaxConnectedDevices; i++) {
		if (Instance().mConnectedProviders[i] == nullptr) {
			continue;
		}

		bt_addr_le_t addr = Instance().mConnectedProviders[i]->GetBtAddress();
		if (memcmp(&addr, bt_conn_get_dst(conn), sizeof(addr)) == 0) {
			provider = Instance().mConnectedProviders[i];
			break;
		}
	}

	/* The device with given address was not found.  */
	if (!provider) {
		LOG_ERR("The Bluetooth LE provider cannot be found for the paired device, GATT discovery will not be started");
		return;
	}

	/* Once pairing completed successfully, start GATT discovery procedure. */
	StartGattDiscovery(conn, provider);
}

void BLEConnectivityManager::PairingFailed(struct bt_conn *conn, enum bt_security_err reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_ERR("Pairing failed conn: %s, reason %d\n", addr, reason);

	Instance().RemoveBLEProvider(*bt_conn_get_dst(conn));
}

void BLEConnectivityManager::SetPincode(bt_addr_le_t addr, unsigned int pincode)
{
	/* Iterate through the Bluetooth providers list to find the one matching given address and obtain its connection
	 * object. */
	for (auto i = 0; i < kMaxConnectedDevices; i++) {
		if (mConnectedProviders[i]) {
			bt_addr_le_t address = mConnectedProviders[i]->GetBtAddress();
			if (memcmp(&address, &addr, sizeof(address)) == 0) {
				int err = bt_conn_auth_passkey_entry(mConnectedProviders[i]->GetConnectionObject(),
								     pincode);
				if (err != 0) {
					LOG_ERR("Failed to set authentication pincode, reason %d", err);
				}
			}
		}
	}
}
#endif /* CONFIG_BT_SMP */

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

	Platform::UniquePtr<DiscoveryHandlerCtx> discoveryCtx(Platform::New<DiscoveryHandlerCtx>());
	if (!discoveryCtx) {
		return;
	}

	discoveryCtx->mProvider = provider;
	discoveryCtx->mDiscoveryData = dm;

	CHIP_ERROR err = chip::DeviceLayer::PlatformMgr().ScheduleWork(
		[](intptr_t context) {
			Platform::UniquePtr<DiscoveryHandlerCtx> ctx(reinterpret_cast<DiscoveryHandlerCtx *>(context));
			if (!ctx->mProvider->IsInitiallyConnected()) {
				/* Provider is not initalized, so we need to call the first connection callback. */
				CHIP_ERROR err = ctx->mProvider->GetBLEBridgedDevice().mFirstConnectionCallback(
					ctx->mDiscoveryData,
					ctx->mProvider->GetBLEBridgedDevice().mFirstConnectionCallbackContext);
				ctx->mProvider->ConfirmInitialConnection();
				VerifyOrReturn(CHIP_NO_ERROR == err, bt_gatt_dm_data_release(ctx->mDiscoveryData);
					       Instance().RemoveBLEProvider(ctx->mProvider->GetBtAddress()););
			}

			if (CHIP_NO_ERROR != ctx->mProvider->NotifyReachableStatusChange(true)) {
				LOG_WRN("The device has not been notified about the status change.");
			}

			if (0 != ctx->mProvider->ParseDiscoveredData(ctx->mDiscoveryData)) {
				LOG_ERR("Cannot parse the GATT discovered data.");
			}
			bt_gatt_dm_data_release(ctx->mDiscoveryData);
		},
		reinterpret_cast<intptr_t>(discoveryCtx.get()));

	if (CHIP_NO_ERROR == err) {
		discoveryCtx.release();
	} else {
		bt_gatt_dm_data_release(dm);
	}
}

void BLEConnectivityManager::DiscoveryNotFound(bt_conn *conn, void *context)
{
	LOG_ERR("GATT service could not be found during the discovery");

	BLEBridgedDeviceProvider *provider = reinterpret_cast<BLEBridgedDeviceProvider *>(context);
	if (provider) {
		if (!provider->IsInitiallyConnected()) {
			provider->GetBLEBridgedDevice().mFirstConnectionCallback(
				false, provider->GetBLEBridgedDevice().mFirstConnectionCallbackContext);
		} else {
			Instance().mRecovery.NotifyProviderToRecover(provider);
		}
	}
}

void BLEConnectivityManager::DiscoveryError(bt_conn *conn, int err, void *context)
{
	LOG_ERR("The GATT discovery procedure failed with %d", err);

	BLEBridgedDeviceProvider *provider = reinterpret_cast<BLEBridgedDeviceProvider *>(context);

	if (!provider->IsInitiallyConnected()) {
		provider->GetBLEBridgedDevice().mFirstConnectionCallback(
			false, provider->GetBLEBridgedDevice().mFirstConnectionCallbackContext);
	}
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

#ifdef CONFIG_BT_SMP
	int err = bt_conn_auth_cb_register(&auth_callbacks);
	if (err) {
		LOG_ERR("Failed to register Bluetooth LE authorization callbacks.");
		return System::MapErrorZephyr(err);
	}

	err = bt_conn_auth_info_cb_register(&conn_auth_info_callbacks);
	if (err) {
		LOG_ERR("Failed to register Bluetooth LE authorization info callbacks.");
		return System::MapErrorZephyr(err);
	}
#endif /* CONFIG_BT_SMP */

	bt_scan_init_param scan_init = {
		.connect_if_match = 0,
	};

	bt_scan_init(&scan_init);
	bt_scan_cb_register(&scan_cb);

	return PrepareFilterForUuid();
}

CHIP_ERROR BLEConnectivityManager::PrepareFilterForUuid()
{
	bt_scan_filter_disable();
	bt_scan_filter_remove_all();

	int err = -1;

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

CHIP_ERROR BLEConnectivityManager::PrepareFilterForAddress(bt_addr_le_t *addr)
{
	bt_scan_filter_disable();
	bt_scan_filter_remove_all();

	int err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_ADDR, addr);
	if (err) {
		LOG_ERR("Failed to set scanning filter");
		return System::MapErrorZephyr(err);
	}

	err = bt_scan_filter_enable(BT_SCAN_ADDR_FILTER, false);
	if (err) {
		LOG_ERR("Filters cannot be turned on");
		return System::MapErrorZephyr(err);
	}

	return CHIP_NO_ERROR;
}

CHIP_ERROR BLEConnectivityManager::Scan(ScanDoneCallback callback, void *context, uint32_t scanTimeoutMs)
{
	CHIP_ERROR ret = CHIP_NO_ERROR;
	if (callback != ReScanCallback) {
		mRecovery.CancelTimer();
		ret = StopScan();
		VerifyOrExit(ret == CHIP_NO_ERROR, );
	} else if (mScanActive) {
		LOG_ERR("Scan is already in progress");
		return CHIP_ERROR_INCORRECT_STATE;
	}

	mScannedDevicesCounter = 0;
	memset(mScannedDevices, 0, sizeof(mScannedDevices));

	if (!callback) {
		return CHIP_ERROR_INVALID_ARGUMENT;
	}

	mScanDoneCallback = callback;
	mScanDoneCallbackContext = context;
	ret = System::MapErrorZephyr(bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE));
	VerifyOrExit(ret == CHIP_NO_ERROR, );

	k_timer_start(&mScanTimer, K_MSEC(scanTimeoutMs), K_NO_WAIT);
	mScanActive = true;

exit:
	if (ret != CHIP_NO_ERROR) {
		LOG_ERR("Scanning failed to start (err %s)", ErrorStr(ret));
	}

	return ret;
}

void BLEConnectivityManager::Recovery::RemoveRecovered(BLEBridgedDeviceProvider *provider)
{
	if (!provider) {
		return;
	}

	sys_snode_t *node;
	sys_snode_t *tmpNodeSafe;
	ListItem *item;

	/* Iterate through the list of providers and remove the requested one. */
	SYS_SLIST_FOR_EACH_NODE_SAFE (&Instance().mRecovery.mListToRecover, node, tmpNodeSafe) {
		item = reinterpret_cast<ListItem *>(node);
		if (!item) {
			return;
		}

		if (item->mProvider == provider) {
			sys_slist_find_and_remove(&Instance().mRecovery.mListToRecover, item);
			Platform::Delete(item);
			return;
		}
	}
}

void BLEConnectivityManager::ReScanCallback(ScanResult &result, void *context)
{
	DeviceLayer::PlatformMgr().ScheduleWork(
		[](intptr_t context) {
			BLEBridgedDeviceProvider *provider;
			ScanResult result = *reinterpret_cast<ScanResult *>(context);
			sys_snode_t *node;
			sys_snode_t *tmpNodeSafe;
			Recovery::ListItem *item;
			bool providerFound = false;
			bt_addr_le_t providerAddress;

			/* Filter out devices to recover from a scan result and put them to the queue to reconnect. */
			SYS_SLIST_FOR_EACH_NODE_SAFE (&Instance().mRecovery.mListToRecover, node, tmpNodeSafe) {
				item = reinterpret_cast<Recovery::ListItem *>(node);

				if (!item) {
					return;
				}

				providerFound = false;

				for (uint8_t i = 0; i < result.mCount && !providerFound; i++) {
					providerAddress = item->mProvider->GetBtAddress();
					if (memcmp(&providerAddress, &result.mDevices[i].mAddr,
						   sizeof(result.mDevices[i].mAddr)) == 0) {
						providerFound = true;
						Instance().mRecovery.PutProvider(
							item->mProvider, &Instance().mRecovery.mListToReconnect);
					}
				}

				/* Increase failed attempts counter of all devices to recover that were not detected. */
				if (!providerFound) {
					item->mProvider->NotifyFailedRecovery();
				}
			}

			if (sys_slist_is_empty(&Instance().mRecovery.mListToReconnect)) {
				Instance().mRecovery.StartTimer();
			} else {
				provider = Instance().mRecovery.GetProvider(&Instance().mRecovery.mListToReconnect);
				DeviceLayer::PlatformMgr().ScheduleWork(
					[](intptr_t context) {
						Instance().Reconnect(
							reinterpret_cast<BLEBridgedDeviceProvider *>(context));
					},
					reinterpret_cast<intptr_t>(provider));
			}
		},
		reinterpret_cast<intptr_t>(&result));
}

CHIP_ERROR BLEConnectivityManager::StopScan()
{
	if (!mScanActive) {
		return CHIP_NO_ERROR;
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

	if (Instance().mRecovery.IsNeeded()) {
		Instance().mRecovery.StartTimer();
	}

	Instance().mScanResult.mDevices = Instance().mScannedDevices;
	Instance().mScanResult.mCount = Instance().mScannedDevicesCounter;
	Instance().mScanDoneCallback(Instance().mScanResult, Instance().mScanDoneCallbackContext);
}

CHIP_ERROR BLEConnectivityManager::Reconnect(BLEBridgedDeviceProvider *provider)
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

	char addrStr[BT_ADDR_LE_STR_LEN];
	bt_addr_le_to_str(&provider->GetBLEBridgedDevice().mAddr, addrStr, sizeof(addrStr));

	int err = bt_conn_le_create(&provider->GetBLEBridgedDevice().mAddr, create_param, connParams, &conn);

	if (err) {
		LOG_ERR("Creating reconnection failed (err %d)", err);
		return System::MapErrorZephyr(err);
	} else {
		provider->SetConnectionObject(conn);
	}

	return CHIP_NO_ERROR;
}

CHIP_ERROR BLEConnectivityManager::Connect(BLEBridgedDeviceProvider *provider, ConnectionSecurityRequest *request)
{
	if (!provider) {
		return CHIP_ERROR_INVALID_ARGUMENT;
	}

#ifdef CONFIG_BT_SMP
	if (!request) {
		return CHIP_ERROR_INVALID_ARGUMENT;
	}

	mConnectionSecurityRequest.mCallback = request->mCallback;
	mConnectionSecurityRequest.mContext = request->mContext;
#endif /* CONFIG_BT_SMP */

	mRecovery.CancelTimer();
	StopScan();

	bt_conn *conn{};
	bt_addr_le_t btAddress = provider->GetBtAddress();
	bt_le_conn_param *connParams = GetScannedDeviceConnParams(btAddress);

	if (!connParams) {
		LOG_ERR("Failed to get conn params");
		RemoveBLEProvider(btAddress);
		return CHIP_ERROR_INTERNAL;
	}

	int err = bt_conn_le_create(&provider->GetBLEBridgedDevice().mAddr, create_param, connParams, &conn);

	if (err) {
		LOG_ERR("Creating connection failed (err %d)", err);
		RemoveBLEProvider(btAddress);
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

#ifdef CONFIG_BT_SMP
	bt_unpair(BT_ID_DEFAULT, bt_conn_get_dst(provider->GetBLEBridgedDevice().mConn));
#endif
	bt_conn_disconnect(provider->GetBLEBridgedDevice().mConn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	bt_conn_unref(provider->GetBLEBridgedDevice().mConn);

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

CHIP_ERROR BLEConnectivityManager::GetScannedDeviceUuid(uint16_t &uuid, uint8_t index)
{
	if (index >= mScannedDevicesCounter) {
		return CHIP_ERROR_INVALID_ARGUMENT;
	}

	uuid = mScannedDevices[index].mUuid;

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
	sys_slist_init(&mListToRecover);
	sys_slist_init(&mListToReconnect);
	k_timer_init(&mRecoveryTimer, TimerTimeoutCallback, nullptr);
	k_timer_user_data_set(&mRecoveryTimer, this);
}

void BLEConnectivityManager::Recovery::NotifyProviderToRecover(BLEBridgedDeviceProvider *provider)
{
	if (provider) {
		PutProvider(provider, &mListToRecover);
		StartTimer();
	}
}

void BLEConnectivityManager::Recovery::TimerTimeoutCallback(k_timer *timer)
{
	if (!Instance().mScanActive) {
		/* Schedule scan only if there is any device to be recovered and there is no device to be
		 * re-connected.*/
		if (sys_slist_is_empty(&Instance().mRecovery.mListToReconnect) &&
		    !sys_slist_is_empty(&Instance().mRecovery.mListToRecover)) {
			DeviceLayer::PlatformMgr().ScheduleWork(
				[](intptr_t context) {
					Instance().Scan(ReScanCallback, nullptr, kRecoveryScanTimeoutMs);
				},
				0);
		}
	}
}

BLEBridgedDeviceProvider *BLEConnectivityManager::Recovery::GetProvider(sys_slist_t *list)
{
	ListItem *item = reinterpret_cast<ListItem *>(sys_slist_get(list));
	BLEBridgedDeviceProvider *provider = nullptr;

	if (item) {
		provider = item->mProvider;
		Platform::Delete(item);
	}

	return provider;
}

bool BLEConnectivityManager::Recovery::PutProvider(BLEBridgedDeviceProvider *provider, sys_slist_t *list)
{
	if (sys_slist_len(list) >= kMaxConnectedDevices) {
		return false;
	}

	ListItem *item = Platform::New<ListItem>();

	if (!item) {
		return false;
	}

	item->mProvider = provider;
	sys_slist_append(list, item);

	return true;
}

uint16_t BLEConnectivityManager::Recovery::GetFailedRecoveryAttempts()
{
	sys_snode_t *node;
	sys_snode_t *tmpNodeSafe;
	ListItem *item;
	uint16_t attempts = UINT16_MAX;

	/* Find the smallest number or failed recovery attempts from all providers. */
	SYS_SLIST_FOR_EACH_NODE_SAFE (&Instance().mRecovery.mListToRecover, node, tmpNodeSafe) {
		item = reinterpret_cast<ListItem *>(node);

		if (!item) {
			return 0;
		}

		uint16_t newAttempts = item->mProvider->GetFailedRecoveryAttempts();

		attempts = newAttempts < attempts ? newAttempts : attempts;
	}

	return attempts;
}

void BLEConnectivityManager::Recovery::StartTimer()
{
	uint16_t attempts = GetFailedRecoveryAttempts();
	/* Calculate next recovery time as a quadratic function of failed attempts number.
	 * This allows to scan the less frequently, the longer no devices are detected.
	 */
	uint32_t time = ((attempts * attempts) + 1) * kRecoveryIntervalSec;

	time = time < kRecoveryMaxIntervalSec ? time : kRecoveryMaxIntervalSec;

	k_timer_start(&mRecoveryTimer, K_SECONDS(time), K_NO_WAIT);
}

} /* namespace Nrf */
