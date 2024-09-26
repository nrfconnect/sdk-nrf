/**
 *
 * @file
 *
 * @copyright Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

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

#define MAX_BITS_PER_REQUEST (1 << 19)		 /* NIST.SP.800-90Ar1:Table 3 */
#define RESEED_INTERVAL	     ((uint64_t)1 << 48) /* 2^48 as per NIST spec */

/** @brief Magic number to signal that PRNG context is initialized */
#define CRACEN_PRNG_INITIALIZED (0xA5B4A5B4)

/*
 * This driver uses a global context and discards the context passed from the user. We do that
 * because we are not aware of a requirement for multiple PRNG contexts from the users of the
 * driver. PRNG initialization is slow and consumes a lot of power, even though we don't have clear
 * numbers on the speed and power consumption at the moment our educated guess is that these issues
 * will cause trouble in the future and so we simplify the driver here.
 */
static cracen_prng_context_t prng;

NRF_SECURITY_MUTEX_DEFINE(cracen_prng_context_mutex);

/*
 * @brief Internal function to enable TRNG and get entropy for initial seed and
 * reseed
 *
 * @return 0 on success, nonzero on failure.
 */
static int trng_get_seed(char *dst, int num_trng_bytes)
{
	int sx_err = SX_ERR_RESET_NEEDED;
	struct sx_trng trng;

	while (true) {
		switch (sx_err) {
		case SX_ERR_RESET_NEEDED:
			sx_err = sx_trng_open(&trng, NULL);
			if (sx_err != SX_OK) {
				return sx_err;
			}
		/* fallthrough */
		case SX_ERR_HW_PROCESSING:
			/* Generate random numbers */
			sx_err = sx_trng_get(&trng, dst, num_trng_bytes);
			if (sx_err == SX_ERR_RESET_NEEDED) {
				(void)sx_trng_close(&trng);
			}
			break;
		default:
			(void)sx_trng_close(&trng);
			return sx_err;
		}
	}
}

/**
 *  Implementation of the CTR_DRBG_Update process as described in NIST.SP.800-90Ar1
 *  with ctr_len equal to blocklen.
 */
static psa_status_t ctr_drbg_update(uint8_t *data)
{
	psa_status_t status = SX_OK;

	char temp[CRACEN_PRNG_ENTROPY_SIZE + CRACEN_PRNG_NONCE_SIZE] __aligned(
		CONFIG_DCACHE_LINE_SIZE);
	size_t temp_length = 0;

	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;

	psa_set_key_type(&attr, PSA_KEY_TYPE_AES);
	psa_set_key_bits(&attr, PSA_BYTES_TO_BITS(sizeof(prng.key)));
	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_ENCRYPT);

	while (temp_length < sizeof(temp)) {
		si_be_add(prng.V, SX_BLKCIPHER_AES_BLK_SZ, 1);
		_Static_assert((sizeof(temp) % SX_BLKCIPHER_AES_BLK_SZ) == 0);

		status = sx_blkcipher_ecb_simple(prng.key, sizeof(prng.key), prng.V, sizeof(prng.V),
						 temp + temp_length, SX_BLKCIPHER_AES_BLK_SZ);

		if (status != PSA_SUCCESS) {
			return status;
		}
		temp_length += SX_BLKCIPHER_AES_BLK_SZ;
	}

	if (data) {
		si_xorbytes(temp, data, sizeof(temp));
	}

	memcpy(prng.key, temp, sizeof(prng.key));
	memcpy(prng.V, temp + sizeof(prng.key), sizeof(prng.V));

	return status;
}

/* Instantiation of CTR_DRBG without derivation function. */
psa_status_t cracen_init_random(cracen_prng_context_t *context)
{
	(void)context;

	int sx_err = SX_ERR_CORRUPTION_DETECTED;

	char entropy[CRACEN_PRNG_ENTROPY_SIZE +
		     CRACEN_PRNG_NONCE_SIZE]; /* DRBG entropy + nonce buffer. */

	if (prng.initialized == CRACEN_PRNG_INITIALIZED) {
		return PSA_SUCCESS;
	}

	nrf_security_mutex_lock(&cracen_prng_context_mutex);
	safe_memset(&prng, sizeof(prng), 0, sizeof(prng));

	/* Get the entropy used to seed the DRBG */
	sx_err = trng_get_seed(entropy, sizeof(entropy));

	if (sx_err != SX_OK) {
		goto exit;
	}

	psa_status_t status = ctr_drbg_update(entropy);

	if (status != PSA_SUCCESS) {
		sx_err = SX_ERR_PLATFORM_ERROR;
		goto exit;
	}

	/* Erase the entropy seed from memory after usage to protect against leakage. */
	safe_memset(entropy, sizeof(entropy), 0, CRACEN_PRNG_ENTROPY_SIZE + CRACEN_PRNG_NONCE_SIZE);

	prng.reseed_counter = 1;
	prng.initialized = CRACEN_PRNG_INITIALIZED;

exit:
	nrf_security_mutex_unlock(&cracen_prng_context_mutex);

	return silex_statuscodes_to_psa(sx_err);
}

psa_status_t cracen_get_random(cracen_prng_context_t *context, uint8_t *output, size_t output_size)
{
	(void)context;

	psa_status_t status = PSA_SUCCESS;
	size_t len_left = output_size;

	if (output_size > 0 && output == NULL) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (output_size > PSA_BITS_TO_BYTES(MAX_BITS_PER_REQUEST)) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	nrf_security_mutex_lock(&cracen_prng_context_mutex);

	if (prng.reseed_counter == 0) {
		status = cracen_init_random(context);

		if (status != PSA_SUCCESS) {
			goto exit;
		}
	}

	if (prng.reseed_counter >= RESEED_INTERVAL) {
		char entropy[CRACEN_PRNG_ENTROPY_SIZE +
			     CRACEN_PRNG_NONCE_SIZE]; /* DRBG entropy + nonce buffer. */
		int sx_err = trng_get_seed(entropy, sizeof(entropy));

		if (sx_err) {
			status = PSA_ERROR_HARDWARE_FAILURE;
			goto exit;
		}
		status = ctr_drbg_update(entropy);
		if (status != PSA_SUCCESS) {
			goto exit;
		}
		safe_memset(entropy, sizeof(entropy), 0,
			    CRACEN_PRNG_ENTROPY_SIZE + CRACEN_PRNG_NONCE_SIZE);
	}

	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;

	psa_set_key_type(&attr, PSA_KEY_TYPE_AES);
	psa_set_key_bits(&attr, PSA_BYTES_TO_BITS(sizeof(prng.key)));
	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_ENCRYPT);

	if (status != PSA_SUCCESS) {
		return status;
	}

	while (len_left > 0) {
		size_t cur_len = MIN(len_left, SX_BLKCIPHER_AES_BLK_SZ);
		char temp[SX_BLKCIPHER_AES_BLK_SZ] __aligned(CONFIG_DCACHE_LINE_SIZE);

		si_be_add(prng.V, SX_BLKCIPHER_AES_BLK_SZ, 1);
		status = sx_blkcipher_ecb_simple(prng.key, sizeof(prng.key), prng.V, sizeof(prng.V),
						 temp, sizeof(temp));

		if (status != PSA_SUCCESS) {
			goto exit;
		}

		for (int i = 0; i < cur_len; i++) {
			output[i] = temp[i];
		}

		len_left -= cur_len;
		output += cur_len;
	}

	status = ctr_drbg_update(NULL);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	prng.reseed_counter += 1;

exit:
	nrf_security_mutex_unlock(&cracen_prng_context_mutex);
	return status;
}

psa_status_t cracen_free_random(cracen_prng_context_t *context)
{
	(void)context;
	return PSA_SUCCESS;
}
