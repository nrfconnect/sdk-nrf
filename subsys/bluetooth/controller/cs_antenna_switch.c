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

BUILD_ASSERT(DT_NODE_HAS_PROP(ANTENNA_SWITCH_NODE, ant_gpios));

#if MULTIPLEXED
#if CONFIG_BT_CTLR_SDC_CS_NUM_ANTENNAS == 2
BUILD_ASSERT(DT_PROP_LEN(ANTENNA_SWITCH_NODE, ant_gpios) >= 1);
#else
BUILD_ASSERT(DT_PROP_LEN(ANTENNA_SWITCH_NODE, ant_gpios) >= 2);
#endif
#else
BUILD_ASSERT(DT_PROP_LEN(ANTENNA_SWITCH_NODE, ant_gpios) >= CONFIG_BT_CTLR_SDC_CS_NUM_ANTENNAS);
#endif /* MULTIPLEXED */

static const struct gpio_dt_spec gpio_dt_spec_table[] = {
	GPIO_DT_SPEC_GET_BY_IDX_OR(ANTENNA_SWITCH_NODE, ant_gpios, 0, NULL),
	GPIO_DT_SPEC_GET_BY_IDX_OR(ANTENNA_SWITCH_NODE, ant_gpios, 1, NULL),
#if !MULTIPLEXED
	GPIO_DT_SPEC_GET_BY_IDX_OR(ANTENNA_SWITCH_NODE, ant_gpios, 2, NULL),
	GPIO_DT_SPEC_GET_BY_IDX_OR(ANTENNA_SWITCH_NODE, ant_gpios, 3, NULL),
#endif
};

/* Antenna control below is implemented as described in the CS documentation.
 * https://docs.nordicsemi.com/bundle/ncs-latest/page/nrfxlib/softdevice_controller/doc/channel_sounding.html#multiple_antennas_support
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
	int ret;
#if MULTIPLEXED
	ret = gpio_pin_set_dt(&gpio_dt_spec_table[0], antenna_number & (1 << 0));
	__ASSERT_NO_MSG(ret == 0);

	ret = gpio_pin_set_dt(&gpio_dt_spec_table[1], antenna_number & (1 << 1));
	__ASSERT_NO_MSG(ret == 0);
#else
	static uint8_t currently_active_antenna;

	if (currently_active_antenna != antenna_number) {
		ret = gpio_pin_set_dt(&gpio_dt_spec_table[currently_active_antenna], false);
		__ASSERT_NO_MSG(ret == 0);

		ret = gpio_pin_set_dt(&gpio_dt_spec_table[antenna_number], true);
		__ASSERT_NO_MSG(ret == 0);
	}

	currently_active_antenna = antenna_number;
#endif
}

void cs_antenna_switch_enable(void)
{
	int32_t ret;

	for (uint8_t i = 0; i < ARRAY_SIZE(gpio_dt_spec_table); i++) {
		ret = gpio_pin_configure_dt(&gpio_dt_spec_table[i], GPIO_OUTPUT_INACTIVE);
		__ASSERT(ret == 0, "Failed to enable GPIOs for CS antenna switching (%d)", ret);
	}
}
