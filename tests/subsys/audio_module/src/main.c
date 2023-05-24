/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/fff.h>
#include <zephyr/ztest.h>
#include <errno.h>

#include "fakes.h"

/* This function runs before each test */
static void run_before(void *fixture)
{
	ARG_UNUSED(fixture);

	/* Register resets */
	DO_FOREACH_FAKE(RESET_FAKE);

	/* Reset common FFF internal structures */
	FFF_RESET_HISTORY();
}

ZTEST_SUITE(suite_audio_module_bad_param, NULL, NULL, NULL, NULL, NULL);
ZTEST_SUITE(suite_audio_module_functional, NULL, NULL, run_before, NULL, NULL);
