/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <esb.h>
#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/pm/device_runtime.h>
#include <dk_buttons_and_leds.h>

LOG_MODULE_REGISTER(esb_ptx, CONFIG_ESB_PTX_APP_LOG_LEVEL);

static bool ready = true;
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
		LOG_DBG("TX SUCCESS EVENT %u attempts", event->tx_attempts);
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
#if IS_ENABLED(CONFIG_ESB_MPSL_TIMESLOT)
	case ESB_EVENT_TIMESLOT_FAILED:
		LOG_ERR("Error in Timeslot handling");
		break;
#endif
	}
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
	if (IS_ENABLED(CONFIG_ESB_FAST_SWITCHING)) {
		config.use_fast_ramp_up = true;
	}

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

static void leds_update(uint8_t value)
{
	uint32_t leds_mask =
		(!(value % 8 > 0 && value % 8 <= 4) ? DK_LED1_MSK : 0) |
		(!(value % 8 > 1 && value % 8 <= 5) ? DK_LED2_MSK : 0) |
		(!(value % 8 > 2 && value % 8 <= 6) ? DK_LED3_MSK : 0) |
		(!(value % 8 > 3) ? DK_LED4_MSK : 0);

	dk_set_leds(leds_mask);
}

int main(void)
{
	int err;

	LOG_INF("Enhanced ShockBurst ptx sample");

#if defined(CONFIG_SOC_SERIES_NRF54H)
	const struct device *dtm_uart = DEVICE_DT_GET_OR_NULL(DT_CHOSEN(zephyr_console));

	if (dtm_uart != NULL) {
		int ret = pm_device_runtime_get(dtm_uart);

		if (ret < 0) {
			printk("Failed to get DTM UART runtime PM: %d\n", ret);
		}
	}
#endif /* defined(CONFIG_SOC_SERIES_NRF54H) */

	err = dk_leds_init();
	if (err) {
		LOG_ERR("LEDs initialization failed, err %d", err);
		return 0;
	}

	err = esb_initialize();
	if (err) {
		LOG_ERR("ESB initialization failed, err %d", err);
		return 0;
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
