/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>

#include "dtm.h"

int main(void)
{
	int err;
	const struct device *uart = DEVICE_DT_GET(DT_CHOSEN(ncs_dtm_uart));
	bool is_msb_read = false;
	uint8_t rx_byte;
	uint16_t dtm_cmd;
	uint16_t dtm_evt;
	int64_t msb_time;

	printk("Starting Direct Test Mode example\n");

	if (!device_is_ready(uart)) {
		printk("UART device not ready\n");
	}

	err = dtm_init();
	if (err) {
		printk("Error during DTM initialization: %d\n", err);
	}

	for (;;) {
		dtm_wait();

		err = uart_poll_in(uart, &rx_byte);
		if (err) {
			if (err != -1) {
				printk("UART polling error: %d\n", err);
			}

			/* Nothing read from the UART. */
			continue;
		}

		if (!is_msb_read) {
			/* This is first byte of two-byte command. */
			is_msb_read = true;
			dtm_cmd = rx_byte << 8;
			msb_time = k_uptime_get();

			/* Go back and wait for 2nd byte of command word. */
			continue;
		}

		/* This is the second byte read; combine it with the first and
		 * process command.
		 */
		if ((k_uptime_get() - msb_time) >
		    DTM_UART_SECOND_BYTE_MAX_DELAY) {
			/* More than ~5mS after msb: Drop old byte, take the
			 * new byte as MSB. The variable is_msb_read will
			 * remain true.
			 */
			dtm_cmd = rx_byte << 8;
			msb_time = k_uptime_get();
			/* Go back and wait for 2nd byte of command word. */
			continue;
		}

		/* 2-byte UART command received. */
		is_msb_read = false;
		dtm_cmd |= rx_byte;

		printk("Sending 0x%04X DTM command\n", dtm_cmd);

		if (dtm_cmd_put(dtm_cmd) != DTM_SUCCESS) {
			/* Extended error handling may be put here.
			 * Default behavior is to return the event on the UART;
			 * the event report will reflect any lack of success.
			 */
		}

		/* Retrieve result of the operation. This implementation will
		 * busy-loop for the duration of the byte transmissions on the
		 * UART.
		 */
		if (dtm_event_get(&dtm_evt)) {
			printk("Received 0x%04X DTM event\n", dtm_evt);

			/* Report command status on the UART. */

			/* Transmit MSB of the result. */
			uart_poll_out(uart, (dtm_evt >> 8) & 0xFF);

			/* Transmit LSB of the result. */
			uart_poll_out(uart, dtm_evt & 0xFF);
		}
	}
}
