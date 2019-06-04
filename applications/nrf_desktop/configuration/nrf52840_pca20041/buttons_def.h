/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include "buttons.h"

/* This configuration file is included only once from button module and holds
 * information about pins forming keyboard matric.
 */

/* This structure enforces the header file is included only once in the build.
 * Violating this requirement triggers a multiple definition error at link time.
 */
const struct {} buttons_def_include_once;

static const char * const port_map[] = {
	DT_GPIO_P0_DEV_NAME,
	DT_GPIO_P1_DEV_NAME
};

static const struct button col[] = {};

static const struct button row[] = {
	{ .port = 0, .pin = 2 },
	{ .port = 1, .pin = 15 },
	{ .port = 1, .pin = 14 },
	{ .port = 0, .pin = 31 },
	{ .port = 0, .pin = 29 },
	{ .port = 0, .pin = 24 },
	{ .port = 0, .pin = 22 },
	{ .port = 0, .pin = 4 },
};
