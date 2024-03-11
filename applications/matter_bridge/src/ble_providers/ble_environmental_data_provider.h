/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "ble_bridged_device.h"
#include "ble_connectivity_manager.h"
#include "bridged_device_data_provider.h"

class BleEnvironmentalDataProvider : public Nrf::BLEBridgedDeviceProvider {
public:
	explicit BleEnvironmentalDataProvider(UpdateAttributeCallback updateCallback, InvokeCommandCallback commandCallback) : Nrf::BLEBridgedDeviceProvider(updateCallback, commandCallback) {}
	~BleEnvironmentalDataProvider() { Unsubscribe(); }

	void Init() override;
	void NotifyUpdateState(chip::ClusterId clusterId, chip::AttributeId attributeId, void *data,
			       size_t dataSize) override;
	CHIP_ERROR UpdateState(chip::ClusterId clusterId, chip::AttributeId attributeId, uint8_t *buffer) override;
	const bt_uuid *GetServiceUuid() override;
	int ParseDiscoveredData(bt_gatt_dm *discoveredData) override;

private:
	static constexpr uint32_t kMeasurementsIntervalMs{ CONFIG_BRIDGE_BLE_DEVICE_POLLING_INTERVAL };

	void StartHumidityTimer();
	void StopHumidityTimer() { k_timer_stop(&mHumidityTimer); }
	void Subscribe();
	void Unsubscribe();
	bool CheckSubscriptionParameters(bt_gatt_subscribe_params *params);

	CHIP_ERROR ParseTemperatureCharacteristic(bt_gatt_dm *discoveredData);
	CHIP_ERROR ParseHumidityCharacteristic(bt_gatt_dm *discoveredData);

	static uint8_t GattTemperatureNotifyCallback(bt_conn *conn, bt_gatt_subscribe_params *params, const void *data,
						     uint16_t length);
	static uint8_t GattHumidityNotifyCallback(bt_conn *conn, bt_gatt_subscribe_params *params, const void *data,
						  uint16_t length);
	static void NotifyTemperatureAttributeChange(intptr_t context);
	static void NotifyHumidityAttributeChange(intptr_t context);

	static void ReadGATTHumidity(intptr_t context);
	static void HumidityTimerTimeoutCallback(k_timer *timer);
	static uint8_t HumidityGATTReadCallback(bt_conn *conn, uint8_t att_err, bt_gatt_read_params *params,
						const void *data, uint16_t read_len);

	uint16_t mTemperatureValue{};
	uint16_t mHumidityValue{};

	uint16_t mTemperatureCharacteristicHandle{};
	uint16_t mHumidityCharacteristicHandle{};

	bt_gatt_subscribe_params mGattTemperatureSubscribeParams{};
	bt_gatt_subscribe_params mGattHumiditySubscribeParams{};

	uint16_t mCccTemperatureHandle{};
	uint16_t mCccHumidityHandle{};

	k_timer mHumidityTimer;

	static bt_gatt_read_params sHumidityReadParams;
};
