/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <ztest.h>
#include <pm_config.h>
#include <psa/crypto.h>
#include <tfm_crypto_defs.h>

#define STATE_TEST_WRITE 0x54834352
#define STATE_TEST_LOAD 0x17029357
#define STATE_TEST_ERROR 0x72490698
#define STATE_TEST_INVALID 0x98296028

/* This test involves multiple resets, so this variable is __noinit to keep
 * state between resets.
 */
static __noinit volatile uint32_t state;

#if defined(CONFIG_HAS_HW_NRF_CC312)
__ALIGNED(4)
static uint8_t test_label[16] = {
	0xe2, 0x75, 0xab, 0x25, 0x00, 0x3b, 0x15, 0xe1,
	0xa1, 0x98, 0x78, 0x5b, 0x4e, 0x57, 0x43, 0x9a
};

__ALIGNED(4)
static uint8_t expected_key[32] = {
	0xaf, 0xf8, 0x48, 0x65, 0x43, 0xbd, 0x31, 0x76,
	0xa4, 0xbe, 0xee, 0xd6, 0x7d, 0x67, 0xdf, 0x48,
	0xf4, 0x5e, 0x98, 0xc1, 0xa7, 0x1b, 0x1e, 0x3a,
	0x62, 0xc9, 0x6a, 0xc2, 0xdb, 0x4b, 0x2e, 0xeb
};

#else
__ALIGNED(4)
static uint8_t test_label[19] = {
	0x29, 0xd6, 0xa6, 0x8d, 0x3f, 0xfb, 0x39, 0x1f,
	0x03, 0x6d, 0xd5, 0x3b, 0xe3, 0x54, 0xe4, 0x02,
	0x9a, 0x69, 0x5a
};

__ALIGNED(4)
static uint8_t expected_key[16] = {
	0x02, 0xaa, 0xe4, 0xe3, 0x9a, 0x4c, 0xec, 0xe8,
	0x4a, 0x28, 0xde, 0x9d, 0x4d, 0x05, 0xbd, 0xb1
};
#endif


static void test_hw_unique_key_tfm1(void)
{
#ifdef CONFIG_SOC_NRF5340_CPUAPP
	uint8_t out_key[32];
#else
	uint8_t out_key[16];
#endif
	uint32_t out_len;
	psa_key_id_t key_id = 1234;
	psa_status_t status;
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_key_derivation_operation_t op = PSA_KEY_DERIVATION_OPERATION_INIT;

	/* Set the key attributes for the storage key */
	psa_set_key_usage_flags(&attributes,
			(PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT | PSA_KEY_USAGE_EXPORT));
	psa_set_key_algorithm(&attributes, PSA_ALG_AEAD_WITH_TAG_LENGTH(PSA_ALG_CMAC, 16));
	psa_set_key_type(&attributes, PSA_KEY_TYPE_AES);
	psa_set_key_bits(&attributes, sizeof(out_key) * 8);

	/* Set up a key derivation operation with HUK derivation as the alg */
	status = psa_key_derivation_setup(&op, TFM_CRYPTO_ALG_HUK_DERIVATION);
	zassert_equal(PSA_SUCCESS, status, NULL);

	/* Supply the PS key label as an input to the key derivation */
	status = psa_key_derivation_input_bytes(&op, PSA_KEY_DERIVATION_INPUT_LABEL,
						test_label,
						sizeof(test_label));
	zassert_equal(PSA_SUCCESS, status, "psa_key_derivation_input_bytes failed: %d\n", status);

	/* Create the storage key from the key derivation operation */
	status = psa_key_derivation_output_key(&attributes, &op, &key_id);
	zassert_equal(PSA_SUCCESS, status, "psa_key_derivation_output_key failed: %d\n", status);

	/* Free resources associated with the key derivation operation */
	status = psa_key_derivation_abort(&op);
	zassert_equal(PSA_SUCCESS, status, NULL);

	status = psa_export_key(key_id, out_key, sizeof(out_key), &out_len);
	zassert_equal(PSA_SUCCESS, status, NULL);
	zassert_equal(out_len, sizeof(out_key), NULL);

	zassert_mem_equal(expected_key, out_key, sizeof(expected_key), NULL);
}

void test_main(void)
{
	ztest_test_suite(test_hw_unique_key_tfm,
			ztest_unit_test(test_hw_unique_key_tfm1)
			);
	ztest_run_test_suite(test_hw_unique_key_tfm);
}
