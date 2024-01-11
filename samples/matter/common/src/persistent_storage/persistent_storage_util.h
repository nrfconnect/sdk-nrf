/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include <zephyr/settings/settings.h>
#include <zephyr/sys/cbprintf.h>

namespace Nrf {

/**
 * @brief Class representing single settings tree node and containing information about its key.
 */
class PersistentStorageNode {
public:
	static constexpr auto kMaxKeyNameLength = (SETTINGS_MAX_NAME_LEN + 1);

	/**
	 * @brief Constructor assigns name of a key for this settings node and sets parent node address in the tree
	 * hierarchy. The node does not need to have a parent (parent = nullptr), if it is a top level element.
	 */
	PersistentStorageNode(const char *keyName, size_t keyNameLength, PersistentStorageNode *parent = nullptr)
	{
		if (keyName && keyNameLength < kMaxKeyNameLength) {
			memcpy(mKeyName, keyName, keyNameLength);
			mKeyName[keyNameLength] = '\0';
		}
		mParent = parent;
	}

	/**
	 * @brief Gets complete settings key name for this node including its position in the tree hierarchy. The key
	 * name is a key name specific for this node concatenated with the names of all parents up to the top of the
	 * tree.
	 *
	 * @return true if the key has been created successfully
	 * @return false otherwise
	 */
	bool GetKey(char *key);

private:
	PersistentStorageNode *mParent = nullptr;
	char mKeyName[kMaxKeyNameLength] = { 0 };
};

/**
 * @brief Class to manage peristent storage using generic PersistentStorageNode data unit that wraps settings key
 * information.
 */
class PersistentStorage {
public:
	/**
	 * @brief Load all settings from persistent storage
	 *
	 * @return true if settings have been loaded
	 * @return false otherwise
	 */
	bool Init();

	/**
	 * @brief Store data into settings
	 *
	 * @param node address of settings tree node containing information about the key
	 * @param data data to store into a specific settings key
	 * @param dataSize a size of input buffer
	 * @return true if key has been written successfully
	 * @return false an error occurred
	 */
	bool Store(PersistentStorageNode *node, const void *data, size_t dataSize);

	/**
	 * @brief Load data from settings
	 *
	 * @param node address of settings tree node containing information about the key
	 * @param data data buffer to load from a specific settings key
	 * @param dataMaxSize a size of data buffer to load
	 * @param outSize an actual size of read data
	 * @return true if key has been loaded successfully
	 * @return false an error occurred
	 */
	bool Load(PersistentStorageNode *node, void *data, size_t dataMaxSize, size_t &outSize);

	/**
	 * @brief Check if given key entry exists in settings
	 *
	 * @param node address of settings tree node containing information about the key
	 * @return true if key entry exists
	 * @return false if key entry does not exist
	 */
	bool HasEntry(PersistentStorageNode *node);

	/**
	 * @brief Remove given key entry from settings
	 *
	 * @param node address of settings tree node containing information about the key
	 * @return true if key entry has been removed successfully
	 * @return false an error occurred
	 */
	bool Remove(PersistentStorageNode *node);

	static PersistentStorage &Instance()
	{
		static PersistentStorage sInstance;
		return sInstance;
	}

private:
	bool LoadEntry(const char *key, void *data = nullptr, size_t dataMaxSize = 0, size_t *outSize = nullptr);
};

} /* namespace Nrf */
