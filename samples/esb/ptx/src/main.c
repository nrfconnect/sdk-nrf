/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#include <clock_control.h>
#include <gpio.h>
#include <irq.h>
#include <logging/log.h>
#include <nrf.h>
#include <nrf_esb.h>
#include <zephyr.h>
#include <zephyr/types.h>

LOG_MODULE_REGISTER(esb_ptx);

#define LED_ON 0
#define LED_OFF 1

static bool ready = true;
static struct device *led_port;
static struct nrf_esb_payload rx_payload;
static struct nrf_esb_payload tx_payload = NRF_ESB_CREATE_PAYLOAD(0,
	0x01, 0x00, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08);

#define _RADIO_SHORTS_COMMON                                                   \
	(RADIO_SHORTS_READY_START_Msk | RADIO_SHORTS_END_DISABLE_Msk |         \
	 RADIO_SHORTS_ADDRESS_RSSISTART_Msk |                                  \
	 RADIO_SHORTS_DISABLED_RSSISTOP_Msk)

void esb_event_handler(struct nrf_esb_evt const *event)
{
	ready = true;

	switch (event->evt_id) {
	case NRF_ESB_EVENT_TX_SUCCESS:
		LOG_DBG("TX SUCCESS EVENT");
		break;
	case NRF_ESB_EVENT_TX_FAILED:
		LOG_DBG("TX FAILED EVENT");
		break;
	case NRF_ESB_EVENT_RX_RECEIVED:
		while (nrf_esb_read_rx_payload(&rx_payload) == 0) {
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
	struct device *hfclk;

	hfclk = device_get_binding(DT_INST_0_NORDIC_NRF_CLOCK_LABEL "_16M");
	if (!hfclk) {
		LOG_ERR("HF Clock device not found!");
		return -EIO;
	}

	err = clock_control_on(hfclk, NULL);
	if (err && (err != -EINPROGRESS)) {
		LOG_ERR("HF clock start fail: %d", err);
		return err;
	}

	LOG_DBG("HF clock started");
	return 0;
}

int esb_init(void)
{
	int err;
	/* These are arbitrary default addresses. In end user products
	 * different addresses should be used for each set of devices.
	 */
	u8_t base_addr_0[4] = {0xE7, 0xE7, 0xE7, 0xE7};
	u8_t base_addr_1[4] = {0xC2, 0xC2, 0xC2, 0xC2};
	u8_t addr_prefix[8] = {0xE7, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8};

	struct nrf_esb_config config = NRF_ESB_DEFAULT_CONFIG;

	config.protocol = NRF_ESB_PROTOCOL_ESB_DPL;
	config.retransmit_delay = 600;
	config.bitrate = NRF_ESB_BITRATE_2MBPS;
	config.event_handler = esb_event_handler;
	config.mode = NRF_ESB_MODE_PTX;
	config.selective_auto_ack = true;

	err = nrf_esb_init(&config);

	if (err) {
		return err;
	}

	err = nrf_esb_set_base_address_0(base_addr_0);
	if (err) {
		return err;
	}

	err = nrf_esb_set_base_address_1(base_addr_1);
	if (err) {
		return err;
	}

	err = nrf_esb_set_prefixes(addr_prefix, ARRAY_SIZE(addr_prefix));
	if (err) {
		return err;
	}

	return 0;
}

static int leds_init(void)
{
	led_port = device_get_binding(DT_ALIAS_LED0_GPIOS_CONTROLLER);
	if (!led_port) {
		LOG_ERR("Could not bind to LED port %s",
			DT_ALIAS_LED0_GPIOS_CONTROLLER);
		return -EIO;
	}

	const u8_t pins[] = {DT_ALIAS_LED0_GPIOS_PIN, DT_ALIAS_LED1_GPIOS_PIN,
			     DT_ALIAS_LED2_GPIOS_PIN, DT_ALIAS_LED3_GPIOS_PIN};

	for (size_t i = 0; i < ARRAY_SIZE(pins); i++) {
		int err = gpio_pin_configure(led_port, pins[i], GPIO_DIR_OUT);

		if (err) {
			LOG_ERR("Unable to configure LED%u, err %d", i, err);
			led_port = NULL;
			return err;
		}
	}

	return 0;
}

static void leds_update(u8_t value)
{
	bool led0_status = !(value % 8 > 0 && value % 8 <= 4);
	bool led1_status = !(value % 8 > 1 && value % 8 <= 5);
	bool led2_status = !(value % 8 > 2 && value % 8 <= 6);
	bool led3_status = !(value % 8 > 3);

	if (led_port != NULL) {
		(void)gpio_write(led_port, GPIO_ACCESS_BY_PORT, 0,
				 led0_status << DT_ALIAS_LED0_GPIOS_PIN |
				 led1_status << DT_ALIAS_LED1_GPIOS_PIN |
				 led2_status << DT_ALIAS_LED2_GPIOS_PIN |
				 led3_status << DT_ALIAS_LED3_GPIOS_PIN);
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

	err = esb_init();
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
			nrf_esb_flush_tx();
			leds_update(tx_payload.data[1]);

			err = nrf_esb_write_payload(&tx_payload);
			if (err) {
				LOG_ERR("Payload write failed, err %d", err);
			}
			tx_payload.data[1]++;
		}
		k_sleep(100);
	}
}
