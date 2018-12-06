/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include "debug.h"
#include <nrf.h>
#include <nrfx_uarte.h>
#include <misc/util.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#ifdef CONFIG_SOC_SERIES_NRF91X
#define UART NRF_UARTE0_S
#else
#define UART NRF_UARTE0
#endif

nrfx_uarte_t uart = NRFX_UARTE_INSTANCE(0);
static nrfx_uarte_config_t config;

#ifdef CONFIG_SOC_SERIES_NRF91X
#define DT_NORDIC_NRF_UART_UART_0_TX_PIN DT_NORDIC_NRF_UARTE_UART_0_TX_PIN
#define DT_NORDIC_NRF_UART_UART_0_RX_PIN DT_NORDIC_NRF_UARTE_UART_0_RX_PIN
#endif

void uart_init(void)
{
	config.pseltxd = DT_NORDIC_NRF_UART_UART_0_TX_PIN;
	config.pselrxd = DT_NORDIC_NRF_UART_UART_0_RX_PIN;
	config.pselcts = NRF_UARTE_PSEL_DISCONNECTED;
	config.pselrts = NRF_UARTE_PSEL_DISCONNECTED;
	config.p_context = NULL, config.hwfc = NRF_UARTE_HWFC_DISABLED;
	config.parity = 0;
	config.baudrate = UARTE_BAUDRATE_BAUDRATE_Baud115200;

#ifdef CONFIG_SOC_SERIES_NRF91X
	uart.p_reg = (NRF_UARTE_Type *)NRF_UARTE0_S_BASE;
#else
	uart.p_reg = (NRF_UARTE_Type *)NRF_UARTE0_BASE;
#endif
	uart.drv_inst_idx = NRFX_UARTE0_INST_IDX;
	nrfx_uarte_init(&uart, &config, NULL);
}

void uart_uninit(void)
{
	nrfx_uarte_uninit(&uart);
}

void uart_vprintf(const char *__fmt, va_list argptr)
{
	char _str[CONFIG_SB_DEBUG_PRINTF_BUF_LEN];
	int len = vsnprintf(_str, CONFIG_SB_DEBUG_PRINTF_BUF_LEN, __fmt, argptr);

	if (len > 0) {
		uart_print(_str, min(len, CONFIG_SB_DEBUG_PRINTF_BUF_LEN - 1));
	}
}


void uart_printf(const char *__fmt, ...)
{
	va_list argptr;

	va_start(argptr, __fmt);
	uart_vprintf(__fmt, argptr);
	va_end(argptr);
}


void uart_print(const char *str, size_t len)
{
	nrfx_uarte_tx(&uart, str, len);
}
