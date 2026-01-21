/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include "common.h"

static const struct device *counter =
	DEVICE_DT_GET(DT_PHANDLE(DT_COMPAT_GET_ANY_STATUS_OKAY(vnd_busy_sim), counter));
static struct test_data test_data;

static void next_alarm(const struct device *counter, struct counter_alarm_cfg *alarm_cfg)
{
	int err;
	int mul = IS_ENABLED(CONFIG_NO_OPTIMIZATIONS) ? 10 : 1;

	alarm_cfg->ticks = (50 + sys_rand32_get() % 300) * mul;

	err = counter_set_channel_alarm(counter, 0, alarm_cfg);
	__ASSERT_NO_MSG(err == 0);
}

static nrf_gpio_pin_pull_t pin_toggle(uint32_t pin, nrf_gpio_pin_pull_t pull)
{
	nrf_gpio_cfg(pin, NRF_GPIO_PIN_DIR_INPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, pull,
		     NRF_GPIO_PIN_S0S1, NRF_GPIO_PIN_NOSENSE);
	return (pull == NRF_GPIO_PIN_PULLUP) ? NRF_GPIO_PIN_PULLDOWN : NRF_GPIO_PIN_PULLUP;
}

void pins_toggle(int32_t tx_pin)
{
	static nrf_gpio_pin_pull_t pull = NRF_GPIO_PIN_PULLUP;
	static nrf_gpio_pin_pull_t tx_pull = NRF_GPIO_PIN_PULLUP;
	bool req_rdy_pin_toggle;
	bool tx_pin_toggle;

	if (tx_pin > 0) {
		uint32_t rnd = sys_rand32_get();

		req_rdy_pin_toggle = (rnd & 0x6) == 0;
		tx_pin_toggle = rnd & 0x1;
	} else {
		req_rdy_pin_toggle = true;
		tx_pin_toggle = false;
	}

	if (req_rdy_pin_toggle) {
		pull = pin_toggle(test_data.req_rdy_pin, pull);
	}

	if (tx_pin_toggle) {
		tx_pull = pin_toggle(tx_pin, tx_pull);
	}
}

void pins_to_default(bool use_req_pin, int32_t tx_pin)
{
	uint32_t pin = test_data.req_rdy_pin;
	NRF_GPIO_Type *reg = nrf_gpio_pin_port_decode(&pin);

	reg->PIN_CNF[pin] = test_data.req_rdy_cnf;
	if (tx_pin > 0) {
		nrf_gpio_cfg(tx_pin, NRF_GPIO_PIN_DIR_OUTPUT, NRF_GPIO_PIN_INPUT_DISCONNECT,
			     NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0S1, NRF_GPIO_PIN_NOSENSE);
	}
}

void counter_alarm_callback(const struct device *dev, uint8_t chan_id, uint32_t ticks,
			    void *user_data)
{
	struct test_data *data = (struct test_data *)user_data;

	pins_toggle(data->tx_pin);
	next_alarm(dev, &data->alarm_cfg);
}

void floating_pins_start(bool use_req_pin, int32_t tx_pin)
{
	uint32_t req_rdy_pin = use_req_pin ?
		DT_INST_PROP(0, req_pin) : DT_INST_PROP(0, rdy_pin);

	test_data.alarm_cfg.callback = counter_alarm_callback;
	test_data.alarm_cfg.flags = COUNTER_ALARM_CFG_EXPIRE_WHEN_LATE;
	test_data.alarm_cfg.user_data = (void *)&test_data;
	test_data.tx_pin = use_req_pin ? tx_pin : -1;
	test_data.req_rdy_pin = req_rdy_pin;

	NRF_GPIO_Type *reg = nrf_gpio_pin_port_decode(&req_rdy_pin);

	test_data.req_rdy_cnf = reg->PIN_CNF[req_rdy_pin];

	counter_start(counter);
	next_alarm(counter, &test_data.alarm_cfg);
}

void floating_pins_stop(bool use_req_pin, int32_t tx_pin)
{
	counter_cancel_channel_alarm(counter, 0);
	counter_stop(counter);
	pins_to_default(use_req_pin, tx_pin);
}
