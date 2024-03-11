/** PSA cryptographic message random driver for Silex Insight offload hardware.
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
#include "cracen_psa_primitives.h"
#include <cracen/statuscodes.h>
#include <sicrypto/sicrypto.h>
#include <sicrypto/drbgctr.h>
#include <sxsymcrypt/trng.h>
#include <sxsymcrypt/aes.h>
#include <sxsymcrypt/keyref.h>
#include <zephyr/kernel.h>

/*
 * This driver uses a global context and discards the context passed from the user.
 * We do that because we are not aware of a requirement for multiple PRNG contexts
 * from the users of the driver. In addition the PRNG context consumes a lot of memory (~1Kb)
 * and want to avoid initializing multiple contexts without a clear requirement for this.
 * PRNG initialization is slow and consumes a lot of power, even though we don't have
 * clear numbers on the speed and power consumption at the moment our educated guess is
 * that these issues will cause trouble in the future and so we simplify the driver here.
 */
static cracen_prng_context_t prng;
K_MUTEX_DEFINE(cracen_prng_context_mutex);

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

/* @brief Internal function to reseed PRNG with entropy
 *
 * @return 0 on success, nonzero on failure.
 */
static int reseed_prng(struct sitask *const task, size_t entropy_len)
{
	int sx_err;
	char entropy[CRACEN_PRNG_ENTROPY_SIZE +
		     CRACEN_PRNG_NONCE_SIZE]; /* DRBG entropy + nonce buffer. */

	if (entropy_len > sizeof(entropy)) {
		return SX_ERR_TOO_BIG;
	}

	/* Get TRNG data for reseeding */
	sx_err = trng_get_seed(entropy, entropy_len);
	if (sx_err != SX_OK) {
		return sx_err;
	}

	/* Provide new seed to DRBG (Nonce not applicable) */
	si_task_consume(task, entropy, entropy_len);

	/* TODO: Add personalization string support when doing NCSDK-19963*/
	/* si_task_consume(task, pers, pers_len); */

	/* Run the DRBG reseeding */
	si_task_run(task);

	sx_err = si_task_wait(task);

	/* Erase the entropy seed from memory after usage to protect against
	 * leakage.
	 */
	safe_memset(entropy, sizeof(entropy), 0, entropy_len);

	return sx_err;
}

psa_status_t cracen_init_random(cracen_prng_context_t *context)
{
	(void)context;

	int sx_err = SX_ERR_CORRUPTION_DETECTED;

	struct sitask *const task = &prng.task;
	char entropy[CRACEN_PRNG_ENTROPY_SIZE +
		     CRACEN_PRNG_NONCE_SIZE]; /* DRBG entropy + nonce buffer. */

	k_mutex_lock(&cracen_prng_context_mutex, K_FOREVER);

	if (prng.initialized == CRACEN_PRNG_INITIALIZED) {
		sx_err = SX_OK;
		goto exit;
	}

	/* Get the entropy used to seed the DRBG */
	sx_err = trng_get_seed(entropy, sizeof(entropy));

	if (sx_err != SX_OK) {
		goto exit;
	}

	/* Create a task for DRBG with no environment */
	si_task_init(task, prng.workmem, CRACEN_PRNG_WORKMEM_SIZE);

	/* Configure DRBG with initial SEED value */
	si_prng_create_drbg_aes_ctr(task, CRACEN_PRNG_KEY_SIZE, entropy);

	/* TODO: Support personalization string when doing NCSDK-19963 */
	/* si_task_consume(task, pers, pers_len); */

	/* Instantiate the PRNG */
	si_task_run(task);

	sx_err = si_task_wait(task);

	/* Erase the entropy seed from memory after usage to protect against leakage. */
	safe_memset(entropy, sizeof(entropy), 0, CRACEN_PRNG_ENTROPY_SIZE + CRACEN_PRNG_NONCE_SIZE);

	if (sx_err == SX_ERR_READY) {
		/* Set a magic value to state that PRNG is initialized */
		prng.initialized = CRACEN_PRNG_INITIALIZED;
	}

exit:
	k_mutex_unlock(&cracen_prng_context_mutex);

	return silex_statuscodes_to_psa(sx_err);
}

psa_status_t cracen_get_random(cracen_prng_context_t *context, uint8_t *output, size_t output_size)
{
	(void)context;

	int sx_err = SX_OK;
	size_t len_left = output_size;

	struct sitask *const task = &prng.task;

	if (output_size > 0 && output == NULL) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (prng.initialized != CRACEN_PRNG_INITIALIZED) {
		psa_status_t status = cracen_init_random(context);

		if (status != PSA_SUCCESS) {
			return status;
		}
	}

	k_mutex_lock(&cracen_prng_context_mutex, K_FOREVER);

	while (len_left > 0) {
		/* Clamp to largest request size. */
		size_t cur_len = MIN(len_left, CRACEN_PRNG_MAX_REQUEST_SIZE);

		/* Produce PRNG data */
		si_task_produce(task, output, cur_len);
		sx_err = si_task_wait(task);

		switch (sx_err) {
		case SX_ERR_RESEED_NEEDED:
			/* Reseed using the TRNG (nonce not applicable) */
			sx_err = reseed_prng(task, CRACEN_PRNG_ENTROPY_SIZE);
			if (sx_err == SX_ERR_READY) {
				continue;
			}
			goto exit;
		case SX_ERR_READY:
			len_left -= cur_len;
			output += cur_len;
			break;
		default:
			goto exit;
		}
	}

exit:
	k_mutex_unlock(&cracen_prng_context_mutex);
	return silex_statuscodes_to_psa(sx_err);
}

psa_status_t cracen_free_random(cracen_prng_context_t *context)
{
	(void)context;
	return PSA_SUCCESS;
}
