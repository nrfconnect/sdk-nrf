/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <drivers/clock_control.h>
#include <drivers/clock_control/nrf_clock_control.h>
#include <drivers/gpio.h>
#include <irq.h>
#include <logging/log.h>
#include <nrf.h>
#include <esb.h>
#include <zephyr.h>
#include <zephyr/types.h>

LOG_MODULE_REGISTER(esb_ptx, CONFIG_ESB_PTX_APP_LOG_LEVEL);

#define LED_ON 0
#define LED_OFF 1

#define DT_DRV_COMPAT nordic_nrf_clock

static bool ready = true;
static const struct device *led_port;
static struct esb_payload rx_payload;
static struct esb_payload tx_payload = ESB_CREATE_PAYLOAD(0,
	0x01, 0x00, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08);

#define _RADIO_SHORTS_COMMON                                                   \
	(RADIO_SHORTS_READY_START_Msk | RADIO_SHORTS_END_DISABLE_Msk |         \
	 RADIO_SHORTS_ADDRESS_RSSISTART_Msk |                                  \
	 RADIO_SHORTS_DISABLED_RSSISTOP_Msk)

void event_handler(struct esb_evt const *event)
{
	ready = true;

	switch (event->evt_id) {
	case ESB_EVENT_TX_SUCCESS:
		LOG_DBG("TX SUCCESS EVENT");
		break;
	case ESB_EVENT_TX_FAILED:
		LOG_DBG("TX FAILED EVENT");
		break;
	case ESB_EVENT_RX_RECEIVED:
		while (esb_read_rx_payload(&rx_payload) == 0) {
			LOG_DBG("Packet received, len %d : "
				"0x%02x, 0x%02x, 0x%02x, 0x%02x, "
				"0x%02x, 0x%02x, 0x%02x, 0x%02x",
				rx_payload.length, rx_payload.data[0],
				rx_payload.data[1], rx_payload.data[2],
				rx_payload.data[3], rx_payload.data[4],
				rx_payload.data[5], rx_payload.data[6],
				rx_payload.data[7]);
		}
		break;
	}
}

int clocks_start(void)
{
	int err;
	const struct device *clk;

	clk = device_get_binding(DT_INST_LABEL(0));
	if (!clk) {
		LOG_ERR("Clock device not found!");
		return -EIO;
	}

	err = clock_control_on(clk, CLOCK_CONTROL_NRF_SUBSYS_HF);
	if (err && (err != -EINPROGRESS)) {
		LOG_ERR("HF clock start fail: %d", err);
		return err;
	}

	/* Block until clock is started.
	 */
	while (clock_control_get_status(clk, CLOCK_CONTROL_NRF_SUBSYS_HF) !=
		CLOCK_CONTROL_STATUS_ON) {

	}

	LOG_DBG("HF clock started");
	return 0;
}

int esb_initialize(void)
{
	int err;
	/* These are arbitrary default addresses. In end user products
	 * different addresses should be used for each set of devices.
	 */
	uint8_t base_addr_0[4] = {0xE7, 0xE7, 0xE7, 0xE7};
	uint8_t base_addr_1[4] = {0xC2, 0xC2, 0xC2, 0xC2};
	uint8_t addr_prefix[8] = {0xE7, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8};

	struct esb_config config = ESB_DEFAULT_CONFIG;

	config.protocol = ESB_PROTOCOL_ESB_DPL;
	config.retransmit_delay = 600;
	config.bitrate = ESB_BITRATE_2MBPS;
	config.event_handler = event_handler;
	config.mode = ESB_MODE_PTX;
	config.selective_auto_ack = true;

	err = esb_init(&config);

	if (err) {
		return err;
	}

	err = esb_set_base_address_0(base_addr_0);
	if (err) {
		return err;
	}

	err = esb_set_base_address_1(base_addr_1);
	if (err) {
		return err;
	}

	err = esb_set_prefixes(addr_prefix, ARRAY_SIZE(addr_prefix));
	if (err) {
		return err;
	}

	return 0;
}

static int leds_init(void)
{
	led_port = device_get_binding(DT_GPIO_LABEL(DT_ALIAS(led0), gpios));
	if (!led_port) {
		LOG_ERR("Could not bind to LED port %s",
			DT_GPIO_LABEL(DT_ALIAS(led0), gpios));
		return -EIO;
	}

	const uint8_t pins[] = {DT_GPIO_PIN(DT_ALIAS(led0), gpios),
			     DT_GPIO_PIN(DT_ALIAS(led1), gpios),
			     DT_GPIO_PIN(DT_ALIAS(led2), gpios),
			     DT_GPIO_PIN(DT_ALIAS(led3), gpios)};

	for (size_t i = 0; i < ARRAY_SIZE(pins); i++) {
		int err = gpio_pin_configure(led_port, pins[i], GPIO_OUTPUT);

		if (err) {
			LOG_ERR("Unable to configure LED%u, err %d", i, err);
			led_port = NULL;
			return err;
		}
	}

	return 0;
}

static void leds_update(uint8_t value)
{
	bool led0_status = !(value % 8 > 0 && value % 8 <= 4);
	bool led1_status = !(value % 8 > 1 && value % 8 <= 5);
	bool led2_status = !(value % 8 > 2 && value % 8 <= 6);
	bool led3_status = !(value % 8 > 3);

	gpio_port_pins_t mask =
		1 << DT_GPIO_PIN(DT_ALIAS(led0), gpios) |
		1 << DT_GPIO_PIN(DT_ALIAS(led1), gpios) |
		1 << DT_GPIO_PIN(DT_ALIAS(led2), gpios) |
		1 << DT_GPIO_PIN(DT_ALIAS(led3), gpios);

	gpio_port_value_t val =
		led0_status << DT_GPIO_PIN(DT_ALIAS(led0), gpios) |
		led1_status << DT_GPIO_PIN(DT_ALIAS(led1), gpios) |
		led2_status << DT_GPIO_PIN(DT_ALIAS(led2), gpios) |
		led3_status << DT_GPIO_PIN(DT_ALIAS(led3), gpios);

	if (led_port != NULL) {
		(void)gpio_port_set_masked_raw(led_port, mask, val);
	}
}

void main(void)
{
	int err;

	LOG_INF("Enhanced ShockBurst ptx sample");

	err = clocks_start();
	if (err) {
		return;
	}

	leds_init();

	err = esb_initialize();
	if (err) {
		LOG_ERR("ESB initialization failed, err %d", err);
		return;
	}

	LOG_INF("Initialization complete");
	LOG_INF("Sending test packet");

	tx_payload.noack = false;
	while (1) {
		if (ready) {
			ready = false;
			esb_flush_tx();
			leds_update(tx_payload.data[1]);

			err = esb_write_payload(&tx_payload);
			if (err) {
				LOG_ERR("Payload write failed, err %d", err);
			}
			tx_payload.data[1]++;
		}
		k_sleep(K_MSEC(100));
	}
}
