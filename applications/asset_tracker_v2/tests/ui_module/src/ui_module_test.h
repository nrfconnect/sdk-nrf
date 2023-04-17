/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef UI_MODULE_TEST_H_
#define UI_MODULE_TEST_H_

#include <zephyr/init.h>

/* Redefine SYS_INIT so that setup() is not run before the test runner starts. */
#undef SYS_INIT
#define SYS_INIT(_init_fn, _level, _prio)

/* Suppress linker that warns that setup() is not used. This is because the function is run
 * at SYS_INIT, which is now redefined.
 */
static int setup(void) __attribute__((unused));

#endif /* UI_MODULE_TEST */
