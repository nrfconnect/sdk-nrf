/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once
#include <zephyr/settings/settings.h>

namespace Nrf
{

struct SettingsReadEntry {
	void *destination;
	size_t destinationBufferSize;
	size_t readSize;
	bool result;
};

int SettingsLoadEntryCallback(const char *name, size_t entrySize, settings_read_cb readCb, void *cbArg, void *param);

} /* namespace Nrf */
