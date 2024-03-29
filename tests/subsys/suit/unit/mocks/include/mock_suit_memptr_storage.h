/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MOCK_SUIT_MEMPTR_STORAGE_H__
#define MOCK_SUIT_MEMPTR_STORAGE_H__

#include <zephyr/fff.h>
#include <mock_globals.h>

#include <suit_memptr_storage.h>

FAKE_VALUE_FUNC(int, suit_memptr_storage_get, memptr_storage_handle_t *);
FAKE_VALUE_FUNC(int, suit_memptr_storage_release, memptr_storage_handle_t);
FAKE_VALUE_FUNC(int, suit_memptr_storage_ptr_store, memptr_storage_handle_t, const uint8_t *,
		size_t);
FAKE_VALUE_FUNC(int, suit_memptr_storage_ptr_get, memptr_storage_handle_t, const uint8_t **,
		size_t *);

static inline void mock_suit_memptr_storage_reset(void)
{
	RESET_FAKE(suit_memptr_storage_get);
	RESET_FAKE(suit_memptr_storage_release);
	RESET_FAKE(suit_memptr_storage_ptr_store);
	RESET_FAKE(suit_memptr_storage_ptr_get);

	suit_memptr_storage_get_fake.return_val = SUIT_PLAT_SUCCESS;
	suit_memptr_storage_release_fake.return_val = SUIT_PLAT_SUCCESS;
	suit_memptr_storage_ptr_store_fake.return_val = SUIT_PLAT_SUCCESS;
	suit_memptr_storage_ptr_get_fake.return_val = SUIT_PLAT_SUCCESS;
}

#endif /* MOCK_SUIT_MEMPTR_STORAGE_H__ */
