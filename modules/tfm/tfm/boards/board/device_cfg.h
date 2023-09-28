/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DEVICE_CFG_H__
#define DEVICE_CFG_H__

#include <autoconf.h>

/* ARRAY_SIZE causes a conflict as it is defined both by TF-M and indirectly by devicetree.h */
#undef ARRAY_SIZE
#include <zephyr/devicetree.h>

#if defined(CONFIG_TFM_SECURE_UART0) || DOMAIN_NS == 1U
#define TFM_UART uart0
#endif /* defined(CONFIG_TFM_SECURE_UART0) || DOMAIN_NS == 1U */

#if defined(CONFIG_TFM_SECURE_UART1) && DOMAIN_NS != 1U
#define TFM_UART uart1
#endif /* defined(CONFIG_TFM_SECURE_UART1) */

#define DEFAULT_UART_BAUDRATE DT_PROP_OR(DT_NODELABEL(TFM_UART), current_speed, 115200)

#if DT_PROP(DT_NODELABEL(TFM_UART), hw_flow_control)
#define DEFAULT_UART_CONTROL ARM_USART_FLOW_CONTROL_RTS_CTS
#else
#define DEFAULT_UART_CONTROL 0
#endif

#endif /* DEVICE_CFG_H__ */
