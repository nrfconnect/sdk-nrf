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
#elif defined(CONFIG_BUILD_WITH_TFM)
#include <psa/crypto.h>
#include <psa/crypto_extra.h>
#include <tfm_ns_interface.h>
#else
#include "nrf_cc3xx_platform_ctr_drbg.h"

static nrf_cc3xx_platform_ctr_drbg_context_t ctr_drbg_ctx;
#endif

#define CTR_DRBG_MAX_REQUEST 1024

static int entropy_cc3xx_rng_get_entropy(
	const struct device *dev,
	uint8_t *buffer,
	uint16_t length)
{
	int res = -EINVAL;

	__ASSERT_NO_MSG(dev != NULL);
	__ASSERT_NO_MSG(buffer != NULL);


#if defined(CONFIG_BUILD_WITH_TFM)

	res = psa_generate_random(buffer, length);
	if (res != PSA_SUCCESS) {
		return -EINVAL;
	}

#else
	size_t olen;
	size_t offset = 0;
	size_t chunk_size = CTR_DRBG_MAX_REQUEST;
	/** This is a call from a secure app, in which case entropy is
	 *  gathered using CC3xx HW using the CTR_DRBG features of the
	 *  nrf_cc310_platform/nrf_cc312_platform library.
	 */
	while (offset < length) {

		if ((length - offset) < CTR_DRBG_MAX_REQUEST) {
			chunk_size = length - offset;
		}

		#if defined(CONFIG_SPM)
			/** This is a call from a non-secure app that
			 * enables secure services, in which case entropy
			 * is gathered by calling through SPM.
			 */
			res = spm_request_random_number(buffer + offset,
								chunk_size,
								&olen);
		#else
			/** This is a call from a secure app, in which
			 * case entropy is gathered using CC3xx HW
			 * using the CTR_DRBG features of the
			 * nrf_cc310_platform/nrf_cc312_platform library.
			 */
			res = nrf_cc3xx_platform_ctr_drbg_get(&ctr_drbg_ctx,
								buffer + offset,
								chunk_size,
								&olen);
		#endif

		if (olen != chunk_size) {
			return -EINVAL;
		}

		if (res != 0) {
			break;
		}

		offset += chunk_size;
	}
#endif

	return res;
}

static int entropy_cc3xx_rng_init(const struct device *dev)
{
	(void)dev;

	#if defined(CONFIG_BUILD_WITH_TFM)
		int ret = -1;

		ret = psa_crypto_init();
		if (ret != PSA_SUCCESS) {
			return -EINVAL;
		}

	#elif !defined(CONFIG_SPM)
		int ret = 0;

		ret = nrf_cc3xx_platform_ctr_drbg_init(&ctr_drbg_ctx, NULL, 0);
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
 * non-secure applications, calling through SPM leaves our application
 * devicetree without an actual cryptocell node, so we fall back on
 * cryptocell_sw. This works, but it's a bit hacky and requires an out
 * of tree zephyr patch.
 */
#error "No cryptocell or cryptocell_sw node labels in the devicetree"
#endif

DEVICE_DT_DEFINE(CRYPTOCELL_NODE_ID, entropy_cc3xx_rng_init,
		 NULL, NULL, NULL, PRE_KERNEL_1,
		 CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &entropy_cc3xx_rng_api);
