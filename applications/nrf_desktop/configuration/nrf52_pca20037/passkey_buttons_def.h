/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include "passkey_buttons.h"

/* This configuration file is included only once from passkey_buttons module
 * and holds information about buttons used to input passkey.
 */

/* This structure enforces the header file is included only once in the build.
 * Violating this requirement triggers a multiple definition error at link time.
 */
const struct {} passkey_buttons_def_include_once;

const static u16_t confirm_keys[] = {
	KEY_ID(0x04, 0x0A),   /* enter */
	KEY_ID(0x02, 0x0E),   /* keypad enter */
};

const static u16_t delete_keys[] = {
	KEY_ID(0x01, 0x0A),   /* backspace */
};

const static struct passkey_input_config input_configs[] = {
	{
		.key_ids = {
			KEY_ID(0x07, 0x08),   /* 0 */
			KEY_ID(0x07, 0x01),   /* 1 */
			KEY_ID(0x07, 0x02),   /* 2 */
			KEY_ID(0x07, 0x03),   /* 3 */
			KEY_ID(0x07, 0x04),   /* 4 */
			KEY_ID(0x06, 0x04),   /* 5 */
			KEY_ID(0x06, 0x05),   /* 6 */
			KEY_ID(0x07, 0x05),   /* 7 */
			KEY_ID(0x07, 0x06),   /* 8 */
			KEY_ID(0x07, 0x07),   /* 9 */
		},
	},
	{
		.key_ids = {
			KEY_ID(0x03, 0x0C),   /* keypad 0 */
			KEY_ID(0x02, 0x0B),   /* keypad 1 */
			KEY_ID(0x02, 0x0C),   /* keypad 2 */
			KEY_ID(0x02, 0x0D),   /* keypad 3 */
			KEY_ID(0x01, 0x0B),   /* keypad 4 */
			KEY_ID(0x01, 0x0C),   /* keypad 5 */
			KEY_ID(0x01, 0x0D),   /* keypad 6 */
			KEY_ID(0x00, 0x0B),   /* keypad 7 */
			KEY_ID(0x00, 0x0C),   /* keypad 8 */
			KEY_ID(0x00, 0x0D),   /* keypad 9 */
		},
	},
};
