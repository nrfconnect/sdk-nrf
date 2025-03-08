/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "persistent_storage_settings.h"
#include "settings_utils.h"

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

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
	SettingsReadEntry entry{ data, dataMaxSize, 0, false };
	settings_load_subtree_direct(key, SettingsLoadEntryCallback, &entry);

	if (!entry.result) {
		return false;
	}

	if (outSize != nullptr) {
		*outSize = entry.readSize;
	}

	return true;
}
} /* namespace Nrf */
