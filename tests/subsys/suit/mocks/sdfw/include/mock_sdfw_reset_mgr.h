/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MOCK_SDFW_RESET_MGR_H__
#define MOCK_SDFW_RESET_MGR_H__

#include <zephyr/fff.h>
#include <mock_globals.h>

#include <sdfw/reset_mgr.h>

FAKE_VALUE_FUNC(int, reset_mgr_reset_domains_sync);
FAKE_VALUE_FUNC(int, reset_mgr_init_and_boot_processor, nrf_processor_t, uintptr_t, uintptr_t);

static inline void mock_sdfw_reset_mgr_reset(void)
{
	RESET_FAKE(reset_mgr_reset_domains_sync);
	RESET_FAKE(reset_mgr_init_and_boot_processor);
}

#endif /* MOCK_SDFW_RESET_MGR_H__ */
