/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @brief Additional Unity support code for the native_posix board.
 */
#include <zephyr/kernel.h>
#ifdef CONFIG_BOARD_NATIVE_POSIX
#include "posix_board_if.h"
#endif

/** @brief A generic suite teardown which implements platform specific tear down. */
int generic_suiteTearDown(int num_failures)
{
	int ret = num_failures > 0 ? 1 : 0;
	/* Sanitycheck bases the result of native_posix based unit tests on the
	 * output:
	 */
	printk("PROJECT EXECUTION %s\n",
	       num_failures == 0 ? "SUCCESSFUL" : "FAILED");

#ifdef CONFIG_BOARD_NATIVE_POSIX
	/* The native posix board will loop forever after leaving the runner's
	 * main, so we have to explicitly call exit() to terminate the test.
	 */
	posix_exit(ret);

	/* Should be unreachable: */
	return 1;
#endif

	return ret;
}

__weak int test_suiteTearDown(int num_failures)
{
	return generic_suiteTearDown(num_failures);
}
