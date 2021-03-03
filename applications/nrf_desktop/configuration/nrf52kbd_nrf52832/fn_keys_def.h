/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <caf/key_id.h>

/* This configuration file is included only once from fn_keys module and holds
 * information about key with alternate functions.
 */

/* This structure enforces the header file is included only once in the build.
 * Violating this requirement triggers a multiple definition error at link time.
 */
const struct {} fn_key_def_include_once;

static const uint16_t fn_keys[] = {
	KEY_ID(0x01, 0x03), /* f3 -> e-mail */
	KEY_ID(0x03, 0x01), /* esc -> fn lock */
	KEY_ID(0x03, 0x03), /* f4 -> calculator */
	KEY_ID(0x03, 0x0A), /* f11 -> play/pause */
	KEY_ID(0x05, 0x0A), /* f12 -> next */
	KEY_ID(0x06, 0x02), /* f1 -> sleep */
	KEY_ID(0x06, 0x03), /* f2 -> internet */
	KEY_ID(0x06, 0x0A), /* f9 -> find */
	KEY_ID(0x06, 0x0C), /* insert -> prt scr */
	KEY_ID(0x06, 0x0D), /* page up -> scroll lock */
	KEY_ID(0x06, 0x0E), /* home -> pause break */
	KEY_ID(0x07, 0x0A), /* f10 -> previous */
	KEY_ID(0x07, 0x0D), /* page down -> peer control */
};
