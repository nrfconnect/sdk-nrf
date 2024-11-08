/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MOCK_SDFW_VPRS_H__
#define MOCK_SDFW_VPRS_H__

#include <zephyr/fff.h>
#include <mock_globals.h>

#include <sdfw/vprs.h>

FAKE_VALUE_FUNC(int, vprs_sysctrl_start, uintptr_t);

static inline void mock_sdfw_vprs_reset(void)
{
	RESET_FAKE(vprs_sysctrl_start);
}

#endif /* MOCK_SDFW_VPRS_H__ */
