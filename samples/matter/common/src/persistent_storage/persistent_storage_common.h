/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include <zephyr/sys/cbprintf.h>

namespace Nrf
{
enum PSErrorCode : uint8_t { Failure, Success, NotSupported };

/**
 * @brief Class representing single tree node and containing information about its key.
 */
class PersistentStorageNode {
public:
	static constexpr uint8_t kMaxKeyNameLength{ CONFIG_NCS_SAMPLE_MATTER_STORAGE_MAX_KEY_LEN };

	/**
	 * @brief Constructor assigns name of a key for this node and sets parent node address in the tree
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
	 * @brief Gets complete key name for this node including its position in the tree hierarchy. The key
	 * name is a key name specific for this node concatenated with the names of all parents up to the top of the
	 * tree.
	 *
	 * @return true if the key has been created successfully
	 * @return false otherwise
	 */
	bool GetKey(char *key)
	{
		if (!key || mKeyName[0] == '\0') {
			return false;
		}

		/* Recursively call GetKey method for the parents until the full key name including all hierarchy levels
		 * will be created. */
		if (mParent != nullptr) {
			char parentKey[kMaxKeyNameLength];

			if (!mParent->GetKey(parentKey)) {
				return false;
			}

			int result = snprintf(key, kMaxKeyNameLength, "%s/%s", parentKey, mKeyName);

			/* snprintf() returns the number of characters written not counting the terminating null. */
			if (result < 0 || result + 1 > kMaxKeyNameLength) {
				return false;
			}

		} else {
			/* In case of not having a parent, return only own key name. */
			strncpy(key, mKeyName, kMaxKeyNameLength);
		}

		return true;
	}

private:
	PersistentStorageNode *mParent = nullptr;
	char mKeyName[kMaxKeyNameLength] = { 0 };
};

} /* namespace Nrf */
