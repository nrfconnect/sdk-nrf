/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <psa/crypto.h>
#include <tfm_crypto_defs.h>

#if defined(CONFIG_HAS_HW_NRF_CC312)
__ALIGNED(4)
static uint8_t test_label[16] = {
	0xe2, 0x75, 0xab, 0x25, 0x00, 0x3b, 0x15, 0xe1,
	0xa1, 0x98, 0x78, 0x5b, 0x4e, 0x57, 0x43, 0x9a
};

__ALIGNED(4)
static uint8_t expected_key[32] = {
	0xa9, 0x4d, 0x17, 0x7f, 0x5b, 0x6a, 0xc4, 0x6e,
	0x64, 0xe1, 0xae, 0xdb, 0x93, 0xee, 0x8a, 0xec,
	0x47, 0x70, 0x9a, 0x40, 0xeb, 0xb2, 0xc2, 0x66,
	0x4f, 0xfe, 0x5b, 0x25, 0x2b, 0x09, 0xd4, 0x75
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
	0x75, 0x70, 0xca, 0xb8, 0x88, 0x6d, 0xcc, 0x91,
	0x9a, 0xfe, 0xfc, 0x34, 0xf1, 0xc1, 0xc5, 0xf5
};
#endif


ZTEST(test_hw_unique_key_tfm, test_hw_unique_key_tfm1)
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
	psa_set_key_algorithm(&attributes, PSA_ALG_AEAD_WITH_SHORTENED_TAG(PSA_ALG_CMAC, 16));
	psa_set_key_type(&attributes, PSA_KEY_TYPE_AES);
	psa_set_key_bits(&attributes, sizeof(out_key) * 8);

	status = psa_key_derivation_setup(&op, PSA_ALG_HKDF(PSA_ALG_SHA_256));
	zassert_equal(PSA_SUCCESS, status, "status %ld != PSA_SUCCESS\r\n");

	/* Set up a key derivation operation with HUK  */
	status = psa_key_derivation_input_key(&op, PSA_KEY_DERIVATION_INPUT_SECRET,
					      TFM_BUILTIN_KEY_ID_HUK);

	/* Supply the PS key label as an input to the key derivation */
	status = psa_key_derivation_input_bytes(&op, PSA_KEY_DERIVATION_INPUT_INFO,
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

	for (int i = 0; i < sizeof(expected_key); i++) {
		printk("%02x ", out_key[i]);
	}
	printk("\r\n");

	zassert_mem_equal(expected_key, out_key, sizeof(expected_key), NULL);
}

ZTEST_SUITE(test_hw_unique_key_tfm, NULL, NULL, NULL, NULL, NULL);
