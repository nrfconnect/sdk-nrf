/*
 * Copyright (c) 2022-2024 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>

#ifdef CONFIG_TFM_PSA_TEST_NONE
#error "No PSA test suite set. See "Building and Running" in README."
#endif

int main(void)
{
	printk("Should not be printed, expected TF-M's NS application to be run instead.\n");
	k_panic();

	return 0;		/* unreachable */
}
