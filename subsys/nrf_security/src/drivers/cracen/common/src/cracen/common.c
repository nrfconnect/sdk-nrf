/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <cracen/common.h>

#include <cracen/cracen_kmu.h>
#include <cracen_psa_ctr_drbg.h>
#include <hal/nrf_cracen.h>
#include <cracen/lib_kmu.h>
#include <cracen/mem_helpers.h>
#include <cracen/statuscodes.h>
#include <silexpk/core.h>
#include <silexpk/ec_curves.h>
#include <silexpk/ik.h>
#include <stddef.h>
#include <sxsymcrypt/hash.h>
#include <sxsymcrypt/hashdefs.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <psa/nrf_platform_key_ids.h>

LOG_MODULE_DECLARE(cracen, CONFIG_CRACEN_LOG_LEVEL);

#define NOT_ENABLED_HASH_ALG (0)

psa_status_t silex_statuscodes_to_psa(int sx_status)
{
	if (sx_status != SX_OK) {
		LOG_DBG("SX_ERR %d", sx_status);
	}

	switch (sx_status) {
	case SX_OK:
	case SX_ERR_READY:
		return PSA_SUCCESS;

	case SX_ERR_INCOMPATIBLE_HW:
		return PSA_ERROR_NOT_SUPPORTED;

	case SX_ERR_FEED_AFTER_DATA:
	case SX_ERR_TOO_BIG:
		return PSA_ERROR_NOT_PERMITTED;

	case SX_ERR_OUTPUT_BUFFER_TOO_SMALL:
		return PSA_ERROR_BUFFER_TOO_SMALL;

	case SX_ERR_DMA_FAILED:
	case SX_ERR_RETRY:
		return PSA_ERROR_HARDWARE_FAILURE;

	case SX_ERR_UNINITIALIZED_OBJ:
		return PSA_ERROR_BAD_STATE;

	case SX_ERR_INVALID_TAG:
	case SX_ERR_INVALID_SIGNATURE:
	case SX_ERR_OUT_OF_RANGE:
		return PSA_ERROR_INVALID_SIGNATURE;

	case SX_ERR_INVALID_REQ_SZ:
		return PSA_ERROR_DATA_INVALID;

	case SX_ERR_UNSUPPORTED_HASH_ALG:
		return PSA_ERROR_NOT_SUPPORTED;

	case SX_ERR_WORKMEM_BUFFER_TOO_SMALL:
		return PSA_ERROR_INSUFFICIENT_MEMORY;

	case SX_ERR_INSUFFICIENT_ENTROPY:
	case SX_ERR_TOO_MANY_ATTEMPTS:
		return PSA_ERROR_INSUFFICIENT_ENTROPY;

	case SX_ERR_INVALID_CIPHERTEXT:
		/* This can happen in psa_asymmetric_decrypt. PSA Crypto specification
		 * does not list an appropriate error code for this.
		 */
		return PSA_ERROR_INVALID_ARGUMENT;

	case SX_ERR_INVALID_ARG:
	case SX_ERR_INPUT_BUFFER_TOO_SMALL:
	default:
		return PSA_ERROR_INVALID_ARGUMENT;
	}
}

psa_status_t hash_get_algo(psa_algorithm_t alg, const struct sxhashalg **sx_hash_algo)
{
	*sx_hash_algo = NOT_ENABLED_HASH_ALG;

	switch (PSA_ALG_GET_HASH(alg)) {
	case PSA_ALG_SHA_1:
		IF_ENABLED(PSA_NEED_CRACEN_SHA_1, (*sx_hash_algo = &sxhashalg_sha1));
		break;
	case PSA_ALG_SHA_224:
		IF_ENABLED(PSA_NEED_CRACEN_SHA_224, (*sx_hash_algo = &sxhashalg_sha2_224));
		break;
	case PSA_ALG_SHA_256:
		IF_ENABLED(PSA_NEED_CRACEN_SHA_256, (*sx_hash_algo = &sxhashalg_sha2_256));
		break;
	case PSA_ALG_SHA_384:
		IF_ENABLED(PSA_NEED_CRACEN_SHA_384, (*sx_hash_algo = &sxhashalg_sha2_384));
		break;
	case PSA_ALG_SHA_512:
		IF_ENABLED(PSA_NEED_CRACEN_SHA_512, (*sx_hash_algo = &sxhashalg_sha2_512));
		break;
	case PSA_ALG_SHA3_224:
		IF_ENABLED(PSA_NEED_CRACEN_SHA3_224, (*sx_hash_algo = &sxhashalg_sha3_224));
		break;
	case PSA_ALG_SHA3_256:
		IF_ENABLED(PSA_NEED_CRACEN_SHA3_256, (*sx_hash_algo = &sxhashalg_sha3_256));
		break;
	case PSA_ALG_SHA3_384:
		IF_ENABLED(PSA_NEED_CRACEN_SHA3_384, (*sx_hash_algo = &sxhashalg_sha3_384));
		break;
	case PSA_ALG_SHA3_512:
		IF_ENABLED(PSA_NEED_CRACEN_SHA3_512, (*sx_hash_algo = &sxhashalg_sha3_512));
		break;
	case PSA_ALG_SHAKE256_512:
		IF_ENABLED(PSA_NEED_CRACEN_SHAKE256_512, (*sx_hash_algo = &sxhashalg_shake256_64));
		break;
	default:
		return PSA_ALG_IS_HASH(alg) ? PSA_ERROR_NOT_SUPPORTED : PSA_ERROR_INVALID_ARGUMENT;
	}

	return (*sx_hash_algo == NOT_ENABLED_HASH_ALG) ? PSA_ERROR_NOT_SUPPORTED : PSA_SUCCESS;
}

psa_status_t cracen_rnd_in_range(uint8_t *n, size_t sz, const uint8_t *upperlimit,
				 size_t retry_limit)
{
	size_t retries = 0;

	/* Fill leading zeroes. */
	while (upperlimit[0] == 0) {
		*n = 0;
		n++;
		upperlimit++;
		sz--;
	}

	uint8_t msb_mask;

	for (msb_mask = 0xFF; upperlimit[0] & msb_mask; msb_mask <<= 1) {
		;
	}
	msb_mask = ~msb_mask;

	while (retries++ < retry_limit) {
		psa_status_t status = cracen_get_random(NULL, n, sz);

		if (status) {
			return status;
		}
		n[0] &= msb_mask;

		int ge = cracen_be_cmp(n, upperlimit, sz, 0);

		if (ge == -1) {

			bool is_zero = constant_memcmp_is_zero(n, sz);

			if (is_zero == false) {
				return PSA_SUCCESS;
			}
		}
	}

	return PSA_ERROR_INSUFFICIENT_ENTROPY;
}

void cracen_xorbytes(uint8_t *a, const uint8_t *b, size_t sz)
{
	for (size_t i = 0; i < sz; i++, a++, b++) {
		*a = *a ^ *b;
	}
}

int cracen_prepare_ik_key(const uint8_t *user_data)
{
#ifdef CONFIG_CRACEN_IKG_SEED_LOAD
	if (!nrf_cracen_seedram_lock_check(NRF_CRACEN)) {
		if (lib_kmu_push_slot(CONFIG_CRACEN_IKG_SEED_KMU_SLOT + 0) ||
		    lib_kmu_push_slot(CONFIG_CRACEN_IKG_SEED_KMU_SLOT + 1) ||
		    lib_kmu_push_slot(CONFIG_CRACEN_IKG_SEED_KMU_SLOT + 2)) {
			return SX_ERR_INVALID_KEYREF;
		}
		nrf_cracen_seedram_lock_enable_set(NRF_CRACEN, true);
	}
#endif

	__attribute__((unused)) struct sx_pk_config_ik cfg = {};

#ifdef CONFIG_CRACEN_IKG_PERSONALIZED_KEYS
	cfg.key_bundle = (const uint32_t *)user_data;
	cfg.key_bundle_sz = 1; /* size of the owner_id is one 32-bit word */
#endif

#if defined(CONFIG_CRACEN_IKG)
	return sx_pk_ik_derive_keys(&cfg);
#else
	return PSA_ERROR_NOT_SUPPORTED;
#endif
}

static int cracen_clean_ik_key(const uint8_t *user_data)
{
	return SX_OK;
}

static bool cracen_is_ikg_key(const psa_key_attributes_t *attributes)
{
	switch (MBEDTLS_SVC_KEY_ID_GET_KEY_ID(psa_get_key_id(attributes))) {
	case CRACEN_BUILTIN_IDENTITY_KEY_ID:
	case CRACEN_BUILTIN_MKEK_ID:
	case CRACEN_BUILTIN_MEXT_ID:
		return true;
	default:
		return false;
	}
};

static psa_status_t cracen_load_ikg_keyref(const psa_key_attributes_t *attributes,
					   const uint8_t *key_buffer, size_t key_buffer_size,
					   struct sxkeyref *k)
{
	k->prepare_key = cracen_prepare_ik_key;
	k->clean_key = cracen_clean_ik_key;

	/* IKG keys are identified from the ID */
	(void)key_buffer;
	(void)key_buffer_size;

	switch (MBEDTLS_SVC_KEY_ID_GET_KEY_ID(psa_get_key_id(attributes))) {
	case CRACEN_BUILTIN_MKEK_ID:
		k->cfg = CRACEN_INTERNAL_HW_KEY1_ID;
		break;
	case CRACEN_BUILTIN_MEXT_ID:
		k->cfg = CRACEN_INTERNAL_HW_KEY2_ID;
		break;
	default:
		return PSA_ERROR_INVALID_ARGUMENT;
	};

	k->owner_id = MBEDTLS_SVC_KEY_ID_GET_OWNER_ID(psa_get_key_id(attributes));
	k->user_data = (const uint8_t *)&k->owner_id;
	return PSA_SUCCESS;
}

psa_status_t cracen_load_keyref(const psa_key_attributes_t *attributes, const uint8_t *key_buffer,
				size_t key_buffer_size, struct sxkeyref *k)
{
	safe_memzero(k, sizeof(*k));

#ifdef PSA_NEED_CRACEN_KMU_DRIVER
	if (PSA_KEY_LIFETIME_GET_LOCATION(psa_get_key_lifetime(attributes)) ==
	    PSA_KEY_LOCATION_CRACEN_KMU) {
		kmu_opaque_key_buffer *key = (kmu_opaque_key_buffer *)key_buffer;
		enum cracen_kmu_metadata_key_usage_scheme key_usage_scheme = key->key_usage_scheme;

		k->clean_key = cracen_kmu_clean_key;
		k->prepare_key = cracen_kmu_prepare_key;
		k->user_data = key_buffer;

		switch (key_usage_scheme) {
		case CRACEN_KMU_KEY_USAGE_SCHEME_RAW:
		case CRACEN_KMU_KEY_USAGE_SCHEME_ENCRYPTED:
			k->sz = PSA_BITS_TO_BYTES(psa_get_key_bits(attributes));
			k->key = kmu_push_area;

			return PSA_SUCCESS;
		case CRACEN_KMU_KEY_USAGE_SCHEME_PROTECTED:
			k->sz = PSA_BITS_TO_BYTES(psa_get_key_bits(attributes));
			k->key = (const uint8_t *)CRACEN_PROTECTED_RAM_AES_KEY0;

			return PSA_SUCCESS;

		default:
			return PSA_ERROR_NOT_PERMITTED;
		}
	}
#endif
	if (PSA_KEY_LIFETIME_GET_LOCATION(psa_get_key_lifetime(attributes)) ==
	    PSA_KEY_LOCATION_CRACEN) {

		if (cracen_is_ikg_key(attributes)) {
			if (IS_ENABLED(CONFIG_CRACEN_IKG)) {
				return cracen_load_ikg_keyref(attributes, key_buffer,
							      key_buffer_size, k);
			} else {
				return PSA_ERROR_NOT_SUPPORTED;
			}
		}

		k->owner_id = MBEDTLS_SVC_KEY_ID_GET_OWNER_ID(psa_get_key_id(attributes));
		k->user_data = (const uint8_t *)&k->owner_id;
		k->prepare_key = NULL;
		k->clean_key = NULL;

		switch (MBEDTLS_SVC_KEY_ID_GET_KEY_ID(psa_get_key_id(attributes))) {
		case CRACEN_PROTECTED_RAM_AES_KEY0_ID:
			k->sz = 32;
			k->key = (const uint8_t *)CRACEN_PROTECTED_RAM_AES_KEY0;
			break;
		default:
			if (key_buffer_size == 0) {
				return PSA_ERROR_CORRUPTION_DETECTED;
			}

			/* Normal transparent key. */
			k->key = key_buffer;
			k->sz = key_buffer_size;
		}
	} else {
		k->key = key_buffer;
		k->sz = key_buffer_size;
	}

	return PSA_SUCCESS;
}

void cracen_be_add(uint8_t *v, size_t sz, size_t summand)
{
	while (sz > 0) {
		sz--;
		summand += v[sz];
		v[sz] = summand & 0xFF;
		summand >>= 8;
	}
}

int cracen_be_sub(const uint8_t *a, const uint8_t *b, uint8_t *c, size_t sz)
{
	unsigned int borrow = 0;
	unsigned int ai;
	unsigned int bi;
	unsigned int tmp;

	while (sz > 0) {
		sz--;
		ai = (unsigned int)a[sz];
		bi = (unsigned int)b[sz];

		/* tmp will be in 0..0xFFFFFFFF; if underflow occurred for the byte subtraction
		 * (ai < bi + borrow) then the high bits of tmp will be set (tmp >= 0xFFFFFF00).
		 * Shifting right by 8 and masking the LSB yields 1 in that case, 0 otherwise.
		 */
		tmp = ai - bi - borrow;
		c[sz] = (uint8_t)tmp;
		borrow = (tmp >> 8) & 1u;
	}

	return borrow ? -1 : 0;
}

void cracen_be_rshift(const uint8_t *a, int n, uint8_t *r, size_t sz)
{
	unsigned int carry = 0;
	unsigned int result;
	unsigned int start_index;
	size_t byte_shift = (size_t)(n >> 3);	/* n / 8 */
	size_t bit_shift  = (size_t)(n & 7);	/* n % 8 */

	safe_memzero(r, sz);

	for (size_t i = 0; i < sz - byte_shift; i++) {
		start_index = i + byte_shift;
		result = (a[i] >> bit_shift) | (carry << (8 - bit_shift));
		carry = a[i] & ((1u << bit_shift) - 1u);

		r[start_index] = result;
	}
}

int cracen_be_cmp(const uint8_t *a, const uint8_t *b, size_t sz, int carry)
{
	unsigned int neq = 0;
	unsigned int gt = 0;
	unsigned int ucarry;
	unsigned int d;
	unsigned int lt;

	/* transform carry to work with unsigned numbers */
	ucarry = 0x100 + carry;

	for (int i = sz - 1; i >= 0; i--) {
		d = ucarry + a[i] - b[i];
		ucarry = 0xFF + (d >> 8);
		neq |= d & 0xFF;
	}

	neq |= ucarry & 0xFF;
	lt = ucarry < 0x100;
	gt = neq && !lt;

	return (gt ? 1 : 0) - (lt ? 1 : 0);
}

void cracen_be_xor(uint8_t *buf, size_t buf_sz, size_t xor_val)
{
	while (buf_sz > 0 && xor_val > 0) {
		buf_sz--;
		buf[buf_sz] ^= xor_val & 0xFF;
		xor_val >>= 8;
	}
}

int cracen_hash_all_inputs_with_context(struct sxhash *hashopctx, const uint8_t *inputs[],
					const size_t input_lengths[], size_t input_count,
					const struct sxhashalg *hashalg, uint8_t *digest)
{
	int status;

	status = sx_hash_create(hashopctx, hashalg, sizeof(*hashopctx));
	if (status != SX_OK) {
		return status;
	}

	for (size_t i = 0; i < input_count; i++) {
		status = sx_hash_feed(hashopctx, inputs[i], input_lengths[i]);
		if (status != SX_OK) {
			return status;
		}
	}
	status = sx_hash_digest(hashopctx, digest);
	if (status != SX_OK) {
		return status;
	}

	status = sx_hash_wait(hashopctx);

	return status;
}

int cracen_hash_all_inputs(const uint8_t *inputs[], const size_t input_lengths[],
			   size_t input_count, const struct sxhashalg *hashalg, uint8_t *digest)
{
	struct sxhash hashopctx;

	return cracen_hash_all_inputs_with_context(&hashopctx, inputs, input_lengths, input_count,
						   hashalg, digest);
}

int cracen_hash_input(const uint8_t *input, const size_t input_length,
		      const struct sxhashalg *hashalg, uint8_t *digest)
{
	return cracen_hash_all_inputs(&input, &input_length, 1, hashalg, digest);
}

int cracen_hash_input_with_context(struct sxhash *hashopctx, const uint8_t *input,
				   const size_t input_length, const struct sxhashalg *hashalg,
				   uint8_t *digest)
{
	return cracen_hash_all_inputs_with_context(hashopctx, &input, &input_length, 1, hashalg,
						   digest);
}
