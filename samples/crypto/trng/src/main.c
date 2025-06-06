/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>
#include <stdio.h>
#include <stdlib.h>
#include <psa/crypto.h>
#include <psa/crypto_extra.h>

#include <psa/crypto.h>
#include <psa/crypto_values.h>
#include "common.h"
#include "cracen_psa.h"
#include "cracen_psa_primitives.h"
#include <sxsymcrypt/blkcipher.h>
#include <cracen/statuscodes.h>
#include <security/cracen.h>
#include <sicrypto/sicrypto.h>
#include <sicrypto/util.h>
#include <sxsymcrypt/trng.h>
#include <sxsymcrypt/aes.h>
#include <sxsymcrypt/keyref.h>
#include <zephyr/kernel.h>
#include <nrf_security_mutexes.h>

#include <hal/nrf_cracen.h>
#include <security/cracen.h>
#include <nrf_security_mutexes.h>

#define APP_SUCCESS		(0)
#define APP_ERROR		(-1)
#define APP_SUCCESS_MESSAGE "Example finished successfully!"
#define APP_ERROR_MESSAGE "Example exited with error!"

char dst[1024];

#define PRINT_HEX(p_label, p_text, len)\
	({\
		LOG_INF("---- %s (len: %u): ----", p_label, len);\
		LOG_HEXDUMP_INF(p_text, len, "Content:");\
		LOG_INF("---- %s end  ----", p_label);\
	})

LOG_MODULE_REGISTER(arbitraryname, LOG_LEVEL_DBG);\

int crypto_init(void)
{
	psa_status_t status;

	/* Initialize PSA Crypto */
	status = psa_crypto_init();
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_crypto_init failed! (Error: %d)", status);
		return APP_ERROR;
	}

	return APP_SUCCESS;
}

int main3(void)
{
	int status;
	int num_trng_bytes = 16;

	int sx_err = SX_ERR_RESET_NEEDED;
	struct sx_trng trng;
	int i = 0;
	LOG_INF("starting trng test");
	while (true) {
		LOG_INF("run number %d", i++);
		switch (sx_err) {
		case SX_ERR_RESET_NEEDED:
			sx_err = sx_trng_open(&trng, NULL);
			if (sx_err != SX_OK) {
				return sx_err;
			}
			LOG_INF("open sx_err: %d", sx_err);

		/* fallthrough */
		case SX_ERR_HW_PROCESSING:
			/* Generate random numbers */
			sx_err = sx_trng_get(&trng, dst, num_trng_bytes);
			if (sx_err == SX_ERR_RESET_NEEDED) {
				(void)sx_trng_close(&trng);
			}
			LOG_INF("get sx_err: %d", sx_err);

			break;
		default:
			(void)sx_trng_close(&trng);
			LOG_INF("close sx_err: %d", sx_err);
			sx_err = SX_ERR_RESET_NEEDED;
		}
	}
	LOG_INF("sx_err: %d", sx_err);

	return APP_SUCCESS;
}

int main(void)
{
	int status;

	int sx_err = SX_ERR_RESET_NEEDED;
	struct sx_trng trng;
	int i = 0;
	status = crypto_init();
	LOG_INF("starting trng test");

	while (true) {
		status = psa_generate_random(dst, 128);
		printk("status is %d\n", status);
	}

	return APP_SUCCESS;
}
