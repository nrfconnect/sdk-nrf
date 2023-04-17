/*
 * Copyright (c) 2019-2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/init.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <zephyr/device.h>

#include <nrf_cc3xx_platform.h>

#if CONFIG_HW_CC3XX

static int hw_cc3xx_init_internal(void)
{

	int res;

	/* Initialize the cc3xx HW with or without RNG support */
#if CONFIG_ENTROPY_CC3XX
	res = nrf_cc3xx_platform_init();
#else
	res = nrf_cc3xx_platform_init_no_rng();
#endif

	return res;
}

static int hw_cc3xx_init(void)
{
	int res;

	/* Set the RTOS abort APIs */
	nrf_cc3xx_platform_abort_init();

	/* Set the RTOS mutex APIs */
	nrf_cc3xx_platform_mutex_init();

	/* Enable the hardware */
	res = hw_cc3xx_init_internal();
	return res;
}

/* Driver initalization done when mutex is not usable (pre kernel) */
SYS_INIT(hw_cc3xx_init_internal, PRE_KERNEL_1,
	 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

/* Driver initialization when mutex is usable (post kernel) */
SYS_INIT(hw_cc3xx_init, POST_KERNEL,
	 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

#endif /* CONFIG_HW_CC3XX */

#if CONFIG_HW_CC3XX_INTERRUPT

void hw_cc3XX_interrupt_init(void)
{
	IRQ_CONNECT(DT_ARM_CRYPTOCELL_310_ARM_CRYPTOCELL_310_IRQ_0,
		    DT_ARM_CRYPTOCELL_310_ARM_CRYPTOCELL_310_IRQ_0_PRIORITY,
		    CRYPTOCELL_IRQHandler, NULL, 0);
	irq_enable(DT_ARM_CRYPTOCELL_310_ARM_CRYPTOCELL_310_IRQ_0);
}

#endif /* CONFIG_HW_CC3XX_INTERRUPT */
