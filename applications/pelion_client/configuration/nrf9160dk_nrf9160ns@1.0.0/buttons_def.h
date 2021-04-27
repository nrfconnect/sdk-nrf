/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <caf/gpio_pins.h>

/* This configuration file is included only once from button module and holds
 * information about pins forming keyboard matrix.
 */

/* This structure enforces the header file is included only once in the build.
 * Violating this requirement triggers a multiple definition error at link time.
 */
const struct {} buttons_def_include_once;

static const struct gpio_pin col[] = {};

static const struct gpio_pin row[] = {
	{ .port = 0, .pin = 6 },
	{ .port = 0, .pin = 7 },
	{ .port = 0, .pin = 8 },
	{ .port = 0, .pin = 9 },
};
