/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/**
 * @file
 * @brief Additional Unity support code for the native_posix board.
 */
#include <zephyr.h>
#include "posix_board_if.h"

int suiteTearDown(int num_failures)
{
	/* Sanitycheck bases the result of native_posix based unit tests on the
	 * output:
	 */
	printk("PROJECT EXECUTION %s\n",
	       num_failures == 0 ? "SUCCESSFUL" : "FAILED");

	/* The native posix board will loop forever after leaving the runner's
	 * main, so we have to explicitly call exit() to terminate the test.
	 */
	posix_exit(num_failures > 0 ? 1 : 0);

	/* Should be unreachable: */
	return 1;
}
