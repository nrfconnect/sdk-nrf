/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#ifndef _BUTTONS_H_
#define _BUTTONS_H_

struct button {
	u8_t port;
	u8_t pin;
};

#if CONFIG_BOARD_NRF52840_PCA20041
static const char * const port_map[] = {DT_GPIO_P0_DEV_NAME};

const struct button col[] = {
	{ .port = 0, .pin = 2 },
	{ .port = 0, .pin = 21 },
	{ .port = 0, .pin = 20 },
	{ .port = 0, .pin = 19 },
};

const struct button row[] = {
	{ .port = 0, .pin = 29 },
	{ .port = 0, .pin = 31 },
	{ .port = 0, .pin = 22 },
	{ .port = 0, .pin = 24 },
};
#elif CONFIG_BOARD_NRF52840_PCA10059
static const char * const port_map[] = {DT_GPIO_P1_DEV_NAME};

const struct button col[] = {};
const struct button row[] = {
	{
		.port = 0,
		.pin = SW0_GPIO_PIN,
	},
};
#else
#error "Buttons not supported on this platform"
#endif

#endif /* _BUTTONS_H_ */
