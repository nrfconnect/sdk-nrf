/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>

#include "bl_crypto.h"
#include "test_vector.c"


ZTEST(bl_crypto_test, test_ecdsa_verify)
{
	uint32_t hash_len = ARRAY_SIZE(hash_sha256);

	/* Success */
	int retval = bl_secp256r1_validate(hash_sha256, hash_len, pub_concat, sig_concat);
	zassert_equal(0, retval, "retval: 0x%x", retval);

	retval = bl_secp256r1_validate(const_hash_sha256, hash_len, pub_concat, sig_concat);
	zassert_equal(0, retval, "retval: 0x%x", retval);

	retval = bl_secp256r1_validate(hash_sha256, hash_len, const_pub_concat, sig_concat);
	zassert_equal(0, retval, "retval: 0x%x", retval);

	retval = bl_secp256r1_validate(hash_sha256, hash_len, pub_concat, const_sig_concat);
	zassert_equal(0, retval, "retval: 0x%x", retval);

	retval = bl_secp256r1_validate(hash_sha256, hash_len, const_pub_concat, const_sig_concat);
	zassert_equal(0, retval, "retval: 0x%x", retval);

	retval = bl_secp256r1_validate(const_hash_sha256, hash_len, const_pub_concat, const_sig_concat);
	zassert_equal(0, retval, "retval: 0x%x", retval);

	retval = bl_secp256r1_validate(const_hash_sha256, hash_len, const_pub_concat, const_sig_concat);
	zassert_equal(0, retval, "retval: 0x%x", retval);

	retval = bl_secp256r1_validate(image_fw_hash, hash_len, image_public_key, image_gen_sig);
	zassert_equal(0, retval, "gen sig failed retval: 0x%x", retval);

	retval = bl_secp256r1_validate(image_fw_hash, hash_len, image_public_key, image_fw_sig);
	zassert_equal(0, retval, "fw_sig retval: 0x%x", retval);
	image_fw_hash[0]++;
	retval = bl_secp256r1_validate(image_fw_hash, hash_len, image_public_key, image_fw_sig);
	image_fw_hash[0]--;
	zassert_not_equal(0, retval, "fw_sig retval: 0x%x", retval);

	retval = bl_secp256r1_validate(const_fw_hash, hash_len, image_public_key, image_fw_sig);
	zassert_equal(0, retval, "const_fw retval: 0x%x", retval);

	retval = bl_secp256r1_validate(image_fw_hash, hash_len, const_public_key, image_fw_sig);
	zassert_equal(0, retval, "const_pubk retval: 0x%x", retval);

	retval = bl_secp256r1_validate(image_fw_hash, hash_len, image_public_key, const_fw_sig);
	zassert_equal(0, retval, "const_sig retval: 0x%x", retval);

	retval = bl_secp256r1_validate(image_fw_hash, hash_len, image_public_key, const_fw_sig);
	zassert_equal(0, retval, "const_sig retval: 0x%x", retval);

	retval = bl_secp256r1_validate(image_fw_hash, hash_len, image_public_key, const_gen_sig);
	zassert_equal(0, retval, "const_gen_sig retval: 0x%x", retval);

	retval = bl_secp256r1_validate(image_fw_hash, hash_len, const_public_key, const_fw_sig);
	zassert_equal(0, retval, "const_sig const_pk retval: 0x%x", retval);

	retval = bl_secp256r1_validate(const_fw_hash, hash_len, const_public_key, const_fw_sig);
	zassert_equal(0, retval, "const_sig const_pk const_fw retval: 0x%x", retval);

	retval = bl_secp256r1_validate(const_fw_hash, hash_len, image_public_key, const_fw_sig);
	zassert_equal(0, retval, "const_sig const_fw retval: 0x%x", retval);

	retval = bl_secp256r1_validate(const_fw_hash, hash_len, const_public_key, image_fw_sig);
	zassert_equal(0, retval, "const_pk const_fw retval: 0x%x", retval);

	/* pub key does not match */
	pub_concat[1]++;
	retval = bl_secp256r1_validate(hash_sha256, hash_len, pub_concat, sig_concat);
	pub_concat[1]--;
	zassert_equal(-ESIGINV, retval, "retval: 0x%x", retval);

	pub_concat[1]++;
	retval = bl_secp256r1_validate(hash_sha256, hash_len, pub_concat, sig_concat);
	zassert_equal(-ESIGINV, retval, "retval: 0x%x", retval);
	retval = bl_secp256r1_validate(hash_sha256, hash_len, pub_concat, const_sig_concat);
	zassert_equal(-ESIGINV, retval, "retval: 0x%x", retval);
	retval = bl_secp256r1_validate(const_hash_sha256, hash_len, pub_concat, const_sig_concat);
	zassert_equal(-ESIGINV, retval, "retval: 0x%x", retval);
	pub_concat[1]--;

	/* hash does not match */
	hash_sha256[2]++;
	retval = bl_secp256r1_validate(hash_sha256, hash_len, pub_concat, sig_concat);
	hash_sha256[2]--;
	zassert_equal(-ESIGINV, retval, "retval: 0x%x", retval);

	/* signature does not match */
	sig_concat[3]++;
	retval = bl_secp256r1_validate(hash_sha256, hash_len, pub_concat, sig_concat);
	sig_concat[3]--;
	zassert_equal(-ESIGINV, retval, "retval: 0x%x", retval);

	/* Null pointer passed as hash */
	retval = bl_secp256r1_validate(NULL, hash_len, pub_concat, sig_concat);
	zassert_equal(-EINVAL, retval, "retval: 0x%x", retval);

	/* Wrong hash LEN */
	retval = bl_secp256r1_validate(hash_sha256, 24, pub_concat, sig_concat);
	zassert_equal(-EINVAL, retval, "retval: 0x%x", retval);

	/* Null pointer passed as public key */
	retval = bl_secp256r1_validate(hash_sha256, hash_len, NULL, sig_concat);
	zassert_equal(-EINVAL, retval, "retval: 0x%x", retval);

	/* Null pointer passed as signature */
	retval = bl_secp256r1_validate(hash_sha256, hash_len, pub_concat, NULL);
	zassert_equal(-EINVAL, retval, "retval: 0x%x", retval);
}

void test_sha256_string(const uint8_t * input, uint32_t input_len, const uint8_t * test_vector, bool eq)
{
	int rc = 0;
	uint8_t output[32] = {0};
	bl_sha256_ctx_t ctx;
	static uint32_t run_count;

	rc = bl_sha256_init(&ctx);
	zassert_equal(0, rc, "bl_sha256_init failed retval was: %d", rc);
	rc = bl_sha256_update(&ctx, input, input_len);
	zassert_equal(0, rc, "bl_sha256_update failed retval was: %d", rc);
	rc = bl_sha256_finalize(&ctx, output);
	zassert_equal(0, rc, "bl_sha256_finalize failed retval was: %d", rc);

	for(size_t i = 0; i < ARRAY_SIZE(output); i++) {
		if(eq){
			zassert_equal(test_vector[i], output[i], "positive test failed (run no. %d) for i = %d", run_count, i);
		}else {
			zassert_not_equal(test_vector[i], output[i], "negative test failed (run no. %d) for i = %d", run_count, i);
		}
	}

	rc = bl_sha256_verify(input, input_len, test_vector);

	int expected_rc = eq ? 0 : -EHASHINV;
	zassert_equal(expected_rc, rc, "bl_sha256_verify returned %d instead of %d", rc, expected_rc);

	run_count++;
}
const uint8_t input3[] = "test vector should fail";
const uint8_t input2[] = "test vector";

ZTEST(bl_crypto_test, test_sha256)
{
	test_sha256_string(NULL, 0, sha256_empty_string, true);
	test_sha256_string(input2, strlen(input2), sha256_test_vector_string, true);

	test_sha256_string(input3, strlen(input3), sha256_test_vector_string, false);
	test_sha256_string(mcuboot_key, ARRAY_SIZE(mcuboot_key), mcuboot_key_hash, true);
	test_sha256_string(mcuboot_key, ARRAY_SIZE(mcuboot_key), sha256_test_vector_string, false);

	/* Size restrictions. */
#if CONFIG_FLASH_SIZE > 300
	test_sha256_string(const_fw_data, ARRAY_SIZE(const_fw_data),
			   image_fw_hash, true);
	test_sha256_string(long_input, ARRAY_SIZE(long_input), long_input_hash, true);
	test_sha256_string(long_input, ARRAY_SIZE(long_input), sha256_test_vector_string, false);
#endif

	test_sha256_string(hash_in1, 1, hash_res1, true);
	test_sha256_string(hash_in2, 3, hash_res2, true);
	test_sha256_string(hash_in3, 56, hash_res3, true);

	test_sha256_string(hash_in, 55, hash_res55, true);
	test_sha256_string(hash_in, 56, hash_res56, true);
	test_sha256_string(hash_in, 57, hash_res57, true);
	test_sha256_string(hash_in, 63, hash_res63, true);
	test_sha256_string(hash_in, 64, hash_res64, true);
	test_sha256_string(hash_in, 65, hash_res65, true);
}

ZTEST(bl_crypto_test, test_bl_root_of_trust_verify)
{

	/* Success. */
	int retval = bl_root_of_trust_verify(pk, pk_hash, sig, firmware,
			sizeof(firmware));

	zassert_equal(0, retval, "retval was %d", retval);

	retval = bl_root_of_trust_verify(const_pk, const_pk_hash, const_sig, const_firmware,
			sizeof(const_firmware));
	zassert_equal(0, retval, "retval was %d", retval);

	zassert_equal(0, retval, "retval was %d", retval);
	/* pk doesn't match pk_hash. */
	pk[1]++;
	retval = bl_root_of_trust_verify(pk, pk_hash, sig, firmware,
			sizeof(firmware));
	pk[1]--;

	zassert_equal(-EHASHINV, retval, "retval was %d", retval);

	/* metadata doesn't match signature */
	firmware[0]++;
	retval = bl_root_of_trust_verify(pk, pk_hash, sig, firmware,
			sizeof(firmware));
	firmware[0]--;

	zassert_equal(-ESIGINV, retval, "retval was %d", retval);
}

ZTEST_SUITE(bl_crypto_test, NULL, NULL, NULL, NULL, NULL);
