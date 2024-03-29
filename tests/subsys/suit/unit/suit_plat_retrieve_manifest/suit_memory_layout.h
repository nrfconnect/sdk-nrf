/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/* @brief File with copies of selected function headers
 * to satisfy the compiler. The real header includes unneeded bloat from
 * zephyr/device.h
 */

#ifndef SUIT_MEMORY_LAYOUT_H__
#define SUIT_MEMORY_LAYOUT_H__

#include <stdbool.h>

bool suit_memory_global_address_is_in_external_memory(uintptr_t address);

#endif /* SUIT_MEMORY_LAYOUT_H__ */
