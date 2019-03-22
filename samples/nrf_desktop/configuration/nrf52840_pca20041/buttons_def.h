/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include "buttons.h"

/* This file must be included only once */
const struct {} buttons_def_include_once;

static const char * const port_map[] = {DT_GPIO_P0_DEV_NAME};

static const struct button col[] = {
	{ .port = 0, .pin = 2 },
	{ .port = 0, .pin = 21 },
	{ .port = 0, .pin = 20 },
	{ .port = 0, .pin = 19 },
};

static const struct button row[] = {
	{ .port = 0, .pin = 29 },
	{ .port = 0, .pin = 31 },
	{ .port = 0, .pin = 22 },
	{ .port = 0, .pin = 24 },
};
