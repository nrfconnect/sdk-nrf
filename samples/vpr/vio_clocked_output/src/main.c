/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include <hal/nrf_vpr_csr.h>
#include <hal/nrf_vpr_csr_vio.h>
#include <hal/nrf_vpr_csr_vtim.h>
#if defined(CONFIG_SOC_SERIES_NRF54L)
#include <hal/nrf_gpio.h>
#endif

#define TX_VIO_MASK BIT(CONFIG_APP_VIO_PIN)
#define BAUD_RATE   CONFIG_APP_UART_BAUD_RATE

/* 8N1 framing. */
#define UART_DATA_BITS 8

static void uart_send_byte(uint8_t byte)
{
	/* Prevent any interrupts to disturb the critical timing.*/
	unsigned int key = irq_lock();

	/* Configure counter 0 to load the top value once it hits 0. */
	nrf_vpr_csr_vtim_count_mode_set(0, NRF_VPR_CSR_VTIM_COUNT_RELOAD);

	/* Buffer the start bit and start counter 0 by setting it to a value larger than 0. */
	nrf_vpr_csr_vio_out_buffered_set(0);
	nrf_vpr_csr_vtim_simple_counter_set(0, 1);

	/* Buffer data bits, LSB first.
	 * Once counter 0 hits 0 the buffered bit is output.
	 * The next buffered set call will only return once the previous bit is output.
	 */
	for (int i = 0; i < UART_DATA_BITS; i++) {
		bool output_bit = (byte >> i) & 0x1;

		nrf_vpr_csr_vio_out_buffered_set(output_bit ? TX_VIO_MASK : 0);
	}

	/* Schedule the stop bit and wait for it to output. */
	nrf_vpr_csr_vio_out_buffered_set(TX_VIO_MASK);
	nrf_vpr_csr_vtim_simple_wait_set(0, false, 0);

	/* The stop bit is now output. Stop reloading the timer and wait for it to hit 0. */
	nrf_vpr_csr_vtim_count_mode_set(0, NRF_VPR_CSR_VTIM_COUNT_STOP);
	nrf_vpr_csr_vtim_simple_wait_set(0, false, 0);

	/* Stop bit has been send, we are now ready to go to sleep or send the next byte. */
	irq_unlock(key);
}

static void uart_send_string(const char *str)
{
	while (*str != '\0') {
		uart_send_byte((uint8_t)*str++);
	}
}

int main(void)
{
	static const char msg[] = "Hello world!\n";
	uint32_t bit_period = (SystemCoreClock + BAUD_RATE / 2) / BAUD_RATE;

	printf("VPR VIO clocked-output (UART TX) sample on %s\n", CONFIG_BOARD_TARGET);
	printf("Core clock: %u Hz, baud: %d, bit period: %u cycles\n", SystemCoreClock, BAUD_RATE,
	       bit_period);
	printf("Emulated UART TX on VIO index %d\n", CONFIG_APP_VIO_PIN);

#if defined(CONFIG_SOC_SERIES_NRF54L)
	/* On the nRF54L series the FLPR routes the pin to the VPR at runtime.
	 * On SoCs that route pins through the UICR periphconf this is done by the vpr_launcher
	 * devicetree overlay.
	 */
	nrf_gpio_pin_control_select(
		NRF_GPIO_PIN_MAP(CONFIG_APP_TX_GPIO_PORT, CONFIG_APP_TX_GPIO_PIN),
		NRF_GPIO_PIN_SEL_VPR);
#endif

	/* Enable the VPR real-time peripherals (VIO + VTIM). */
	nrf_vpr_csr_rtperiph_enable_set(true);

	/* Configure counter 0 by setting the top register to the bit period. */
	nrf_vpr_csr_vtim_simple_counter_top_set(0, bit_period - 1);

	/* Configure the TX pin as a VIO output, idle high. */
	nrf_vpr_csr_vio_out_set(TX_VIO_MASK);
	nrf_vpr_csr_vio_dir_set(TX_VIO_MASK);

	while (1) {
		uart_send_string(msg);

		k_msleep(1000);
	}

	return 0;
}
