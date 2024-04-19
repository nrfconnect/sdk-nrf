/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "persistent_storage_common.h"

namespace Nrf
{

class PersistentStorageImpl;

/**
 * @brief Interface class to manage persistent storage using generic PersistentStorageNode data unit.
 *
 * PersistentStorage interface consists of NonSecure* and Secure* methods that implement the persistence related
 * operations. The NonSecure* prefix indicates that the method does not provide additional security layer and keeps the
 * raw material in a persistent storage. Secure* prefix indicates that the method shall leverage the additional security
 * layer, like PSA Certified API, that typically causes extra computational overhead and memory usage.
 */
class PersistentStorage {
public:
	/**
	 * @brief Perform the initialization required before using the storage.
	 *
	 * @return true if success.
	 * @return false otherwise.
	 */
	PSErrorCode NonSecureInit();

	/**
	 * @brief Store data into the persistent storage.
	 *
	 * @param node address of the tree node containing information about the key.
	 * @param data data to store into a specific key.
	 * @param dataSize a size of input buffer.
	 * @return true if key has been written successfully.
	 * @return false an error occurred.
	 */
	PSErrorCode NonSecureStore(PersistentStorageNode *node, const void *data, size_t dataSize);

	/**
	 * @brief Load data from the persistent storage.
	 *
	 * @param node address of the tree node containing information about the key.
	 * @param data data buffer to load from a specific key.
	 * @param dataMaxSize a size of data buffer to load.
	 * @param outSize an actual size of read data.
	 * @return true if key has been loaded successfully.
	 * @return false an error occurred.
	 */
	PSErrorCode NonSecureLoad(PersistentStorageNode *node, void *data, size_t dataMaxSize, size_t &outSize);

	/**
	 * @brief Check if given key entry exists in the persistent storage.
	 *
	 * @param node address of the tree node containing information about the key.
	 * @return true if key entry exists.
	 * @return false if key entry does not exist.
	 */
	PSErrorCode NonSecureHasEntry(PersistentStorageNode *node);

	/**
	 * @brief Remove given key entry from the persistent storage.
	 *
	 * @param node address of the tree node containing information about the key.
	 * @return true if key entry has been removed successfully.
	 * @return false an error occurred.
	 */
	PSErrorCode NonSecureRemove(PersistentStorageNode *node);

	/* Secure storage API counterparts.*/
	PSErrorCode SecureInit();
	PSErrorCode SecureStore(PersistentStorageNode *node, const void *data, size_t dataSize);
	PSErrorCode SecureLoad(PersistentStorageNode *node, void *data, size_t dataMaxSize, size_t &outSize);
	PSErrorCode SecureHasEntry(PersistentStorageNode *node);
	PSErrorCode SecureRemove(PersistentStorageNode *node);

protected:
	PersistentStorage() = default;
	~PersistentStorage() = default;

	PersistentStorage(const PersistentStorage &) = delete;
	PersistentStorage(PersistentStorage &&) = delete;
	PersistentStorage &operator=(const PersistentStorage &) = delete;
	PersistentStorage &operator=(PersistentStorage &&) = delete;

private:
	PersistentStorageImpl *Impl();
};

/* This should be defined by the implementation class. */
extern PersistentStorage &GetPersistentStorage();

} /* namespace Nrf */

#include "persistent_storage_impl.h"
namespace Nrf
{

inline PersistentStorageImpl *PersistentStorage::Impl()
{
	return static_cast<PersistentStorageImpl *>(this);
}

/* Non secure storage API. */
inline PSErrorCode PersistentStorage::NonSecureInit()
{
	return Impl()->_NonSecureInit();
};

inline PSErrorCode PersistentStorage::NonSecureStore(PersistentStorageNode *node, const void *data, size_t dataSize)
{
	return Impl()->_NonSecureStore(node, data, dataSize);
}

inline PSErrorCode PersistentStorage::NonSecureLoad(PersistentStorageNode *node, void *data, size_t dataMaxSize,
						    size_t &outSize)
{
	return Impl()->_NonSecureLoad(node, data, dataMaxSize, outSize);
}

inline PSErrorCode PersistentStorage::NonSecureHasEntry(PersistentStorageNode *node)
{
	return Impl()->_NonSecureHasEntry(node);
}

inline PSErrorCode PersistentStorage::NonSecureRemove(PersistentStorageNode *node)
{
	return Impl()->_NonSecureRemove(node);
}

/* Secure storage API. */
inline PSErrorCode PersistentStorage::SecureInit()
{
	return Impl()->_SecureInit();
};

inline PSErrorCode PersistentStorage::SecureStore(PersistentStorageNode *node, const void *data, size_t dataSize)
{
	return Impl()->_SecureStore(node, data, dataSize);
}

inline PSErrorCode PersistentStorage::SecureLoad(PersistentStorageNode *node, void *data, size_t dataMaxSize,
						 size_t &outSize)
{
	return Impl()->_SecureLoad(node, data, dataMaxSize, outSize);
}

inline PSErrorCode PersistentStorage::SecureHasEntry(PersistentStorageNode *node)
{
	return Impl()->_SecureHasEntry(node);
}

inline PSErrorCode PersistentStorage::SecureRemove(PersistentStorageNode *node)
{
	return Impl()->_SecureRemove(node);
}

} /* namespace Nrf */
