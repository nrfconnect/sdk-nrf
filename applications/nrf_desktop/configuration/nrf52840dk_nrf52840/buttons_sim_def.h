/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <caf/key_id.h>

/* This configuration file is included only once from buttons_sim module
 * and holds information about generated button presses sequence.
 */

/* This structure enforces the header file is included only once in the build.
 * Violating this requirement triggers a multiple definition error at link time.
 */
const struct {} buttons_sim_def_include_once;

const static uint16_t simulated_key_sequence[] = {
	KEY_ID(0x00, 0x11), /* N */
	KEY_ID(0x00, 0x12), /* O */
	KEY_ID(0x00, 0x15), /* R */
	KEY_ID(0x00, 0x07), /* D */
	KEY_ID(0x00, 0x0C), /* I */
	KEY_ID(0x00, 0x06), /* C */
	KEY_ID(0x00, 0x2C), /* spacebar */
};
