/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "settings_utils.h"
#include <string.h>

namespace Nrf
{

namespace
{
	/* Random magic bytes to represent an empty value.
	   It is needed because Zephyr settings subsystem does not distinguish an empty value from no value. */
	constexpr uint8_t kEmptyValue[] = { 0x22, 0xa6, 0x54, 0xd1, 0x39 };
	constexpr size_t kEmptyValueSize = sizeof(kEmptyValue);

} // namespace

int SettingsLoadEntryCallback(const char *name, size_t entrySize, settings_read_cb readCb, void *cbArg, void *param)
{
	SettingsReadEntry &entry = *static_cast<SettingsReadEntry *>(param);

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
} /* namespace Nrf */
