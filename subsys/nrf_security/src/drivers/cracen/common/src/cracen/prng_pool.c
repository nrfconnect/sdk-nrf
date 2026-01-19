/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stddef.h>
#include <stdint.h>
#include <cracen_psa.h>
#include <cracen_psa_ctr_drbg.h>
#include <cracen/statuscodes.h>
#include <security/cracen.h>
#include <zephyr/kernel.h>
#include <nrf_security_mutexes.h>

/* We want to avoid reserving excessive RAM and invoking
 * the PRNG too often. 32 was arbitrarily chosen here
 * and can be freely adjusted later when we test this
 * in practice.
 *
 * The CONFIG_SOC_NRF54H20_CPUSEC secure element only has 29kB of RAM
 * so we have a smaller pool for this platform.
 */
#ifdef CONFIG_SOC_NRF54H20_CPUSEC
#define PRNG_POOL_SIZE (8)
#else
#define PRNG_POOL_SIZE (32)
#endif

static uint32_t prng_pool[PRNG_POOL_SIZE];
static uint32_t prng_pool_remaining;


NRF_SECURITY_MUTEX_DEFINE(cracen_prng_pool_mutex);

int cracen_prng_value_from_pool(uint32_t *prng_value)
{
	int status = SX_OK;

	nrf_security_mutex_lock(cracen_prng_pool_mutex);

	if (prng_pool_remaining == 0) {
		psa_status_t psa_status =
			cracen_get_random(NULL, (uint8_t *)prng_pool, sizeof(prng_pool));
		if (psa_status != PSA_SUCCESS) {
			status = SX_ERR_UNKNOWN_ERROR;
			goto exit;
		}

		prng_pool_remaining = PRNG_POOL_SIZE;
	}

	*prng_value = prng_pool[prng_pool_remaining - 1];
	prng_pool_remaining--;

exit:
	nrf_security_mutex_unlock(cracen_prng_pool_mutex);
	return status;
}
