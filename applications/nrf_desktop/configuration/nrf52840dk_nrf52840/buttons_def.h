/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include "gpio_pins.h"

/* This configuration file is included only once from button module and holds
 * information about pins forming keyboard matrix.
 */

/* This structure enforces the header file is included only once in the build.
 * Violating this requirement triggers a multiple definition error at link time.
 */
const struct {} buttons_def_include_once;

static const struct gpio_pin col[] = {};

static const struct gpio_pin row[] = {
	{ .port = 0, .pin = DT_ALIAS_SW0_GPIOS_PIN },
	{ .port = 0, .pin = DT_ALIAS_SW1_GPIOS_PIN },
	{ .port = 0, .pin = DT_ALIAS_SW2_GPIOS_PIN },
	{ .port = 0, .pin = DT_ALIAS_SW3_GPIOS_PIN },
};
