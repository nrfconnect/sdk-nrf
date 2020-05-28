/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
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
	{ .port = 0, .pin = DT_GPIO_PIN(DT_ALIAS(sw0), gpios) },
	{ .port = 0, .pin = DT_GPIO_PIN(DT_ALIAS(sw1), gpios) },
	{ .port = 0, .pin = DT_GPIO_PIN(DT_ALIAS(sw2), gpios) },
	{ .port = 0, .pin = DT_GPIO_PIN(DT_ALIAS(sw3), gpios) },
};
