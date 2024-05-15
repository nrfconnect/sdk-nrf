/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "../persistent_storage_common.h"
#include "util/finite_map.h"

#include <protected_storage.h>

namespace Nrf
{
class PersistentStorageSecure {
protected:
	PSErrorCode _NonSecureInit();
	PSErrorCode _NonSecureStore(PersistentStorageNode *node, const void *data, size_t dataSize);
	PSErrorCode _NonSecureLoad(PersistentStorageNode *node, void *data, size_t dataMaxSize, size_t &outSize);
	PSErrorCode _NonSecureHasEntry(PersistentStorageNode *node);
	PSErrorCode _NonSecureRemove(PersistentStorageNode *node);

	PSErrorCode _SecureInit();
	PSErrorCode _SecureStore(PersistentStorageNode *node, const void *data, size_t dataSize);
	PSErrorCode _SecureLoad(PersistentStorageNode *node, void *data, size_t dataMaxSize, size_t &outSize);
	PSErrorCode _SecureHasEntry(PersistentStorageNode *node);
	PSErrorCode _SecureRemove(PersistentStorageNode *node);

private:
	static constexpr size_t kMaxEntriesNumber = CONFIG_NCS_SAMPLE_MATTER_SECURE_STORAGE_MAX_ENTRY_NUMBER;
	static constexpr psa_storage_uid_t kKeyOffset = CONFIG_NCS_SAMPLE_MATTER_SECURE_STORAGE_PSA_KEY_VALUE_OFFSET;
	static constexpr uint8_t kMapUidRelativeOffset = 1;

	/* Needed to interface with FiniteMap. */
	struct StringWrapper {
		static constexpr auto kSize = PersistentStorageNode::kMaxKeyNameLength;
		StringWrapper() {}
		StringWrapper(char *str) { strcpy(mStr, str); }

		~StringWrapper() {}

		/* Disable copy semantics and implement move semantics. */
		StringWrapper(const StringWrapper &other) = delete;
		StringWrapper &operator=(const StringWrapper &other) = delete;

		StringWrapper(StringWrapper &&other)
		{
			strcpy(mStr, other.mStr);
			other.Clear();
		}

		StringWrapper &operator=(StringWrapper &&other)
		{
			if (this != &other) {
				this->Clear();
				strcpy(mStr, other.mStr);
				other.Clear();
			}
			return *this;
		}

		operator bool() const { return !strcmp(mStr, ""); }
		/* This will work also when comparing against a raw string. */
		bool operator==(const StringWrapper &other) { return (strcmp(mStr, other.mStr) == 0); }

		void Clear() { memset(mStr, 0, kSize); }

		char mStr[kSize] = { 0 };
	};

	using UidMap = FiniteMap<psa_storage_uid_t, StringWrapper, kMaxEntriesNumber>;
	using Byte = char;
	/* We have the range of the UID [0x40000, 40000 + kMaxEntriesNumber], so 4 bytes is enough to store it.
	   Note that psa_storage_uid_t is 8 byte long.*/
	using SerializedUIDType = uint32_t;

	static PSErrorCode SerializeUIDMap(Byte *buff, size_t buffSize, size_t &outSize);
	static PSErrorCode DeserializeUIDMap(const Byte *buff, size_t buffSize);
	static PSErrorCode StoreUIDMap();
	static PSErrorCode LoadUIDMap();
	static bool HasEntry(PersistentStorageNode *node, psa_storage_uid_t &uid);
	static psa_storage_uid_t UIDFromString(char *str, bool *alreadyInTheMap = nullptr);

	static constexpr size_t kMaxMapSerializationBufferSize =
		(PersistentStorageNode::kMaxKeyNameLength + sizeof(SerializedUIDType)) * kMaxEntriesNumber +
		sizeof(UidMap::ElementCounterType);

	static UidMap sUidMap;
	static Byte sSerializedMapBuff[kMaxMapSerializationBufferSize];
};

inline PSErrorCode PersistentStorageSecure::_NonSecureInit()
{
	return PSErrorCode::NotSupported;
};

inline PSErrorCode PersistentStorageSecure::_NonSecureStore(PersistentStorageNode *node, const void *data,
							    size_t dataSize)
{
	return PSErrorCode::NotSupported;
}

inline PSErrorCode PersistentStorageSecure::_NonSecureLoad(PersistentStorageNode *node, void *data, size_t dataMaxSize,
							   size_t &outSize)
{
	return PSErrorCode::NotSupported;
}

inline PSErrorCode PersistentStorageSecure::_NonSecureHasEntry(PersistentStorageNode *node)
{
	return PSErrorCode::NotSupported;
}

inline PSErrorCode PersistentStorageSecure::_NonSecureRemove(PersistentStorageNode *node)
{
	return PSErrorCode::NotSupported;
}

} /* namespace Nrf */
