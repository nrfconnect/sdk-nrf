/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "credentials_storage.h"

#include <persistent_storage/persistent_storage.h>

#include <zephyr/sys/cbprintf.h>

#ifdef CONFIG_LOCK_PRINT_STORAGE_STATUS
#include <zephyr/fs/nvs.h>
#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>

LOG_MODULE_DECLARE(storage_manager, CONFIG_CHIP_APP_LOG_LEVEL);

namespace
{
bool GetStorageFreeSpace(size_t &freeBytes)
{
	void *storage = nullptr;
	int status = settings_storage_get(&storage);
	if (status != 0 || !storage) {
		LOG_ERR("CredentialsStorage: Cannot read NVS free space [error: %d]", status);
		return false;
	}
	freeBytes = nvs_calc_free_space(static_cast<nvs_fs *>(storage));
	return true;
}
} /* namespace */
#endif

/* Currently the secure storage is available only for non-Wi-Fi builds,
   because NCS Wi-Fi implementation does not support PSA API yet. */
#ifdef CONFIG_CHIP_WIFI
#define PSInit NonSecureInit
#define PSStore NonSecureStore
#define PSLoad NonSecureLoad
#else
#define PSInit SecureInit
#define PSStore SecureStore
#define PSLoad SecureLoad
#endif

bool CredentialsStorage::Init()
{
	return (Nrf::Success == Nrf::GetPersistentStorage().PSInit());
}

bool CredentialsStorage::PrepareKeyName(Type storageType, uint16_t index, uint16_t subindex)
{
	memset(mKeyName, '\0', sizeof(mKeyName));

	uint8_t limitedIndex = static_cast<uint8_t>(index);
	uint8_t limitedSubindex = static_cast<uint8_t>(subindex);

	switch (storageType) {
	case Type::User:
		if (0 == limitedIndex && 0 == limitedSubindex) {
			(void)snprintf(mKeyName, kMaxCredentialsName, "%s/%s", kCredentialsPrefix, kUserPrefix);
		} else if (0 == limitedSubindex) {
			(void)snprintf(mKeyName, kMaxCredentialsName, "%s/%s/%u", kCredentialsPrefix, kUserPrefix,
				       limitedIndex);
		} else {
			(void)snprintf(mKeyName, kMaxCredentialsName, "%s/%s/%u/%u", kCredentialsPrefix, kUserPrefix,
				       limitedIndex, limitedSubindex);
		}
		return true;
	case Type::Credential:
		if (0 == limitedIndex && 0 == limitedSubindex) {
			(void)snprintf(mKeyName, kMaxCredentialsName, "%s", kCredentialsPrefix);
		} else if (0 == limitedSubindex) {
			(void)snprintf(mKeyName, kMaxCredentialsName, "%s/%u", kCredentialsPrefix, limitedIndex);
		} else {
			(void)snprintf(mKeyName, kMaxCredentialsName, "%s/%u/%u", kCredentialsPrefix, limitedIndex,
				       limitedSubindex);
		}
		return true;
	case Type::UsersIndexes:
		(void)snprintf(mKeyName, kMaxCredentialsName, "%s/%s", kCredentialsPrefix, kUserCounterPrefix);
		return true;
	case Type::CredentialsIndexes:
		(void)snprintf(mKeyName, kMaxCredentialsName, "%s/%u/%s", kCredentialsPrefix, limitedIndex,
			       kCredentialsCounterPrefix);
		return true;
	case Type::RequirePIN:
		(void)snprintf(mKeyName, kMaxCredentialsName, "%s", kRequirePinPrefix);
		return true;
	default:
		break;
	}

	return false;
}

bool CredentialsStorage::Store(Type storageType, const void *data, size_t dataSize, uint16_t index, uint16_t subindex)
{
	if (data == nullptr || !PrepareKeyName(storageType, index, subindex)) {
		return false;
	}

	Nrf::PersistentStorageNode node{ mKeyName, strlen(mKeyName) + 1 };
	bool ret = (Nrf::PSErrorCode::Success == Nrf::GetPersistentStorage().PSStore(&node, data, dataSize));

#ifdef CONFIG_LOCK_PRINT_STORAGE_STATUS
	if (ret) {
		LOG_DBG("CredentialsStorage: Stored %s of size: %d bytes",
			storageType == Type::User ? "user" : "credential", dataSize);

		size_t storageFreeSpace;
		if (GetStorageFreeSpace(storageFreeSpace)) {
			LOG_DBG("CredentialsStorage: Free space: %d bytes", storageFreeSpace);
		}
	}
#endif

	return ret;
}

bool CredentialsStorage::Load(Type storageType, void *data, size_t dataSize, size_t &outSize, uint16_t index,
			      uint16_t subindex)
{
	if (data == nullptr || !PrepareKeyName(storageType, index, subindex)) {
		return false;
	}

	Nrf::PersistentStorageNode node{ mKeyName, strlen(mKeyName) + 1 };
	Nrf::PSErrorCode result = Nrf::GetPersistentStorage().PSLoad(&node, data, dataSize, outSize);

	return (Nrf::PSErrorCode::Success == result);
}
