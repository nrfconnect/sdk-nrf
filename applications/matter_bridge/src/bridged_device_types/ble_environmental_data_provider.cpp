/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "ble_environmental_data_provider.h"

#include <bluetooth/gatt_dm.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

using namespace ::chip;
using namespace ::chip::app;

static bt_uuid *sServiceUuid = BT_UUID_ESS;
static bt_uuid *sUuidTemperature = BT_UUID_TEMPERATURE;
static bt_uuid *sUuidHumidity = BT_UUID_HUMIDITY;
static bt_uuid *sUuidCcc = BT_UUID_GATT_CCC;

bt_gatt_read_params BleEnvironmentalDataProvider::sHumidityReadParams{};

BleEnvironmentalDataProvider *GetProvider(bt_conn *conn)
{
	BLEBridgedDevice *device = BLEConnectivityManager::Instance().FindBLEBridgedDevice(conn);
	VerifyOrReturnValue(device, nullptr);
	return reinterpret_cast<BleEnvironmentalDataProvider *>(device->mProvider);
}

bt_uuid *BleEnvironmentalDataProvider::GetServiceUuid()
{
	return sServiceUuid;
}

uint8_t BleEnvironmentalDataProvider::GattTemperatureNotifyCallback(bt_conn *conn, bt_gatt_subscribe_params *params,
								    const void *data, uint16_t length)
{
	BleEnvironmentalDataProvider *provider = GetProvider(conn);
	VerifyOrExit(provider, );
	VerifyOrExit(data, );
	VerifyOrExit(length == sizeof(mTemperatureValue), );

	/* Save data received in notification. */
	memcpy(&provider->mTemperatureValue, data, length);
	DeviceLayer::PlatformMgr().ScheduleWork(NotifyTemperatureAttributeChange, reinterpret_cast<intptr_t>(provider));

exit:

	return BT_GATT_ITER_CONTINUE;
}

uint8_t BleEnvironmentalDataProvider::GattHumidityNotifyCallback(bt_conn *conn, bt_gatt_subscribe_params *params,
								 const void *data, uint16_t length)
{
	BleEnvironmentalDataProvider *provider = GetProvider(conn);
	VerifyOrExit(provider, );
	VerifyOrExit(data, );
	VerifyOrExit(length == sizeof(mHumidityValue), );

	/* Save data received in notification. */
	memcpy(&provider->mHumidityValue, data, length);
	DeviceLayer::PlatformMgr().ScheduleWork(NotifyHumidityAttributeChange, reinterpret_cast<intptr_t>(provider));

exit:

	return BT_GATT_ITER_CONTINUE;
}

void BleEnvironmentalDataProvider::Init()
{
	/* Configure subscription for the temperature characteristic */
	mGattTemperatureSubscribeParams.ccc_handle = mCccTemperatureHandle;
	mGattTemperatureSubscribeParams.value_handle = mTemperatureCharacteristicHandle;
	mGattTemperatureSubscribeParams.value = BT_GATT_CCC_NOTIFY;
	mGattTemperatureSubscribeParams.notify = BleEnvironmentalDataProvider::GattTemperatureNotifyCallback;

	/* Configure subscription for the humidity characteristic */
	mGattHumiditySubscribeParams.ccc_handle = mCccHumidityHandle;
	mGattHumiditySubscribeParams.value_handle = mHumidityCharacteristicHandle;
	mGattHumiditySubscribeParams.value = BT_GATT_CCC_NOTIFY;
	mGattHumiditySubscribeParams.notify = BleEnvironmentalDataProvider::GattHumidityNotifyCallback;

	Subscribe();
}

void BleEnvironmentalDataProvider::NotifyUpdateState(chip::ClusterId clusterId, chip::AttributeId attributeId,
						     void *data, size_t dataSize)
{
	if (mUpdateAttributeCallback) {
		mUpdateAttributeCallback(*this, clusterId, attributeId, data, dataSize);
	}
}

CHIP_ERROR BleEnvironmentalDataProvider::UpdateState(chip::ClusterId clusterId, chip::AttributeId attributeId,
						     uint8_t *)
{
	return CHIP_ERROR_UNSUPPORTED_CHIP_FEATURE;
}

int BleEnvironmentalDataProvider::ParseDiscoveredData(bt_gatt_dm *discoveredData)
{
	VerifyOrReturnError(CHIP_NO_ERROR == ParseTemperatureCharacteristic(discoveredData), -ENXIO);
	VerifyOrReturnError(CHIP_NO_ERROR == ParseHumidityCharacteristic(discoveredData), -ENXIO);

	return 0;
}

CHIP_ERROR BleEnvironmentalDataProvider::ParseTemperatureCharacteristic(bt_gatt_dm *discoveredData)
{
	const bt_gatt_dm_attr *gatt_chrc = bt_gatt_dm_char_by_uuid(discoveredData, sUuidTemperature);
	VerifyOrReturnError(gatt_chrc, CHIP_ERROR_INTERNAL, LOG_ERR("No temperature characteristic found."));

	const bt_gatt_dm_attr *gatt_desc = bt_gatt_dm_desc_by_uuid(discoveredData, gatt_chrc, sUuidTemperature);
	VerifyOrReturnError(gatt_desc, CHIP_ERROR_INTERNAL, LOG_ERR("No temperature characteristic value found."));

	mTemperatureCharacteristicHandle = gatt_desc->handle;

	gatt_desc = bt_gatt_dm_desc_by_uuid(discoveredData, gatt_chrc, sUuidCcc);
	VerifyOrReturnError(gatt_desc, CHIP_ERROR_INTERNAL, LOG_ERR("No temperature CCC descriptor found."));

	mCccTemperatureHandle = gatt_desc->handle;

	return CHIP_NO_ERROR;
}

CHIP_ERROR BleEnvironmentalDataProvider::ParseHumidityCharacteristic(bt_gatt_dm *discoveredData)
{
	const bt_gatt_dm_attr *gatt_chrc = bt_gatt_dm_char_by_uuid(discoveredData, sUuidHumidity);
	VerifyOrReturnError(gatt_chrc, CHIP_ERROR_INTERNAL, LOG_ERR("No humidity characteristic found."));

	const bt_gatt_dm_attr *gatt_desc = bt_gatt_dm_desc_by_uuid(discoveredData, gatt_chrc, sUuidHumidity);
	VerifyOrReturnError(gatt_desc, CHIP_ERROR_INTERNAL, LOG_ERR("No humidity characteristic value found."));

	mHumidityCharacteristicHandle = gatt_desc->handle;

	gatt_desc = bt_gatt_dm_desc_by_uuid(discoveredData, gatt_chrc, sUuidCcc);
	/* This is acceptable because we will emulate the subscription for humidity sensor. */
	VerifyOrReturnError(gatt_desc, CHIP_NO_ERROR, LOG_INF("No humidity CCC descriptor found."));

	mCccHumidityHandle = gatt_desc->handle;

	return CHIP_NO_ERROR;
}

void BleEnvironmentalDataProvider::Subscribe()
{
	VerifyOrReturn(mDevice && mDevice->mConn, LOG_ERR("Invalid connection object"));

	if (CheckSubscriptionParameters(&mGattTemperatureSubscribeParams)) {
		int err = bt_gatt_subscribe(mDevice->mConn, &mGattTemperatureSubscribeParams);
		if (err) {
			LOG_ERR("Subscribe to temperature characteristic failed with error %d", err);
		}
	} else {
		LOG_ERR("Invalid temperature subscription parameters provided");
	}

	if (CheckSubscriptionParameters(&mGattHumiditySubscribeParams)) {
		int err = bt_gatt_subscribe(mDevice->mConn, &mGattHumiditySubscribeParams);
		if (err) {
			LOG_ERR("Subscribe to humidity characteristic failed with error %d", err);
		}
	} else {
		LOG_INF("Invalid humidity subscription parameters provided, starting emulated subscription");
		/* The fallback is based on polling for the current GATT humidity value */
		sHumidityReadParams.func = BleEnvironmentalDataProvider::HumidityGATTReadCallback;
		sHumidityReadParams.handle_count = 0;
		sHumidityReadParams.by_uuid.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
		sHumidityReadParams.by_uuid.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
		sHumidityReadParams.by_uuid.uuid = sUuidHumidity;
		StartHumidityTimer();
	}
}

void BleEnvironmentalDataProvider::Unsubscribe()
{
	VerifyOrReturn(mDevice && mDevice->mConn, LOG_ERR("Invalid connection object"));

	int err = bt_gatt_unsubscribe(mDevice->mConn, &mGattTemperatureSubscribeParams);
	if (err) {
		LOG_INF("Cannot unsubscribe from temperature characteristic (error %d)", err);
	}

	err = bt_gatt_unsubscribe(mDevice->mConn, &mGattHumiditySubscribeParams);
	if (err) {
		LOG_INF("Cannot unsubscribe from humidity characteristic (error %d)", err);
	}
	StopHumidityTimer();
}

bool BleEnvironmentalDataProvider::CheckSubscriptionParameters(bt_gatt_subscribe_params *params)
{
	/* If any of these is not met, the bt_gatt_subscribe() generates an assert at runtime */
	VerifyOrReturnValue(params && params->notify, false);
	VerifyOrReturnValue(params->value, false);
	VerifyOrReturnValue(params->ccc_handle, false);

	return true;
}

void BleEnvironmentalDataProvider::NotifyTemperatureAttributeChange(intptr_t context)
{
	BleEnvironmentalDataProvider *provider = reinterpret_cast<BleEnvironmentalDataProvider *>(context);

	provider->NotifyUpdateState(Clusters::TemperatureMeasurement::Id,
				    Clusters::TemperatureMeasurement::Attributes::MeasuredValue::Id,
				    &provider->mTemperatureValue, sizeof(provider->mTemperatureValue));
}

void BleEnvironmentalDataProvider::NotifyHumidityAttributeChange(intptr_t context)
{
	BleEnvironmentalDataProvider *provider = reinterpret_cast<BleEnvironmentalDataProvider *>(context);

	provider->NotifyUpdateState(Clusters::RelativeHumidityMeasurement::Id,
				    Clusters::RelativeHumidityMeasurement::Attributes::MeasuredValue::Id,
				    &provider->mHumidityValue, sizeof(provider->mHumidityValue));
}

void BleEnvironmentalDataProvider::StartHumidityTimer()
{
	k_timer_init(&mHumidityTimer, BleEnvironmentalDataProvider::HumidityTimerTimeoutCallback, nullptr);
	k_timer_user_data_set(&mHumidityTimer, this);
	k_timer_start(&mHumidityTimer, K_MSEC(kMeasurementsIntervalMs), K_MSEC(kMeasurementsIntervalMs));
}

void BleEnvironmentalDataProvider::HumidityTimerTimeoutCallback(k_timer *timer)
{
	VerifyOrReturn(timer && timer->user_data, LOG_ERR("Invalid context"));

	BleEnvironmentalDataProvider *provider = reinterpret_cast<BleEnvironmentalDataProvider *>(timer->user_data);
	VerifyOrReturn(provider, LOG_ERR("Invalid provider  object"));
	VerifyOrReturn(provider->mDevice && provider->mDevice->mConn, LOG_ERR("Invalid connection object"));

	DeviceLayer::PlatformMgr().ScheduleWork(ReadGATTHumidity, reinterpret_cast<intptr_t>(provider));
}

void BleEnvironmentalDataProvider::ReadGATTHumidity(intptr_t context)
{
	BleEnvironmentalDataProvider *provider = reinterpret_cast<BleEnvironmentalDataProvider *>(context);

	int err = bt_gatt_read(provider->mDevice->mConn, &sHumidityReadParams);
	if (err) {
		LOG_INF("GATT read failed (err %d)", err);
	}
}

uint8_t BleEnvironmentalDataProvider::HumidityGATTReadCallback(bt_conn *conn, uint8_t att_err,
							       bt_gatt_read_params *params, const void *data,
							       uint16_t read_len)
{
	BleEnvironmentalDataProvider *provider = GetProvider(conn);
	VerifyOrReturnValue(provider, BT_GATT_ITER_STOP, LOG_ERR("Invalid provider object"));

	if (!att_err && (read_len == sizeof(provider->mHumidityValue))) {
		const uint16_t newValue = *(static_cast<const uint16_t *>(data));
		if (newValue != provider->mHumidityValue) {
			provider->mHumidityValue = newValue;
			DeviceLayer::PlatformMgr().ScheduleWork(NotifyHumidityAttributeChange,
								reinterpret_cast<intptr_t>(provider));
		}
	} else {
		LOG_ERR("Unsuccessful GATT read operation (err %d)", att_err);
	}

	return BT_GATT_ITER_STOP;
}
