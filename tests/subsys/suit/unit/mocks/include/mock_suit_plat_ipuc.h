/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MOCK_SUIT_PLAT_IPUC_H__
#define MOCK_SUIT_PLAT_IPUC_H__

#include <zephyr/fff.h>
#include <mock_globals.h>
#include <suit_plat_ipuc.h>

FAKE_VALUE_FUNC(int, suit_plat_ipuc_declare, suit_component_t);
FAKE_VALUE_FUNC(int, suit_plat_ipuc_revoke, suit_component_t);

static inline void mock_suit_plat_ipuc_reset(void)
{
	RESET_FAKE(suit_plat_ipuc_declare);
	RESET_FAKE(suit_plat_ipuc_revoke);
}

#endif /* MOCK_SUIT_PLAT_IPUC_H__ */
