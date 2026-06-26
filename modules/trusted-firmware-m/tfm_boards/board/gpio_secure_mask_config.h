/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DTS_TO_TFM_CONFIG_H__
#define DTS_TO_TFM_CONFIG_H__

#include <zephyr/autoconf.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/util.h>
#include <nrfx.h>
#include <tfm-pinctrl.h>

#define DT_GPIO_PORT_NODE(port) DT_NODELABEL(gpio##port)
#define DT_GPIO_PORT_EXISTS(port)                                                                  \
	(DT_NODE_EXISTS(DT_GPIO_PORT_NODE(port)) &&                                                \
	 DT_NODE_HAS_STATUS(DT_GPIO_PORT_NODE(port), okay))

/*
 * Derive a GPIO port secure-pin bitmask from a peripheral's DTS pinctrl
 * definition.
 *
 * DTS path traversed:
 *   <peripheral><N>                      (peripheral node, e.g. uart<N>, spi<N>)
 *     pinctrl-0                          (default pinctrl state, index 0)
 *       group1, group2, ...              (pinctrl group child nodes)
 *         psels = < NRF_PSEL(...) ... >  (array of encoded port+pin+function values)
 *
 * Each NRF_PSEL value encodes port and pin as an absolute pin number:
 *   absolute_pin = port * 32 + pin_within_port
 *
 * The macros below represent each traversal level and accumulate a per-port
 * bitmask of pins used by the selected secure peripheral instances.
 */

/* One absolute pin number to one GPIO port bit contribution. */
#define TFM_PSEL_ABS_PIN_TO_GPIO_MASK(abs_pin, port)                                               \
	(((abs_pin) != NRF_PIN_DISCONNECTED && NRF_PIN_NUMBER_TO_PORT(abs_pin) == (port))          \
		 ? BIT(NRF_PIN_NUMBER_TO_PIN(abs_pin))                                             \
		 : 0U)

/* DT_FOREACH_PROP_ELEM callback: one psels[] entry to one mask contribution. */
#define TFM_PSEL_TO_GPIO_MASK(node_id, prop, idx, port)                                            \
	TFM_PSEL_ABS_PIN_TO_GPIO_MASK(NRF_GET_PIN(DT_PROP_BY_IDX(node_id, prop, idx)), port)

/* OR all psels[] entries within one pinctrl group node. */
#define TFM_PINCTRL_GROUP_TO_GPIO_MASK(node_id, port)                                              \
	DT_FOREACH_PROP_ELEM_SEP_VARGS(node_id, psels, TFM_PSEL_TO_GPIO_MASK, (|), port)

/* For nodes without pinctrl-0, contribute 0 instead of failing DT expansion. */
#define TFM_NODE_PINCTRL_GPIO_MASK(node_id, port)                                                  \
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, pinctrl_0),                                          \
		    (DT_FOREACH_CHILD_SEP_VARGS(DT_PINCTRL_BY_NAME(node_id, default, 0),           \
						TFM_PINCTRL_GROUP_TO_GPIO_MASK, (|), port)),       \
		    (0U))

/* OR all group nodes in a peripheral default pinctrl state. */
#define TFM_UART_GPIO_MASK(idx, port)                                                              \
	TFM_NODE_PINCTRL_GPIO_MASK(DT_NODELABEL(UTIL_CAT(uart, idx)), port)

#define TFM_SPIM_GPIO_MASK(idx, port)                                                              \
	TFM_NODE_PINCTRL_GPIO_MASK(DT_NODELABEL(UTIL_CAT(spi, idx)), port)

#if defined(CONFIG_TFM_SECURE_UART0)
#define TFM_SECURE_UART_IDX 0
#elif defined(CONFIG_TFM_SECURE_UART1)
#define TFM_SECURE_UART_IDX 1
#elif defined(CONFIG_TFM_SECURE_UART00)
#define TFM_SECURE_UART_IDX 00
#elif defined(CONFIG_TFM_SECURE_UART20)
#define TFM_SECURE_UART_IDX 20
#elif defined(CONFIG_TFM_SECURE_UART21)
#define TFM_SECURE_UART_IDX 21
#elif defined(CONFIG_TFM_SECURE_UART22)
#define TFM_SECURE_UART_IDX 22
#elif defined(CONFIG_TFM_SECURE_UART30)
#define TFM_SECURE_UART_IDX 30
#endif

#if defined(CONFIG_NRF_SPIM0_SECURE)
#define TFM_SECURE_SPIM_IDX 0
#elif defined(CONFIG_NRF_SPIM1_SECURE)
#define TFM_SECURE_SPIM_IDX 1
#elif defined(CONFIG_NRF_SPIM2_SECURE)
#define TFM_SECURE_SPIM_IDX 2
#elif defined(CONFIG_NRF_SPIM3_SECURE)
#define TFM_SECURE_SPIM_IDX 3
#elif defined(CONFIG_NRF_SPIM4_SECURE)
#define TFM_SECURE_SPIM_IDX 4
#elif defined(CONFIG_NRF_SPIM00_SECURE)
#define TFM_SECURE_SPIM_IDX 00
#elif defined(CONFIG_NRF_SPIM20_SECURE)
#define TFM_SECURE_SPIM_IDX 20
#elif defined(CONFIG_NRF_SPIM21_SECURE)
#define TFM_SECURE_SPIM_IDX 21
#elif defined(CONFIG_NRF_SPIM22_SECURE)
#define TFM_SECURE_SPIM_IDX 22
#elif defined(CONFIG_NRF_SPIM23_SECURE)
#define TFM_SECURE_SPIM_IDX 23
#elif defined(CONFIG_NRF_SPIM30_SECURE)
#define TFM_SECURE_SPIM_IDX 30
#endif

/* Get PIN mask for UART for all ports */
#if DT_GPIO_PORT_EXISTS(0) && defined(TFM_SECURE_UART_IDX)
#define TFM_SECURE_UART_GPIO0_PIN_MASK TFM_UART_GPIO_MASK(TFM_SECURE_UART_IDX, 0U)
#else
#define TFM_SECURE_UART_GPIO0_PIN_MASK 0U
#endif

#if DT_GPIO_PORT_EXISTS(1) && defined(TFM_SECURE_UART_IDX)
#define TFM_SECURE_UART_GPIO1_PIN_MASK TFM_UART_GPIO_MASK(TFM_SECURE_UART_IDX, 1U)
#else
#define TFM_SECURE_UART_GPIO1_PIN_MASK 0U
#endif

#if DT_GPIO_PORT_EXISTS(2) && defined(TFM_SECURE_UART_IDX)
#define TFM_SECURE_UART_GPIO2_PIN_MASK TFM_UART_GPIO_MASK(TFM_SECURE_UART_IDX, 2U)
#else
#define TFM_SECURE_UART_GPIO2_PIN_MASK 0U
#endif

/* Get PIN mask for SPIM for all ports */
#if DT_GPIO_PORT_EXISTS(0) && defined(TFM_SECURE_SPIM_IDX)
#define TFM_SECURE_SPIM_GPIO0_PIN_MASK TFM_SPIM_GPIO_MASK(TFM_SECURE_SPIM_IDX, 0U)
#else
#define TFM_SECURE_SPIM_GPIO0_PIN_MASK 0U
#endif

#if DT_GPIO_PORT_EXISTS(1) && defined(TFM_SECURE_SPIM_IDX)
#define TFM_SECURE_SPIM_GPIO1_PIN_MASK TFM_SPIM_GPIO_MASK(TFM_SECURE_SPIM_IDX, 1U)
#else
#define TFM_SECURE_SPIM_GPIO1_PIN_MASK 0U
#endif

#if DT_GPIO_PORT_EXISTS(2) && defined(TFM_SECURE_SPIM_IDX)
#define TFM_SECURE_SPIM_GPIO2_PIN_MASK TFM_SPIM_GPIO_MASK(TFM_SECURE_SPIM_IDX, 2U)
#else
#define TFM_SECURE_SPIM_GPIO2_PIN_MASK 0U
#endif

/* Get manual config for all ports */
#if DT_GPIO_PORT_EXISTS(0) && defined(CONFIG_NRF_GPIO0_PIN_MASK_SECURE)
#define TFM_SECURE_MANUAL_GPIO0_PIN_MASK CONFIG_NRF_GPIO0_PIN_MASK_SECURE
#else
#define TFM_SECURE_MANUAL_GPIO0_PIN_MASK 0U
#endif

#if DT_GPIO_PORT_EXISTS(1) && defined(CONFIG_NRF_GPIO1_PIN_MASK_SECURE)
#define TFM_SECURE_MANUAL_GPIO1_PIN_MASK CONFIG_NRF_GPIO1_PIN_MASK_SECURE
#else
#define TFM_SECURE_MANUAL_GPIO1_PIN_MASK 0U
#endif

#if DT_GPIO_PORT_EXISTS(2) && defined(CONFIG_NRF_GPIO2_PIN_MASK_SECURE)
#define TFM_SECURE_MANUAL_GPIO2_PIN_MASK CONFIG_NRF_GPIO2_PIN_MASK_SECURE
#else
#define TFM_SECURE_MANUAL_GPIO2_PIN_MASK 0U
#endif

#define TFM_SECURE_GPIO_MASK_PORT_0                                                                \
	(TFM_SECURE_MANUAL_GPIO0_PIN_MASK | TFM_SECURE_UART_GPIO0_PIN_MASK |                       \
	 TFM_SECURE_SPIM_GPIO0_PIN_MASK)

#define TFM_SECURE_GPIO_MASK_PORT_1                                                                \
	(TFM_SECURE_MANUAL_GPIO1_PIN_MASK | TFM_SECURE_UART_GPIO1_PIN_MASK |                       \
	 TFM_SECURE_SPIM_GPIO1_PIN_MASK)

#define TFM_SECURE_GPIO_MASK_PORT_2                                                                \
	(TFM_SECURE_MANUAL_GPIO2_PIN_MASK | TFM_SECURE_UART_GPIO2_PIN_MASK |                       \
	 TFM_SECURE_SPIM_GPIO2_PIN_MASK)

#endif /* DTS_TO_TFM_CONFIG_H__ */
