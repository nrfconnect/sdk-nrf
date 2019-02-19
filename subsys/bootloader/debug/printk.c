/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <stdio.h>
#include <misc/util.h>
#include <debug.h>

void printk(const char *fmt, ...)
{
#ifdef CONFIG_SECURE_BOOT_DEBUG
	char _str[CONFIG_SB_DEBUG_PRINTF_BUF_LEN];
	va_list argptr;

	va_start(argptr, fmt);
	int len = vsnprintf(_str, CONFIG_SB_DEBUG_PRINTF_BUF_LEN, fmt, argptr);

	len = MIN(len, CONFIG_SB_DEBUG_PRINTF_BUF_LEN - 1);
	if (len > 0) {
#ifdef CONFIG_SB_DEBUG_PORT_SEGGER_RTT
		SEGGER_RTT_Write(0, _str, len);
#elif CONFIG_SB_DEBUG_PORT_UART
		uart_print(_str, len);
#endif
	}
	va_end(argptr);
#endif
}
