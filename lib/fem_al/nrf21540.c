/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <string.h>

#include <zephyr/init.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

#include "fem_interface.h"

#define NRF21540_NODE DT_NODELABEL(nrf_radio_fem)

/* nRF21540 Front-End-Module maximum gain register value */
#define NRF21540_TX_GAIN_MAX 31

enum nrf21540_ant {
	/** Antenna 1 output. */
	NRF21540_ANT1,

	/** Antenna 2 output. */
	NRF21540_ANT2
};

#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), ant_sel_gpios)
static struct nrf21540 {
	const struct gpio_dt_spec ant_sel;
} nrf21540_cfg = {
	.ant_sel = GPIO_DT_SPEC_GET(NRF21540_NODE, ant_sel_gpios),
};
#endif /* DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), ant_sel_gpios) */


static int gpio_configure(void)
{
	int err = 0;

#if DT_NODE_HAS_PROP(NRF21540_NODE, ant_sel_gpios)
	/* Configure Antenna select pin */
	if (!device_is_ready(nrf21540_cfg.ant_sel.port)) {
		return -ENODEV;
	}

	/* Configure Antenna select pin */
	err = gpio_pin_configure_dt(&nrf21540_cfg.ant_sel, GPIO_OUTPUT_INACTIVE);
#endif /* DT_NODE_HAS_PROP(NRF21540_NODE, ant_sel_gpios) */

	return err;
}

static int nrf21540_init(void)
{
	int err;

	err = gpio_configure();
	if (err) {
		return err;
	}

	return 0;
}

static int tx_power_control_validate(fem_tx_power_control tx_power_control)
{
	if (IS_ENABLED(CONFIG_MPSL_FEM_NRF21540_GPIO_SPI)) {
		return (tx_power_control > NRF21540_TX_GAIN_MAX) ? -EINVAL : 0;
	} else {
		return ((tx_power_control == 0) || (tx_power_control == 1)) ? 0 : -EINVAL;
	}
}

static int8_t default_tx_output_power_get(void)
{
	return CONFIG_MPSL_FEM_NRF21540_TX_GAIN_DB;
}

#if DT_NODE_HAS_PROP(NRF21540_NODE, ant_sel_gpios)
static int nrf21540_antenna_select(enum fem_antenna ant)
{
	int err = 0;

	switch (ant) {
	case FEM_ANTENNA_1:
		err = gpio_pin_set_dt(&nrf21540_cfg.ant_sel, 0);
		break;

	case FEM_ANTENNA_2:
		err = gpio_pin_set_dt(&nrf21540_cfg.ant_sel, 1);

		break;

	default:
		err = -EINVAL;
		break;
	}

	return err;
}
#else
static int nrf21540_antenna_select(enum fem_antenna ant)
{
	return -ENOTSUP;
}
#endif /* DT_NODE_HAS_PROP(NRF21540_NODE, ant_sel_gpios) */

static const struct fem_interface_api nrf21540_api = {
	.tx_power_control_validate = tx_power_control_validate,
	.default_tx_output_power_get = default_tx_output_power_get,
	.antenna_select = nrf21540_antenna_select
};

static int nrf21540_setup(void)
{
	int err;

	err = nrf21540_init();
	if (err) {
		return err;
	}

	return fem_interface_api_set(&nrf21540_api);
}

SYS_INIT(nrf21540_setup, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
