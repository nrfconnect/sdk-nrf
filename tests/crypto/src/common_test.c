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

/* Dummy context since we don't use it in the external_rng function */
char drbg_ctx;

int external_rng(void *ctx, unsigned char *output, size_t len)
{
	/* No context is required for the nrf_cc3xx_platform library */
	(void) ctx;
	int out_len;
	int ret = nrf_cc3xx_platform_ctr_drbg_get(NULL, output, len, &out_len);
	return ret;
}

int (*drbg_random)(void *, unsigned char *, size_t) = &external_rng;

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
