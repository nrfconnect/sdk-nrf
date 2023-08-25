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

void BridgeManager::Init()
{
	/* The first dynamic endpoint is the last fixed endpoint + 1. */
	mFirstDynamicEndpointId = static_cast<EndpointId>(
		static_cast<int>(emberAfEndpointFromIndex(static_cast<uint16_t>(emberAfFixedEndpointCount() - 1))) + 1);

	mCurrentDynamicEndpointId = mFirstDynamicEndpointId;

	/* Disable the placeholder endpoint */
	emberAfEndpointEnableDisable(emberAfEndpointFromIndex(static_cast<uint16_t>(emberAfFixedEndpointCount() - 1)),
				     false);
}

CHIP_ERROR BridgeManager::AddBridgedDevices(MatterBridgedDevice *devices[], BridgedDeviceDataProvider *dataProvider,
					    uint8_t deviceListSize)
{
	VerifyOrReturnError(devices && dataProvider, CHIP_ERROR_INTERNAL);

	dataProvider->Init();

	/* Wrap input data into unique_ptr to avoid memory leakage in case any error occurs. */
	Platform::UniquePtr<MatterBridgedDevice> devicesPtr[deviceListSize];
	for (auto i = 0; i < deviceListSize; ++i) {
		devicesPtr[i] = std::move(Platform::UniquePtr<MatterBridgedDevice>(devices[i]));
	}
	Platform::UniquePtr<BridgedDeviceDataProvider> dataProviderPtr(dataProvider);

	/* Maximum number of Matter bridged devices is controlled inside mDevicesMap,
	   but the data providers may be created independently, so let's ensure we do not
	   violate the maximum number of supported instances. */
	VerifyOrReturnError(mDevicesMap.FreeSlots() >= deviceListSize, CHIP_ERROR_NO_MEMORY,
			    LOG_ERR("Not enough free slots in the map"));
	VerifyOrReturnError(mNumberOfProviders + 1 <= kMaxDataProviders, CHIP_ERROR_NO_MEMORY,
			    LOG_ERR("Maximum number of providers exceeded"));
	mNumberOfProviders++;

	bool cumulativeStatus{ true };
	bool status{ true };
	for (auto i = 0; i < deviceListSize; ++i) {
		MatterBridgedDevice *device = devicesPtr[i].get();
		status = AddSingleDevice(device, dataProviderPtr.get());
		if (status) {
			devicesPtr[i].release();
		}
		cumulativeStatus &= status;
	}

	if (cumulativeStatus) {
		dataProviderPtr.release();
		return CHIP_NO_ERROR;
	}
	return CHIP_ERROR_NO_MEMORY;
}

CHIP_ERROR BridgeManager::RemoveBridgedDevice(uint16_t endpoint)
{
	uint8_t index = 0;
	while (index < kMaxBridgedDevices) {
		if (mDevicesMap.Contains(index)) {
			auto &devicePair = mDevicesMap[index];
			if (devicePair.mDevice->GetEndpointId() == endpoint) {
				LOG_INF("Removed dynamic endpoint %d (index=%d)", endpoint, index);
				/* Free dynamically allocated memory */
				emberAfClearDynamicEndpoint(index);
				return SafelyRemoveDevice(index);
			}
		}
		index++;
	}
	return CHIP_ERROR_NOT_FOUND;
}

CHIP_ERROR BridgeManager::SafelyRemoveDevice(uint8_t index)
{
	uint16_t duplicatedItemKeys[kMaxBridgedDevices];
	uint8_t size;
	bool removeProvider = true;

	auto &devicePair = mDevicesMap[index];
	if (mDevicesMap.ValueDuplicated(devicePair, duplicatedItemKeys, size)) {
		for (uint8_t i = 0; i < size; i++) {
			/* We must consider only true duplicates, so skip the current index, as
			 * it would lead to comparing the same values each time. */
			if (duplicatedItemKeys[i] != index) {
				auto &duplicate = mDevicesMap[duplicatedItemKeys[i]];
				if (duplicate.mDevice == devicePair.mDevice) {
					devicePair.mDevice = nullptr;
				}
				if (duplicate.mProvider == devicePair.mProvider) {
					devicePair.mProvider = nullptr;
					removeProvider = false;
				}
				/* At least one real duplicate found. */
				break;
			}
		}
	}
	if (mDevicesMap.Erase(index)) {
		if (removeProvider) {
			mNumberOfProviders--;
		}
		return CHIP_NO_ERROR;
	} else {
		LOG_ERR("Cannot remove bridged devices under index=%d", index);
		return CHIP_ERROR_NOT_FOUND;
	}
}

bool BridgeManager::AddSingleDevice(MatterBridgedDevice *device, BridgedDeviceDataProvider *dataProvider)
{
	uint8_t index{ 0 };
	while (index < kMaxBridgedDevices) {
		/* Find the first empty index in the bridged devices list */
		if (!mDevicesMap.Contains(index)) {
			if (mDevicesMap.Insert(index, BridgedDevicePair(device, dataProvider))) {
				while (true) {
					auto *storedDevice = mDevicesMap[index].mDevice;
					/* Allocate endpoints for provided devices */
					EmberAfStatus ret = emberAfSetDynamicEndpoint(
						index, mCurrentDynamicEndpointId, storedDevice->mEp,
						Span<DataVersion>(storedDevice->mDataVersion,
								  storedDevice->mDataVersionSize),
						Span<const EmberAfDeviceType>(storedDevice->mDeviceTypeList,
									      storedDevice->mDeviceTypeListSize));

					if (ret == EMBER_ZCL_STATUS_SUCCESS) {
						LOG_INF("Added device to dynamic endpoint %d (index=%d)",
							mCurrentDynamicEndpointId, index);
						storedDevice->Init(mCurrentDynamicEndpointId);
						return true;
					} else if (ret != EMBER_ZCL_STATUS_DUPLICATE_EXISTS) {
						LOG_ERR("Failed to add dynamic endpoint: Internal error!");
						if (CHIP_NO_ERROR != SafelyRemoveDevice(index)) {
							LOG_ERR("Cannot remove device from the map!");
						}
						return false;
					}

					/* Handle wrap condition */
					if (++mCurrentDynamicEndpointId < mFirstDynamicEndpointId) {
						mCurrentDynamicEndpointId = mFirstDynamicEndpointId;
					}
				}
			}
		}
		index++;
	}

	LOG_ERR("Failed to add dynamic endpoint: No endpoints or indexes available!");
	return false;
}

CHIP_ERROR BridgeManager::HandleRead(uint16_t index, ClusterId clusterId,
				     const EmberAfAttributeMetadata *attributeMetadata, uint8_t *buffer,
				     uint16_t maxReadLength)
{
	VerifyOrReturnError(attributeMetadata && buffer, CHIP_ERROR_INVALID_ARGUMENT);
	VerifyOrReturnValue(Instance().mDevicesMap.Contains(index), CHIP_ERROR_INTERNAL);

	auto *device = Instance().mDevicesMap[index].mDevice;
	return device->HandleRead(clusterId, attributeMetadata->attributeId, buffer, maxReadLength);
}

CHIP_ERROR BridgeManager::HandleWrite(uint16_t index, ClusterId clusterId,
				      const EmberAfAttributeMetadata *attributeMetadata, uint8_t *buffer)
{
	VerifyOrReturnError(attributeMetadata && buffer, CHIP_ERROR_INVALID_ARGUMENT);
	VerifyOrReturnValue(Instance().mDevicesMap.Contains(index), CHIP_ERROR_INTERNAL);

	auto *device = Instance().mDevicesMap[index].mDevice;
	CHIP_ERROR err = device->HandleWrite(clusterId, attributeMetadata->attributeId, buffer);

	/* After updating MatterBridgedDevice state, forward request to the non-Matter device. */
	if (err == CHIP_NO_ERROR) {
		return Instance().mDevicesMap[index].mProvider->UpdateState(clusterId, attributeMetadata->attributeId,
									    buffer);
	}
	return err;
}

void BridgeManager::HandleUpdate(BridgedDeviceDataProvider &dataProvider, ClusterId clusterId, AttributeId attributeId,
				 void *data, size_t dataSize)
{
	VerifyOrReturn(data);

	/* The state update was triggered by non-Matter device, find bridged Matter device to update it as well. */
	for (auto &item : Instance().mDevicesMap.mMap) {
		if (item.value.mProvider == &dataProvider) {
			/* If the Bridged Device state was updated successfully, schedule sending Matter data report. */
			auto *device = item.value.mDevice;
			if (CHIP_NO_ERROR == device->HandleAttributeChange(clusterId, attributeId, data, dataSize)) {
				MatterReportingAttributeChangeCallback(device->GetEndpointId(), clusterId, attributeId);
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
	    BridgeManager::Instance().HandleRead(endpointIndex, clusterId, attributeMetadata, buffer, maxReadLength)) {
		return EMBER_ZCL_STATUS_SUCCESS;
	} else {
		return EMBER_ZCL_STATUS_FAILURE;
	}
}

EmberAfStatus emberAfExternalAttributeWriteCallback(EndpointId endpoint, ClusterId clusterId,
						    const EmberAfAttributeMetadata *attributeMetadata, uint8_t *buffer)
{
	uint16_t endpointIndex = emberAfGetDynamicIndexFromEndpoint(endpoint);

	if (CHIP_NO_ERROR ==
	    BridgeManager::Instance().HandleWrite(endpointIndex, clusterId, attributeMetadata, buffer)) {
		return EMBER_ZCL_STATUS_SUCCESS;
	} else {
		return EMBER_ZCL_STATUS_FAILURE;
	}
}
