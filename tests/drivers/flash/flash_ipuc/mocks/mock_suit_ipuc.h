/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MOCK_SUIT_IPUC_H__
#define MOCK_SUIT_IPUC_H__

#include <suit_ipuc.h>
#include <zephyr/fff.h>

DEFINE_FFF_GLOBALS;

FAKE_VALUE_FUNC(int, suit_ipuc_get_count, size_t *);
FAKE_VALUE_FUNC(int, suit_ipuc_get_info, size_t, struct zcbor_string *, suit_manifest_role_t *);
FAKE_VALUE_FUNC(int, suit_ipuc_write_setup, struct zcbor_string *, struct zcbor_string *,
		struct zcbor_string *);
FAKE_VALUE_FUNC(int, suit_ipuc_write, struct zcbor_string *, size_t, uintptr_t, size_t, bool);

static inline void mock_suit_ipuc_reset(void)
{
	RESET_FAKE(suit_ipuc_get_count);
	RESET_FAKE(suit_ipuc_get_info);
	RESET_FAKE(suit_ipuc_write_setup);
	RESET_FAKE(suit_ipuc_write);
}

#endif /* MOCK_SUIT_IPUC_H__ */
