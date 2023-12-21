/*
 * Copyright (c) 2021 - 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef __RTE_DEVICE_H
#define __RTE_DEVICE_H

#include <autoconf.h>

/* ARRAY_SIZE causes a conflict as it is defined both by TF-M and indirectly by devicetree.h */
#undef ARRAY_SIZE
#include <zephyr/devicetree.h>

#include <nrf-pinctrl.h>

/* Configuration settings for Driver_FLASH.c and Driver_USART.c */

#define UART_PIN_INIT(node_id, prop, idx)		\
	DT_PROP_BY_IDX(node_id, prop, idx),

#define RTE_USART_PINS(idx)						\
{ \
	DT_FOREACH_CHILD_VARGS( \
		DT_PINCTRL_BY_NAME(DT_NODELABEL(uart##idx), default, 0), \
		DT_FOREACH_PROP_ELEM, psels, UART_PIN_INIT \
	) \
}

#define RTE_FLASH0 1

/*
 * The defines RTE_USART0, RTE_USART1, etc. determine if
 * Driver_USART.c instantiates UART instance 0, 1, etc..
 */

#if defined(CONFIG_TFM_SECURE_UART0) || DOMAIN_NS == 1U
#define RTE_USART0 1
#endif

#if defined(CONFIG_TFM_SECURE_UART1) && DOMAIN_NS != 1U
#define RTE_USART1 1
#endif

/*
 * Note that the defines RTE_USART0_PINS, RTE_USART1_PINS, etc. are
 * used by Driver_USART.c, but only when RTE_USART0, RTE_USART1
 * etc. are defined to 1.
 */
#define RTE_USART0_PINS RTE_USART_PINS(0)
#define RTE_USART1_PINS RTE_USART_PINS(1)

#endif  /* __RTE_DEVICE_H */
