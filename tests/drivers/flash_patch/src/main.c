/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <ztest.h>
#include <nrfx.h>


void test_flash_patch1(void)
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

void test_main(void)
{
	ztest_test_suite(test_flash_patch,
			 ztest_unit_test(test_flash_patch1)
	);
	ztest_run_test_suite(test_flash_patch);
}
