/*
 * Copyright (c) 2026 Nordic Semiconductor
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef RTFW_TIMER_PLATFORM_H_
#define RTFW_TIMER_PLATFORM_H_

#include <zephyr/devicetree.h>
#include <zephyr/dt-bindings/gpio/gpio.h>
#include <zephyr/irq.h>
#include <soc.h>
#include <hal/nrf_timer.h>

#define RTFW_TIMER_NODE DT_ALIAS(rt_timer)
#define RTFW_USER_NODE  DT_PATH(zephyr_user)

BUILD_ASSERT(DT_NODE_EXISTS(RTFW_TIMER_NODE),
	     "timer/GPIO backend requires the rt-timer alias");
BUILD_ASSERT(CONFIG_SAMPLE_RTFW_IRQ_PRIORITY < ZERO_LATENCY_LEVELS,
	     "RTFW priority must be inside the zero-latency priority range");
BUILD_ASSERT(!IS_ENABLED(CONFIG_PM),
	     "timer/GPIO sample does not support system PM");
#if defined(CONFIG_SOC_SERIES_NRF54L)
BUILD_ASSERT(CONFIG_SAMPLE_RTFW_IRQ_PRIORITY == 1,
	     "nRF54L RTFW must leave priority 0 to MPSL");
#else
BUILD_ASSERT(CONFIG_SAMPLE_RTFW_IRQ_PRIORITY == 0,
	     "nRF54H20 app-core RTFW must use priority 0");
#endif

#define RTFW_TIMER      ((NRF_TIMER_Type *)DT_REG_ADDR(RTFW_TIMER_NODE))
#define RTFW_TIMER_IRQN DT_IRQN(RTFW_TIMER_NODE)
#define RTFW_GPIO_ABS   NRF_DT_GPIOS_TO_PSEL(RTFW_USER_NODE, rt_gpios)

BUILD_ASSERT((DT_GPIO_FLAGS(RTFW_USER_NODE, rt_gpios) & GPIO_ACTIVE_LOW) == 0,
	     "rt-gpios must be active-high");

#if defined(CONFIG_SAMPLE_RTFW_DEBUG_PIN)
#define RTFW_DEBUG_GPIO_ABS NRF_DT_GPIOS_TO_PSEL(RTFW_USER_NODE, rt_debug_gpios)
BUILD_ASSERT((DT_GPIO_FLAGS(RTFW_USER_NODE, rt_debug_gpios) & GPIO_ACTIVE_LOW) == 0,
	     "rt-debug-gpios must be active-high");
#endif

#endif /* RTFW_TIMER_PLATFORM_H_ */
