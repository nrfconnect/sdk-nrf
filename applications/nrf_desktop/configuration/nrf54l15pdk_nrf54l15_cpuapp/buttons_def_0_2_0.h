/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
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
	{ .port = 1, .pin = DT_GPIO_PIN(DT_NODELABEL(button0), gpios) },
	{ .port = 1, .pin = DT_GPIO_PIN(DT_NODELABEL(button1), gpios) },
	/* GPIOs assigned to Button 3 and Button 4 on the PDK v0.2.1 do not support interrupts.
	 * Because of this, the GPIOs cannot be used by the CAF buttons module.
	 */
};
