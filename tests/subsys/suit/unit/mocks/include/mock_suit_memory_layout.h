/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */


#ifndef MOCK_SUIT_MEMORY_LAYOUT_H__
#define MOCK_SUIT_MEMORY_LAYOUT_H__

#include <zephyr/fff.h>
#include <mock_globals.h>
#include <suit_memory_layout.h>

FAKE_VALUE_FUNC(bool, suit_memory_global_address_is_in_external_memory, uintptr_t);

static inline void mock_suit_memory_layout_reset(void)
{
	RESET_FAKE(suit_memory_global_address_is_in_external_memory);
}

#endif /* MOCK_SUIT_MEMORY_LAYOUT_H__ */
