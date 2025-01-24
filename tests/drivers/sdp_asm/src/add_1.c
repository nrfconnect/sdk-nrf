/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdint.h>

extern volatile uint8_t arg_uint8_t;
extern volatile uint16_t arg_uint16_t;
extern volatile uint32_t arg_uint32_t;

void hrt_add_1(void)
{
	arg_uint8_t += 2;
	arg_uint16_t += 3;
	arg_uint32_t += 4;
}
