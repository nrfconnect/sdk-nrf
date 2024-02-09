/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 *   This file implements a generic bluetooth external radio 1w coexistence
 *   interface.
 */
#include <stddef.h>
#include <stdint.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/devicetree.h>
#include <soc_nrf_common.h>
#include <nrfx.h>
#include <nrfx_ppi.h>
#include <nrfx_gpiote.h>
#include <mpsl_coex.h>

#define COEX_TIMER NRF_TIMER1

#if DT_NODE_HAS_STATUS(DT_PHANDLE(DT_NODELABEL(radio), coex), okay)
#define COEX_NODE DT_PHANDLE(DT_NODELABEL(radio), coex)
#else
#define COEX_NODE DT_INVALID_NODE
#error No enabled coex nodes registered in DTS.
#endif

#if !(DT_NODE_HAS_COMPAT(COEX_NODE, sdc_radio_coex_one_wire))
#error Selected coex node is not compatible with sdc-radio-coex-one-wire.
#endif

#if !IS_ENABLED(CONFIG_SOC_SERIES_NRF52X)
#error Bluetooth coex is only supported on the nRF52 series
#endif

#define MPSL_COEX_BT_GPIO_POLARITY_GET(dt_property)                                                \
	((GPIO_ACTIVE_LOW & DT_GPIO_FLAGS(COEX_NODE, dt_property)) ? false : true)

static volatile bool coex_enabled;

static void coex_enable_callback(void)
{
	coex_enabled = true;
}

static int mpsl_cx_bt_interface_1wire_config_set(void)
{
	nrfx_err_t err = NRFX_SUCCESS;
	mpsl_coex_gpiote_cfg_t *gpiote_cfg;
	nrf_ppi_channel_t ppi_channel;
	mpsl_coex_if_t coex_if_bt;
	mpsl_coex_1wire_gpiote_if_t *coex_if = &coex_if_bt.interfaces.coex_1wire_gpiote;
	const nrfx_gpiote_t gpiote = NRFX_GPIOTE_INSTANCE(
		NRF_DT_GPIOTE_INST(COEX_NODE, grant_gpios));

	/* Allocate GPIOTE and PPI channels for the GRANT line */
	gpiote_cfg = &coex_if->grant_cfg;
	if (nrfx_gpiote_channel_alloc(&gpiote, &gpiote_cfg->gpiote_ch_id) != NRFX_SUCCESS) {
		return -ENOMEM;
	}
	if (nrfx_ppi_channel_alloc(&ppi_channel) != NRFX_SUCCESS) {
		return -ENOMEM;
	}
	gpiote_cfg->gpio_pin = NRF_DT_GPIOS_TO_PSEL(COEX_NODE, grant_gpios);
	gpiote_cfg->active_high = MPSL_COEX_BT_GPIO_POLARITY_GET(grant_gpios);
	gpiote_cfg->ppi_ch_id = ppi_channel;

	/* update concurrency mode */
	coex_if->concurrency_mode = DT_PROP(COEX_NODE, concurrency_mode);

	/* Enable support for coex APIs (make sure it's not link-time optimized out) */
	mpsl_coex_support_1wire_gpiote_if();

	/* Enable 1-wire coex implementation */
	coex_if_bt.if_id = MPSL_COEX_1WIRE_GPIOTE_ID;
	err = (int)mpsl_coex_enable(&coex_if_bt, coex_enable_callback);

	/** If enable API call is successful, wait for initialization to complete
	 *  by waiting for the supplied callback to be called.
	 */
	while (!err && !coex_enabled) {
		k_yield();
	}

	return err;
}

static int mpsl_cx_init(void)
{

	return mpsl_cx_bt_interface_1wire_config_set();
}

SYS_INIT(mpsl_cx_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
