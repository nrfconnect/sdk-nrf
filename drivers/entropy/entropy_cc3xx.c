/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/init.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <zephyr/kernel.h>
#include <zephyr/drivers/entropy.h>
#include <zephyr/sys/util.h>

#if defined(CONFIG_BUILD_WITH_TFM)
#include <psa/crypto.h>
#include <psa/crypto_extra.h>
#include <tfm_ns_interface.h>
#else
#include "nrf_cc3xx_platform_ctr_drbg.h"
#endif

#define CTR_DRBG_MAX_REQUEST 1024

static int entropy_cc3xx_rng_get_entropy(const struct device *dev,
					 uint8_t *buffer, uint16_t length)
{
	__ASSERT_NO_MSG(dev != NULL);
	__ASSERT_NO_MSG(buffer != NULL);
	int err = EINVAL;

#if defined(CONFIG_BUILD_WITH_TFM)
	psa_status_t status = PSA_ERROR_GENERIC_ERROR;

	status = psa_generate_random(buffer, length);
	if (status == PSA_SUCCESS) {
		err = 0;
	}

	return err;
#else
	size_t olen;
	size_t offset = 0;
	size_t chunk_size = CTR_DRBG_MAX_REQUEST;
	/* This is a call from a secure app, in which case entropy is
	 * gathered using CC3xx HW using the CTR_DRBG features of the
	 * nrf_cc310_platform/nrf_cc312_platform library.
	 */
	while (offset < length) {
		if ((length - offset) < CTR_DRBG_MAX_REQUEST) {
			chunk_size = length - offset;
		}

		int ret = -1;

		/* This is a call from a secure app, in which case entropy is
		 * gathered using CC3xx HW using the CTR_DRBG features of the
		 * nrf_cc310_platform/nrf_cc312_platform library.
		 * When the given context is NULL, a global internal ctr_drbg
		 * context is being used.
		 */
		ret = nrf_cc3xx_platform_ctr_drbg_get(NULL, buffer + offset,
						      chunk_size, &olen);
		if (ret != 0) {
			return -EINVAL;
		}

		if (olen != chunk_size) {
			return -EINVAL;
		}

		offset += chunk_size;
	}

	if (offset == length) {
		err = 0;
	}

	return err;
#endif /* defined(CONFIG_BUILD_WITH_TFM) */
}

static int entropy_cc3xx_rng_init(const struct device *dev)
{
	(void)dev;

#if defined(CONFIG_BUILD_WITH_TFM)
	psa_status_t status;

	status = psa_crypto_init();
	if (status != PSA_SUCCESS) {
		return -EINVAL;
	}
#else
	int ret;

	/* When the given context is NULL, a global internal
	 * ctr_drbg context is being used.
	 */
	ret = nrf_cc3xx_platform_ctr_drbg_init(NULL, NULL, 0);
	if (ret != 0) {
		return -EINVAL;
	}
#endif

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
 * non-secure applications, calling through TF-M leaves our application
 * devicetree without an actual cryptocell node, so we fall back on
 * cryptocell_sw. This works, but it's a bit hacky and requires an out
 * of tree zephyr patch.
 */
#error "No cryptocell or cryptocell_sw node labels in the devicetree"
#endif

DEVICE_DT_DEFINE(CRYPTOCELL_NODE_ID, entropy_cc3xx_rng_init,
		 NULL, NULL, NULL, PRE_KERNEL_1,
		 CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &entropy_cc3xx_rng_api);
