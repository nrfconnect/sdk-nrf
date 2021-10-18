/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 *   This file implements a generic bluetooth external radio coexistence
 *   interface.
 */
#include <stddef.h>
#include <stdint.h>

#include <device.h>
#include <drivers/gpio.h>
#include <devicetree.h>
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

#if !(DT_NODE_HAS_COMPAT(COEX_NODE, sdc_radio_coex_three_wire))
#error Selected coex node is not compatible with sdc-radio-coex-three-wire.
#endif

#if !IS_ENABLED(CONFIG_SOC_SERIES_NRF52X)
#error Bluetooth coex is only supported on the nRF52 series.
#endif

#define MPSL_COEX_BT_GPIO_POLARITY_GET(dt_property)                                                \
	((GPIO_ACTIVE_LOW & DT_GPIO_FLAGS(COEX_NODE, dt_property)) ? false : true)

static volatile bool coex_enabled;

static void coex_enable_callback(void)
{
	coex_enabled = true;
}

int mpsl_cx_bt_interface_3wire_config_set(void)
{
	nrfx_err_t err = NRFX_SUCCESS;
	mpsl_coex_gpiote_cfg_t *gpiote_cfg;
	nrf_ppi_channel_t ppi_channel;
	mpsl_coex_if_t coex_if_bt;
	mpsl_coex_802152_3wire_gpiote_if_t *coex_if = &coex_if_bt.interfaces.coex_3wire_gpiote;

	/* Allocate GPIOTE and PPI channels for the REQUEST line */
	gpiote_cfg = &coex_if->request_cfg;
	if (nrfx_gpiote_channel_alloc(&gpiote_cfg->gpiote_ch_id) != NRFX_SUCCESS) {
		return -ENOMEM;
	}
	if (nrfx_ppi_channel_alloc(&ppi_channel) != NRFX_SUCCESS) {
		return -ENOMEM;
	}
	gpiote_cfg->gpio_pin = NRF_DT_GPIOS_TO_PSEL(COEX_NODE, req_gpios);
	gpiote_cfg->active_high = MPSL_COEX_BT_GPIO_POLARITY_GET(req_gpios);
	gpiote_cfg->ppi_ch_id = ppi_channel;

	/* Allocate GPIOTE and PPI channels for the PRIORITY/DIRECTION line */
	gpiote_cfg = &coex_if->priority_cfg;
	if (nrfx_gpiote_channel_alloc(&gpiote_cfg->gpiote_ch_id) != NRFX_SUCCESS) {
		return -ENOMEM;
	}
	if (nrfx_ppi_channel_alloc(&ppi_channel) != NRFX_SUCCESS) {
		return -ENOMEM;
	}
	gpiote_cfg->gpio_pin = NRF_DT_GPIOS_TO_PSEL(COEX_NODE, pri_dir_gpios);
	gpiote_cfg->active_high = MPSL_COEX_BT_GPIO_POLARITY_GET(pri_dir_gpios);
	gpiote_cfg->ppi_ch_id = ppi_channel;

	/* Allocate GPIOTE and PPI channels for the GRANT line */
	gpiote_cfg = &coex_if->grant_cfg;
	if (nrfx_gpiote_channel_alloc(&gpiote_cfg->gpiote_ch_id) != NRFX_SUCCESS) {
		return -ENOMEM;
	}
	if (nrfx_ppi_channel_alloc(&ppi_channel) != NRFX_SUCCESS) {
		return -ENOMEM;
	}
	gpiote_cfg->gpio_pin = NRF_DT_GPIOS_TO_PSEL(COEX_NODE, grant_gpios);
	gpiote_cfg->active_high = MPSL_COEX_BT_GPIO_POLARITY_GET(grant_gpios);
	gpiote_cfg->ppi_ch_id = ppi_channel;

	/* Allocate additional PPI channel (used for handling radio signal) */
	if (nrfx_ppi_channel_alloc(&ppi_channel) != NRFX_SUCCESS) {
		return -ENOMEM;
	}
	coex_if->additional_ppi_ch_id = ppi_channel;

	coex_if->type_delay_us = DT_PROP(COEX_NODE, type_delay_us);
	coex_if->radio_delay_us = DT_PROP(COEX_NODE, radio_delay_us);

	/* is_rx_active_level = 1 -> PRIO uses same level for [HIGH PRIO] and [RX dir] */
	/* is_rx_active_level = 0 -> PRIO uses same level for [HIGH PRIO] and [TX dir] */
	coex_if->is_rx_active_level = DT_PROP(COEX_NODE, is_rx_active_level);
	coex_if->p_timer_instance = COEX_TIMER;

	/* Enable support for coex APIs (make sure it's not link-time optimized out) */
	mpsl_coex_support_802152_3wire_gpiote_if();

	/* Enable 3-wire coex implementation */
	coex_if_bt.if_id = MPSL_COEX_802152_3WIRE_GPIOTE_ID;
	err = (int)mpsl_coex_enable(&coex_if_bt, coex_enable_callback);

	/** If enable API call is successful, wait for initialization to complete
	 *  by waiting for the supplied callback to be called.
	 */
	while (!err && !coex_enabled) {
		k_yield();
	}

	return err;
}
