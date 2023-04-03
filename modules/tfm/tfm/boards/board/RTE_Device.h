/*
 * Copyright (c) 2021 - 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef __RTE_DEVICE_H
#define __RTE_DEVICE_H

#include <autoconf.h>

/* Configuration settings for Driver_USART0. */
#if defined(CONFIG_TFM_SECURE_UART0) || DOMAIN_NS == 1U

#define   RTE_USART0                    1
/* Pin Selection (0xFFFFFFFF means Disconnected) */
#define   RTE_USART0_TXD_PIN            CONFIG_TFM_UART0_TXD_PIN
#define   RTE_USART0_RXD_PIN            CONFIG_TFM_UART0_RXD_PIN
#define   RTE_USART0_RTS_PIN            CONFIG_TFM_UART0_RTS_PIN
#define   RTE_USART0_CTS_PIN            CONFIG_TFM_UART0_CTS_PIN

#if defined(CONFIG_TFM_UART0_HWFC_ENABLED)
#define   RTE_USART0_HWFC               NRF_UARTE_HWFC_ENABLED
#else
#define   RTE_USART0_HWFC               NRF_UARTE_HWFC_DISABLED
#endif

#endif /* defined(CONFIG_TFM_SECURE_UART0) || DOMAIN_NS == 1U */

/* Configuration settings for Driver_USART1. */
#if defined(CONFIG_TFM_SECURE_UART1)

#define   RTE_USART1                    1
/* Pin Selection (0xFFFFFFFF means Disconnected) */
#define   RTE_USART1_TXD_PIN            CONFIG_TFM_UART1_TXD_PIN
#define   RTE_USART1_RXD_PIN            CONFIG_TFM_UART1_RXD_PIN
#define   RTE_USART1_RTS_PIN            CONFIG_TFM_UART1_RTS_PIN
#define   RTE_USART1_CTS_PIN            CONFIG_TFM_UART1_CTS_PIN

#if defined(CONFIG_TFM_UART1_HWFC_ENABLED)
#define   RTE_USART1_HWFC               NRF_UARTE_HWFC_ENABLED
#else
#define   RTE_USART1_HWFC               NRF_UARTE_HWFC_DISABLED
#endif

#endif /* defined(CONFIG_TFM_SECURE_UART1) */

/* Configuration settings for Driver_FLASH0. */
#define   RTE_FLASH0                    1

#endif  /* __RTE_DEVICE_H */
