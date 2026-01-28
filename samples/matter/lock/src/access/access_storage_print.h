/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "access_storage.h"

#include <cstddef>

/**
 * @brief Print a log message that access data has been stored.
 *
 * Additionally, print the current storage free space.
 *
 * @param type Type of stored data.
 * @param dataSize Size of the stored data in bytes.
 * @param success Whether the storage operation was successful.
 */
void PrintAccessDataStored(AccessStorage::Type type, size_t dataSize, bool success);
