/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "bridge_storage_manager.h"
#include "bridge_manager.h"
#include "platform/ConfigurationManager.h"

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

namespace
{
template <class T> bool LoadDataToObject(Nrf::PersistentStorageNode *node, T &data)
{
	size_t readSize = 0;
	const Nrf::PSErrorCode status = Nrf::GetPersistentStorage().NonSecureLoad(node, &data, sizeof(T), readSize);

	return status == Nrf::PSErrorCode::Success ? sizeof(T) == readSize : false;
}

Nrf::PersistentStorageNode CreateIndexNode(uint8_t bridgedDeviceIndex, Nrf::PersistentStorageNode *parent)
{
	char index[Nrf::BridgeStorageManager::kMaxIndexLength + 1] = { 0 };

	snprintf(index, sizeof(index), "%d", bridgedDeviceIndex);

	return Nrf::PersistentStorageNode(index, strnlen(index,sizeof(index)), parent);
}

} /* namespace */

namespace Nrf
{

#ifdef CONFIG_BRIDGE_MIGRATE_VERSION_1
template <> bool BridgeStorageManager::LoadBridgedDevice(BridgedDeviceV1 &device, uint8_t index)
{
	Nrf::PersistentStorageNode id = CreateIndexNode(index, &mBridgedDevice);
	size_t readSize = 0;
	uint8_t buffer[sizeof(BridgedDeviceV1) + kMaxUserDataSize];
	uint16_t counter = 0;
	const uint8_t mandatoryItemsSize =
		sizeof(device.mEndpointId) + sizeof(device.mDeviceType) + sizeof(device.mNodeLabelLength);

	if (Nrf::GetPersistentStorage().NonSecureLoad(&id, buffer, sizeof(BridgedDeviceV1) + kMaxUserDataSize,
						      readSize) != PSErrorCode::Success) {
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
#endif

template <> bool BridgeStorageManager::LoadBridgedDevice(BridgedDeviceV2 &device, uint8_t index)
{
	Nrf::PersistentStorageNode id = CreateIndexNode(index, &mBridgedDevice);
	size_t readSize = 0;
	uint8_t buffer[sizeof(BridgedDeviceV2) + kMaxUserDataSize];
	uint16_t counter = 0;
	const uint8_t mandatoryItemsSize =
		sizeof(device.mEndpointId) + sizeof(device.mDeviceType) + sizeof(device.mUniqueIDLength);

	if (Nrf::GetPersistentStorage().NonSecureLoad(&id, buffer, sizeof(BridgedDeviceV2) + kMaxUserDataSize,
						      readSize) != PSErrorCode::Success) {
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
	memcpy(&device.mUniqueIDLength, buffer + counter, sizeof(device.mUniqueIDLength));
	counter += sizeof(device.mUniqueIDLength);

	/* Validate that read size is big enough to include all expected data. */
	if (readSize < counter + device.mUniqueIDLength) {
		return false;
	}

	memcpy(device.mUniqueID, buffer + counter, device.mUniqueIDLength);
	counter += device.mUniqueIDLength;

	/* Validate that read size is big enough to include all expected data. */
	if (readSize < counter + sizeof(device.mNodeLabelLength)) {
		return false;
	}

	memcpy(&device.mNodeLabelLength, buffer + counter, sizeof(device.mNodeLabelLength));
	counter += sizeof(device.mNodeLabelLength);

	/* Validate that read size is big enough to include all expected data. */
	if (readSize < counter + device.mNodeLabelLength) {
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

bool BridgeStorageManager::Init()
{
	const PSErrorCode status = Nrf::GetPersistentStorage().NonSecureInit(&mBridge);

	if (status != PSErrorCode::Success) {
		return false;
	}

	/* Perform data migration from previous data structure versions if needed. */
	return MigrateData();
}

void BridgeStorageManager::FactoryReset()
{
	Nrf::GetPersistentStorage().NonSecureFactoryReset();
}

#ifdef CONFIG_BRIDGE_MIGRATE_PRE_2_7_0
bool BridgeStorageManager::MigrateDataOldScheme(uint8_t bridgedDeviceIndex)
{
	BridgedDevice device;
	if (!LoadBridgedDeviceEndpointId(device.mEndpointId, bridgedDeviceIndex)) {
		return false;
	}

	/* Ignore an error, as node label is optional, so it may not be found. */
	if (!LoadBridgedDeviceNodeLabel(device.mNodeLabel, sizeof(device.mNodeLabel), device.mNodeLabelLength,
					bridgedDeviceIndex)) {
		device.mNodeLabelLength = 0;
	}

	if (!LoadBridgedDeviceType(device.mDeviceType, bridgedDeviceIndex)) {
		return false;
	}

#ifdef CONFIG_BRIDGED_DEVICE_BT
	bt_addr_le_t btAddr;

	if (!LoadBtAddress(btAddr, bridgedDeviceIndex)) {
		return false;
	}

	/* Insert Bluetooth LE address as a part of implementation specific user data. */
	device.mUserDataSize = sizeof(btAddr);
	device.mUserData = reinterpret_cast<uint8_t *>(&btAddr);
#endif

	/* Generate UniqueID */
	CHIP_ERROR result =
		chip::DeviceLayer::ConfigurationMgrImpl().GenerateUniqueId(device.mUniqueID, sizeof(device.mUniqueID));
	if (result != CHIP_NO_ERROR) {
		return false;
	}
	device.mUniqueIDLength = strlen(device.mUniqueID);

	/* Store all information using a new scheme. */
	if (!StoreBridgedDevice(device, bridgedDeviceIndex)) {
		return false;
	}

	/* Remove all information described using an old scheme. */
	RemoveBridgedDeviceEndpointId(bridgedDeviceIndex);
	RemoveBridgedDeviceNodeLabel(bridgedDeviceIndex);
	RemoveBridgedDeviceType(bridgedDeviceIndex);

#ifdef CONFIG_BRIDGED_DEVICE_BT
	RemoveBtAddress(bridgedDeviceIndex);
#endif

	return true;
}
#endif

#ifdef CONFIG_BRIDGE_MIGRATE_VERSION_1
bool BridgeStorageManager::MigrateDataVersion1(uint8_t bridgedDeviceIndex)
{
	BridgedDeviceV1 v1;
	BridgedDevice device;

#ifdef CONFIG_BRIDGED_DEVICE_BT
	bt_addr_le_t btAddr;

	/* Insert Bluetooth LE address as a part of implementation specific user data. */
	v1.mUserDataSize = sizeof(btAddr);
	v1.mUserData = reinterpret_cast<uint8_t *>(&btAddr);
#endif

	/* Load all information from old scheme */
	if (!LoadBridgedDevice(v1, bridgedDeviceIndex)) {
		return false;
	}

	/* Copy all information to new scheme */
	device.mEndpointId = v1.mEndpointId;
	device.mDeviceType = v1.mDeviceType;
	device.mNodeLabelLength = v1.mNodeLabelLength;
	memcpy(device.mNodeLabel, v1.mNodeLabel, v1.mNodeLabelLength);
	device.mUserDataSize = v1.mUserDataSize;
	device.mUserData = v1.mUserData;

	/* Generate UniqueID */
	CHIP_ERROR result =
		chip::DeviceLayer::ConfigurationMgrImpl().GenerateUniqueId(device.mUniqueID, sizeof(device.mUniqueID));
	if (result != CHIP_NO_ERROR) {
		return false;
	}
	device.mUniqueIDLength = strlen(device.mUniqueID);

	/* Store all information using new scheme */
	if (!StoreBridgedDevice(device, bridgedDeviceIndex)) {
		return false;
	}

	return true;
}
#endif

bool BridgeStorageManager::MigrateData()
{
	/* Check if migration is needed to provide backward compatibility between releases.
	 * Perform migration in following cases:
	 * 1) If the version key is missing - it means that the pre-2.7.0 release structure is used.
	 * 2) If the version key is present but the version number does not match kCurrentVersion.
	 */
	uint8_t version;
	const bool versionPresent = LoadDataToObject(&mVersion, version);
	const bool migrationNeeded = !versionPresent || version != kCurrentVersion;

	if (!migrationNeeded) {
		/* No migration needed */
		return true;
	}

	if (versionPresent && (version < 1 || version > kCurrentVersion)) {
		/* Not supported version */
		return false;
	}

	uint8_t count;
	uint8_t indexes[BridgeManager::kMaxBridgedDevices] = { 0 };
	size_t indexesCount = 0;

	if (LoadBridgedDevicesCount(count) &&
	    LoadBridgedDevicesIndexes(indexes, BridgeManager::kMaxBridgedDevices, indexesCount)) {
		/* Migrate all devices */
		for (size_t i = 0; i < indexesCount; i++) {
			if (!versionPresent) {
#ifdef CONFIG_BRIDGE_MIGRATE_PRE_2_7_0
				if (!MigrateDataOldScheme(indexes[i])) {
					return false;
				}
#else
				/* Migration not enabled */
				LOG_ERR("Migration of old data scheme not enabled.");
				return false;
#endif
			} else if (version == 1) {
#ifdef CONFIG_BRIDGE_MIGRATE_VERSION_1
				if (!MigrateDataVersion1(indexes[i])) {
					return false;
				}
#else
				/* Migration not enabled */
				LOG_ERR("Migration of data scheme version 1 not enabled.");
				return false;
#endif
			}
		}
	}

	/* Store current version */
	version = kCurrentVersion;
	const PSErrorCode status = Nrf::GetPersistentStorage().NonSecureStore(&mVersion, &version, sizeof(version));

	return status == PSErrorCode::Success;
}

bool BridgeStorageManager::StoreBridgedDevicesCount(uint8_t count)
{
	const PSErrorCode status =
		Nrf::GetPersistentStorage().NonSecureStore(&mBridgedDevicesCount, &count, sizeof(count));

	return status == PSErrorCode::Success;
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

	const PSErrorCode status = Nrf::GetPersistentStorage().NonSecureStore(&mBridgedDevicesIndexes, indexes, count);

	return status == PSErrorCode::Success;
}

bool BridgeStorageManager::LoadBridgedDevicesIndexes(uint8_t *indexes, uint8_t maxCount, size_t &count)
{
	if (!indexes) {
		return false;
	}

	const PSErrorCode status =
		Nrf::GetPersistentStorage().NonSecureLoad(&mBridgedDevicesIndexes, indexes, maxCount, count);

	return status == PSErrorCode::Success;
}

#ifdef CONFIG_BRIDGE_MIGRATE_PRE_2_7_0
bool BridgeStorageManager::LoadBridgedDeviceEndpointId(uint16_t &endpointId, uint8_t bridgedDeviceIndex)
{
	Nrf::PersistentStorageNode id = CreateIndexNode(bridgedDeviceIndex, &mBridgedDeviceEndpointId);

	return LoadDataToObject(&id, endpointId);
}

bool BridgeStorageManager::RemoveBridgedDeviceEndpointId(uint8_t bridgedDeviceIndex)
{
	Nrf::PersistentStorageNode id = CreateIndexNode(bridgedDeviceIndex, &mBridgedDeviceEndpointId);
	const PSErrorCode status = Nrf::GetPersistentStorage().NonSecureRemove(&id);

	return status == PSErrorCode::Success;
}

bool BridgeStorageManager::LoadBridgedDeviceNodeLabel(char *label, size_t labelMaxLength, size_t &labelLength,
						      uint8_t bridgedDeviceIndex)
{
	Nrf::PersistentStorageNode id = CreateIndexNode(bridgedDeviceIndex, &mBridgedDeviceNodeLabel);
	const PSErrorCode status = Nrf::GetPersistentStorage().NonSecureLoad(&id, label, labelMaxLength, labelLength);

	return status == PSErrorCode::Success;
}

bool BridgeStorageManager::RemoveBridgedDeviceNodeLabel(uint8_t bridgedDeviceIndex)
{
	Nrf::PersistentStorageNode id = CreateIndexNode(bridgedDeviceIndex, &mBridgedDeviceNodeLabel);
	const PSErrorCode status = Nrf::GetPersistentStorage().NonSecureRemove(&id);

	return status == PSErrorCode::Success;
}

bool BridgeStorageManager::LoadBridgedDeviceType(uint16_t &deviceType, uint8_t bridgedDeviceIndex)
{
	Nrf::PersistentStorageNode id = CreateIndexNode(bridgedDeviceIndex, &mBridgedDeviceType);

	return LoadDataToObject(&id, deviceType);
}

bool BridgeStorageManager::RemoveBridgedDeviceType(uint8_t bridgedDeviceIndex)
{
	Nrf::PersistentStorageNode id = CreateIndexNode(bridgedDeviceIndex, &mBridgedDeviceType);
	const PSErrorCode status = Nrf::GetPersistentStorage().NonSecureRemove(&id);

	return status == PSErrorCode::Success;
}
#endif

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
	memcpy(buffer + counter, &device.mUniqueIDLength, sizeof(device.mUniqueIDLength));
	counter += sizeof(device.mUniqueIDLength);
	memcpy(buffer + counter, device.mUniqueID, device.mUniqueIDLength);
	counter += device.mUniqueIDLength;
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

	const PSErrorCode status = Nrf::GetPersistentStorage().NonSecureStore(&id, buffer, counter);

	return status == PSErrorCode::Success;
}

bool BridgeStorageManager::RemoveBridgedDevice(uint8_t index)
{
	Nrf::PersistentStorageNode id = CreateIndexNode(index, &mBridgedDevice);
	const PSErrorCode status = Nrf::GetPersistentStorage().NonSecureRemove(&id);

	return status == PSErrorCode::Success;
}

#ifdef CONFIG_BRIDGE_MIGRATE_PRE_2_7_0
#ifdef CONFIG_BRIDGED_DEVICE_BT
bool BridgeStorageManager::LoadBtAddress(bt_addr_le_t &addr, uint8_t bridgedDeviceIndex)
{
	Nrf::PersistentStorageNode id = CreateIndexNode(bridgedDeviceIndex, &mBtAddress);

	return LoadDataToObject(&id, addr);
}

bool BridgeStorageManager::RemoveBtAddress(uint8_t bridgedDeviceIndex)
{
	Nrf::PersistentStorageNode id = CreateIndexNode(bridgedDeviceIndex, &mBtAddress);
	const PSErrorCode status = Nrf::GetPersistentStorage().NonSecureRemove(&id);

	return status == PSErrorCode::Success;
}
#endif
#endif

} /* namespace Nrf */
