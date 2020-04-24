/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <init.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <zephyr.h>
#include <drivers/entropy.h>

#if defined(CONFIG_SPM)
#include "secure_services.h"
#else
#include "nrf_cc310_platform_entropy.h"
#endif

static int entropy_cc310_rng_get_entropy(struct device *dev, u8_t *buffer,
					 u16_t length)
{
	int res = -EINVAL;
	size_t olen;

	__ASSERT_NO_MSG(dev != NULL);
	__ASSERT_NO_MSG(buffer != NULL);

#if defined(CONFIG_SPM)
	/** This is a call from a non-secure app that enables secure services,
	 *  in which case entropy is gathered by calling through SPM
	 */
	res = spm_request_random_number(buffer, length, &olen);
	if (olen != length) {
		return -EINVAL;
	}

#else
	/** This is a call from a secure app, in which case entropy is gathered
	 *  using CC310 HW using the nrf_cc310_platform library.
	 */
	res = nrf_cc310_platform_entropy_get(buffer, length, &olen);
	if (olen != length) {
		return -EINVAL;
	}
#endif

	return res;
}

static int entropy_cc310_rng_init(struct device *dev)
{
	/* No initialization is required */
	(void)dev;

	return 0;
}

static const struct entropy_driver_api entropy_cc310_rng_api = {
	.get_entropy = entropy_cc310_rng_get_entropy
};

DEVICE_AND_API_INIT(entropy_cc310_rng, CONFIG_ENTROPY_NAME,
		    &entropy_cc310_rng_init,
		    NULL,
		    NULL,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &entropy_cc310_rng_api);
