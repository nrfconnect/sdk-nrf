/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <stdbool.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/device_runtime.h>

#include "dtm_twowire_transport.h"

LOG_MODULE_REGISTER(dtm_tw_transport, CONFIG_DTM_TW_TRANSPORT_LOG_LEVEL);

/* The DTM maximum wait time in milliseconds for the UART command second byte.
 * Based on Bluetooth Core Specification v6.2, Vol 6, Part F, 3.5 Timing - command and event
 */
#define DTM_UART_SECOND_BYTE_MAX_DELAY_MS 5

#define DTM_UART DT_CHOSEN(zephyr_console)

/* UART Baudrate used to communicate with the DTM library. */
#define DTM_UART_BAUDRATE 19200

/* The UART poll cycle in micro seconds.
 * A baud rate of e.g. 19200 bits / second, and 8 data bits, 1 start/stop bit,
 * no flow control, give the time to transmit a byte:
 * 10 bits * 1/19200 = approx: 520 us.
 */
#define DTM_UART_POLL_CYCLE_US ((uint32_t) (10 * 1e6 / DTM_UART_BAUDRATE))

static const struct device *dtm_uart = DEVICE_DT_GET_OR_NULL(DTM_UART);

int dtm_tw_transport_init(void)
{
	if (!device_is_ready(dtm_uart)) {
		LOG_ERR("UART device not ready");
		return -EIO;
	}

	int ret;

#if defined(CONFIG_SOC_SERIES_NRF54H)
	/* Use pm_device_runtime_get() to keep the UART powered on the nRF54H20 */
	ret = pm_device_runtime_get(dtm_uart);

	if (ret < 0) {
		LOG_ERR("Failed to get DTM UART runtime PM: %d\n", ret);
		return ret;
	}
#endif /* defined(CONFIG_SOC_SERIES_NRF54H) */

	/* Configure UART */
	struct uart_config cfg;

	ret = uart_config_get(dtm_uart, &cfg);

	if (ret != 0) {
		return ret;
	}

	cfg.baudrate = DTM_UART_BAUDRATE;
	return uart_configure(dtm_uart, &cfg);
}

uint16_t dtm_tw_transport_read(void)
{
	bool is_msb_read = false;
	uint8_t rx_byte;
	uint16_t tw_packet = 0;
	int64_t msb_time_ms = 0;
	int err;

	for (;;) {
		k_sleep(K_USEC(DTM_UART_POLL_CYCLE_US));

		err = uart_poll_in(dtm_uart, &rx_byte);
		if (err) {
			if (err != -1) {
				LOG_ERR("UART polling error: %d", err);
			}

			/* Nothing read from the UART */
			continue;
		}

		if (!is_msb_read) {
			/* This is first byte of two-byte command. */
			is_msb_read = true;
			tw_packet = rx_byte << 8;
			msb_time_ms = k_uptime_get();

			/* Go back and wait for 2nd byte of command word. */
			continue;
		}

		/* This is the second byte read; combine it with the first and
		 * process command.
		 */
		if ((k_uptime_get() - msb_time_ms) >
				DTM_UART_SECOND_BYTE_MAX_DELAY_MS) {
			/* More than ~5mS after MSB: Drop old byte, take the
			 * new byte as MSB. The variable is_msb_read will
			 * remain true.
			 */
			tw_packet = rx_byte << 8;
			msb_time_ms = k_uptime_get();
			/* Go back and wait for 2nd byte of command word. */
			LOG_DBG("Received byte discarded");
			continue;
		} else {
			tw_packet |= rx_byte;
			LOG_INF("Received 0x%04x command", tw_packet);
			return tw_packet;
		}
	}
}

void dtm_tw_transport_write(uint16_t tw_packet)
{
	LOG_INF("Sending 0x%04x packet", tw_packet);

	uart_poll_out(dtm_uart, (tw_packet >> 8) & 0xFF);
	uart_poll_out(dtm_uart, tw_packet & 0xFF);
}
