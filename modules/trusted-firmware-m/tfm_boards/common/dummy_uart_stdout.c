/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "uart_stdout.h"
#include <stdbool.h>

bool stdio_is_initialized(void)
{
	return false;
}

int stdio_output_string(const char *str, uint32_t len)
{
	return 0;
}

void stdio_init(void)
{
}

void stdio_uninit(void)
{
}
