/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/devicetree.h>
#include <nrf_802154.h>
#include <nrf_802154_config.h>

LOG_MODULE_REGISTER(nrf_802154_configure);

/* NRF21540 FEM */
#if IS_ENABLED(CONFIG_MPSL_FEM_NRF21540_GPIO) || IS_ENABLED(CONFIG_MPSL_FEM_NRF21540_GPIO_SPI)
#define LNA_GAIN CONFIG_MPSL_FEM_NRF21540_RX_GAIN_DB
/* Simple GPIO FEM */
#elif IS_ENABLED(CONFIG_MPSL_FEM_SIMPLE_GPIO)
#if !DT_NODE_EXISTS(DT_NODELABEL(nrf_radio_fem))
#error Node with label 'nrf_radio_fem' not found in the devicetree.
#endif
#define LNA_GAIN DT_PROP(DT_NODELABEL(nrf_radio_fem), rx_gain_db)
/* No FEM */
#else
#define LNA_GAIN 0U
#endif

static void ccaed_threshold_configure(void)
{
	nrf_802154_cca_cfg_t cca_cfg;

	nrf_802154_cca_cfg_get(&cca_cfg);

	/* Overwrite thresholds accounting for RX track gain. */
	cca_cfg.ed_threshold = NRF_802154_CCA_ED_THRESHOLD_DEFAULT + LNA_GAIN;
	cca_cfg.corr_threshold = NRF_802154_CCA_CORR_THRESHOLD_DEFAULT + LNA_GAIN;

	nrf_802154_cca_cfg_set(&cca_cfg);
}

static int nrf_802154_configure(void)
{

	ccaed_threshold_configure();

	return 0;
}

#if IS_ENABLED(CONFIG_NRF_802154_SER_RADIO)
#define INIT_LEVEL APPLICATION
#define INIT_PRIO CONFIG_APPLICATION_INIT_PRIORITY
#elif IS_ENABLED(CONFIG_IEEE802154_NRF5)
#define INIT_LEVEL POST_KERNEL
#define INIT_PRIO CONFIG_IEEE802154_NRF5_INIT_PRIO
#else
#define INIT_LEVEL POST_KERNEL
/* There is no defined priority of nRF 802.15.4 Radio Driver's initialization.
 * No priority validation can be performed.
 */
#endif

#if defined(INIT_PRIO)
BUILD_ASSERT(INIT_PRIO < CONFIG_NRF_802154_RADIO_CONFIG_PRIO,
	     "nRF 802.15.4 driver configuration would not be performed after its initialization");
#endif

SYS_INIT(nrf_802154_configure, INIT_LEVEL, CONFIG_NRF_802154_RADIO_CONFIG_PRIO);
