/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include <errno.h>

#include <zephyr/init.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

#include "fem.h"
#include "fem_interface.h"


#define GENERIC_FEM_NODE DT_NODELABEL(nrf_radio_fem)

static struct generic_fem {
#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), ant_sel_gpios)
	const struct gpio_dt_spec ant_sel;
#endif
#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), csd_gpios)
	const struct gpio_dt_spec csd;
#endif
#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), cps_gpios)
	const struct gpio_dt_spec cps;
#endif
#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), chl_gpios)
	const struct gpio_dt_spec chl;
#endif

} generic_fem_cfg __used = {
#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), ant_sel_gpios)
	.ant_sel = GPIO_DT_SPEC_GET(GENERIC_FEM_NODE, ant_sel_gpios),
#endif
#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), csd_gpios)
	.csd = GPIO_DT_SPEC_GET(GENERIC_FEM_NODE, csd_gpios),
#endif
#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), cps_gpios)
	.cps = GPIO_DT_SPEC_GET(GENERIC_FEM_NODE, cps_gpios),
#endif
#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), chl_gpios)
	.chl = GPIO_DT_SPEC_GET(GENERIC_FEM_NODE, chl_gpios)
#endif
};

static int gpio_config(void)
{
	int err = 0;

#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), ant_sel_gpios)
	err = gpio_pin_configure(generic_fem_cfg.ant_sel.port, generic_fem_cfg.ant_sel.pin,
				 generic_fem_cfg.ant_sel.dt_flags | GPIO_OUTPUT_INACTIVE);
	if (err) {
		return err;
	}
#endif /* DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), ant_sel_gpios) */

#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), csd_gpios)
	err = gpio_pin_configure(generic_fem_cfg.csd.port, generic_fem_cfg.csd.pin,
				 generic_fem_cfg.csd.dt_flags | GPIO_OUTPUT_INACTIVE);
	if (err) {
		return err;
	}
#endif /* DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), csd_gpios) */

#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), cps_gpios)
	err = gpio_pin_configure(generic_fem_cfg.cps.port, generic_fem_cfg.cps.pin,
				 generic_fem_cfg.cps.dt_flags | GPIO_OUTPUT_INACTIVE);
	if (err) {
		return err;
	}

#endif /* DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), cps_gpios) */

#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), chl_gpios)
	err = gpio_pin_configure(generic_fem_cfg.chl.port, generic_fem_cfg.chl.pin,
				 generic_fem_cfg.chl.dt_flags | GPIO_OUTPUT_INACTIVE);
	if (err) {
		return err;
	}
#endif /* DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), chl_gpios) */

	return err;
}

static int generic_fem_init(void)
{
	return gpio_config();
}

static int generic_fem_power_up(void)
{
	int err = 0;

#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), csd_gpios)
	err = gpio_pin_set(generic_fem_cfg.csd.port, generic_fem_cfg.csd.pin, 1);
#endif /* DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), csd_gpios) */

#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), cps_gpios)
	if (!err) {
		if (IS_ENABLED(CONFIG_SKYWORKS_BYPASS_MODE)) {
			err = gpio_pin_set(generic_fem_cfg.cps.port, generic_fem_cfg.cps.pin, 1);
		}
	}
#endif /* DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), cps_gpios) */

#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), chl_gpios)
	if (!err) {
		if (IS_ENABLED(CONFIG_SKYWORKS_HIGH_POWER_MODE)) {
			err = gpio_pin_set(generic_fem_cfg.chl.port, generic_fem_cfg.chl.pin, 1);
		}
	}
#endif /* DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), chl_gpios) */

	return err;
}

static int generic_fem_power_down(void)
{
	int err = 0;

#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), cps_gpios)
	if (IS_ENABLED(CONFIG_SKYWORKS_BYPASS_MODE)) {
		err = gpio_pin_set(generic_fem_cfg.cps.port, generic_fem_cfg.cps.pin, 0);
	}
#endif /* DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), cps_gpios) */

#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), chl_gpios)
	if (!err) {
		if (IS_ENABLED(CONFIG_SKYWORKS_HIGH_POWER_MODE)) {
			err = gpio_pin_set(generic_fem_cfg.chl.port, generic_fem_cfg.chl.pin, 0);
		}
	}
#endif /* DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), chl_gpios) */

#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), csd_gpios)
	if (!err) {
		err = gpio_pin_set(generic_fem_cfg.csd.port, generic_fem_cfg.csd.pin, 0);
	}
#endif /* DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), csd_gpios) */

	return err;
}

static uint32_t tx_default_gain_get(void)
{
	return DT_PROP(DT_NODELABEL(nrf_radio_fem), tx_gain_db);
}

#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), ant_sel_gpios)
static int generic_fem_antenna_select(enum fem_antenna ant)
{
	int err = 0;

	switch (ant) {
	case FEM_ANTENNA_1:
		err = gpio_pin_set(generic_fem_cfg.ant_sel.port,
				   generic_fem_cfg.ant_sel.pin, 0);
		break;

	case FEM_ANTENNA_2:
		err = gpio_pin_set(generic_fem_cfg.ant_sel.port,
				   generic_fem_cfg.ant_sel.pin, 1);

		break;

	default:
		err = -EINVAL;
		break;
	}

	return err;
}
#else
static int generic_fem_antenna_select(enum fem_antenna ant)
{
	return -ENOTSUP;
}
#endif /* DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), ant_sel_gpios) */

static const struct fem_interface_api generic_fem_api = {
	.power_up = generic_fem_power_up,
	.power_down = generic_fem_power_down,
	.antenna_select = generic_fem_antenna_select,
	.tx_default_gain_get = tx_default_gain_get,
};

static int generic_fem_setup(void)
{
	int err;

	err = generic_fem_init();
	if (err) {
		return err;
	}

	return fem_interface_api_set(&generic_fem_api);
}

SYS_INIT(generic_fem_setup, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
