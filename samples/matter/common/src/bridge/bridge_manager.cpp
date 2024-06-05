/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "bridge_manager.h"
#include "bridge/bridge_storage_manager.h"

#include "binding/binding_handler.h"

#include <app-common/zap-generated/ids/Clusters.h>
#include <app/reporting/reporting.h>
#include <app/util/generic-callbacks.h>
#include <lib/support/Span.h>

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

using namespace ::chip;
using namespace ::chip::app;

namespace Nrf
{

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

	if (!Nrf::BridgeStorageManager::Instance().Init()) {
		LOG_INF("BridgeStorageManager initialization failed.");
		return CHIP_ERROR_INTERNAL;
	}

	Nrf::Matter::BindingHandler::Init();

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
	Platform::UniquePtr<BridgedDeviceDataProvider> providerPtr(dataProvider);

	VerifyOrReturnError(devicesPairIndexes, CHIP_ERROR_INTERNAL);
	CHIP_ERROR err =
		DoAddBridgedDevices(devices, providerPtr.get(), deviceListSize, devicesPairIndexes, endpoints, indexes);
	/* The ownership was transferred unconditionally. */
	providerPtr.release();
	return err;
}

CHIP_ERROR BridgeManager::AddBridgedDevices(MatterBridgedDevice *devices[], BridgedDeviceDataProvider *dataProvider,
					    uint8_t deviceListSize, uint8_t devicesPairIndexes[],
					    uint16_t endpointIds[])
{
	Platform::UniquePtr<BridgedDeviceDataProvider> providerPtr(dataProvider);
	chip::Optional<uint8_t> indexes[deviceListSize];

	VerifyOrReturnError(devicesPairIndexes, CHIP_ERROR_INTERNAL);

	for (auto i = 0; i < deviceListSize; i++) {
		indexes[i].SetValue(devicesPairIndexes[i]);
	}
	CHIP_ERROR err = DoAddBridgedDevices(devices, providerPtr.get(), deviceListSize, devicesPairIndexes,
					     endpointIds, indexes);
	/* The ownership was transferred unconditionally. */
	providerPtr.release();
	return err;
}

CHIP_ERROR BridgeManager::DoAddBridgedDevices(MatterBridgedDevice *devices[], BridgedDeviceDataProvider *dataProvider,
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
			if (err == CHIP_ERROR_NO_MEMORY) {
				LOG_ERR("The device object was not constructed properly due to the lack of memory");
			}
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
							if (err == CHIP_ERROR_NO_MEMORY) {
								LOG_ERR("The device object was not constructed properly due to the lack of memory");
							}
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

	/* Make sure that data that is going to be wrapped in the Span objects is valid,
	   otherwise, the Span may make the application abort(). */
	VerifyOrReturnError(storedDevice, CHIP_ERROR_NO_MEMORY);
	VerifyOrReturnError(storedDevice->mDataVersion && storedDevice->mDataVersionSize > 0, CHIP_ERROR_NO_MEMORY);
	VerifyOrReturnError(storedDevice->mDeviceTypeList && storedDevice->mDeviceTypeListSize > 0,
			    CHIP_ERROR_NO_MEMORY);

	CHIP_ERROR err = emberAfSetDynamicEndpoint(
		index, endpointId, storedDevice->mEp,
		Span<DataVersion>(storedDevice->mDataVersion, storedDevice->mDataVersionSize),
		Span<const EmberAfDeviceType>(storedDevice->mDeviceTypeList, storedDevice->mDeviceTypeListSize),
		kAggregatorEndpointId);

	if (err == CHIP_NO_ERROR) {
		LOG_INF("Added device to dynamic endpoint %d (index=%d)", endpointId, index);
		storedDevice->Init(endpointId);
		return CHIP_NO_ERROR;
	} else if (err != CHIP_ERROR_ENDPOINT_EXISTS) {
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
	CHIP_ERROR err{ CHIP_NO_ERROR };

	VerifyOrReturnError(devices || dataProvider, CHIP_ERROR_INVALID_ARGUMENT);
	VerifyOrExit(devices, err = CHIP_ERROR_INVALID_ARGUMENT);
	VerifyOrExit(dataProvider, err = CHIP_ERROR_INVALID_ARGUMENT);

	/* Maximum number of Matter bridged devices is controlled inside mDevicesMap,
	   but the data providers may be created independently, so let's ensure we do not
	   violate the maximum number of supported instances. */
	VerifyOrExit(mDevicesMap.FreeSlots() >= deviceListSize, err = CHIP_ERROR_NO_MEMORY);

	for (auto i = 0; i < deviceListSize; ++i) {
		err = AddSingleDevice(devices[i], dataProvider, devicesPairIndexes[i], endpointIds[i]);

		if (err != CHIP_NO_ERROR) {
			return err;
		}
	}
exit:
	/* This method takes care of the resources, so objects have to be deleted in case of failures. */
	if (err != CHIP_NO_ERROR) {
		if (dataProvider) {
			chip::Platform::Delete(dataProvider);
		}

		if (devices) {
			for (auto i = 0; i < deviceListSize; ++i) {
				chip::Platform::Delete(devices[i]);
			}
		}
	}
	return err;
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

	uint8_t *data;
	size_t dataLen;

	/* The buffer content differs depending on the attribute type, so there are some conversions required. */
	switch (attributeMetadata->attributeType) {
	case ZCL_OCTET_STRING_ATTRIBUTE_TYPE:
	case ZCL_CHAR_STRING_ATTRIBUTE_TYPE:
		/* These are pascal strings that contain length encoded as a first byte of data. */
		data = buffer + 1;
		memcpy(&dataLen, buffer, 1);
		break;
	case ZCL_LONG_OCTET_STRING_ATTRIBUTE_TYPE:
	case ZCL_LONG_CHAR_STRING_ATTRIBUTE_TYPE:
		/* These are pascal long strings that contain length encoded as a first two bytes of data. */
		data = buffer + 2;
		memcpy(&dataLen, buffer, 2);
		break;
	default:
		/* Other numerical attribute types are represented in a normal way. */
		data = buffer;
		dataLen = attributeMetadata->size;
		break;
	}

	CHIP_ERROR err = device->HandleWrite(clusterId, attributeMetadata->attributeId, data, dataLen);

	/* After updating MatterBridgedDevice state, forward request to the non-Matter device. */
	if (err == CHIP_NO_ERROR) {
		CHIP_ERROR updateError = Instance().mDevicesMap[index].mProvider->UpdateState(
			clusterId, attributeMetadata->attributeId, buffer);
		/* This is acceptable that not all writable attributes can be reflected in the provider device. */
		if (updateError != CHIP_ERROR_UNSUPPORTED_CHIP_FEATURE) {
			return updateError;
		}
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

void BridgeManager::HandleCommand(BridgedDeviceDataProvider &dataProvider, ClusterId clusterId, CommandId commandId,
				  Nrf::Matter::BindingHandler::InvokeCommand invokeCommand)
{
	Nrf::Matter::BindingHandler::BindingData *bindingData =
		Platform::New<Nrf::Matter::BindingHandler::BindingData>();

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

	Nrf::Matter::BindingHandler::RunBoundClusterAction(bindingData);
}

BridgedDeviceDataProvider *BridgeManager::GetProvider(EndpointId endpoint, uint16_t &deviceType)
{
	uint16_t endpointIndex = emberAfGetDynamicIndexFromEndpoint(endpoint);
	if (Instance().mDevicesMap.Contains(endpointIndex)) {
		BridgedDevicePair &bridgedDevices = Instance().mDevicesMap[endpointIndex];
		deviceType = bridgedDevices.mDevice->GetDeviceType();
		return bridgedDevices.mProvider;
	}
	return nullptr;
}

} /* namespace Nrf */

Protocols::InteractionModel::Status
emberAfExternalAttributeReadCallback(EndpointId endpoint, ClusterId clusterId,
				     const EmberAfAttributeMetadata *attributeMetadata, uint8_t *buffer,
				     uint16_t maxReadLength)
{
	uint16_t endpointIndex = emberAfGetDynamicIndexFromEndpoint(endpoint);

	if (CHIP_NO_ERROR == Nrf::BridgeManager::Instance().HandleRead(endpointIndex, clusterId, attributeMetadata,
								       buffer, maxReadLength)) {
		return Protocols::InteractionModel::Status::Success;
	} else {
		return Protocols::InteractionModel::Status::Failure;
	}
}

Protocols::InteractionModel::Status
emberAfExternalAttributeWriteCallback(EndpointId endpoint, ClusterId clusterId,
				      const EmberAfAttributeMetadata *attributeMetadata, uint8_t *buffer)
{
	uint16_t endpointIndex = emberAfGetDynamicIndexFromEndpoint(endpoint);

	if (CHIP_NO_ERROR ==
	    Nrf::BridgeManager::Instance().HandleWrite(endpointIndex, clusterId, attributeMetadata, buffer)) {
		return Protocols::InteractionModel::Status::Success;
	} else {
		return Protocols::InteractionModel::Status::Failure;
	}
}
