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
#include <hal/nrf_gpio.h>

/* Rounded bit period in clock ticks. */
#define BIT_PERIOD  (SystemCoreClock +  CONFIG_APP_UART_BAUD_RATE/ 2) / CONFIG_APP_UART_BAUD_RATE

#define TX_VIO_PIN  CONFIG_APP_VIO_PIN
#define TX_VIO_MASK BIT(TX_VIO_PIN)

#if !NRF_VPR_HAS_OUTMODE_SEL && TX_VIO_PIN != 0
#error "Only VIO 0 is supported for shift output on this SOC"
#endif

/* 8N1 framing: 1 start bit + 8 data bits + 1 stop bit. */
#define UART_DATA_BITS  8
#define UART_FRAME_BITS (1 + UART_DATA_BITS + 1)

/*
 * Copy byte into an 8N1 frame
 *   bit 0      : start bit (0)
 *   bits 1..8  : data bits (LSB first)
 *   bit 9      : stop bit (1)
 */
static inline uint32_t uart_frame(uint8_t byte)
{
	return (uint32_t)byte << 1 | BIT(UART_FRAME_BITS - 1);
}

static void uart_send_string(const char *str)
{
	/* Prevent kernel tick from messing things up. */
	unsigned int key = irq_lock();

	/*
	 * Set the configuration for the first byte.
	 * Shift mode with a single pin.
	 * This pin can only be chosen on some devices, for other devices it needs to be VIO 0.
	 */
	nrf_vpr_csr_vio_mode_out_t out_mode = {
		.mode = NRF_VPR_CSR_VIO_SHIFT_OUTB,
		.frame_width = 1,
#if NRF_VPR_HAS_OUTMODE_SEL
		.sel = TX_VIO_PIN,
#endif
	};

	nrf_vpr_csr_vio_mode_out_set(&out_mode);
	nrf_vpr_csr_vio_shift_cnt_out_set(UART_FRAME_BITS - 1);

	/**
	 * Set the configuration for all the other bytes.
	 * These buffered values are loaded when shift_count == 0 on the CNT0 event.
	 */
	nrf_vpr_csr_vio_shift_ctrl_t shift_ctrl = {
		.shift_count = UART_FRAME_BITS - 1,
		.out_mode = NRF_VPR_CSR_VIO_SHIFT_OUTB,
		.frame_width = 1,
#if NRF_VPR_HAS_OUTMODE_SEL
		.sel = TX_VIO_PIN,
#endif
	};

	nrf_vpr_csr_vio_shift_ctrl_buffered_set(&shift_ctrl);

	/**
	 * Buffer the first byte.
	 * Every CNT0 event (CNT0 hits 0):
	 *   - The right most bit will be output.
	 *   - The other bits are shifted right.
	 *   - Shift_count will be decreased by one.
	 */
	nrf_vpr_csr_vio_out_buffered_set(uart_frame((uint8_t)str[0]));

	/**
	 * Set counter to (re)load the bit period every time Shift_count hits 0.
	 * Start counter with value 1 so it hits 0 and outputs the first bit immediately.
	 */
	nrf_vpr_csr_vtim_simple_counter_top_set(0, BIT_PERIOD - 1);
	nrf_vpr_csr_vtim_count_mode_set(0, NRF_VPR_CSR_VTIM_COUNT_RELOAD);
	nrf_vpr_csr_vtim_simple_counter_set(0, 1);

	/*
	 * Timing critical output of the remaining bytes plus the termination.
	 *
	 * To allow for maximum baudrate (BIT_PERIOD == 1, for example 16 Mbit at 16 MHz clock)
	 * we need to use manually optimized assembly code as we only have 10 clock ticks for each
	 * byte and the termination.
	 *
	 * Equivalent C:
	 *	for (const char *p = &str[1]; *p != '\0'; p++) {
	 *		// Set next byte (blocks until Shift_count == 0)
	 *		nrf_vpr_csr_vio_out_buffered_set(uart_frame((uint8_t)*p));
	 *	}
	 *
	 *	// Stop shifting after the last bit.
	 *	nrf_vpr_csr_vio_shift_ctrl_t shift_ctrl_end = {
	 *		.out_mode = NRF_VPR_CSR_VIO_SHIFT_NONE,
	 *		.frame_width = 1,
	 *		.sel = TX_VIO_PIN,
	 *	};
	 *
	 *	nrf_vpr_csr_vio_shift_ctrl_buffered_set(&shift_ctrl_end);
	 *
	 *	// Wait for shift_count to hit 0 and leave output high.
	 *	nrf_vpr_csr_vio_out_buffered_set(1);
	 */
	__asm__ volatile(
		"loop%=:\n"
		"	addi	%[p], %[p], 1\n"		/* p++; */
		"	lbu	t0, 0(%[p])\n"			/* frame = *p */
		"	beqz	t0, exit%=\n"			/* if (frame == 0) goto: exit; */
		"	slli	t0, t0, 1\n"			/* frame = frame << 1; */
		"	ori	t0, t0, %[stopbit]\n"		/* frame |= 0x200; */
		"	csrw	%[outb], t0\n"			/* OUTB = frame; (stalls) */
		"	j	loop%=\n"			/* goto: loop; */
		"exit%=:\n"
		"	csrw	%[sctrl], %[shift_ctrl_end]\n"	/* SHIFTCTRLB.MODE = NoShifting; */
		"	csrwi	%[outb], 1\n"			/* OUTB = 1; (stalls) */
		: [p] "+r"(str)
		: [outb] "i"(VPRCSR_NORDIC_OUTB),
		  [sctrl] "i"(VPRCSR_NORDIC_SHIFTCTRLB),
		  [stopbit] "i"(BIT(UART_FRAME_BITS - 1)),
		  [shift_ctrl_end] "r"(
	NRF_VPR_CSR_VIO_SHIFT_NONE << VPRCSR_NORDIC_SHIFTCTRLB_OUTMODEB_FRAMEWIDTH_Pos |
	1 << VPRCSR_NORDIC_SHIFTCTRLB_OUTMODEB_FRAMEWIDTH_Pos
#if NRF_VPR_HAS_OUTMODE_SEL
	| TX_VIO_PIN << VPRCSR_NORDIC_SHIFTCTRLB_OUTMODEB_SEL_Pos
#endif
				      )
		: "t0", "memory");

	/* Stop the clock. */
	nrf_vpr_csr_vtim_count_mode_set(0, NRF_VPR_CSR_VTIM_COUNT_STOP);

	/* Resume kernel tick so we can use sleep. */
	irq_unlock(key);
}

int main(void)
{
	static const char msg[] = "Hello world!\n";

	printf("VPR VIO shift-output (UART TX) sample on %s\n",	CONFIG_BOARD_TARGET);
	printf("Core clock: %u Hz, baud: %d, bit period: %u cycles\n",
		SystemCoreClock, CONFIG_APP_UART_BAUD_RATE, BIT_PERIOD);
	printf("Emulated UART TX on VIO index %d\n", TX_VIO_PIN);

#if NRF_GPIO_HAS_SEL
	/**
	 * Assign the GPIO pin to the VPR core.
	 * If this is not supported it needs to be done using UICR by configuring the pin in
	 * the devicetree of the app core image.
	 */
	nrf_gpio_pin_control_select(
		NRF_GPIO_PIN_MAP(CONFIG_APP_TX_GPIO_PORT, CONFIG_APP_TX_GPIO_PIN),
		NRF_GPIO_PIN_SEL_VPR);
#endif

	/* Enable the VPR real-time peripherals (VIO + VTIM). */
	nrf_vpr_csr_rtperiph_enable_set(true);

	/* Configure the TX pin as a VIO output, start high. */
	nrf_vpr_csr_vio_out_set(TX_VIO_MASK);
	nrf_vpr_csr_vio_dir_set(TX_VIO_MASK);

	while (1) {
		uart_send_string(msg);

		k_msleep(1000);
	}

	return 0;
}
