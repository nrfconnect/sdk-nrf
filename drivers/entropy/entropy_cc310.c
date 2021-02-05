/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <init.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <zephyr.h>
#include <drivers/entropy.h>
#include <sys/util.h>

#if defined(CONFIG_SPM)
#include "secure_services.h"
#else
#include "nrf_cc3xx_platform_entropy.h"
#endif

static int entropy_cc3xx_rng_get_entropy(
	const struct device *dev,
	uint8_t *buffer,
	uint16_t length)
{
	int res = -EINVAL;
	size_t olen;

	__ASSERT_NO_MSG(dev != NULL);
	__ASSERT_NO_MSG(buffer != NULL);

#if defined(CONFIG_SPM)
	size_t offset = 0;
	size_t to_copy;
	uint8_t spm_buf[144]; /* 144 is the only length supported by
			       * spm_request_random_number.
			       */

	/** This is a call from a non-secure app that enables secure services,
	 *  in which case entropy is gathered by calling through SPM
	 */
	while (length > 0) {
		res = spm_request_random_number(spm_buf, sizeof(spm_buf),
						&olen);
		if (res < 0) {
			return res;
		}

		if (olen != sizeof(spm_buf)) {
			return -EINVAL;
		}

		to_copy = MIN(length, sizeof(spm_buf));

		memcpy(buffer + offset, spm_buf, to_copy);
		length -= to_copy;
		offset += to_copy;
	}

#else
	/** This is a call from a secure app, in which case entropy is
	 *  gathered using CC3xx HW using the
	 *  nrf_cc310_platform/nrf_cc312_platform library.
	 */
	res = nrf_cc3xx_platform_entropy_get(buffer, length, &olen);
	if (olen != length) {
		return -EINVAL;
	}
#endif

	return res;
}

static int entropy_cc3xx_rng_init(const struct device *dev)
{
	/* No initialization is required */
	(void)dev;

	return 0;
}

static const struct entropy_driver_api entropy_cc3xx_rng_api = {
	.get_entropy = entropy_cc3xx_rng_get_entropy
};

#if DT_NODE_HAS_STATUS(DT_NODELABEL(cryptocell), okay)
#define CRYPTOCELL_NODE_ID DT_NODELABEL(cryptocell)
#elif DT_NODE_HAS_STATUS(DT_NODELABEL(cryptocell_sw), okay)
#define CRYPTOCELL_NODE_ID DT_NODELABEL(cryptocell_sw)
#else
/*
 * TODO is there a better way to handle this?
 *
 * The problem is that when this driver is configured for use by
 * non-secure applications, calling through SPM leaves our application
 * devicetree without an actual cryptocell node, so we fall back on
 * cryptocell_sw. This works, but it's a bit hacky and requires an out
 * of tree zephyr patch.
 */
#error "No cryptocell or cryptocell_sw node labels in the devicetree"
#endif

DEVICE_DT_DEFINE(CRYPTOCELL_NODE_ID, entropy_cc3xx_rng_init,
		 device_pm_control_nop, NULL, NULL, POST_KERNEL,
		 CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &entropy_cc3xx_rng_api);
