/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include <cstdint>
#include <lib/core/CHIPError.h>
#include <string>

/**
 * Specialization for a persistent uint16_t that:
 * - Rounds to {0,2000,4000,6000,8000,10000}
 */
class PersistentPercent {
public:
	explicit PersistentPercent(const char *key, uint16_t defaultValue = 0) : mKey(key), mValue(defaultValue) {}

	uint16_t Get() const { return mValue; }
	CHIP_ERROR Set(uint16_t newValue, bool forceStore = false);

	CHIP_ERROR Load();

private:
	static constexpr uint16_t allowed[] = { 0, 2000, 4000, 6000, 8000, 10000 };
	uint16_t RoundToStep(uint16_t v);
	CHIP_ERROR Store(uint16_t val);

	std::string mKey;
	uint16_t mValue;
	uint16_t mStored;
};
