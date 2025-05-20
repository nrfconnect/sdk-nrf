/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MOCK_SUIT_IPUC_SDFW_H__
#define MOCK_SUIT_IPUC_SDFW_H__

#include <zephyr/fff.h>
#include <mock_globals.h>
#include <suit_metadata.h>

FAKE_VALUE_FUNC(int, suit_ipuc_sdfw_declare, suit_component_t, suit_manifest_role_t);
FAKE_VALUE_FUNC(int, suit_ipuc_sdfw_revoke, suit_component_t);

static inline void mock_suit_plat_ipuc_reset(void)
{
	RESET_FAKE(suit_ipuc_sdfw_declare);
	RESET_FAKE(suit_ipuc_sdfw_revoke);
}

#endif /* MOCK_SUIT_IPUC_SDFW_H__ */
