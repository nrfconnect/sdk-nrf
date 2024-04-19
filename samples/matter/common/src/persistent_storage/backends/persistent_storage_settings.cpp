/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "persistent_storage_settings.h"

#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

namespace
{
struct ReadEntry {
	void *destination;
	size_t destinationBufferSize;
	size_t readSize;
	bool result;
};

/* Random magic bytes to represent an empty value.
   It is needed because Zephyr settings subsystem does not distinguish an empty value from no value. */
constexpr uint8_t kEmptyValue[] = { 0x22, 0xa6, 0x54, 0xd1, 0x39 };
constexpr size_t kEmptyValueSize = sizeof(kEmptyValue);

int LoadEntryCallback(const char *name, size_t entrySize, settings_read_cb readCb, void *cbArg, void *param)
{
	ReadEntry &entry = *static_cast<ReadEntry *>(param);

	/* name != nullptr -> If requested key X is empty, process the next one
	   name != '\0' -> process just node X and ignore all its descendants: X */
	if (name != nullptr && *name != '\0') {
		return 0;
	}

	if (!entry.destination || 0 == entry.destinationBufferSize) {
		/* Just found the key, do not try to read value */
		entry.result = true;
		return 1;
	}

	uint8_t emptyValue[kEmptyValueSize];

	if (entrySize == kEmptyValueSize && readCb(cbArg, emptyValue, kEmptyValueSize) == kEmptyValueSize &&
	    memcmp(emptyValue, kEmptyValue, kEmptyValueSize) == 0) {
		/* There are wrong bytes stored - the same as the magic ones defined above */
		entry.result = false;
		return 1;
	}

	const ssize_t bytesRead = readCb(cbArg, entry.destination, entry.destinationBufferSize);
	entry.readSize = bytesRead > 0 ? bytesRead : 0;

	if (entrySize > entry.destinationBufferSize) {
		entry.result = false;
	} else {
		entry.result = bytesRead > 0;
	}

	return 1;
}
} /* namespace */

namespace Nrf
{
PSErrorCode PersistentStorageSettings::_NonSecureInit()
{
	return settings_load() ? PSErrorCode::Failure : PSErrorCode::Success;
}

PSErrorCode PersistentStorageSettings::_NonSecureStore(PersistentStorageNode *node, const void *data, size_t dataSize)
{
	if (!data || !node) {
		return PSErrorCode::Failure;
	}

	char key[PersistentStorageNode::kMaxKeyNameLength];

	if (!node->GetKey(key)) {
		return PSErrorCode::Failure;
	}

	return (settings_save_one(key, data, dataSize) ? PSErrorCode::Failure : PSErrorCode::Success);
}

PSErrorCode PersistentStorageSettings::_NonSecureLoad(PersistentStorageNode *node, void *data, size_t dataMaxSize,
						      size_t &outSize)
{
	if (!data || !node) {
		return PSErrorCode::Failure;
	}

	char key[PersistentStorageNode::kMaxKeyNameLength];

	if (!node->GetKey(key)) {
		return PSErrorCode::Failure;
	}

	size_t resultSize;

	bool result = LoadEntry(key, data, dataMaxSize, &resultSize);

	if (result) {
		outSize = resultSize;
	}

	return (result ? PSErrorCode::Success : PSErrorCode::Failure);
}

PSErrorCode PersistentStorageSettings::_NonSecureHasEntry(PersistentStorageNode *node)
{
	if (!node) {
		return PSErrorCode::Failure;
	}

	char key[PersistentStorageNode::kMaxKeyNameLength];

	if (!node->GetKey(key)) {
		return PSErrorCode::Failure;
	}

	return (LoadEntry(key) ? PSErrorCode::Success : PSErrorCode::Failure);
}

PSErrorCode PersistentStorageSettings::_NonSecureRemove(PersistentStorageNode *node)
{
	if (!node) {
		return PSErrorCode::Failure;
	}

	char key[PersistentStorageNode::kMaxKeyNameLength];

	if (!node->GetKey(key)) {
		return PSErrorCode::Failure;
	}

	if (!LoadEntry(key)) {
		return PSErrorCode::Failure;
	}

	settings_delete(key);

	return PSErrorCode::Success;
}

bool PersistentStorageSettings::LoadEntry(const char *key, void *data, size_t dataMaxSize, size_t *outSize)
{
	ReadEntry entry{ data, dataMaxSize, 0, false };
	settings_load_subtree_direct(key, LoadEntryCallback, &entry);

	if (!entry.result) {
		return false;
	}

	if (outSize != nullptr) {
		*outSize = entry.readSize;
	}

	return true;
}
} /* namespace Nrf */
