/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/* This file must be included only once */
const struct {} buttons_def_include_once;

static const char * const port_map[] = {DT_GPIO_P1_DEV_NAME};

static const struct button col[] = {};

static const struct button row[] = {
	{ .port = 0, .pin = SW0_GPIO_PIN },
};
