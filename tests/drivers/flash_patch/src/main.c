/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <nrfx.h>


ZTEST(test_flash_patch, test_flash_patch1)
{
#ifdef CONFIG_DISABLE_FLASH_PATCH
	zassert_equal(NRF_UICR->DEBUGCTRL, 0xFFFF00FF,
		"Flash patch protection missing (0x%x).\n",
		NRF_UICR->DEBUGCTRL);
#else
	zassert_equal(NRF_UICR->DEBUGCTRL, 0xFFFFFFFF,
		"Flash patch protection wrongly applied (0x%x).\n",
		NRF_UICR->DEBUGCTRL);
#endif
}

ZTEST_SUITE(test_flash_patch, NULL, NULL, NULL, NULL, NULL);
