/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdint.h>
#include "cs_antenna_switch.h"

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/pm/device_runtime.h>

#include <hal/nrf_gpio.h>

#if DT_NODE_EXISTS(DT_NODELABEL(cs_antenna_switch))
#define ANTENNA_SWITCH_NODE DT_NODELABEL(cs_antenna_switch)
#else
#define ANTENNA_SWITCH_NODE DT_INVALID_NODE
#error No channel sounding antenna switch nodes registered in DTS.
#endif

#if !DT_NODE_HAS_COMPAT(ANTENNA_SWITCH_NODE, nordic_bt_cs_antenna_switch)
#error Configured antenna switch node is not compatible.
#endif

#if DT_NODE_HAS_PROP(ANTENNA_SWITCH_NODE, multiplexing_mode)
#define MULTIPLEXED DT_PROP(ANTENNA_SWITCH_NODE, multiplexing_mode)
#else
#define MULTIPLEXED false
#endif

#define ANTENNA_NOT_SET 0xFF

BUILD_ASSERT(DT_NODE_HAS_PROP(ANTENNA_SWITCH_NODE, ant_gpios));

#define NUM_GPIOS DT_PROP_LEN(ANTENNA_SWITCH_NODE, ant_gpios)

#if MULTIPLEXED
#if CONFIG_BT_CTLR_SDC_CS_NUM_ANTENNAS == 2
BUILD_ASSERT(NUM_GPIOS >= 1);
#else
BUILD_ASSERT(NUM_GPIOS >= 2);
#endif
#else
BUILD_ASSERT(NUM_GPIOS >= CONFIG_BT_CTLR_SDC_CS_NUM_ANTENNAS);
#endif /* MULTIPLEXED */

static uint8_t currently_active_antenna = ANTENNA_NOT_SET;
static const struct gpio_dt_spec gpio_dt_spec_table[] = {
	GPIO_DT_SPEC_GET_BY_IDX_OR(ANTENNA_SWITCH_NODE, ant_gpios, 0, {0}),
	GPIO_DT_SPEC_GET_BY_IDX_OR(ANTENNA_SWITCH_NODE, ant_gpios, 1, {0}),
#if !MULTIPLEXED
	GPIO_DT_SPEC_GET_BY_IDX_OR(ANTENNA_SWITCH_NODE, ant_gpios, 2, {0}),
	GPIO_DT_SPEC_GET_BY_IDX_OR(ANTENNA_SWITCH_NODE, ant_gpios, 3, {0}),
#endif
};

#define ANT_GPIO_NODE(i)	     DT_PHANDLE_BY_IDX(ANTENNA_SWITCH_NODE, ant_gpios, i)
#define ANT_GPIO_PORT(i)	     DT_PROP(ANT_GPIO_NODE(i), port)
#define ANT_GPIO_FLAGS(i)	     DT_GPIO_FLAGS_BY_IDX(ANTENNA_SWITCH_NODE, ant_gpios, i)
#define ANT_GPIO_PIN(i)		     DT_GPIO_PIN_BY_IDX(ANTENNA_SWITCH_NODE, ant_gpios, i)
#define CONCAT_HELPER(prefix, value) prefix##value
#define CONCAT_MACRO(prefix, value)  CONCAT_HELPER(prefix, value)

typedef struct {
	NRF_GPIO_Type *port;
	uint8_t pin;
	uint32_t flags;
} antenna_pin;

static const antenna_pin pins_table[] = {
	{.port = CONCAT_MACRO(NRF_P, ANT_GPIO_PORT(0)),
	 .pin = ANT_GPIO_PIN(0),
	 .flags = ANT_GPIO_FLAGS(0)},
	{.port = CONCAT_MACRO(NRF_P, ANT_GPIO_PORT(1)),
	 .pin = ANT_GPIO_PIN(1),
	 .flags = ANT_GPIO_FLAGS(1)},
#if !MULTIPLEXED
	{.port = CONCAT_MACRO(NRF_P, ANT_GPIO_PORT(2)),
	 .pin = ANT_GPIO_PIN(2),
	 .flags = ANT_GPIO_FLAGS(2)},
	{.port = CONCAT_MACRO(NRF_P, ANT_GPIO_PORT(3)),
	 .pin = ANT_GPIO_PIN(3),
	 .flags = ANT_GPIO_FLAGS(3)},
#endif
};

static void m_gpio_pin_set(uint8_t pin_idx, uint32_t value)
{
	/* Use nrf_gpio APIs here since this function can be called from
	 * interrupt context and must not use Zephyr APIs/blocking APIs.
	 */
	if (pins_table[pin_idx].flags & GPIO_ACTIVE_LOW) {
		nrf_gpio_port_pin_write(pins_table[pin_idx].port, pins_table[pin_idx].pin, !value);
	} else {
		nrf_gpio_port_pin_write(pins_table[pin_idx].port, pins_table[pin_idx].pin, value);
	}
}

/* Antenna control below is implemented as described in the CS documentation.
 *
 * Example valid device tree configuration for the nRF54L15:
 *
 * / {
 *   cs_antenna_switch: cs-antenna-config {
 *     status = "okay";
 *     compatible = "nordic,bt-cs-antenna-switch";
 *     ant-gpios = <&gpio1 11 (GPIO_ACTIVE_HIGH)>,
 *                 <&gpio1 12 (GPIO_ACTIVE_HIGH)>,
 *                 <&gpio1 13 (GPIO_ACTIVE_HIGH)>,
 *                 <&gpio1 14 (GPIO_ACTIVE_HIGH)>;
 *     multiplexing-mode = <0>;
 *   };
 * };
 *
 */
void cs_antenna_switch_func(uint8_t antenna_number)
{
#if MULTIPLEXED
	m_gpio_pin_set(0, antenna_number & (1 << 0));
#if NUM_GPIOS > 1
	m_gpio_pin_set(1, antenna_number & (1 << 1));
#endif
#else
	if (currently_active_antenna != antenna_number) {
		if (currently_active_antenna != ANTENNA_NOT_SET) {
			m_gpio_pin_set(currently_active_antenna, false);
		}
		m_gpio_pin_set(antenna_number, true);
	}

	currently_active_antenna = antenna_number;
#endif
}

void cs_antenna_switch_init(void)
{
	int err;
	for (uint8_t i = 0; i < NUM_GPIOS; i++) {
		err = gpio_pin_configure_dt(&gpio_dt_spec_table[i], GPIO_OUTPUT_INACTIVE);
		__ASSERT(err == 0, "Failed to initialize GPIOs for CS antenna pin (%d)", err);
#if defined(CONFIG_PM_DEVICE_RUNTIME)
		/* Manually manage the PM for the antenna switch GPIOs,
		 * as this cannot be done by the cs_antenna_switch_func
		 * which is called from interrupt context.
		 */
		err = pm_device_runtime_get(gpio_dt_spec_table[i].port);
		__ASSERT(err == 0, "Failed pm_device_runtime_get for CS antenna pin (%d)", err);
#endif
	}
}

void cs_antenna_switch_clear(void)
{
	for (uint8_t i = 0; i < NUM_GPIOS; i++) {
		m_gpio_pin_set(i, false);
#if defined(CONFIG_PM_DEVICE_RUNTIME)
		/* Manually manage the PM for the antenna switch GPIOs,
		 * as this cannot be done by the cs_antenna_switch_func
		 * which is called from interrupt context.
		 */
		int err;

		err = pm_device_runtime_put(gpio_dt_spec_table[i].port);
		__ASSERT(err == 0, "Failed pm_device_runtime_put for CS antenna pin (%d)", err);
#endif
	}

	currently_active_antenna = ANTENNA_NOT_SET;
}
