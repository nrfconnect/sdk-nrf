/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "persistent_storage_secure.h"

namespace Nrf
{

PersistentStorageSecure::UidMap PersistentStorageSecure::sUidMap{};
PersistentStorageSecure::Byte PersistentStorageSecure::sSerializedMapBuff[kMaxMapSerializationBufferSize]{};

PSErrorCode PersistentStorageSecure::_SecureInit()
{
	return LoadUIDMap();
}

PSErrorCode PersistentStorageSecure::_SecureStore(PersistentStorageNode *node, const void *data, size_t dataSize)
{
	/* Check if we can store more assets and return prematurely if the limit has been already reached. */
	if (kMaxEntriesNumber <= sUidMap.Size()) {
		return PSErrorCode::Failure;
	}

	char key[PersistentStorageNode::kMaxKeyNameLength];

	if (node->GetKey(key)) {
		psa_storage_uid_t uid = UIDFromString(key);
		psa_status_t status = psa_ps_set(uid, dataSize, data, PSA_STORAGE_FLAG_NONE);

		if (status == PSA_SUCCESS) {
			if (sUidMap.Insert(uid, StringWrapper(key))) {
				/* The map has been updated, store it. */
				if (PSErrorCode::Success != StoreUIDMap()) {
					/* We cannot store the updated UID map, so it's pointless to keep the data
					 * associated with calculated UID persistently. */
					psa_ps_remove(uid);
				} else {
					return PSErrorCode::Success;
				}
			}
			/* It fine for the Insert() to fail, in case the given key is already present in the map. */
			return PSErrorCode::Success;
		}
	}
	return PSErrorCode::Failure;
}

PSErrorCode PersistentStorageSecure::_SecureLoad(PersistentStorageNode *node, void *data, size_t dataMaxSize,
						 size_t &outSize)
{
	psa_storage_uid_t uid;
	if (HasEntry(node, uid)) {
		psa_status_t status = psa_ps_get(uid, 0, dataMaxSize, data, &outSize);
		return (status == PSA_SUCCESS ? PSErrorCode::Success : PSErrorCode::Failure);
	}
	return PSErrorCode::Failure;
}

PSErrorCode PersistentStorageSecure::_SecureHasEntry(PersistentStorageNode *node)
{
	psa_storage_uid_t uid;
	return (HasEntry(node, uid) ? PSErrorCode::Success : PSErrorCode::Failure);
}

PSErrorCode PersistentStorageSecure::_SecureRemove(PersistentStorageNode *node)
{
	char key[PersistentStorageNode::kMaxKeyNameLength];

	if (node->GetKey(key)) {
		bool alreadyInTheMap{ false };
		psa_storage_uid_t uid = UIDFromString(key, &alreadyInTheMap);
		if (alreadyInTheMap) {
			psa_status_t status = psa_ps_remove(uid);

			if (status == PSA_SUCCESS) {
				sUidMap.Erase(uid);
				return PSErrorCode::Success;
			}
		}
	}

	return PSErrorCode::Failure;
}

psa_storage_uid_t PersistentStorageSecure::UIDFromString(char *str, bool *alreadyInTheMap)
{
	for (auto &it : sUidMap.mMap) {
		if (it.value == str) {
			if (alreadyInTheMap) {
				*alreadyInTheMap = true;
			}
			return static_cast<psa_storage_uid_t>(it.key);
		}
	}

	/* The first available UID under kKeyOffset is reserved for the UID Map, so we need to include that when storing
	 * regular keys by adding kMapUidRelativeOffset. */
	uint16_t slot = sUidMap.GetFirstFreeSlot();
	if (alreadyInTheMap) {
		*alreadyInTheMap = false;
	}
	return static_cast<psa_storage_uid_t>(slot + kKeyOffset + kMapUidRelativeOffset);
}

PSErrorCode PersistentStorageSecure::StoreUIDMap()
{
	size_t outSize{ 0 };
	if (PSErrorCode::Success == SerializeUIDMap(sSerializedMapBuff, kMaxMapSerializationBufferSize, outSize)) {
		psa_status_t status = psa_ps_set(kKeyOffset, outSize, sSerializedMapBuff, PSA_STORAGE_FLAG_NONE);
		if (status == PSA_SUCCESS) {
			return PSErrorCode::Success;
		}
	}
	return PSErrorCode::Failure;
}

PSErrorCode PersistentStorageSecure::SerializeUIDMap(Byte *buff, size_t buffSize, size_t &outSize)
{
	size_t keySize{ sizeof(SerializedUIDType) };
	size_t valueSize{ 0 };
	UidMap::ElementCounterType offset{ 0 };
	UidMap::ElementCounterType mapSize = sUidMap.Size();

	if (buffSize < kMaxMapSerializationBufferSize) {
		return PSErrorCode::Failure;
	}

	memcpy(buff + offset, &mapSize, sizeof(mapSize));
	offset += sizeof(mapSize);

	for (auto it = std::begin(sUidMap.mMap); it != std::end(sUidMap.mMap) - sUidMap.FreeSlots(); ++it) {
		valueSize = strlen(it->value.mStr) + 1;
		SerializedUIDType keyToSerialize = static_cast<SerializedUIDType>(it->key);
		memcpy(buff + offset, &keyToSerialize, keySize);
		offset += keySize;
		memcpy(buff + offset, it->value.mStr, valueSize);
		offset += valueSize;
	}
	outSize = offset;
	return PSErrorCode::Success;
}

PSErrorCode PersistentStorageSecure::LoadUIDMap()
{
	size_t outSize{ 0 };

	psa_status_t status = psa_ps_get(kKeyOffset, 0, kMaxMapSerializationBufferSize, sSerializedMapBuff, &outSize);

	if (status == PSA_SUCCESS) {
		if (PSErrorCode::Success == DeserializeUIDMap(sSerializedMapBuff, kMaxMapSerializationBufferSize)) {
			return PSErrorCode::Success;
		}
	}
	return PSErrorCode::Failure;
}

PSErrorCode PersistentStorageSecure::DeserializeUIDMap(const Byte *buff, size_t buffSize)
{
	UidMap::ElementCounterType mapSize{ 0 };
	UidMap::ElementCounterType offset{ 0 };
	SerializedUIDType key{ 0 };
	char value[PersistentStorageNode::kMaxKeyNameLength] = { 0 };

	memcpy(&mapSize, buff + offset, sizeof(mapSize));
	offset += sizeof(mapSize);

	for (uint16_t element = 0; element < mapSize; ++element) {
		memcpy(&key, buff + offset, sizeof(key));
		offset += sizeof(key);
		strcpy(value, buff + offset);
		offset += strlen(value) + 1;
		sUidMap.Insert(static_cast<psa_storage_uid_t>(key), StringWrapper(value));
	}

	/* The stored size shall equal the resulting size. */
	if (sUidMap.Size() != mapSize) {
		return PSErrorCode::Failure;
	}

	return PSErrorCode::Success;
}

bool PersistentStorageSecure::HasEntry(PersistentStorageNode *node, psa_storage_uid_t &uid)
{
	char key[PersistentStorageNode::kMaxKeyNameLength];
	bool alreadyInTheMap{ false };

	if (node->GetKey(key)) {
		uid = UIDFromString(key, &alreadyInTheMap);
	}

	return alreadyInTheMap;
}

} /* namespace Nrf */
