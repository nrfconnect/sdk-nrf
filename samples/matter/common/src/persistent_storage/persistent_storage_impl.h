/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "persistent_storage.h"

#ifdef CONFIG_NCS_SAMPLE_MATTER_SETTINGS_STORAGE_BACKEND
#include "backends/persistent_storage_settings.h"
#endif

#ifdef CONFIG_NCS_SAMPLE_MATTER_SECURE_STORAGE_BACKEND
#include "backends/persistent_storage_secure.h"
#endif

namespace Nrf
{
class PersistentStorageImpl : public PersistentStorage
#ifdef CONFIG_NCS_SAMPLE_MATTER_SETTINGS_STORAGE_BACKEND
	,
			      public PersistentStorageSettings
#endif
#ifdef CONFIG_NCS_SAMPLE_MATTER_SECURE_STORAGE_BACKEND
	,
			      public PersistentStorageSecure
#endif
{
	friend class PersistentStorage;

#ifdef CONFIG_NCS_SAMPLE_MATTER_SETTINGS_STORAGE_BACKEND

	using PersistentStorageSettings::_NonSecureHasEntry;
	using PersistentStorageSettings::_NonSecureInit;
	using PersistentStorageSettings::_NonSecureLoad;
	using PersistentStorageSettings::_NonSecureRemove;
	using PersistentStorageSettings::_NonSecureStore;
#endif

#ifdef CONFIG_NCS_SAMPLE_MATTER_SECURE_STORAGE_BACKEND
	using PersistentStorageSecure::_SecureHasEntry;
	using PersistentStorageSecure::_SecureInit;
	using PersistentStorageSecure::_SecureLoad;
	using PersistentStorageSecure::_SecureRemove;
	using PersistentStorageSecure::_SecureStore;
#endif
};

/* Implementation of the generic PersistentStorage getter. */
inline PersistentStorage &GetPersistentStorage()
{
	static PersistentStorageImpl sInstance;
	return sInstance;
}

} /* namespace Nrf */
