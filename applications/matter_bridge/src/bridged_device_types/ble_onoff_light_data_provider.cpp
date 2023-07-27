/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "ble_onoff_light_data_provider.h"

#include <bluetooth/gatt_dm.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

using namespace ::chip;
using namespace ::chip::app;

static bt_uuid *sServiceUuid = BT_UUID_LBS;
static bt_uuid *sUuidLED = BT_UUID_LBS_LED;
static bt_uuid *sUuidButton = BT_UUID_LBS_BUTTON;
static bt_uuid *sUuidCcc = BT_UUID_GATT_CCC;

uint8_t BleOnOffLightDataProvider::GattNotifyCallback(bt_conn *conn, bt_gatt_subscribe_params *params, const void *data,
						      uint16_t length)
{
	BLEBridgedDevice *dev = BLEConnectivityManager::Instance().FindBLEBridgedDevice(conn);
	BleOnOffLightDataProvider *provider;

	VerifyOrExit(dev, );
	VerifyOrExit(data, );
	VerifyOrExit(length == sizeof(mOnOff), );

	provider = reinterpret_cast<BleOnOffLightDataProvider *>(dev->mProvider);
	VerifyOrExit(provider, );

	/* Save data received in notification. */
	memcpy(&provider->mOnOff, data, length);
	DeviceLayer::PlatformMgr().ScheduleWork(NotifyAttributeChange, reinterpret_cast<intptr_t>(provider));

exit:

	return BT_GATT_ITER_CONTINUE;
}

void BleOnOffLightDataProvider::Init()
{
	/* Configure subscription for the button characteristic */
	mGattSubscribeParams.ccc_handle = mCccHandle;
	mGattSubscribeParams.value_handle = mButtonCharacteristicHandle;
	mGattSubscribeParams.value = BT_GATT_CCC_NOTIFY;
	mGattSubscribeParams.notify = BleOnOffLightDataProvider::GattNotifyCallback;

	/* TODO: Start GATT subscription to Button characteristic once the Matter Switch device type support will be in
	 * place. */
}

void BleOnOffLightDataProvider::NotifyUpdateState(chip::ClusterId clusterId, chip::AttributeId attributeId, void *data,
						  size_t dataSize)
{
	if (mUpdateAttributeCallback) {
		mUpdateAttributeCallback(*this, Clusters::OnOff::Id, Clusters::OnOff::Attributes::OnOff::Id, data,
					 dataSize);
	}
}

void BleOnOffLightDataProvider::GattWriteCallback(bt_conn *conn, uint8_t err, bt_gatt_write_params *params)
{
	BLEBridgedDevice *dev = BLEConnectivityManager::Instance().FindBLEBridgedDevice(conn);

	if (!dev || !params) {
		return;
	}

	if (params->length != sizeof(mOnOff)) {
		return;
	}

	if (!dev->mProvider) {
		return;
	}

	BleOnOffLightDataProvider *provider = reinterpret_cast<BleOnOffLightDataProvider *>(dev->mProvider);

	/* Save data received in GATT write response. */
	memcpy(&provider->mOnOff, params->data, params->length);

	DeviceLayer::PlatformMgr().ScheduleWork(NotifyAttributeChange, reinterpret_cast<intptr_t>(provider));
}

CHIP_ERROR BleOnOffLightDataProvider::UpdateState(chip::ClusterId clusterId, chip::AttributeId attributeId,
						  uint8_t *buffer)
{
	if (!mDevice) {
		return CHIP_ERROR_INCORRECT_STATE;
	}

	if (clusterId != Clusters::OnOff::Id) {
		return CHIP_ERROR_INVALID_ARGUMENT;
	}

	LOG_INF("Updating state of the BleOnOffLightDataProvider, cluster ID: %u, attribute ID: %u.", clusterId,
		attributeId);

	switch (attributeId) {
	case Clusters::OnOff::Attributes::OnOff::Id: {
		memcpy(mGattWriteDataBuffer, buffer, sizeof(mOnOff));

		mGattWriteParams.data = mGattWriteDataBuffer;
		mGattWriteParams.offset = 0;
		mGattWriteParams.length = sizeof(mOnOff);
		mGattWriteParams.handle = mLedCharacteristicHandle;
		mGattWriteParams.func = BleOnOffLightDataProvider::GattWriteCallback;

		int err = bt_gatt_write(mDevice->mConn, &mGattWriteParams);
		if (err) {
			LOG_ERR("GATT write operation failed");
			return CHIP_ERROR_INTERNAL;
		}

		return CHIP_NO_ERROR;
	}
	default:
		return CHIP_ERROR_INVALID_ARGUMENT;
	}

	return CHIP_NO_ERROR;
}

bt_uuid *BleOnOffLightDataProvider::GetServiceUuid()
{
	return sServiceUuid;
}

int BleOnOffLightDataProvider::ParseDiscoveredData(bt_gatt_dm *discoveredData)
{
	const bt_gatt_dm_attr *gatt_chrc;
	const bt_gatt_dm_attr *gatt_desc;

	gatt_chrc = bt_gatt_dm_char_by_uuid(discoveredData, sUuidLED);
	if (!gatt_chrc) {
		LOG_ERR("No LED characteristic found.");
		return -EINVAL;
	}

	gatt_desc = bt_gatt_dm_desc_by_uuid(discoveredData, gatt_chrc, sUuidLED);
	if (!gatt_desc) {
		LOG_ERR("No LED characteristic value found.");
		return -EINVAL;
	}
	mLedCharacteristicHandle = gatt_desc->handle;

	gatt_chrc = bt_gatt_dm_char_by_uuid(discoveredData, sUuidButton);
	if (!gatt_chrc) {
		LOG_ERR("No Button characteristic found.");
		return -EINVAL;
	}

	gatt_desc = bt_gatt_dm_desc_by_uuid(discoveredData, gatt_chrc, sUuidButton);
	if (!gatt_desc) {
		LOG_ERR("No Button characteristic value found.");
		return -EINVAL;
	}
	mButtonCharacteristicHandle = gatt_desc->handle;

	gatt_desc = bt_gatt_dm_desc_by_uuid(discoveredData, gatt_chrc, sUuidCcc);
	if (!gatt_desc) {
		LOG_ERR("No CCC descriptor found.");
		return -EINVAL;
	}
	mCccHandle = gatt_desc->handle;

	return 0;
}

void BleOnOffLightDataProvider::NotifyAttributeChange(intptr_t context)
{
	BleOnOffLightDataProvider *provider = reinterpret_cast<BleOnOffLightDataProvider *>(context);

	provider->NotifyUpdateState(Clusters::OnOff::Id, Clusters::OnOff::Attributes::OnOff::Id, &provider->mOnOff,
				    sizeof(provider->mOnOff));
}
