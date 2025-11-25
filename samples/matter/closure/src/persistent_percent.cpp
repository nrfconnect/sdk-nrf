/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "persistent_percent.h"

#include <cstdlib>
#include <cstring>
#include <persistent_storage/persistent_storage.h>
#include <platform/CHIPDeviceLayer.h>

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(persistent_percent, CONFIG_CHIP_APP_LOG_LEVEL);

using namespace chip;
using namespace chip::DeviceLayer;

CHIP_ERROR PersistentPercent::Set(uint16_t newValue, bool forceStore)
{
	mValue = newValue;
	uint16_t rounded = RoundToStep(newValue);

	if (forceStore) {
		ReturnErrorOnFailure(Store(mValue));
	} else if (rounded != mStored) {
		ReturnErrorOnFailure(Store(rounded));
	}
	return CHIP_NO_ERROR;
}

CHIP_ERROR PersistentPercent::Store(uint16_t val)
{
	Nrf::PersistentStorageNode node{ mKey.c_str(), mKey.size() + 1 };
	auto err = Nrf::GetPersistentStorage().NonSecureStore(&node, &val, sizeof(val));

	if (err == Nrf::PSErrorCode::Success) {
		mStored = val;
		return CHIP_NO_ERROR;
	}
	return CHIP_ERROR_PERSISTED_STORAGE_FAILED;
}

CHIP_ERROR PersistentPercent::Load()
{
	Nrf::PersistentStorageNode node{ mKey.c_str(), mKey.size() + 1 };
	size_t outSize = 0;
	auto err = Nrf::GetPersistentStorage().NonSecureLoad(&node, &mValue, sizeof(mValue), outSize);

	if (err == Nrf::PSErrorCode::Success) {
		mStored = mValue;
		return CHIP_NO_ERROR;
	}
	// Not found, store default
	return Store(mValue);
}

uint16_t PersistentPercent::RoundToStep(uint16_t v)
{
	uint16_t best = allowed[0];
	uint16_t bestDiff = std::abs((int)allowed[0] - (int)v);

	for (uint16_t a : allowed) {
		uint16_t diff = std::abs((int)a - (int)v);
		if (diff < bestDiff) {
			best = a;
			bestDiff = diff;
		}
	}
	return best;
}
