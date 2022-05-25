/*
 * Copyright (c) 2021 - 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef __RTE_DEVICE_H
#define __RTE_DEVICE_H

#include <autoconf.h>

/* Configuration settings for Driver_USART0. */
#define   RTE_USART0                    1
/* Pin Selection (0xFFFFFFFF means Disconnected) */
#define   RTE_USART0_TXD_PIN            CONFIG_TFM_UART0_TXD_PIN
#define   RTE_USART0_RXD_PIN            CONFIG_TFM_UART0_RXD_PIN
#define   RTE_USART0_RTS_PIN            CONFIG_TFM_UART0_RTS_PIN
#define   RTE_USART0_CTS_PIN            CONFIG_TFM_UART0_CTS_PIN

/* Configuration settings for Driver_USART1. */
#define   RTE_USART1                    CONFIG_TFM_SECURE_UART1
/* Pin Selection (0xFFFFFFFF means Disconnected) */
#if defined(CONFIG_TFM_SECURE_UART1)
#define   RTE_USART1_TXD_PIN            CONFIG_TFM_UART1_TXD_PIN
#define   RTE_USART1_RXD_PIN            CONFIG_TFM_UART1_RXD_PIN
#else
#define   RTE_USART1_TXD_PIN            0xFFFFFFFF
#define   RTE_USART1_RXD_PIN            0xFFFFFFFF
#endif
#define   RTE_USART1_RTS_PIN            0xFFFFFFFF
#define   RTE_USART1_CTS_PIN            0xFFFFFFFF

/* Configuration settings for Driver_FLASH0. */
#define   RTE_FLASH0                    1

#endif  /* __RTE_DEVICE_H */
