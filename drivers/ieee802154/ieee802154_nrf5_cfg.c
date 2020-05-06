/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <init.h>
#include <irq.h>
#include <kernel.h>
#include <logging/log.h>
#include <nrf_802154.h>
#include <nrf_802154_sl_ant_div.h>
#include <nrf_802154_sl_coex.h>
#include <fem/nrf_fem_control_config.h>
#include <hal/nrf_gpio.h>

LOG_MODULE_REGISTER(ieee802154_nrf5_cfg, CONFIG_IEEE802154_NRF5_CFG_LOG_LEVEL);

#if (CONFIG_IEEE802154_NRF5_FEM_PRESENT)
static const nrf_fem_interface_config_t fem_iface_config = {
	.fem_config = {
		.pa_time_gap_us  =
			CONFIG_IEEE802154_NRF5_FEM_PA_TIME_IN_ADVANCE_US,
		.lna_time_gap_us =
			CONFIG_IEEE802154_NRF5_FEM_LNA_TIME_IN_ADVANCE_US,
		.pdn_settle_us   =
			CONFIG_IEEE802154_NRF5_FEM_PDN_SETTLE_US,
		.trx_hold_us     =
			CONFIG_IEEE802154_NRF5_FEM_TRX_HOLD_US,
		.pa_gain_db      = 0,
		.lna_gain_db     = 0,
	},
	.pa_pin_config = {
		.enable       = CONFIG_IEEE802154_NRF5_FEM_PA_PIN_CTRL_ENABLE,
		.active_high  = 1,
		.gpio_pin     = CONFIG_IEEE802154_NRF5_FEM_PA_PIN,
		.gpiote_ch_id = CONFIG_IEEE802154_NRF5_FEM_PA_GPIOTE_CHANNEL,
	},
	.lna_pin_config = {
		.enable       = CONFIG_IEEE802154_NRF5_FEM_LNA_PIN_CTRL_ENABLE,
		.active_high  = 1,
		.gpio_pin     = CONFIG_IEEE802154_NRF5_FEM_LNA_PIN,
		.gpiote_ch_id = CONFIG_IEEE802154_NRF5_FEM_LNA_GPIOTE_CHANNEL,
	},
	.pdn_pin_config = {
		.enable       = CONFIG_IEEE802154_NRF5_FEM_PDN_PIN_CTRL_ENABLE,
		.active_high  = 1,
		.gpio_pin     = CONFIG_IEEE802154_NRF5_FEM_PDN_PIN,
		.gpiote_ch_id = CONFIG_IEEE802154_NRF5_FEM_PDN_GPIOTE_CHANNEL,
	},
	.ppi_ch_id_pdn = CONFIG_IEEE802154_NRF5_FEM_PDN_PPI_CHANNEL,
	.ppi_ch_id_set = CONFIG_IEEE802154_NRF5_FEM_SET_PPI_CHANNEL,
	.ppi_ch_id_clr = CONFIG_IEEE802154_NRF5_FEM_CLR_PPI_CHANNEL,
};
#endif

static void nrf5_gpio_configure(void)
{
#if !CONFIG_IEEE802154_NRF5_ANT_DIVERSITY_MODE_DISABLED
	nrf_gpio_cfg_output(CONFIG_IEEE802154_NRF5_ANT_DIVERSITY_PIN);
	nrf_gpio_pin_clear(CONFIG_IEEE802154_NRF5_ANT_DIVERSITY_PIN);
#endif

#if CONFIG_IEEE802154_NRF5_FEM_PRESENT
	/* Set default FEM pin direction. */
	nrf_gpio_cfg_output(CONFIG_IEEE802154_NRF5_FEM_PA_PIN);
	nrf_gpio_cfg_output(CONFIG_IEEE802154_NRF5_FEM_LNA_PIN);
	nrf_gpio_cfg_output(CONFIG_IEEE802154_NRF5_FEM_PDN_PIN);
	nrf_gpio_cfg_output(CONFIG_IEEE802154_NRF5_FEM_MODE_PIN);

	/* Set default FEM pin polarity. */
	/* Disable PA, radio driver will override this setting */
	nrf_gpio_pin_clear(CONFIG_IEEE802154_NRF5_FEM_PA_PIN);
	/* Disable LNA, radio driver will override this setting */
	nrf_gpio_pin_clear(CONFIG_IEEE802154_NRF5_FEM_LNA_PIN);
	/* Disable FEM, radio driver will override this setting */
	nrf_gpio_pin_clear(CONFIG_IEEE802154_NRF5_FEM_PDN_PIN);
	/* Use POUTA_PROD TX gain by default */
	nrf_gpio_pin_clear(CONFIG_IEEE802154_NRF5_FEM_MODE_PIN);

#if CONFIG_IEEE802154_NRF5_SPI
	nrf_gpio_cfg_output(CONFIG_IEEE802154_NRF5_FEM_MOSI_PIN);
	nrf_gpio_cfg_default(CONFIG_IEEE802154_NRF5_FEM_MISO_PIN);
	nrf_gpio_cfg_output(CONFIG_IEEE802154_NRF5_FEM_CLK_PIN);
	nrf_gpio_cfg_output(CONFIG_IEEE802154_NRF5_FEM_CSN_PIN);

	/* SPI mode not used. Use high polarity by default. */
	nrf_gpio_pin_set(CONFIG_IEEE802154_NRF5_FEM_MOSI_PIN);
	/* SPI mode not used. Use high polarity by default. */
	nrf_gpio_pin_set(CONFIG_IEEE802154_NRF5_FEM_CLK_PIN);
	/* SPI mode not used. Use low polarity by default.*/
	nrf_gpio_pin_clear(CONFIG_IEEE802154_NRF5_FEM_CSN_PIN);
#endif
#endif
}

static int nrf5_cfg_pre_init(struct device *dev)
{
	ARG_UNUSED(dev);

	nrf5_gpio_configure();

#if CONFIG_IEEE802154_NRF5_FEM_PRESENT
	if (!nrf_fem_interface_configuration_set(&fem_iface_config)) {
		LOG_ERR("Failed to configure FEM");
	}
#endif

#if !CONFIG_IEEE802154_NRF5_ANT_DIVERSITY_MODE_DISABLED
	nrf_802154_sl_ant_div_cfg_t cfg = {0};

	cfg.ant_sel_pin = CONFIG_IEEE802154_NRF5_ANT_DIVERSITY_PIN;
	cfg.toggle_time = CONFIG_IEEE802154_NRF5_ANT_DIVERSITY_TOGGLE_TIME;
	cfg.ppi_ch      = CONFIG_IEEE802154_NRF5_ANT_DIVERSITY_PPI_CHANNEL;
	cfg.gpiote_ch   = CONFIG_IEEE802154_NRF5_ANT_DIVERSITY_GPIOTE_CHANNEL;
	cfg.p_timer     = NRFX_CONCAT_2(NRF_TIMER,
		CONFIG_IEEE802154_NRF5_ANT_DIVERSITY_TIMER_INSTANCE);

	nrf_802154_antenna_diversity_config_set(&cfg);
#endif

#if CONFIG_IEEE802154_NRF5_WIFI_COEX
	nrf_802154_wifi_coex_3wire_if_config_t coex_config =
		NRF_802154_COEX_3WIRE_DEFAULT_CONFIG;

	coex_config.request_cfg.gpio_pin =
		CONFIG_IEEE802154_NRF5_WIFI_COEX_REQ_PIN;
	coex_config.request_cfg.active_high =
		CONFIG_IEEE802154_NRF5_WIFI_COEX_REQ_ACTIVE_HIGH;
	coex_config.priority_cfg.gpio_pin =
		CONFIG_IEEE802154_NRF5_WIFI_COEX_PRIO_PIN;
	coex_config.priority_cfg.active_high =
		CONFIG_IEEE802154_NRF5_WIFI_COEX_PRIO_ACTIVE_HIGH;
	coex_config.grant_cfg.gpio_pin =
		CONFIG_IEEE802154_NRF5_WIFI_COEX_GRA_PIN;
	coex_config.grant_cfg.active_high =
		CONFIG_IEEE802154_NRF5_WIFI_COEX_GRA_ACTIVE_HIGH;
	coex_config.grant_cfg.gpiote_ch_id =
		CONFIG_IEEE802154_NRF5_WIFI_COEX_GPIOTE_CHANNEL;

	nrf_802154_wifi_coex_cfg_3wire_set(&coex_config);

	nrf_802154_coex_rx_request_mode_t rx_mode;
	nrf_802154_coex_tx_request_mode_t tx_mode;

#if CONFIG_IEEE802154_NRF5_WIFI_COEX_RX_REQ_MODE_ED
	rx_mode = NRF_802154_COEX_RX_REQUEST_MODE_ENERGY_DETECTION;
#elif CONFIG_IEEE802154_NRF5_WIFI_COEX_RX_REQ_MODE_PREAMBLE
	rx_mode = NRF_802154_COEX_RX_REQUEST_MODE_PREAMBLE;
#elif CONFIG_IEEE802154_NRF5_WIFI_COEX_RX_REQ_MODE_DESTINED
	rx_mode = NRF_802154_COEX_RX_REQUEST_MODE_DESTINED;
#endif

#if CONFIG_IEEE802154_NRF5_WIFI_COEX_TX_REQ_MODE_FRAME_READY
	tx_mode = NRF_802154_COEX_TX_REQUEST_MODE_FRAME_READY;
#elif CONFIG_IEEE802154_NRF5_WIFI_COEX_TX_REQ_MODE_CCA_START
	tx_mode = NRF_802154_COEX_TX_REQUEST_MODE_CCA_START;
#elif CONFIG_IEEE802154_NRF5_WIFI_COEX_TX_REQ_MODE_CCA_DONE
	tx_mode = NRF_802154_COEX_TX_REQUEST_MODE_CCA_DONE;
#endif

	if (!nrf_802154_coex_rx_request_mode_set(rx_mode)) {
		LOG_ERR("Failed to set wifi coex rx request mode");
	}
	if (!nrf_802154_coex_tx_request_mode_set(tx_mode)) {
		LOG_ERR("Failed to set wifi coex tx request mode");
	}

#endif
	return 0;
}

static int nrf5_cfg_post_init(struct device *dev)
{
	ARG_UNUSED(dev);

#if !CONFIG_IEEE802154_NRF5_ANT_DIVERSITY_MODE_DISABLED
	nrf_802154_sl_ant_div_mode_t    rx_ant_div_mode;
	nrf_802154_sl_ant_div_mode_t    tx_ant_div_mode;
	nrf_802154_sl_ant_div_antenna_t ant_div_antenna;

#if CONFIG_IEEE802154_NRF5_ANT_DIVERSITY_MODE_MANUAL
	rx_ant_div_mode = NRF_802154_SL_ANT_DIV_MODE_MANUAL;
	tx_ant_div_mode = NRF_802154_SL_ANT_DIV_MODE_MANUAL;
#elif CONFIG_IEEE802154_NRF5_ANT_DIVERSITY_MODE_AUTO
	rx_ant_div_mode = NRF_802154_SL_ANT_DIV_MODE_AUTO;
	tx_ant_div_mode = NRF_802154_SL_ANT_DIV_MODE_MANUAL;
#else
#error "Invalid Antenna Diversity mode"
#endif

#if CONFIG_IEEE802154_NRF5_ANT_DIVERSITY_ANTENNA_1
	ant_div_antenna = NRF_802154_SL_ANT_DIV_ANTENNA_1;
#elif CONFIG_IEEE802154_NRF5_ANT_DIVERSITY_ANTENNA_2
	ant_div_antenna = NRF_802154_SL_ANT_DIV_ANTENNA_2;
#else
#error "Missing default antenna for Antenna Diversity"
#endif

	nrf_802154_antenna_diversity_init();

	if (!nrf_802154_antenna_diversity_rx_mode_set(rx_ant_div_mode)) {
		LOG_ERR("Failed to set Antenna Diversity RX mode");
	}
	if (!nrf_802154_antenna_diversity_tx_mode_set(tx_ant_div_mode)) {
		LOG_ERR("Failed to set Antenna Diversity TX mode");
	}
	if (!nrf_802154_antenna_diversity_rx_antenna_set(ant_div_antenna)) {
		LOG_ERR("Failed to selected antenna for RX mode");
	}
	if (!nrf_802154_antenna_diversity_tx_antenna_set(ant_div_antenna)) {
		LOG_ERR("Failed to selected antenna for TX mode");
	}
#endif /* !CONFIG_IEEE802154_NRF5_ANT_DIVERSITY_MODE_DISABLED */

#if CONFIG_IEEE802154_NRF5_WIFI_COEX
	if (!nrf_802154_wifi_coex_enable()) {
		LOG_ERR("Failed to enable wifi coex");
	}
#endif

	return 0;
}

SYS_INIT(nrf5_cfg_pre_init, POST_KERNEL, CONFIG_IEEE802154_NRF5_CFG_PRE_PRIO);
SYS_INIT(nrf5_cfg_post_init, POST_KERNEL, CONFIG_IEEE802154_NRF5_CFG_POST_PRIO);
