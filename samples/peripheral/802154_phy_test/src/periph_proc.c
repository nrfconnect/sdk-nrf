/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
/* Purpose: Periphery configuring and processing */

#include <assert.h>
#include <stdint.h>

#include <zephyr/random/random.h>

#include "periph_proc.h"

#include <nrfx_timer.h>
#include <nrfx.h>
#include <hal/nrf_gpio.h>
#include <platform/nrf_802154_temperature.h>

#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>

#include <dk_buttons_and_leds.h>

#if IS_ENABLED(CONFIG_PTT_CLK_OUT)
#include <nrfx_gpiote.h>
#include <gpiote_nrfx.h>
#endif /* IS_ENABLED(CONFIG_PTT_CLK_OUT) */

#if defined(CONFIG_PTT_CACHE_MGMT)
#include <nrfx_nvmc.h>
#endif

#if defined(CONFIG_PTT_POWER_MGMT)
#include <nrfx_power.h>
#endif

#if IS_ENABLED(CONFIG_PTT_CLK_OUT)
#include <helpers/nrfx_gppi.h>
#endif /* IS_ENABLED(CONFIG_PTT_CLK_OUT) */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(periph);

#if IS_ENABLED(CONFIG_PTT_CLK_OUT)
/* Timer instance used for HFCLK output */
#if defined(NRF52_SERIES) || defined(NRF53_SERIES)
#define PTT_CLK_TIMER 2
#elif defined(NRF54L_SERIES)
#define PTT_CLK_TIMER 20
#endif

static nrfx_timer_t clk_timer = NRFX_TIMER_INSTANCE(NRF_TIMER_INST_GET(PTT_CLK_TIMER));

#define GPIOTE_NODE(gpio_node) DT_PHANDLE(gpio_node, gpiote_instance)
#define GPIOTE_INST_AND_COMMA(gpio_node) [DT_PROP(gpio_node, port)] =	 \
	COND_CODE_1(DT_NODE_HAS_PROP(gpio_node, gpiote_instance),	 \
		    (&GPIOTE_NRFX_INST_BY_NODE(GPIOTE_NODE(gpio_node))), \
		    (NULL)),

static nrfx_gpiote_t * const gpiote_inst[GPIO_COUNT] = {
	DT_FOREACH_STATUS_OKAY(nordic_nrf_gpio, GPIOTE_INST_AND_COMMA)
};

#define NRF_GPIOTE_FOR_GPIO(idx)  gpiote_inst[idx]
#define NRF_GPIOTE_FOR_PSEL(psel) gpiote_inst[NRF_PIN_NUMBER_TO_PORT(psel)]

#endif /* IS_ENABLED(CONFIG_PTT_CLK_OUT) */

#define CLOCK_NODE DT_INST(0, nordic_nrf_clock)

static const struct device *gpio_port0_dev = DEVICE_DT_GET(DT_NODELABEL(gpio0));

/* Check if the system has GPIO port 1 */
#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpio1), okay)
static const struct device *gpio_port1_dev = DEVICE_DT_GET(DT_NODELABEL(gpio1));
#endif

#if IS_ENABLED(CONFIG_PTT_CLK_OUT)
nrfx_gppi_handle_t ppi_handle;
uint8_t task_channel;
#endif /* IS_ENABLED(CONFIG_PTT_CLK_OUT) */

#if IS_ENABLED(CONFIG_PTT_CLK_OUT)
static void clk_timer_handler(nrf_timer_event_t event_type, void *context)
{
	/* do nothing */
}
#endif

void periph_init(void)
{
	int ret;

#if IS_ENABLED(CONFIG_PTT_CLK_OUT)
	int err_code;

	uint32_t base_frequency = NRF_TIMER_BASE_FREQUENCY_GET(clk_timer.p_reg);
	nrfx_timer_config_t clk_timer_cfg = NRFX_TIMER_DEFAULT_CONFIG(base_frequency);

	err_code = nrfx_timer_init(&clk_timer, &clk_timer_cfg, clk_timer_handler);
	__ASSERT_NO_MSG(err_code == 0);

	for (int i = 0; i < GPIO_COUNT; ++i) {
		nrfx_gpiote_t *gpiote = NRF_GPIOTE_FOR_GPIO(i);

		if (gpiote == NULL) {
			continue;
		}

		if (!nrfx_gpiote_init_check(gpiote)) {
			err_code = nrfx_gpiote_init(gpiote, 0);
			__ASSERT_NO_MSG(err_code == 0);
		}
	}
#endif

	ret = dk_leds_init();
	assert(ret == 0);

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpio0), okay)
	assert(device_is_ready(gpio_port0_dev));
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpio1), okay)
	assert(device_is_ready(gpio_port1_dev));
#endif
}

uint32_t ptt_random_get_ext(void)
{
	return sys_rand32_get();
}

bool ptt_clk_out_ext(uint8_t pin, bool mode)
{
#if IS_ENABLED(CONFIG_PTT_CLK_OUT)
	uint32_t compare_evt_addr;
	uint32_t tep;
	int err;
	nrfx_gpiote_t *gpiote = NRF_GPIOTE_FOR_PSEL(pin);

	if (!nrf_gpio_pin_present_check(pin) || (gpiote == NULL)) {
		return false;
	}

	if (mode) {
		nrfx_timer_extended_compare(&clk_timer, (nrf_timer_cc_channel_t)0, 1,
					    NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK, false);

		err = nrfx_gpiote_channel_alloc(gpiote, &task_channel);
		if (err != 0) {
			LOG_ERR("nrfx_gpiote_channel_alloc error: %d", err);
			return false;
		}

		nrfx_gpiote_output_config_t config = NRFX_GPIOTE_DEFAULT_OUTPUT_CONFIG;
		nrfx_gpiote_task_config_t const out_config = {
			.task_ch = task_channel,
			.polarity = NRF_GPIOTE_POLARITY_TOGGLE,
			.init_val = NRF_GPIOTE_INITIAL_VALUE_LOW
		};

		/* Initialize output pin. SET task will turn the LED on,
		 * CLR will turn it off and OUT will toggle it.
		 */
		err = nrfx_gpiote_output_configure(gpiote, pin, &config, &out_config);
		if (err != 0) {
			LOG_ERR("nrfx_gpiote_output_configure error: %d", err);
			return false;
		}

		compare_evt_addr =
			nrfx_timer_event_address_get(&clk_timer, NRF_TIMER_EVENT_COMPARE0);
		tep = nrfx_gpiote_out_task_address_get(gpiote, pin);
		nrfx_gpiote_out_task_enable(gpiote, pin);

		/* Allocate a (D)PPI channel. */
		err = nrfx_gppi_conn_alloc(compare_evt_addr, tep, &ppi_handle);
		if (err < 0) {
			LOG_ERR("(D)PPI channel allocation error: %08x", err);
			return false;
		}

		/* Enable (D)PPI channel. */
		nrfx_gppi_conn_enable(ppi_handle);

		nrfx_timer_enable(&clk_timer);
	} else {
		nrfx_timer_disable(&clk_timer);
		nrfx_gpiote_out_task_disable(gpiote, pin);

		compare_evt_addr =
			nrfx_timer_event_address_get(&clk_timer, NRF_TIMER_EVENT_COMPARE0);
		tep = nrfx_gpiote_out_task_address_get(gpiote, pin);
		nrfx_gppi_conn_disable(ppi_handle);
		nrfx_gppi_conn_free(compare_evt_addr, tep, ppi_handle);

		nrfx_gpiote_pin_uninit(gpiote, pin);
		err = nrfx_gpiote_channel_free(gpiote, task_channel);

		if (err != 0) {
			LOG_ERR("Failed to disable GPIOTE channel, error: %d", err);
			return false;
		}
	}
#endif /* IS_ENABLED(CONFIG_PTT_CLK_OUT) */

	return true;
}

static const struct device *get_pin_port(uint32_t *pin)
{
	switch (nrf_gpio_pin_port_number_extract(pin)) {
#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpio0), okay)
	case 0:
		return gpio_port0_dev;
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpio1), okay)
	case 1:
		return gpio_port1_dev;
#endif
	default:
		return NULL;
	}
}

bool ptt_set_gpio_ext(uint8_t pin, uint8_t value)
{
	int ret;
	const struct device *port;
	uint32_t pin_nr = pin;

	if (nrf_gpio_pin_present_check(pin_nr)) {
		port = get_pin_port(&pin_nr);
		if (port == NULL) {
			return false;
		}

		ret = gpio_pin_configure(port, pin_nr, GPIO_OUTPUT);

		if (ret == 0) {
			ret = gpio_pin_set(port, pin_nr, value);
			if (ret == 0) {
				return true;
			}
		}
	}

	return false;
}

bool ptt_get_gpio_ext(uint8_t pin, uint8_t *value)
{
	int ret;
	const struct device *port;
	uint32_t pin_nr = pin;

	if (nrf_gpio_pin_present_check(pin_nr)) {
		port = get_pin_port(&pin_nr);
		if (port == NULL) {
			return false;
		}

		ret = gpio_pin_configure(port, pin_nr, GPIO_INPUT);

		if (ret == 0) {
			*value = gpio_pin_get(port, pin_nr);
			return true;
		}
	}

	return false;
}

void ptt_set_dcdc_ext(bool enable)
{
#if IS_ENABLED(CONFIG_PTT_POWER_MGMT)
#if NRF_POWER_HAS_DCDCEN
	nrf_power_dcdcen_set(NRF_POWER, enable);
#endif

#if NRF_POWER_HAS_DCDCEN_VDDH
	nrf_power_dcdcen_vddh_set(NRF_POWER, enable);
#endif

#if !NRF_POWER_HAS_DCDCEN && !NRF_POWER_HAS_DCDCEN_VDDH
#pragma message "DC-DC related commands have no effect!"
#endif
#endif /* IS_ENABLED(CONFIG_PTT_POWER_MGMT) */
}

bool ptt_get_dcdc_ext(void)
{
#if NRF_POWER_HAS_DCDCEN && NRF_POWER_HAS_DCDCEN_VDDH
	return (nrf_power_dcdcen_get(NRF_POWER) || nrf_power_dcdcen_vddh_get(NRF_POWER));
#elif NRF_POWER_HAS_DCDCEN
	return nrf_power_dcdcen_get(NRF_POWER);
#elif NRF_POWER_HAS_DCDCEN_VDDH
	return nrf_power_dcdcen_vddh_get(NRF_POWER);
#else
	return false;
#endif
}

void ptt_set_icache_ext(bool enable)
{
#if defined(CONFIG_PTT_CACHE_MGMT)
	if (enable) {
		nrfx_nvmc_icache_enable();
	} else {
		nrfx_nvmc_icache_disable();
	}
#endif
}

bool ptt_get_icache_ext(void)
{
#if defined(CONFIG_PTT_CACHE_MGMT)
	return nrf_nvmc_icache_enable_check(NRF_NVMC);
#else
	return false;
#endif
}

bool ptt_get_temp_ext(int32_t *temp)
{
	if (temp == NULL) {
		return false;
	}

	*temp = nrf_802154_temperature_get();

	return true;
}

void ptt_ctrl_led_indication_on_ext(void)
{
	dk_set_led_on(DK_LED1);
}

void ptt_ctrl_led_indication_off_ext(void)
{
	dk_set_led_off(DK_LED1);
}
