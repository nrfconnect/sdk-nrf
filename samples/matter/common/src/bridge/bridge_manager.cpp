/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "bridge_manager.h"

#include <app-common/zap-generated/ids/Clusters.h>
#include <app/reporting/reporting.h>
#include <app/util/generic-callbacks.h>
#include <lib/support/Span.h>

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

using namespace ::chip;
using namespace ::chip::app;

BridgeManager BridgeManager::sBridgeManager;

void BridgeManager::Init()
{
	/* The first dynamic endpoint is the last fixed endpoint + 1. */
	mFirstDynamicEndpointId = static_cast<chip::EndpointId>(
		static_cast<int>(emberAfEndpointFromIndex(static_cast<uint16_t>(emberAfFixedEndpointCount() - 1))) + 1);

	mCurrentDynamicEndpointId = mFirstDynamicEndpointId;

	/* Disable the placeholder endpoint */
	emberAfEndpointEnableDisable(emberAfEndpointFromIndex(static_cast<uint16_t>(emberAfFixedEndpointCount() - 1)),
				     false);
}

CHIP_ERROR BridgeManager::AddBridgedDevices(BridgedDevice *aDevice, BridgedDeviceDataProvider *aDataProvider)
{
	aDataProvider->Init();
	CHIP_ERROR err = AddDevices(aDevice, aDataProvider);

	return err;
}

CHIP_ERROR BridgeManager::RemoveBridgedDevice(uint16_t endpoint)
{
	uint8_t index = 0;

	while (index < kMaxBridgedDevices) {
		if (mDevicesMap.Contains(index)) {
			const DevicePair *devicesPtr = mDevicesMap[index];
			if (devicesPtr->mDevice->GetEndpointId() == endpoint) {
				LOG_INF("Removed dynamic endpoint %d (index=%d)", endpoint, index);
				/* Free dynamically allocated memory */
				emberAfClearDynamicEndpoint(index);
				if (mDevicesMap.Erase(index)) {
					mNumberOfProviders--;
					return CHIP_NO_ERROR;
				} else {
					LOG_ERR("Cannot remove bridged devices under index=%d", index);
					return CHIP_ERROR_NOT_FOUND;
				}
			}
		}
		index++;
	}
	return CHIP_ERROR_NOT_FOUND;
}

CHIP_ERROR BridgeManager::AddDevices(BridgedDevice *aDevice, BridgedDeviceDataProvider *aDataProvider)
{
	uint8_t index = 0;

	Platform::UniquePtr<BridgedDevice> device(aDevice);
	Platform::UniquePtr<BridgedDeviceDataProvider> provider(aDataProvider);
	VerifyOrReturnError(device && provider, CHIP_ERROR_INTERNAL);

	/* Maximum number of Matter bridged devices is controlled inside mDevicesMap,
	   but the data providers may be created independently, so let's ensure we do not
	   violate the maximum number of supported instances. */
	VerifyOrReturnError(!mDevicesMap.IsFull(), CHIP_ERROR_INTERNAL);
	VerifyOrReturnError(mNumberOfProviders + 1 <= kMaxDataProviders, CHIP_ERROR_INTERNAL);
	mNumberOfProviders++;

	while (index < kMaxBridgedDevices) {
		/* Find the first empty index in the bridged devices list */
		if (!mDevicesMap.Contains(index)) {
			mDevicesMap.Insert(index, DevicePair(std::move(device), std::move(provider)));
			EmberAfStatus ret;
			while (true) {
				if (!mDevicesMap[index]) {
					LOG_ERR("Cannot retrieve bridged device from index %d", index);
					return CHIP_ERROR_INTERNAL;
				}
				auto &storedDevice = mDevicesMap[index]->mDevice;
				ret = emberAfSetDynamicEndpoint(
					index, mCurrentDynamicEndpointId, storedDevice->mEp,
					Span<DataVersion>(storedDevice->mDataVersion, storedDevice->mDataVersionSize),
					Span<const EmberAfDeviceType>(storedDevice->mDeviceTypeList,
								      storedDevice->mDeviceTypeListSize));

				if (ret == EMBER_ZCL_STATUS_SUCCESS) {
					LOG_INF("Added device to dynamic endpoint %d (index=%d)",
						mCurrentDynamicEndpointId, index);
					storedDevice->Init(mCurrentDynamicEndpointId);
					return CHIP_NO_ERROR;
				} else if (ret != EMBER_ZCL_STATUS_DUPLICATE_EXISTS) {
					LOG_ERR("Failed to add dynamic endpoint: Internal error!");
					RemoveBridgedDevice(mCurrentDynamicEndpointId); // TODO: check if this is ok, we
											// need to cleanup the unused
											// devices
					return CHIP_ERROR_INTERNAL;
				}

				/* Handle wrap condition */
				if (++mCurrentDynamicEndpointId < mFirstDynamicEndpointId) {
					mCurrentDynamicEndpointId = mFirstDynamicEndpointId;
				}
			}
		}
		index++;
	}

	LOG_ERR("Failed to add dynamic endpoint: No endpoints available!");

	return CHIP_ERROR_NO_MEMORY;
}

CHIP_ERROR BridgeManager::HandleRead(uint16_t index, ClusterId clusterId,
				     const EmberAfAttributeMetadata *attributeMetadata, uint8_t *buffer,
				     uint16_t maxReadLength)
{
	VerifyOrReturnError(attributeMetadata && buffer, CHIP_ERROR_INVALID_ARGUMENT);
	VerifyOrReturnValue(sBridgeManager.mDevicesMap.Contains(index), CHIP_ERROR_INTERNAL);

	auto &device = sBridgeManager.mDevicesMap[index]->mDevice;
	return device->HandleRead(clusterId, attributeMetadata->attributeId, buffer, maxReadLength);
}

CHIP_ERROR BridgeManager::HandleWrite(uint16_t index, ClusterId clusterId,
				      const EmberAfAttributeMetadata *attributeMetadata, uint8_t *buffer)
{
	VerifyOrReturnError(attributeMetadata && buffer, CHIP_ERROR_INVALID_ARGUMENT);
	VerifyOrReturnValue(sBridgeManager.mDevicesMap.Contains(index), CHIP_ERROR_INTERNAL);

	auto &device = sBridgeManager.mDevicesMap[index]->mDevice;
	CHIP_ERROR err = device->HandleWrite(clusterId, attributeMetadata->attributeId, buffer);

	/* After updating Matter BridgedDevice state, forward request to the non-Matter device. */
	if (err == CHIP_NO_ERROR) {
		return sBridgeManager.mDevicesMap[index]->mProvider->UpdateState(
			clusterId, attributeMetadata->attributeId, buffer);
	}
	return err;
}

void BridgeManager::HandleUpdate(BridgedDeviceDataProvider &dataProvider, chip::ClusterId clusterId,
				 chip::AttributeId attributeId, void *data, size_t dataSize)
{
	VerifyOrReturn(data);

	/* The state update was triggered by non-Matter device, find related Matter Bridged Device to update it as
	well.
	 */
	for (auto &item : sBridgeManager.mDevicesMap.mMap) {
		if (item.value.mProvider.get() == &dataProvider) {
			/* If the Bridged Device state was updated successfully, schedule sending Matter data report. */
			if (CHIP_NO_ERROR ==
			    item.value.mDevice->HandleAttributeChange(clusterId, attributeId, data, dataSize)) {
				MatterReportingAttributeChangeCallback(item.value.mDevice->GetEndpointId(), clusterId,
								       attributeId);
			}
		}
	}
}

EmberAfStatus emberAfExternalAttributeReadCallback(EndpointId endpoint, ClusterId clusterId,
						   const EmberAfAttributeMetadata *attributeMetadata, uint8_t *buffer,
						   uint16_t maxReadLength)
{
	uint16_t endpointIndex = emberAfGetDynamicIndexFromEndpoint(endpoint);

	if (CHIP_NO_ERROR ==
	    GetBridgeManager().HandleRead(endpointIndex, clusterId, attributeMetadata, buffer, maxReadLength)) {
		return EMBER_ZCL_STATUS_SUCCESS;
	} else {
		return EMBER_ZCL_STATUS_FAILURE;
	}
}

EmberAfStatus emberAfExternalAttributeWriteCallback(EndpointId endpoint, ClusterId clusterId,
						    const EmberAfAttributeMetadata *attributeMetadata, uint8_t *buffer)
{
	uint16_t endpointIndex = emberAfGetDynamicIndexFromEndpoint(endpoint);

	if (CHIP_NO_ERROR == GetBridgeManager().HandleWrite(endpointIndex, clusterId, attributeMetadata, buffer)) {
		return EMBER_ZCL_STATUS_SUCCESS;
	} else {
		return EMBER_ZCL_STATUS_FAILURE;
	}
}
