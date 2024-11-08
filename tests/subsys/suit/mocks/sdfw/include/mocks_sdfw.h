/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MOCKS_SDFW_H__
#define MOCKS_SDFW_H__

#include <zephyr/fff.h>
#include <mock_globals.h>

#ifdef CONFIG_MOCK_SDFW_ARBITER
#include <mock_sdfw_arbiter.h>
#endif /* CONFIG_MOCK_SDFW_ARBITER */

#ifdef CONFIG_MOCK_SDFW_RESET_MGR
#include <mock_sdfw_reset_mgr.h>
#endif /* CONFIG_MOCK_SDFW_RESET_MGR */

#ifdef CONFIG_MOCK_SDFW_VPRS
#include <mock_sdfw_vprs.h>
#endif /* CONFIG_MOCK_SDFW_VPRS */

static inline void mocks_sdfw_reset(void)
{
#ifdef CONFIG_MOCK_SDFW_ARBITER
	mock_sdfw_arbiter_reset();
#endif /* CONFIG_MOCK_SDFW_ARBITER */

#ifdef CONFIG_MOCK_SDFW_RESET_MGR
	mock_sdfw_reset_mgr_reset();
#endif /* CONFIG_MOCK_SDFW_RESET_MGR */

#ifdef CONFIG_MOCK_SDFW_VPRS
	mock_sdfw_vprs_reset();
#endif /* CONFIG_MOCK_SDFW_VPRS */
}
#endif /* MOCKS_SDFW_H__ */
