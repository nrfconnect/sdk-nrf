/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <init.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr.h>

#if CONFIG_HW_CC310

#include "nrf_cc310_platform.h"

static int hw_cc310_init(struct device *dev) {
	int res;

	__ASSERT_NO_MSG(dev != NULL);

	/* Set the RTOS abort APIs */
	nrf_cc310_platform_abort_init();

	/* Set the RTOS mutex APIs */
	nrf_cc310_platform_mutex_init();

	/* Initialize the cc310 HW with or without RNG support */
#if CONFIG_ENTROPY_CC310
	res = nrf_cc310_platform_init();
#else
	res = nrf_cc310_platform_init_no_rng();
#endif
	return res;
}

DEVICE_INIT(hw_cc310, CONFIG_HW_CC310_NAME, hw_cc310_init,
	    NULL, NULL, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

#endif /* CONFIG_HW_CC310 */
