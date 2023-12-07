/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "bridge_manager.h"

#include "binding_handler.h"

#include <app-common/zap-generated/ids/Clusters.h>
#include <app/reporting/reporting.h>
#include <app/util/generic-callbacks.h>
#include <lib/support/Span.h>

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

using namespace ::chip;
using namespace ::chip::app;

CHIP_ERROR BridgeManager::Init(LoadStoredBridgedDevicesCallback loadStoredBridgedDevicesCb)
{
	if (!loadStoredBridgedDevicesCb) {
		return CHIP_ERROR_INVALID_ARGUMENT;
	}

	if (mIsInitialized) {
		LOG_INF("BridgeManager is already initialized.");
		return CHIP_ERROR_INCORRECT_STATE;
	}

	/* The first dynamic endpoint is the last fixed endpoint + 1. */
	mFirstDynamicEndpointId = static_cast<EndpointId>(
		static_cast<int>(emberAfEndpointFromIndex(static_cast<uint16_t>(emberAfFixedEndpointCount() - 1))) + 1);

	mCurrentDynamicEndpointId = mFirstDynamicEndpointId;

	/* Disable the placeholder endpoint */
	emberAfEndpointEnableDisable(emberAfEndpointFromIndex(static_cast<uint16_t>(emberAfFixedEndpointCount() - 1)),
				     false);

	BindingHandler::GetInstance().Init();

	/* Invoke the callback to load stored devices in a proper moment. */
	CHIP_ERROR err = loadStoredBridgedDevicesCb();

	if (err == CHIP_NO_ERROR) {
		mIsInitialized = true;
	}

	return err;
}

CHIP_ERROR BridgeManager::AddBridgedDevices(MatterBridgedDevice *devices[], BridgedDeviceDataProvider *dataProvider,
					    uint8_t deviceListSize, uint8_t devicesPairIndexes[])
{
	chip::Optional<uint8_t> indexes[deviceListSize];
	uint16_t endpoints[deviceListSize];

	VerifyOrReturnError(devicesPairIndexes, CHIP_ERROR_INTERNAL);

	return AddBridgedDevices(devices, dataProvider, deviceListSize, devicesPairIndexes, endpoints, indexes);
}

CHIP_ERROR BridgeManager::AddBridgedDevices(MatterBridgedDevice *devices[], BridgedDeviceDataProvider *dataProvider,
					    uint8_t deviceListSize, uint8_t devicesPairIndexes[],
					    uint16_t endpointIds[])
{
	chip::Optional<uint8_t> indexes[deviceListSize];

	VerifyOrReturnError(devicesPairIndexes, CHIP_ERROR_INTERNAL);

	for (auto i = 0; i < deviceListSize; i++) {
		indexes[i].SetValue(devicesPairIndexes[i]);
	}

	return AddBridgedDevices(devices, dataProvider, deviceListSize, devicesPairIndexes, endpointIds, indexes);
}

CHIP_ERROR BridgeManager::AddBridgedDevices(MatterBridgedDevice *devices[], BridgedDeviceDataProvider *dataProvider,
					    uint8_t deviceListSize, uint8_t devicesPairIndexes[],
					    uint16_t endpointIds[], chip::Optional<uint8_t> indexes[])
{
	CHIP_ERROR err = AddDevices(devices, dataProvider, deviceListSize, indexes, endpointIds);

	if (err != CHIP_NO_ERROR) {
		return err;
	}

	for (auto i = 0; i < deviceListSize; i++) {
		if (!indexes[i].HasValue()) {
			return CHIP_ERROR_INTERNAL;
		}

		devicesPairIndexes[i] = indexes[i].Value();
	}

	return CHIP_NO_ERROR;
}

CHIP_ERROR BridgeManager::RemoveBridgedDevice(uint16_t endpoint, uint8_t &devicesPairIndex)
{
	uint8_t index = 0;
	while (index < kMaxBridgedDevices) {
		if (mDevicesMap.Contains(index)) {
			auto &devicePair = mDevicesMap[index];
			if (devicePair.mDevice->GetEndpointId() == endpoint) {
				LOG_INF("Removed dynamic endpoint %d (index=%d)", endpoint, index);
				/* Free dynamically allocated memory */
				emberAfClearDynamicEndpoint(index);
				devicesPairIndex = index;
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
	bool removeProvider = true;
	auto &devicePair = mDevicesMap[index];

	uint8_t duplicatesNumber = mDevicesMap.GetDuplicatesCount(devicePair, duplicatedItemKeys);
	/* There must be at least 2 duplicates in the map to determine the real duplicate,
       as the one under the current index is also contained in the map. */
	bool realDuplicatePresent = duplicatesNumber >= 2;

	if (realDuplicatePresent) {
		for (uint8_t i = 0; i < duplicatesNumber; i++) {
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

		/* Find the required index on the list, remove it and move all following indexes one position earlier.
		 */
		bool indexFound = false;
		for (size_t i = 0; i < mDevicesIndexesCounter; i++) {
			if (mDevicesIndexes[i] == index) {
				indexFound = true;
			}

			if (indexFound && ((i + 1) < BridgeManager::kMaxBridgedDevices)) {
				mDevicesIndexes[i] = mDevicesIndexes[i + 1];
			}
		}

		if (indexFound) {
			mDevicesIndexesCounter--;
		} else {
			return CHIP_ERROR_NOT_FOUND;
		}

		return CHIP_NO_ERROR;
	} else {
		LOG_ERR("Cannot remove bridged devices under index=%d", index);
		return CHIP_ERROR_NOT_FOUND;
	}
}

CHIP_ERROR BridgeManager::AddSingleDevice(MatterBridgedDevice *device, BridgedDeviceDataProvider *dataProvider,
					  chip::Optional<uint8_t> &devicesPairIndex, uint16_t endpointId)
{
	uint8_t index{ 0 };
	CHIP_ERROR err;
	uint16_t duplicatedItemKeys[kMaxBridgedDevices];
	BridgedDevicePair pair(device, dataProvider);

	/* Check if the current provider is already contained in the map - if so, there is at least one duplicate */
	bool isNewProvider = (Instance().mDevicesMap.GetDuplicatesCount(pair, duplicatedItemKeys) == 0);

	if (isNewProvider) {
		VerifyOrReturnError(mNumberOfProviders + 1 <= kMaxDataProviders, CHIP_ERROR_NO_MEMORY,
				    LOG_ERR("Maximum number of providers exceeded"));
		mNumberOfProviders++;
		dataProvider->Init();
	}

	/* The adding algorithm differs depending on the devicesPairIndex value:
	 * - If devicesPairIndex has value it means that index and endpoint id are specified and should be assigned
	 * based on the input arguments (e.g. the device was loaded from storage and has to use specific data).
	 * - If devicesPairIndex has no value it means the default monotonically increasing numbering should be used.
	 */
	if (devicesPairIndex.HasValue()) {
		index = devicesPairIndex.Value();
		/* The requested index is already used. */
		if (mDevicesMap.Contains(index)) {
			return CHIP_ERROR_INTERNAL;
		}

		if (!mDevicesMap.Insert(index, std::move(pair))) {
			return CHIP_ERROR_INTERNAL;
		}

		err = CreateEndpoint(index, endpointId);

		if (err == CHIP_NO_ERROR) {
			devicesPairIndex.SetValue(index);
			mDevicesIndexes[mDevicesIndexesCounter] = index;
			mDevicesIndexesCounter++;

			/* Make sure that the following endpoint id assignments will be monotonically continued from the
			 * biggest assigned number. */
			mCurrentDynamicEndpointId =
				mCurrentDynamicEndpointId > endpointId ? mCurrentDynamicEndpointId : endpointId + 1;
		} else {
			/* The pair was added to a map, so we have to take care about removing it in case of failure. */
			mDevicesMap.Erase(index);
		}

		return err;
	} else {
		while (index < kMaxBridgedDevices) {
			/* Find the first empty index in the bridged devices list */
			if (!mDevicesMap.Contains(index)) {
				if (mDevicesMap.Insert(index, std::move(pair))) {
					/* Assign the free endpoint ID. */
					do {
						err = CreateEndpoint(index, mCurrentDynamicEndpointId);

						if (err != CHIP_NO_ERROR) {
							/* The pair was added to a map, so we have to take care about
							 * removing it in case of failure. */
							mDevicesMap.Erase(index);
						}

						/* Handle wrap condition */
						if (++mCurrentDynamicEndpointId < mFirstDynamicEndpointId) {
							mCurrentDynamicEndpointId = mFirstDynamicEndpointId;
						}
					} while (err == CHIP_ERROR_SENTINEL);

					if (err == CHIP_NO_ERROR) {
						devicesPairIndex.SetValue(index);
						mDevicesIndexes[mDevicesIndexesCounter] = index;
						mDevicesIndexesCounter++;
					}

					return err;
				} else {
					/* Failing Insert method means the BridgedDevicePair destructor will be called
					 * and pointers wiped out. It's not safe to iterate further. */
					return CHIP_ERROR_INTERNAL;
				}
			}
			index++;
		}
	}

	LOG_ERR("Failed to add dynamic endpoint: No endpoints or indexes available!");
	return CHIP_ERROR_NO_MEMORY;
}

CHIP_ERROR BridgeManager::CreateEndpoint(uint8_t index, uint16_t endpointId)
{
	if (!mDevicesMap.Contains(index)) {
		LOG_ERR("Cannot retrieve bridged device from index %d", index);
		return CHIP_ERROR_INTERNAL;
	}

	auto *storedDevice = mDevicesMap[index].mDevice;
	EmberAfStatus ret = emberAfSetDynamicEndpoint(
		index, endpointId, storedDevice->mEp,
		Span<DataVersion>(storedDevice->mDataVersion, storedDevice->mDataVersionSize),
		Span<const EmberAfDeviceType>(storedDevice->mDeviceTypeList, storedDevice->mDeviceTypeListSize),
		kAggregatorEndpointId);

	if (ret == EMBER_ZCL_STATUS_SUCCESS) {
		LOG_INF("Added device to dynamic endpoint %d (index=%d)", endpointId, index);
		storedDevice->Init(endpointId);
		return CHIP_NO_ERROR;
	} else if (ret != EMBER_ZCL_STATUS_DUPLICATE_EXISTS) {
		LOG_ERR("Failed to add dynamic endpoint: Internal error!");
		if (CHIP_NO_ERROR != SafelyRemoveDevice(index)) {
			LOG_ERR("Cannot remove device from the map!");
		}
		return CHIP_ERROR_INTERNAL;
	} else {
		return CHIP_ERROR_SENTINEL;
	}
}

CHIP_ERROR BridgeManager::AddDevices(MatterBridgedDevice *devices[], BridgedDeviceDataProvider *dataProvider,
				     uint8_t deviceListSize, chip::Optional<uint8_t> devicesPairIndexes[],
				     uint16_t endpointIds[])
{
	VerifyOrReturnError(devices && dataProvider, CHIP_ERROR_INTERNAL);

	/* Maximum number of Matter bridged devices is controlled inside mDevicesMap,
	   but the data providers may be created independently, so let's ensure we do not
	   violate the maximum number of supported instances. */
	if (mDevicesMap.FreeSlots() < deviceListSize) {
		/* This method takes care of the resources, so objects have to be deleted. */
		for (auto i = 0; i < deviceListSize; ++i) {
			chip::Platform::Delete(devices[i]);
		}
		chip::Platform::Delete(dataProvider);
		return CHIP_ERROR_NO_MEMORY;
	}

	CHIP_ERROR status{ CHIP_NO_ERROR };
	for (auto i = 0; i < deviceListSize; ++i) {
		status = AddSingleDevice(devices[i], dataProvider, devicesPairIndexes[i], endpointIds[i]);

		if (status != CHIP_NO_ERROR) {
			return status;
		}
	}

	return CHIP_NO_ERROR;
}

CHIP_ERROR BridgeManager::HandleRead(uint16_t index, ClusterId clusterId,
				     const EmberAfAttributeMetadata *attributeMetadata, uint8_t *buffer,
				     uint16_t maxReadLength)
{
	VerifyOrReturnError(attributeMetadata && buffer, CHIP_ERROR_INVALID_ARGUMENT);
	VerifyOrReturnValue(Instance().mDevicesMap.Contains(index), CHIP_ERROR_INTERNAL);

	auto *device = Instance().mDevicesMap[index].mDevice;

	/* Handle reads for the generic information for all bridged devices. Provide a valid answer even if device state
	 * is unreachable. */
	switch (clusterId) {
	case Clusters::BridgedDeviceBasicInformation::Id:
		return device->HandleReadBridgedDeviceBasicInformation(attributeMetadata->attributeId, buffer,
								       maxReadLength);
	case Clusters::Identify::Id:
		return device->HandleReadIdentify(attributeMetadata->attributeId, buffer, maxReadLength);
	default:
		break;
	}

	/* Verify if the device is reachable or we should return prematurely. */
	VerifyOrReturnError(device->GetIsReachable(), CHIP_ERROR_INCORRECT_STATE);

	return device->HandleRead(clusterId, attributeMetadata->attributeId, buffer, maxReadLength);
}

CHIP_ERROR BridgeManager::HandleWrite(uint16_t index, ClusterId clusterId,
				      const EmberAfAttributeMetadata *attributeMetadata, uint8_t *buffer)
{
	VerifyOrReturnError(attributeMetadata && buffer, CHIP_ERROR_INVALID_ARGUMENT);
	VerifyOrReturnValue(Instance().mDevicesMap.Contains(index), CHIP_ERROR_INTERNAL);

	auto *device = Instance().mDevicesMap[index].mDevice;

	/* Verify if the device is reachable or we should return prematurely. */
	VerifyOrReturnError(device->GetIsReachable(), CHIP_ERROR_INCORRECT_STATE);

	/* Handle Identify cluster write - for now it does not imply updating the provider's state */
	if (clusterId == Clusters::Identify::Id) {
		return device->HandleWriteIdentify(attributeMetadata->attributeId, buffer, attributeMetadata->size);
	}

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

	/* The state update was triggered by non-Matter device, find bridged Matter device to update it as well.
	 */
	for (auto &item : Instance().mDevicesMap.mMap) {
		if (item.value.mProvider == &dataProvider) {
			/* If the Bridged Device state was updated successfully, schedule sending Matter data
			 * report. */
			auto *device = item.value.mDevice;
			if (CHIP_NO_ERROR == device->HandleAttributeChange(clusterId, attributeId, data, dataSize)) {
				MatterReportingAttributeChangeCallback(device->GetEndpointId(), clusterId, attributeId);
			}
		}
	}
}

void BridgeManager::HandleCommand(BridgedDeviceDataProvider &dataProvider, ClusterId clusterId, CommandId commandId, BindingHandler::InvokeCommand invokeCommand)
{
	BindingHandler::BindingData *bindingData = Platform::New<BindingHandler::BindingData>();

	if (!bindingData) {
		return;
	}

	bindingData->CommandId = commandId;
	bindingData->ClusterId = clusterId;
	bindingData->InvokeCommandFunc = invokeCommand;

	for (auto &item : Instance().mDevicesMap.mMap) {
		if (item.value.mProvider == &dataProvider) {
			auto *device = item.value.mDevice;

			if (emberAfContainsClient(device->GetEndpointId(), clusterId)) {
				bindingData->EndpointId = device->GetEndpointId();
			}
		}
	}

	DeviceLayer::PlatformMgr().ScheduleWork(BindingHandler::DeviceWorkerHandler,
						reinterpret_cast<intptr_t>(bindingData));
}

BridgedDeviceDataProvider *BridgeManager::GetProvider(EndpointId endpoint, MatterBridgedDevice::DeviceType &deviceType)
{
	uint16_t endpointIndex = emberAfGetDynamicIndexFromEndpoint(endpoint);
	if (Instance().mDevicesMap.Contains(endpointIndex)) {
		BridgedDevicePair &bridgedDevices = Instance().mDevicesMap[endpointIndex];
		deviceType = bridgedDevices.mDevice->GetDeviceType();
		return bridgedDevices.mProvider;
	}
	return nullptr;
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
