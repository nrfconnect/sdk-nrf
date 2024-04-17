/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "bridge_storage_manager.h"
#include "bridge_manager.h"

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

namespace
{
template <class T> bool LoadDataToObject(Nrf::PersistentStorageNode *node, T &data)
{
	size_t readSize = 0;

	bool result = Nrf::GetPersistentStorage().NonSecureLoad(node, &data, sizeof(T), readSize);

	return result;
}

Nrf::PersistentStorageNode CreateIndexNode(uint8_t bridgedDeviceIndex, Nrf::PersistentStorageNode *parent)
{
	char index[Nrf::BridgeStorageManager::kMaxIndexLength + 1] = { 0 };

	snprintf(index, sizeof(index), "%d", bridgedDeviceIndex);

	return Nrf::PersistentStorageNode(index, strlen(index), parent);
}

} /* namespace */

namespace Nrf
{

bool BridgeStorageManager::Init()
{
	bool result = Nrf::GetPersistentStorage().NonSecureInit();

	if (!result) {
		return result;
	}

	/* Perform data migration from previous data structure versions if needed. */
	return MigrateData();
}

bool BridgeStorageManager::MigrateData()
{
	uint8_t version;

	/* Check if version key is present in settings to provide backward compatibility between post-2.6.0 releases and
	 * the previous one. If the version key is present it means that the new keys structure is used (only version
	 * equal to 1 is for now valid). Otherwise, the deprecated structure is used, so the migration has to be done.
	 */
	if (!LoadDataToObject(&mVersion, version)) {
		uint8_t count;
		uint8_t indexes[BridgeManager::kMaxBridgedDevices] = { 0 };
		size_t indexesCount = 0;

		if (LoadBridgedDevicesCount(count) &&
		    LoadBridgedDevicesIndexes(indexes, BridgeManager::kMaxBridgedDevices, indexesCount)) {
			/* Load all devices based on the read count number. */
			for (size_t i = 0; i < indexesCount; i++) {
				BridgedDevice device;
				if (!LoadBridgedDeviceEndpointId(device.mEndpointId, i)) {
					return false;
				}

				/* Ignore an error, as node label is optional, so it may not be found. */
				if (!LoadBridgedDeviceNodeLabel(device.mNodeLabel, sizeof(device.mNodeLabel),
								device.mNodeLabelLength, i)) {
					device.mNodeLabelLength = 0;
				}

				if (!LoadBridgedDeviceType(device.mDeviceType, i)) {
					return false;
				}

#ifdef CONFIG_BRIDGED_DEVICE_BT
				bt_addr_le_t btAddr;

				if (!LoadBtAddress(btAddr, i)) {
					return false;
				}

				/* Insert Bluetooth LE address as a part of implementation specific user data. */
				device.mUserDataSize = sizeof(btAddr);
				device.mUserData = reinterpret_cast<uint8_t *>(&btAddr);
#endif

				/* Store all information using a new scheme. */
				if (!StoreBridgedDevice(device, i)) {
					return false;
				}

				/* Remove all information described using an old scheme. */
				RemoveBridgedDeviceEndpointId(i);
				RemoveBridgedDeviceNodeLabel(i);
				RemoveBridgedDeviceType(i);

#ifdef CONFIG_BRIDGED_DEVICE_BT
				RemoveBtAddress(i);
#endif
			}
		}

		version = 1;
		Nrf::GetPersistentStorage().NonSecureStore(&mVersion, &version, sizeof(version));

	} else if (version != 1) {
		/* Currently only no-version or version equal to 1 is supported. */
		return false;
	}
	return true;
}

bool BridgeStorageManager::StoreBridgedDevicesCount(uint8_t count)
{
	return Nrf::GetPersistentStorage().NonSecureStore(&mBridgedDevicesCount, &count, sizeof(count));
}

bool BridgeStorageManager::LoadBridgedDevicesCount(uint8_t &count)
{
	return LoadDataToObject(&mBridgedDevicesCount, count);
}

bool BridgeStorageManager::StoreBridgedDevicesIndexes(uint8_t *indexes, uint8_t count)
{
	if (!indexes) {
		return false;
	}

	return Nrf::GetPersistentStorage().NonSecureStore(&mBridgedDevicesIndexes, indexes, count);
}

bool BridgeStorageManager::LoadBridgedDevicesIndexes(uint8_t *indexes, uint8_t maxCount, size_t &count)
{
	if (!indexes) {
		return false;
	}

	return Nrf::GetPersistentStorage().NonSecureLoad(&mBridgedDevicesIndexes, indexes, maxCount, count);
}

bool BridgeStorageManager::LoadBridgedDeviceEndpointId(uint16_t &endpointId, uint8_t bridgedDeviceIndex)
{
	Nrf::PersistentStorageNode id = CreateIndexNode(bridgedDeviceIndex, &mBridgedDeviceEndpointId);

	return LoadDataToObject(&id, endpointId);
}

bool BridgeStorageManager::RemoveBridgedDeviceEndpointId(uint8_t bridgedDeviceIndex)
{
	Nrf::PersistentStorageNode id = CreateIndexNode(bridgedDeviceIndex, &mBridgedDeviceEndpointId);

	return Nrf::GetPersistentStorage().NonSecureRemove(&id);
}

bool BridgeStorageManager::LoadBridgedDeviceNodeLabel(char *label, size_t labelMaxLength, size_t &labelLength,
						      uint8_t bridgedDeviceIndex)
{
	Nrf::PersistentStorageNode id = CreateIndexNode(bridgedDeviceIndex, &mBridgedDeviceNodeLabel);

	return Nrf::GetPersistentStorage().NonSecureLoad(&id, label, labelMaxLength, labelLength);
}

bool BridgeStorageManager::RemoveBridgedDeviceNodeLabel(uint8_t bridgedDeviceIndex)
{
	Nrf::PersistentStorageNode id = CreateIndexNode(bridgedDeviceIndex, &mBridgedDeviceNodeLabel);

	return Nrf::GetPersistentStorage().NonSecureRemove(&id);
}

bool BridgeStorageManager::LoadBridgedDeviceType(uint16_t &deviceType, uint8_t bridgedDeviceIndex)
{
	Nrf::PersistentStorageNode id = CreateIndexNode(bridgedDeviceIndex, &mBridgedDeviceType);

	return LoadDataToObject(&id, deviceType);
}

bool BridgeStorageManager::RemoveBridgedDeviceType(uint8_t bridgedDeviceIndex)
{
	Nrf::PersistentStorageNode id = CreateIndexNode(bridgedDeviceIndex, &mBridgedDeviceType);

	return Nrf::GetPersistentStorage().NonSecureRemove(&id);
}

bool BridgeStorageManager::LoadBridgedDevice(BridgedDevice &device, uint8_t index)
{
	Nrf::PersistentStorageNode id = CreateIndexNode(index, &mBridgedDevice);
	size_t readSize = 0;
	uint8_t buffer[sizeof(BridgedDevice) + kMaxUserDataSize];
	uint16_t counter = 0;
	const uint8_t mandatoryItemsSize = sizeof(device.mEndpointId) + sizeof(device.mDeviceType) + sizeof(device.mNodeLabelLength);

	if (!Nrf::GetPersistentStorage().NonSecureLoad(&id, buffer, sizeof(BridgedDevice) + kMaxUserDataSize, readSize)) {
		return false;
	}

	/* Validate that read size is big enough to include mandatory data. */
	if (readSize < mandatoryItemsSize) {
		return false;
	}

	/* Deserialize data and copy it from buffer into structure's fields. */
	memcpy(&device.mEndpointId, buffer, sizeof(device.mEndpointId));
	counter += sizeof(device.mEndpointId);
	memcpy(&device.mDeviceType, buffer + counter, sizeof(device.mDeviceType));
	counter += sizeof(device.mDeviceType);
	memcpy(&device.mNodeLabelLength, buffer + counter, sizeof(device.mNodeLabelLength));
	counter += sizeof(device.mNodeLabelLength);

	/* Validate that read size is big enough to include all expected data. */
	if (readSize < mandatoryItemsSize + device.mNodeLabelLength) {
		return false;
	}

	memcpy(device.mNodeLabel, buffer + counter, device.mNodeLabelLength);
	counter += device.mNodeLabelLength;

	/* Check if user prepared a buffer for reading user data. It can be nullptr if not needed. */
	if (!device.mUserData) {
		device.mUserDataSize = 0;
		return true;
	}

	uint8_t inUserDataSize = device.mUserDataSize;
	/* Validate if user buffer size is big enough to fit the stored user data size. */
	if (readSize < counter + sizeof(device.mUserDataSize)) {
		return false;
	}

	memcpy(&device.mUserDataSize, buffer + counter, sizeof(device.mUserDataSize));
	counter += sizeof(device.mUserDataSize);

	/* Validate that user data size value read from the storage is not bigger than the one expected by the user. */
	if (inUserDataSize < device.mUserDataSize) {
		return false;
	}

	/* Validate that read size is big enough to include all expected data. */
	if (readSize < counter + device.mUserDataSize) {
		return false;
	}

	memcpy(device.mUserData, buffer + counter, device.mUserDataSize);
	counter += device.mUserDataSize;

	return true;
}

bool BridgeStorageManager::StoreBridgedDevice(BridgedDevice &device, uint8_t index)
{
	uint8_t buffer[sizeof(BridgedDevice) + kMaxUserDataSize];
	uint16_t counter = 0;
	Nrf::PersistentStorageNode id = CreateIndexNode(index, &mBridgedDevice);

	/* Serialize data structure and insert it into buffer. */
	memcpy(buffer, &device.mEndpointId, sizeof(device.mEndpointId));
	counter += sizeof(device.mEndpointId);
	memcpy(buffer + counter, &device.mDeviceType, sizeof(device.mDeviceType));
	counter += sizeof(device.mDeviceType);
	memcpy(buffer + counter, &device.mNodeLabelLength, sizeof(device.mNodeLabelLength));
	counter += sizeof(device.mNodeLabelLength);
	memcpy(buffer + counter, device.mNodeLabel, device.mNodeLabelLength);
	counter += device.mNodeLabelLength;

	/* Check if there are any user data to save. mUserData can be nullptr if not needed. */
	if (device.mUserData && device.mUserDataSize > 0) {
		if (device.mUserDataSize > kMaxUserDataSize) {
			return false;
		}

		memcpy(buffer + counter, &device.mUserDataSize, sizeof(device.mUserDataSize));
		counter += sizeof(device.mUserDataSize);
		memcpy(buffer + counter, device.mUserData, device.mUserDataSize);
		counter += device.mUserDataSize;
	}

	return Nrf::GetPersistentStorage().NonSecureStore(&id, buffer, counter);
}

bool BridgeStorageManager::RemoveBridgedDevice(uint8_t index)
{
	Nrf::PersistentStorageNode id = CreateIndexNode(index, &mBridgedDevice);

	return Nrf::GetPersistentStorage().NonSecureRemove(&id);
}

#ifdef CONFIG_BRIDGED_DEVICE_BT
bool BridgeStorageManager::LoadBtAddress(bt_addr_le_t &addr, uint8_t bridgedDeviceIndex)
{
	Nrf::PersistentStorageNode id = CreateIndexNode(bridgedDeviceIndex, &mBtAddress);

	return LoadDataToObject(&id, addr);
}

bool BridgeStorageManager::RemoveBtAddress(uint8_t bridgedDeviceIndex)
{
	Nrf::PersistentStorageNode id = CreateIndexNode(bridgedDeviceIndex, &mBtAddress);

	return Nrf::GetPersistentStorage().NonSecureRemove(&id);
}
#endif

} /* namespace Nrf */
