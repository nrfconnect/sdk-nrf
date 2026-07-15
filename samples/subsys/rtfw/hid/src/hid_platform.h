/*
 * Copyright (c) 2026 Nordic Semiconductor
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef RTFW_HID_PLATFORM_H_
#define RTFW_HID_PLATFORM_H_

#include <zephyr/devicetree.h>
#include <zephyr/dt-bindings/gpio/gpio.h>
#include <zephyr/irq.h>
#include <soc.h>
#include <hal/nrf_gpiote.h>

#define RTFW_GPIOTE_NODE DT_ALIAS(rt_gpiote)
#define RTFW_USER_NODE   DT_PATH(zephyr_user)

BUILD_ASSERT(DT_NODE_EXISTS(RTFW_GPIOTE_NODE),
	     "HID backend requires the rt-gpiote alias");
BUILD_ASSERT(CONFIG_SAMPLE_RTFW_IRQ_PRIORITY < ZERO_LATENCY_LEVELS,
	     "RTFW priority must be inside the zero-latency priority range");
BUILD_ASSERT(!IS_ENABLED(CONFIG_PM),
	     "HID sample does not support system PM");
#if defined(CONFIG_SOC_SERIES_NRF54L)
BUILD_ASSERT(CONFIG_SAMPLE_RTFW_IRQ_PRIORITY == 1,
	     "nRF54L RTFW must leave priority 0 to MPSL");
#else
BUILD_ASSERT(CONFIG_SAMPLE_RTFW_IRQ_PRIORITY == 0,
	     "nRF54H20 app-core RTFW must use priority 0");
#endif

#define RTFW_GPIOTE      ((NRF_GPIOTE_Type *)DT_REG_ADDR(RTFW_GPIOTE_NODE))
#define RTFW_GPIOTE_IRQN DT_IRQN(RTFW_GPIOTE_NODE)
#define RTFW_INPUT_ABS   NRF_DT_GPIOS_TO_PSEL(RTFW_USER_NODE, rt_input_gpios)
#define RTFW_INPUT_FLAGS DT_GPIO_FLAGS(RTFW_USER_NODE, rt_input_gpios)
#define RTFW_INPUT_ACTIVE_LOW \
	((RTFW_INPUT_FLAGS & GPIO_ACTIVE_LOW) != 0U)
#define RTFW_INPUT_PULL                                                    \
	((RTFW_INPUT_FLAGS & GPIO_PULL_UP) != 0U ? NRF_GPIO_PIN_PULLUP :   \
	 (RTFW_INPUT_FLAGS & GPIO_PULL_DOWN) != 0U ? NRF_GPIO_PIN_PULLDOWN : \
						     NRF_GPIO_PIN_NOPULL)

BUILD_ASSERT((RTFW_INPUT_FLAGS & (GPIO_PULL_UP | GPIO_PULL_DOWN)) !=
		     (GPIO_PULL_UP | GPIO_PULL_DOWN),
	     "rt-input-gpios cannot request both pull-up and pull-down");

#if defined(CONFIG_SAMPLE_RTFW_DEBUG_PIN)
#define RTFW_DEBUG_GPIO_ABS NRF_DT_GPIOS_TO_PSEL(RTFW_USER_NODE, rt_debug_gpios)
#endif

#endif /* RTFW_HID_PLATFORM_H_ */
