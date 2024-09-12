/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MOCK_H__
#define MOCK_H__

#include <zephyr/fff.h>
#include <mock_globals.h>

#ifdef CONFIG_MOCK_SDFW_ARBITER
#include <mock_sdfw_arbiter.h>
#endif /* CONFIG_MOCK_SDFW_ARBITER */

static inline void mocks_reset(void)
{
#ifdef CONFIG_MOCK_SDFW_ARBITER
	mock_sdfw_arbiter_reset();
#endif /* CONFIG_MOCK_SDFW_ARBITER */
}
#endif /* MOCK_H__ */
