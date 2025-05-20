/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MOCK_SDFW_ARBITER_H__
#define MOCK_SDFW_ARBITER_H__

#include <zephyr/fff.h>
#include <mock_globals.h>

#include <sdfw/arbiter.h>

FAKE_VALUE_FUNC(int, arbiter_mem_access_check, const struct arbiter_mem_params_access *const);

static inline void mock_sdfw_arbiter_reset(void)
{
	RESET_FAKE(arbiter_mem_access_check);
}

#endif /* MOCK_SDFW_ARBITER_H__ */
