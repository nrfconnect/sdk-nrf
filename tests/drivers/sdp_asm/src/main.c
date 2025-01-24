/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <stdio.h>
#include "./add_1.h"
#include "./add_10.h"
#include "./add_100.h"

volatile uint8_t arg_uint8_t;
volatile uint16_t arg_uint16_t;
volatile uint32_t arg_uint32_t;

int main(void)
{
	printf("SDP ASM test on %s\n", CONFIG_BOARD_TARGET);
	printf("initial: %u, %u, %u\n", arg_uint8_t, arg_uint16_t, arg_uint32_t);
	hrt_add_1();
	printf("after add_1: %u, %u, %u\n", arg_uint8_t, arg_uint16_t, arg_uint32_t);
	hrt_add_10();
	printf("after add_10: %u, %u, %u\n", arg_uint8_t, arg_uint16_t, arg_uint32_t);
	hrt_add_100();
	printf("after add_100: %u, %u, %u\n", arg_uint8_t, arg_uint16_t, arg_uint32_t);
	printf("Test completed.\n");

	return 0;
}
