/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <zephyr/sys/printk.h>

#include <zephyr/sys/util.h>
#include <zephyr/sys_clock.h>

#include "common_test.h"

/* Entries must correspond to order in enum test_vector_t found in common_test.h */
const size_t test_vector_sizes[] = {
	sizeof(test_vector_aes_t),	    sizeof(test_vector_aead_t),
	sizeof(test_vector_ecdsa_verify_t), sizeof(test_vector_ecdsa_sign_t),
	sizeof(test_vector_ecdsa_random_t), sizeof(test_vector_ecdh_t),
	sizeof(test_vector_hash_t),	    sizeof(test_vector_hmac_t),
	sizeof(test_vector_hkdf_t),	    sizeof(test_vector_ecjpake_t),
};

/* Entries must correspond to order in enum test_vector_t found in common_test.h */
const size_t test_vector_name_offset[] = {
	offsetof(test_vector_aes_t, p_test_vector_name),
	offsetof(test_vector_aead_t, p_test_vector_name),
	offsetof(test_vector_ecdsa_verify_t, p_test_vector_name),
	offsetof(test_vector_ecdsa_sign_t, p_test_vector_name),
	offsetof(test_vector_ecdsa_random_t, p_test_vector_name),
	offsetof(test_vector_ecdh_t, p_test_vector_name),
	offsetof(test_vector_hash_t, p_test_vector_name),
	offsetof(test_vector_hmac_t, p_test_vector_name),
	offsetof(test_vector_hkdf_t, p_test_vector_name),
	offsetof(test_vector_ecjpake_t, p_test_vector_name),
};

#if !defined(MBEDTLS_PSA_CRYPTO_EXTERNAL_RNG)
static int entropy_func(void *ctx, unsigned char *buf, size_t len)
{
	return entropy_get_entropy(ctx, buf, len);
}
#endif

#if defined(MBEDTLS_PSA_CRYPTO_EXTERNAL_RNG)

/* Dummy context since we don't use it in the external_rng function */
char drbg_ctx;

int init_drbg(const unsigned char *p_optional_seed, size_t len)
{
	if (p_optional_seed != NULL) {
		return -EINVAL;
	}

	return 0;
}

int external_rng(void *ctx, unsigned char *output, size_t len)
{
	int out_len;
	int ret = nrf_cc3xx_platform_ctr_drbg_get(NULL, output, len, &out_len);
	return ret;
}

int (*drbg_random)(void *, unsigned char *, size_t) = &external_rng;

#elif defined(MBEDTLS_CTR_DRBG_C)
mbedtls_ctr_drbg_context drbg_ctx;
int (*drbg_random)(void *, unsigned char *, size_t) = &mbedtls_ctr_drbg_random;

int init_drbg(const unsigned char *p_optional_seed, size_t len)
{
	static const unsigned char ncs_seed[] = "ncs_drbg_seed";

	const unsigned char *p_seed;

	if (p_optional_seed == NULL) {
		p_seed = ncs_seed;
		len = sizeof(ncs_seed);
	} else {
		p_seed = p_optional_seed;
	}

	const struct device *p_device = DEVICE_DT_GET(DT_CHOSEN(zephyr_entropy));

	if (!device_is_ready(p_device)) {
		return -ENODEV;
	}

	// Ensure previously run test is properly deallocated
	// (This frees the mutex inside ctr_drbg context)
	mbedtls_ctr_drbg_free(&drbg_ctx);
	mbedtls_ctr_drbg_init(&drbg_ctx);
	return mbedtls_ctr_drbg_seed(&drbg_ctx, entropy_func, (void *)p_device,
				     p_seed, len);
}
#elif defined(MBEDTLS_HMAC_DRBG_C)

mbedtls_hmac_drbg_context drbg_ctx;
int (*drbg_random)(void *, unsigned char *, size_t) = &mbedtls_hmac_drbg_random;

int init_drbg(const unsigned char *p_optional_seed, size_t len)
{
	static const unsigned char ncs_seed[] = "ncs_drbg_seed";

	const unsigned char *p_seed;

	if (p_optional_seed == NULL) {
		p_seed = ncs_seed;
		len = sizeof(ncs_seed);
	} else {
		p_seed = p_optional_seed;
	}

	// Ensure previously run test is properly deallocated
	// (This frees the mutex inside hmac_drbg context)
	mbedtls_hmac_drbg_free(&drbg_ctx);
	mbedtls_hmac_drbg_init(&drbg_ctx);

	const struct device *p_device = DEVICE_DT_GET(DT_CHOSEN(zephyr_entropy));

	if (!device_is_ready(p_device)) {
		return -ENODEV;
	}

	const mbedtls_md_info_t *p_info =
		mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);

	return mbedtls_hmac_drbg_seed(&drbg_ctx, p_info, entropy_func,
		(void *)p_device, p_seed, len);
}

#endif

const char *get_vector_name(const test_case_t *tc, uint32_t v)
{
	uint32_t tv_offset = test_vector_sizes[tc->vector_type] * v;
	uint32_t name_offset = test_vector_name_offset[tc->vector_type];

	uint32_t tv_addr = (size_t)tc->vectors_start + tv_offset;
	const char **p_name = (const char **)(tv_addr + name_offset);

	return *p_name;
}

uint32_t get_vector_count(const test_case_t *tc)
{
	return ((size_t)tc->vectors_stop - (size_t)tc->vectors_start) /
	       test_vector_sizes[tc->vector_type];
}

size_t hex2bin_safe(const char *hex, uint8_t *buf, size_t buflen)
{
	return hex == NULL ? 0 : hex2bin(hex, strlen(hex), buf, buflen);
}

/* Weak definition, user overridable */
__weak void start_time_measurement(void)
{
	;
}

/* Weak definition, user overridable */
__weak void stop_time_measurement(void)
{
	;
}
