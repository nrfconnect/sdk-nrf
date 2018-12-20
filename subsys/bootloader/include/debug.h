/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef DEBUG_H_
#define DEBUG_H_

#include <stdbool.h>
#include <stdarg.h>
#include <misc/__assert.h>

#ifdef CONFIG_SECURE_BOOT_DEBUG
void printk(const char *fmt, ...);

#ifdef CONFIG_SB_DEBUG_PORT_SEGGER_RTT
#include <SEGGER_RTT.h>
#elif defined(CONFIG_SB_DEBUG_PORT_UART)
#include <debug/uart.h>
#endif /* CONFIG_SB_DEBUG_PORT_SEGGER_RTT */
#else
#define printk(...)
#endif

#endif /* DEBUG_H_ */
