/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef EXAMPLE_TEST_H_
#define EXAMPLE_TEST_H_

#include <zephyr/sys/util.h>
#include <stdbool.h>

/* This header shows how to handle testing of compile time options in a single
 * binary. It is done by overriding the IS_ENABLED macro and redefining it to
 * use a runtime variable.
 *
 * Note that in some cases, it may not be possible to use that feature. That is
 * if const variables are initialized based on the IS_ENABLED result, like:
 *
 * const int x = IS_ENABLED(CONFIG_FOO) ? 1 : 0;
 */
#undef IS_ENABLED
#define IS_ENABLED(x) runtime_##x

extern bool runtime_CONFIG_UUT_PARAM_CHECK;

#endif /* EXAMPLE_TEST_H_ */
