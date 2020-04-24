/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <sys/__assert.h>
#include <random/rand32.h>
#include <logging/log.h>
#include <zboss_api.h>
#include "zb_nrf_crypto.h"
#include "nrf_ecb_driver.h"

#if !defined(CONFIG_ENTROPY_HAS_DRIVER)
#error Entropy driver required for secure random number support
#endif

LOG_MODULE_DECLARE(zboss_osif, CONFIG_ZBOSS_OSIF_LOG_LEVEL);

void zb_osif_rng_init(void)
{
}

zb_uint32_t zb_random_seed(void)
{
	zb_uint32_t rnd_val = 0;
	int err_code;

	err_code = sys_csrand_get(&rnd_val, sizeof(rnd_val));
	__ASSERT_NO_MSG(err_code == 0);
	return rnd_val;
}

void zb_osif_aes_init(void)
{
	nrf_ecb_driver_init();
}

void zb_osif_aes128_hw_encrypt(zb_uint8_t *key, zb_uint8_t *msg, zb_uint8_t *c)
{
	int err_code;

	if (!(c && msg && key)) {
		LOG_ERR("NULL argument passed");
		return;
	}
	nrf_ecb_driver_set_key(key);
	err_code = nrf_ecb_driver_crypt(c, msg);
	__ASSERT_NO_MSG(err_code == 0);
}
