/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <nrfx.h>
#include <esb.h>
#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <dk_buttons_and_leds.h>
#if defined(CONFIG_CLOCK_CONTROL_NRF2)
#include <hal/nrf_lrcconf.h>
#endif
#if NRF54L_ERRATA_20_PRESENT
#include <hal/nrf_power.h>
#endif /* NRF54L_ERRATA_20_PRESENT */
#if defined(NRF54LM20A_ENGA_XXAA)
#include <hal/nrf_clock.h>
#endif /* defined(NRF54LM20A_ENGA_XXAA) */
#if defined(CONFIG_ESB_SNIFFER)
#include <zephyr/sys/byteorder.h>
#include <SEGGER_RTT.h>
#include "sniffer.h"
#endif /* defined(CONFIG_ESB_SNIFFER) */

LOG_MODULE_REGISTER(esb_monitor, CONFIG_ESB_MONITOR_APP_LOG_LEVEL);

#if defined(CONFIG_ESB_SNIFFER)
static struct rtt_frame packet;
#else
static struct esb_payload rx_payload;
#endif /* defined(CONFIG_ESB_SNIFFER) */

#if defined(CONFIG_LED_ENABLE) && !defined(CONFIG_ESB_SNIFFER)
static void leds_update(uint8_t value)
{
	uint32_t leds_mask =
		(!(value % 8 > 0 && value % 8 <= 4) ? DK_LED1_MSK : 0) |
		(!(value % 8 > 1 && value % 8 <= 5) ? DK_LED2_MSK : 0) |
		(!(value % 8 > 2 && value % 8 <= 6) ? DK_LED3_MSK : 0) |
		(!(value % 8 > 3) ? DK_LED4_MSK : 0);

	dk_set_leds(leds_mask);
}
#endif /* defined(CONFIG_LED_ENABLE) && !defined(CONFIG_ESB_SNIFFER) */

#if defined(CONFIG_ESB_SNIFFER)
static void log_packet(void)
{
	uint32_t cycles, ms, len;

	cycles = k_cycle_get_32();
	ms = k_cyc_to_ms_floor32(cycles);
	/* Calculate remaing cycles */
	cycles = cycles - (ms * (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC / 1000));

	/* htonl */
	packet.us = sys_cpu_to_be32(k_cyc_to_us_floor32(cycles));
	packet.ms = sys_cpu_to_be32(ms);

	SEGGER_RTT_LOCK();
	len = SEGGER_RTT_WriteNoLock(CONFIG_ESB_SNIFFER_RTT_DATA_CHANNEL,
					&packet, sizeof(struct rtt_frame));
	SEGGER_RTT_UNLOCK();
	if (len < sizeof(struct rtt_frame)) {
		LOG_ERR("RTT failed to write, rtt buffer too small");
	}
}
#else
static void log_packet(void)
{
	LOG_INF("%u: length = %d, pipe = %d, rssi = %d, noack = %d, pid = %d",
				k_cyc_to_ms_floor32(k_cycle_get_32()), rx_payload.length,
				rx_payload.pipe, rx_payload.rssi, rx_payload.noack, rx_payload.pid);

	LOG_HEXDUMP_INF(rx_payload.data, rx_payload.length, "data:");
	LOG_INF("--------------------------------------------------------------");
}
#endif /* !defined(CONFIG_ESB_SNIFFER) */

void event_handler(struct esb_evt const *event)
{
	switch (event->evt_id) {
	case ESB_EVENT_RX_RECEIVED:
#if defined(CONFIG_ESB_SNIFFER)
		struct esb_payload *payload = &packet.payload;
#else
		struct esb_payload *payload = &rx_payload;
#endif /* !defined(CONFIG_ESB_SNIFFER) */
		int err;

		while ((err = esb_read_rx_payload(payload)) == 0) {
			log_packet();

#if defined(CONFIG_LED_ENABLE) && !defined(CONFIG_ESB_SNIFFER)
			leds_update(rx_payload.data[1]);
#endif /* defined(CONFIG_LED_ENABLE) && !defined(CONFIG_ESB_SNIFFER) */
		}
		if (err && err != -ENODATA) {
			LOG_ERR("Error while reading rx packet");
		}
		break;
	default:
		/* Should never happen */
		LOG_ERR("Unexpected ESB event");
	}
}

#if defined(CONFIG_CLOCK_CONTROL_NRF)
int clocks_start(void)
{
	int err;
	int res;
	struct onoff_manager *clk_mgr;
	struct onoff_client clk_cli;

	clk_mgr = z_nrf_clock_control_get_onoff(CLOCK_CONTROL_NRF_SUBSYS_HF);
	if (!clk_mgr) {
		LOG_ERR("Unable to get the Clock manager");
		return -ENXIO;
	}

	sys_notify_init_spinwait(&clk_cli.notify);

	err = onoff_request(clk_mgr, &clk_cli);
	if (err < 0) {
		LOG_ERR("Clock request failed: %d", err);
		return err;
	}

	do {
		err = sys_notify_fetch_result(&clk_cli.notify, &res);
		if (!err && res) {
			LOG_ERR("Clock could not be started: %d", res);
			return res;
		}
	} while (err);

#if NRF54L_ERRATA_20_PRESENT
	if (nrf54l_errata_20()) {
		nrf_power_task_trigger(NRF_POWER, NRF_POWER_TASK_CONSTLAT);
	}
#endif /* NRF54L_ERRATA_20_PRESENT */

#if defined(NRF54LM20A_ENGA_XXAA)
	/* MLTPAN-39 */
	nrf_clock_task_trigger(NRF_CLOCK, NRF_CLOCK_TASK_PLLSTART);
#endif

	LOG_DBG("HF clock started");
	return 0;
}

#elif defined(CONFIG_CLOCK_CONTROL_NRF2)

int clocks_start(void)
{
	int err;
	int res;
	const struct device *radio_clk_dev =
		DEVICE_DT_GET_OR_NULL(DT_CLOCKS_CTLR(DT_NODELABEL(radio)));
	struct onoff_client radio_cli;

	/** Keep radio domain powered all the time to reduce latency. */
	nrf_lrcconf_poweron_force_set(NRF_LRCCONF010, NRF_LRCCONF_POWER_DOMAIN_1, true);

	sys_notify_init_spinwait(&radio_cli.notify);

	err = nrf_clock_control_request(radio_clk_dev, NULL, &radio_cli);

	do {
		err = sys_notify_fetch_result(&radio_cli.notify, &res);
		if (!err && res) {
			LOG_ERR("Clock could not be started: %d", res);
			return res;
		}
	} while (err == -EAGAIN);

	/* HMPAN-84 */
	if (nrf54h_errata_84()) {
		nrf_lrcconf_clock_always_run_force_set(NRF_LRCCONF000, 0, true);
		nrf_lrcconf_task_trigger(NRF_LRCCONF000, NRF_LRCCONF_TASK_CLKSTART_0);
	}

	LOG_DBG("HF clock started");

	return 0;
}

#else
BUILD_ASSERT(false, "No Clock Control driver");
#endif /* defined(CONFIG_CLOCK_CONTROL_NRF2) */

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
	config.bitrate = ESB_BITRATE_2MBPS;
	config.mode = ESB_MODE_MONITOR;
	config.event_handler = event_handler;
	config.use_fast_ramp_up = true;

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

int main(void)
{
	int err;

	LOG_INF("Enhanced ShockBurst monitor sample");

	err = clocks_start();
	if (err) {
		return 0;
	}

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

#if defined(CONFIG_ESB_SNIFFER)
	err = sniffer_init();
	if (err) {
		LOG_ERR("Sniffer initialization failed, err %d", err);
		return 0;
	}
#else
	err = esb_start_rx();
	if (err) {
		LOG_ERR("ESB start monitor failed, err %d", err);
		return 0;
	}

	LOG_INF("Start receiving packets");
#endif /* !defined(CONFIG_ESB_SNIFFER) */

	/* return to idle thread */
	return 0;
}
