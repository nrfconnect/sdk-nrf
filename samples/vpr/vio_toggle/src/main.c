/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <zephyr/sys/util.h>

#include <hal/nrf_vpr_csr.h>
#include <hal/nrf_vpr_csr_vio.h>

#define VIO_MASK BIT(CONFIG_APP_VIO_PIN)

/* Run exactly N nop instructions (N core cycles) inline. */
#define MULTIPLE_NOPS(n) __asm__ volatile(".rept %c0\n\tnop\n.endr\n" : : "i"(n))

/* Fixed in-loop overhead, in core cycles.*/
#define VIO_WRITE_CYCLES     1
#define LOOP_BRANCH_CYCLES   3
#define HIGH_OVERHEAD_CYCLES (VIO_WRITE_CYCLES)
#define LOW_OVERHEAD_CYCLES  (LOOP_BRANCH_CYCLES + VIO_WRITE_CYCLES)

BUILD_ASSERT(CONFIG_APP_PULSE_HIGH_CYCLES >= HIGH_OVERHEAD_CYCLES,
	     "APP_PULSE_HIGH_CYCLES must be >= the fixed HIGH overhead");
BUILD_ASSERT(CONFIG_APP_PULSE_LOW_CYCLES >= LOW_OVERHEAD_CYCLES,
	     "APP_PULSE_LOW_CYCLES must be >= the fixed LOW overhead");

int main(void)
{
	printf("VPR VIO sample on %s: toggling VIO index %d\n", CONFIG_BOARD_TARGET,
	       CONFIG_APP_VIO_PIN);

	/* Configure the VIO bit as an output, starting low. */
	nrf_vpr_csr_vio_dir_set(VIO_MASK);
	nrf_vpr_csr_vio_out_set(0);

	/* Prevent any interrupts to disturb the critical timing.*/
	unsigned int key = irq_lock();

	while (1) {
		/* High. */
		nrf_vpr_csr_vio_out_set(VIO_MASK);
		MULTIPLE_NOPS(CONFIG_APP_PULSE_HIGH_CYCLES - HIGH_OVERHEAD_CYCLES);

		/* Low. */
		nrf_vpr_csr_vio_out_set(0);
		MULTIPLE_NOPS(CONFIG_APP_PULSE_LOW_CYCLES - LOW_OVERHEAD_CYCLES);
	}

	return 0;
}
