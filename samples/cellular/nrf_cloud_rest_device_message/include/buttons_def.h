/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <caf/gpio_pins.h>

/* This file is included by the common application framework (CAF) library */

/* This configuration file is included only once from buttons module and holds
 * information about pins forming the keyboard matrix.
 */

/* This structure enforces the header file to be included only once in the build.
 * Violating this requirement triggers a multiple definition error at link time.
 */
const struct {} buttons_def_include_once;

static const struct gpio_pin col[] = {};

static const struct gpio_pin row[] = {
	{ .port = 0, .pin = DT_GPIO_PIN(DT_NODELABEL(button0), gpios) },
	{ .port = 0, .pin = DT_GPIO_PIN(DT_NODELABEL(button1), gpios) },
};
