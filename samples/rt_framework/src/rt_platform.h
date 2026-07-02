/*
 * Copyright (c) 2026 Nordic Semiconductor
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * Platform binding for the RT component.
 *
 * The RT-owned resources (time base, signaling peripheral, GPIOs) are selected
 * from devicetree, not hard-coded, so porting the framework to another SoC is a
 * board-overlay change rather than a source change:
 *
 *   aliases {
 *       rt-timer = <&timerXX>;   // RT time base (status = "disabled" for Zephyr)
 *       rt-egu   = <&eguXX>;     // only needed for the data plane
 *   };
 *   zephyr,user {
 *       rt-gpios       = <&gpioX pin flags>;   // toggled by the RT ISR
 *       rt-debug-gpios = <&gpioX pin flags>;   // optional ISR-duration pin
 *   };
 *
 * The peripherals are accessed directly through the nrfx HAL, so we only take
 * the register base and IRQ line from devicetree. The nodes are kept out of
 * Zephyr's reach (status = "disabled" on nRF54L, or "reserved" on nRF54H20
 * where the app core owns a global instance) so no Zephyr driver claims them.
 */

#ifndef RT_PLATFORM_H_
#define RT_PLATFORM_H_

#include <zephyr/devicetree.h>
#include <zephyr/dt-bindings/gpio/gpio.h>
#include <soc.h>
#include <hal/nrf_timer.h>
#if defined(CONFIG_RT_FW_DATA_PLANE)
#include <hal/nrf_egu.h>
#endif

/* RT time base: chosen via the "rt-timer" devicetree alias. */
#define RT_TIMER_NODE DT_ALIAS(rt_timer)
BUILD_ASSERT(DT_NODE_EXISTS(RT_TIMER_NODE),
	     "rt_framework: define an 'rt-timer' alias in the board overlay");
#define RT_TIMER      ((NRF_TIMER_Type *)DT_REG_ADDR(RT_TIMER_NODE))
#define RT_TIMER_IRQN DT_IRQN(RT_TIMER_NODE)

/* RT-owned GPIOs, taken from the /zephyr,user node and resolved to an absolute
 * pin number usable with the GPIO HAL.
 */
#define RT_USER_NODE DT_PATH(zephyr_user)
#define RT_GPIO_ABS  NRF_DT_GPIOS_TO_PSEL(RT_USER_NODE, rt_gpios)
/* The RT ISR drives the pin level directly through the GPIO HAL, so only an
 * active-high output is honoured (the DT polarity/pull flags are not applied).
 */
BUILD_ASSERT((DT_GPIO_FLAGS(RT_USER_NODE, rt_gpios) & GPIO_ACTIVE_LOW) == 0,
	     "rt_framework: rt-gpios must be active-high (HAL path ignores flags)");
#if defined(CONFIG_RT_FW_DEBUG_PIN)
#define RT_DEBUG_ABS NRF_DT_GPIOS_TO_PSEL(RT_USER_NODE, rt_debug_gpios)
BUILD_ASSERT((DT_GPIO_FLAGS(RT_USER_NODE, rt_debug_gpios) & GPIO_ACTIVE_LOW) == 0,
	     "rt_framework: rt-debug-gpios must be active-high (HAL path ignores flags)");
#endif

#if defined(CONFIG_RT_FW_DATA_PLANE)
/* RT signaling peripheral for the data plane: the "rt-egu" alias. */
#define RT_EGU_NODE DT_ALIAS(rt_egu)
BUILD_ASSERT(DT_NODE_EXISTS(RT_EGU_NODE),
	     "rt_framework: define an 'rt-egu' alias when the data plane is enabled");
#define RT_EGU      ((NRF_EGU_Type *)DT_REG_ADDR(RT_EGU_NODE))
#define RT_EGU_IRQN DT_IRQN(RT_EGU_NODE)
#endif

#endif /* RT_PLATFORM_H_ */
