/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 * Copyright (c) 2023 Laird Connectivity
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_REGISTER(lcz_fem_gpio, CONFIG_LCZ_FEM_LOG_LEVEL);

#include <nrfx_gpiote.h>
#include <helpers/nrfx_gppi.h>

#if DT_NODE_HAS_PROP(DT_NODELABEL(radio), fem)
#define FEM_NODE DT_PHANDLE(DT_NODELABEL(radio), fem)
#else
#error "FEM node not found"
#endif

/* NRF_DT_GPIOS_TO_PSEL function handles port */
#if DT_NODE_HAS_PROP(FEM_NODE, pdn_gpios)
#define FEM_PDN_PIN NRF_DT_GPIOS_TO_PSEL(FEM_NODE, pdn_gpios)
#else
#error "Power down pin not found"
#endif

#if DT_NODE_HAS_PROP(DT_NODELABEL(radio), spi_if)
#error "Invalid configuration - FEM SPI interface cannot exist as part of Nordic FEM node"
#endif

#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), ant_sel_gpios)
#error "Invalid configuration - Antenna Select cannot be part of Nordic FEM node"
#endif

#define FEM_GPIO_NODE DT_NODELABEL(lcz_fem)

#define FEM_ANT_SEL DT_PROP(FEM_GPIO_NODE, ant_sel_gpios)
#define FEM_SPI_CLK DT_PROP(FEM_GPIO_NODE, spi_clk_gpios)
#define FEM_SPI_MOSI DT_PROP(FEM_GPIO_NODE, spi_mosi_gpios)
#define FEM_SPI_CSN DT_PROP(FEM_GPIO_NODE, spi_csn_gpios)
#define FEM_SPI_MISO DT_PROP(FEM_GPIO_NODE, spi_miso_gpios)

#if !DT_NODE_EXISTS(FEM_ANT_SEL) || !DT_NODE_EXISTS(FEM_SPI_CLK) ||                                \
	!DT_NODE_EXISTS(FEM_SPI_MOSI) || !DT_NODE_EXISTS(FEM_SPI_MISO) ||                          \
	!DT_NODE_EXISTS(FEM_SPI_CSN)
#error "FEM pin not found in device tree"
#endif

static const struct gpio_dt_spec fem_spi_clk = GPIO_DT_SPEC_GET(FEM_GPIO_NODE, spi_clk_gpios);
static const struct gpio_dt_spec fem_spi_mosi = GPIO_DT_SPEC_GET(FEM_GPIO_NODE, spi_mosi_gpios);
static const struct gpio_dt_spec fem_spi_miso = GPIO_DT_SPEC_GET(FEM_GPIO_NODE, spi_miso_gpios);

#define PDN_PIN FEM_PDN_PIN
#define CSN_PIN NRF_DT_GPIOS_TO_PSEL(FEM_GPIO_NODE, spi_csn_gpios)
#define ANT_PIN NRF_DT_GPIOS_TO_PSEL(FEM_GPIO_NODE, ant_sel_gpios)

static int configure_output(const struct gpio_dt_spec *spec, int value)
{
	int r = 0;

	if (!spec->port) {
		LOG_ERR("port dt spec invalid");
		return -ENOENT;
	}

	do {
		if (!device_is_ready(spec->port)) {
			r = -EAGAIN;
			LOG_ERR("Error: %s port is not ready", spec->port->name);
			break;
		}

		r = gpio_pin_configure_dt(spec, GPIO_OUTPUT);
		if (r != 0) {
			LOG_ERR("Error %d: failed to configure pin %d", r, spec->pin);
			break;
		}

		r = gpio_pin_set_dt(spec, value);

	} while (0);

	if (r < 0) {
		LOG_ERR("Unable to configure output pin");
	}

	return r;
}

static int configure_disconnect(const struct gpio_dt_spec *spec)
{
	int r = 0;

	if (!spec->port) {
		LOG_ERR("port dt spec invalid");
		return -ENOENT;
	}

	do {
		if (!device_is_ready(spec->port)) {
			r = -EAGAIN;
			LOG_ERR("Error: %s port is not ready", spec->port->name);
			break;
		}

		r = gpio_pin_configure_dt(spec, GPIO_DISCONNECTED);
		if (r != 0) {
			LOG_ERR("Error %d: failed to configure pin %d", r, spec->pin);
			break;
		}

	} while (0);

	if (r < 0) {
		LOG_ERR("Unable to configure pin as disconnected");
	}

	return r;
}

/* Chip select [antenna select] should follow state of power down. */
static int setup_follower(nrfx_gpiote_pin_t follower)
{
	nrfx_err_t err;
	uint8_t in_channel;
	uint8_t out_channel;
	uint8_t ppi_channel;

	if (!nrfx_gpiote_is_init()) {
		err = nrfx_gpiote_init(0);
		if (err != NRFX_SUCCESS) {
			LOG_ERR("nrfx_gpiote_init error: 0x%08X", err);
			return -EINVAL;
		}
	}

	err = nrfx_gpiote_channel_alloc(&in_channel);
	if (err != NRFX_SUCCESS) {
		LOG_ERR("Failed to allocate in_channel, error: 0x%08X", err);
		return -EINVAL;
	}

	err = nrfx_gpiote_channel_alloc(&out_channel);
	if (err != NRFX_SUCCESS) {
		LOG_ERR("Failed to allocate out_channel, error: 0x%08X", err);
		return -EINVAL;
	}

	nrf_gpio_cfg_watcher(PDN_PIN);

	const nrfx_gpiote_trigger_config_t trigger_config = {
		.trigger = NRFX_GPIOTE_TRIGGER_TOGGLE,
		.p_in_channel = &in_channel,
	};
	err = nrfx_gpiote_input_configure(PDN_PIN, NULL, &trigger_config, NULL);
	if (err != NRFX_SUCCESS) {
		LOG_ERR("nrfx_gpiote_input_configure error: 0x%08X", err);
		return -EINVAL;
	}

	/* network core? Ensure nrfx state matches 'Zephyr' state (output) */
	static const nrfx_gpiote_output_config_t output_config = {
		.drive = NRF_GPIO_PIN_S0S1,
		.input_connect = NRF_GPIO_PIN_INPUT_DISCONNECT,
		.pull = NRF_GPIO_PIN_NOPULL,
	};
	const nrfx_gpiote_task_config_t task_config = {
		.task_ch = out_channel,
		.polarity = NRF_GPIOTE_POLARITY_TOGGLE,
		.init_val = NRF_GPIOTE_INITIAL_VALUE_LOW,
	};
	err = nrfx_gpiote_output_configure(follower, &output_config, &task_config);
	if (err != NRFX_SUCCESS) {
		LOG_ERR("nrfx_gpiote_output_configure error: 0x%08X", err);
		return -EINVAL;
	}

	nrfx_gpiote_trigger_enable(PDN_PIN, true);
	nrfx_gpiote_out_task_enable(follower);

	err = nrfx_gppi_channel_alloc(&ppi_channel);
	if (err != NRFX_SUCCESS) {
		LOG_ERR("nrfx_gppi_channel_alloc error: 0x%08X", err);
		return -EINVAL;
	}

	nrfx_gppi_channel_endpoints_setup(ppi_channel, nrfx_gpiote_in_event_addr_get(PDN_PIN),
					  nrfx_gpiote_out_task_addr_get(follower));

	nrfx_gppi_channels_enable(BIT(ppi_channel));

	return 0;
}

static int fem_gpio_init(const struct device *dev)
{
	ARG_UNUSED(dev);
	int r;

	/* unused output of FEM (input to SoC) */
	configure_disconnect(&fem_spi_miso);

	/* unused SPI */
	configure_output(&fem_spi_clk, 0);
	configure_output(&fem_spi_mosi, 0);

	/* signals that must follow PDN due to errata */
	r = setup_follower(CSN_PIN);
	LOG_INF("Setup follower for chip select: %d", r);

	if (!IS_ENABLED(CONFIG_LCZ_FEM_INTERNAL_ANTENNA)) {
		r = setup_follower(ANT_PIN);
		LOG_INF("Configured for external antenna variant: %d", r);
	}

	return 0;
}

SYS_INIT(fem_gpio_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
