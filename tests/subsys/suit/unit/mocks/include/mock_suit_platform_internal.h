/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MOCK_SUIT_PLATFORM_INTERNAL_H__
#define MOCK_SUIT_PLATFORM_INTERNAL_H__

#include <zephyr/fff.h>
#include <mock_globals.h>

#include <suit_platform_internal.h>


FAKE_VALUE_FUNC(int, suit_plat_component_id_get, suit_component_t, struct zcbor_string **);
FAKE_VALUE_FUNC(int, suit_plat_component_impl_data_set, suit_component_t, void *);
FAKE_VALUE_FUNC(int, suit_plat_component_impl_data_get, suit_component_t, void **);

static inline void mock_suit_platform_internal_reset(void)
{
	RESET_FAKE(suit_plat_component_id_get);
	RESET_FAKE(suit_plat_component_impl_data_set);
	RESET_FAKE(suit_plat_component_impl_data_get);
}
#endif /* MOCK_SUIT_PLATFORM_INTERNAL_H__ */
