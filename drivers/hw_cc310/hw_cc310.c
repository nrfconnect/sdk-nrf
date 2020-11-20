/*
 * Copyright (c) 2019-2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <init.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr.h>
#include <irq.h>
#include <device.h>

#include <nrf_cc3xx_platform.h>

#if CONFIG_HW_CC3XX

static int hw_cc3xx_init(const struct device *dev)
{
	int res;

	__ASSERT_NO_MSG(dev != NULL);

	/* Set the RTOS abort APIs */
	nrf_cc3xx_platform_abort_init();

	/* Set the RTOS mutex APIs */
	nrf_cc3xx_platform_mutex_init();

	/* Initialize the cc3xx HW with or without RNG support */
#if CONFIG_ENTROPY_CC3XX
	res = nrf_cc3xx_platform_init();
#else
	res = nrf_cc3xx_platform_init_no_rng();
#endif
	return res;
}

DEVICE_AND_API_INIT(hw_cc3xx, CONFIG_HW_CC3XX_NAME, hw_cc3xx_init, NULL, NULL,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, NULL);

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
