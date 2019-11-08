/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/**
 * @file
 * @brief Additional Unity support code for the native_posix board.
 */

#include "posix_board_if.h"

int suiteTearDown(int num_failures)
{
	/* The native posix board will loop forever after leaving the runner's
	 * main, so we have to explicitly call exit() to terminate the test.
	 */
	posix_exit(num_failures > 0 ? -1 : 0);

	/* Should be unreachable: */
	return 1;
}
