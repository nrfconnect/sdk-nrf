/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include <persistent_storage/persistent_storage_common.h>

/**
 * @brief Class to manage credentials storage
 *
 * Eventually credentials will be store to the Zephyr settings to be persistent (regardless of the used persistent
 * storage backend). There are several credential types: PIN, RFID, Fingerprint etc. and each of them is declared as an
 * array with the maximum size defined by Kconfig ...MAX_NUM_CREDENTIALS_PER_USER. Each user has a list of credentials
 * that are assign to them. The new credential should be written with the new index if the previous one is already
 * occupied.
 *
 * Agreed the following settings key convention:
 *
 * /cr/
 *    /<credential type (int)>/
 *                           /cr_idxs (bytes)/
 *                           /<credential index (int)>/
 *                                                    /<credential data (bytes)>
 *    /usr_idxs (bytes)/
 *    /usr/
 *        /<user index (int)>/
 *                           / <user data (bytes)> /
 */

class CredentialsStorage {
public:
	enum class Type : uint8_t { User, Credential, UsersIndexes, CredentialsIndexes, RequirePIN };

	/**
	 * @brief Initialize the persistent storage.
	 *
	 * @return true if success.
	 * @return false otherwise.
	 */
	bool Init();

	/**
	 * @brief Store the entry into the persistent storage.
	 *
	 * @param storageType depending on that there will be a different key set.
	 * @param data data to store under a specific key.
	 * @param dataSize a size of input buffer.
	 * @param index a number to be appended to key (<key>/index/).
	 * @param subindex a number to be appended to key and index (<key>/index/subindex).
	 * @return true if data has been written successfully.
	 * @return false an error occurred.
	 */
	bool Store(Type storageType, const void *data, size_t dataSize, uint16_t index = 0, uint16_t subindex = 0);

	/**
	 * @brief Load an entry from the persistent storage.
	 *
	 * @param storageType depending on that there will be a different key set.
	 * @param data data buffer to load from a specific key.
	 * @param dataSize a size of input buffer.
	 * @param index a number to be appended to key (<key>/index).
	 * @param subindex a number to be appended to key and index (<key>/index/subindex).
	 * @return true if data has been loaded successfully.
	 * @return false an error occurred.
	 */
	bool Load(Type storageType, void *data, size_t dataSize, size_t &outSize, uint16_t index = 0,
		  uint16_t subindex = 0);

	static CredentialsStorage &Instance()
	{
		static CredentialsStorage sInstance;
		return sInstance;
	}

private:
	constexpr static auto kCredentialsPrefix = "cr";
	constexpr static auto kCredentialsCounterPrefix = "cr_idxs";
	constexpr static auto kUserPrefix = "usr";
	constexpr static auto kUserCounterPrefix = "usr_idxs";
	constexpr static auto kRequirePinPrefix = "pin_req";
	constexpr static auto kMaxCredentialsName = Nrf::PersistentStorageNode::kMaxKeyNameLength;

	char mKeyName[CredentialsStorage::kMaxCredentialsName];

	bool PrepareKeyName(Type storageType, uint16_t index, uint16_t subindex);
};
