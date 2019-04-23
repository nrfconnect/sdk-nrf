/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include "buttons.h"

/* This file must be included only once */
const struct {} buttons_def_include_once;

static const char * const port_map[] = {
	DT_GPIO_P0_DEV_NAME,
};

static const struct button row[] = {
	{ .port = 0, .pin = 17 },
	{ .port = 0, .pin = 11 },
	{ .port = 0, .pin = 9  },
	{ .port = 0, .pin = 8  },
	{ .port = 0, .pin = 7  },
	{ .port = 0, .pin = 6  },
	{ .port = 0, .pin = 5  },
	{ .port = 0, .pin = 3  },
	{ .port = 0, .pin = 4  },
	{ .port = 0, .pin = 15 },
	{ .port = 0, .pin = 0  },
	{ .port = 0, .pin = 1  },
	{ .port = 0, .pin = 13 },
	{ .port = 0, .pin = 14 },
	{ .port = 0, .pin = 12 },
	{ .port = 0, .pin = 10 },
	{ .port = 0, .pin = 16 },
	{ .port = 0, .pin = 2  },
};

static const struct button col[] = {
	{ .port = 0, .pin = 31 },
	{ .port = 0, .pin = 24 },
	{ .port = 0, .pin = 23 },
	{ .port = 0, .pin = 22 },
	{ .port = 0, .pin = 20 },
	{ .port = 0, .pin = 21 },
	{ .port = 0, .pin = 19 },
	{ .port = 0, .pin = 18 },
};
