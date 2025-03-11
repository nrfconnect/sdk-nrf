/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "../persistent_storage_common.h"

namespace Nrf
{
class PersistentStorageSettings {
protected:
	PSErrorCode _NonSecureInit(const char *prefix);
	PSErrorCode _NonSecureStore(PersistentStorageNode *node, const void *data, size_t dataSize);
	PSErrorCode _NonSecureLoad(PersistentStorageNode *node, void *data, size_t dataMaxSize, size_t &outSize);
	PSErrorCode _NonSecureHasEntry(PersistentStorageNode *node);
	PSErrorCode _NonSecureRemove(PersistentStorageNode *node);
	PSErrorCode _NonSecureFactoryReset();

	PSErrorCode _SecureInit(const char *prefix);
	PSErrorCode _SecureStore(PersistentStorageNode *node, const void *data, size_t dataSize);
	PSErrorCode _SecureLoad(PersistentStorageNode *node, void *data, size_t dataMaxSize, size_t &outSize);
	PSErrorCode _SecureHasEntry(PersistentStorageNode *node);
	PSErrorCode _SecureRemove(PersistentStorageNode *node);
	PSErrorCode _SecureFactoryReset();

private:
	bool LoadEntry(const char *key, void *data = nullptr, size_t dataMaxSize = 0, size_t *outSize = nullptr);

	const char *mPrefix = nullptr;
};

inline PSErrorCode PersistentStorageSettings::_SecureInit(const char *prefix)
{
	return PSErrorCode::NotSupported;
};

inline PSErrorCode PersistentStorageSettings::_SecureStore(PersistentStorageNode *node, const void *data,
							   size_t dataSize)
{
	return PSErrorCode::NotSupported;
}

inline PSErrorCode PersistentStorageSettings::_SecureLoad(PersistentStorageNode *node, void *data, size_t dataMaxSize,
							  size_t &outSize)
{
	return PSErrorCode::NotSupported;
}

inline PSErrorCode PersistentStorageSettings::_SecureHasEntry(PersistentStorageNode *node)
{
	return PSErrorCode::NotSupported;
}

inline PSErrorCode PersistentStorageSettings::_SecureRemove(PersistentStorageNode *node)
{
	return PSErrorCode::NotSupported;
}

inline PSErrorCode PersistentStorageSettings::_SecureFactoryReset()
{
	return PSErrorCode::NotSupported;
}

} /* namespace Nrf */
