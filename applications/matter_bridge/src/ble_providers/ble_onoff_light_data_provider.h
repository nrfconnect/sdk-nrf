/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "ble_bridged_device.h"
#include "ble_connectivity_manager.h"
#include "bridged_device_data_provider.h"

#include <bluetooth/services/lbs.h>

class BleOnOffLightDataProvider : public BLEBridgedDeviceProvider {
public:
	explicit BleOnOffLightDataProvider(UpdateAttributeCallback callback) : BLEBridgedDeviceProvider(callback) {}

	void Init() override;
	void NotifyUpdateState(chip::ClusterId clusterId, chip::AttributeId attributeId, void *data,
			       size_t dataSize) override;
	CHIP_ERROR UpdateState(chip::ClusterId clusterId, chip::AttributeId attributeId, uint8_t *buffer) override;

	static void NotifyAttributeChange(intptr_t context);
	static void GattWriteCallback(bt_conn *conn, uint8_t err, bt_gatt_write_params *params);
	static uint8_t GattNotifyCallback(bt_conn *conn, bt_gatt_subscribe_params *params, const void *data,
					  uint16_t length);

	bt_uuid *GetServiceUuid() override;
	int ParseDiscoveredData(bt_gatt_dm *discoveredData) override;

private:
	void Subscribe();

	bool mOnOff = false;
	uint16_t mLedCharacteristicHandle;
	bt_gatt_write_params mGattWriteParams;
	uint16_t mButtonCharacteristicHandle;
	uint16_t mCccHandle;
	bt_gatt_subscribe_params mGattSubscribeParams;

	uint8_t mGattWriteDataBuffer[sizeof(mOnOff)];
};
