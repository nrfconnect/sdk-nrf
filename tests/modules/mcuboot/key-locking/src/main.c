/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <stdio.h>

#include <psa/crypto.h>
#include <psa/crypto_types.h>
#include <cracen_psa_kmu.h>
#include <zephyr/sys/util.h>

#define ED25519_TEST_KEY_SLOT	226

#define KEY_LIFETIME	PSA_KEY_LIFETIME_FROM_PERSISTENCE_AND_LOCATION(PSA_KEY_PERSISTENCE_DEFAULT, PSA_KEY_LOCATION_CRACEN_KMU)
#define MAKE_PSA_KMU_KEY_ID(id) PSA_KEY_HANDLE_FROM_CRACEN_KMU_SLOT(CRACEN_KMU_KEY_USAGE_SCHEME_RAW, id)

/* Testing only the first key slot dedicated to ED25519 */
#define DEFAULT_TEST_KEY MAKE_PSA_KMU_KEY_ID(ED25519_TEST_KEY_SLOT)

/* Public key here is generated from bootloader/mcuboot/root-ed25519.pem
 * identified by SHA256
 * f1c516ce7e6ed759e08ede0d81b26ae0074b1ee1eb2fd083e545acd7b0cc3eea.
 * using command:
 *  openssl pkey -in bootloader/mcuboot/root-ed25519.pem -pubout -outform DER | \
 *    tail -c 32 | hexdump --format '8/1 "0x%02x, " "\n"'
 *
 */
static const uint8_t ed25519_key[] = {
	0xd4, 0xb3, 0x1b, 0xa4, 0x9a, 0x3a, 0xdd, 0x3f,
	0x82, 0x5d, 0x10, 0xca, 0x7f, 0x31, 0xb5, 0x0b,
	0x0d, 0xe8, 0x7f, 0x37, 0xcc, 0xc4, 0x9f, 0x1a,
	0x40, 0x3a, 0x5c, 0x13, 0x20, 0xff, 0xb4, 0xe0,
};

/* Message for signature verification; generated using command:
 *  echo "Hello World PSA!" | hexdump --format '8/1 "0x%02x, " "\n"' | \
 *    sed -e 's/0x  ,//g'
 */
static const uint8_t message[] = {
	0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x20, 0x57, 0x6f,
	0x72, 0x6c, 0x64, 0x20, 0x50, 0x53, 0x41, 0x21,
	0x0a,
};

/* Signature of above message, generated using command:
 *  MSG=`mktemp`; echo "Hello World PSA!" > $MSG; \
 *    openssl pkeyutl -sign -inkey bootloader/mcuboot/root-ed25519.pem \
 *    -rawin -in $MSG | hexdump --format '8/1 "0x%02x, " "\n"'
 */
static const uint8_t signature[] = {
	0xce, 0xf2, 0xbd, 0xb8, 0xdb, 0x68, 0xf8, 0xd8,
	0x14, 0x3b, 0x63, 0x6e, 0x69, 0x27, 0x02, 0xc4,
	0x02, 0xb4, 0x99, 0x5e, 0xf8, 0x43, 0x6b, 0x9f,
	0xf1, 0x8b, 0x72, 0x77, 0x7f, 0x04, 0xfe, 0xb3,
	0x39, 0x1b, 0x6e, 0x7b, 0x73, 0xad, 0xb6, 0xe2,
	0x87, 0x23, 0x9e, 0xb9, 0xfa, 0x20, 0x67, 0x40,
	0xa4, 0xd6, 0x3a, 0xf6, 0x0d, 0x5e, 0xfa, 0x83,
	0xa6, 0x43, 0xc0, 0x83, 0xff, 0x4b, 0xbd, 0x0a,
};

static void *test_init(void)
{
	TC_PRINT("## Test for MCUboot KMU key lock effect on an application boot by the bootloader ##\n");
	TC_PRINT("## Testing KMU slot %u, psa_key_id_t handle 0x%lx ##\n",
		 (unsigned)ED25519_TEST_KEY_SLOT, (unsigned long)DEFAULT_TEST_KEY);
	zassert_equal(PSA_SUCCESS, psa_crypto_init());

	return NULL;
}

ZTEST(key_lock_test, test_psa_destroy)
{
	psa_status_t status;

	status = psa_destroy_key(DEFAULT_TEST_KEY);

	/* Destruction of locked KMU stored key */
	zassert_equal(PSA_ERROR_INVALID_ARGUMENT, status, "Expected %d got %d",
		      PSA_ERROR_INVALID_ARGUMENT, status);
}

ZTEST(key_lock_test, test_psa_import_on_popluated)
{
	/* Lock only works when there is key in KMU with a given handle;
	 * empty slot is still usable for import.
	 */
	/* Test is hardcoded for importing ED25519, 255 bit key */
	psa_key_attributes_t key_attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_status_t status = PSA_ERROR_BAD_STATE;
	psa_key_id_t dkey_id = PSA_KEY_TYPE_NONE;

	psa_set_key_usage_flags(&key_attributes, PSA_KEY_USAGE_VERIFY_MESSAGE | PSA_KEY_USAGE_VERIFY_HASH);
	psa_set_key_algorithm(&key_attributes, PSA_ALG_PURE_EDDSA);
	psa_set_key_type(&key_attributes, PSA_KEY_TYPE_ECC_PUBLIC_KEY(PSA_ECC_FAMILY_TWISTED_EDWARDS));
	psa_set_key_bits(&key_attributes, 255);
	psa_set_key_lifetime(&key_attributes, KEY_LIFETIME);

	psa_set_key_id(&key_attributes, DEFAULT_TEST_KEY);

	status = psa_import_key(&key_attributes, ed25519_key, ARRAY_SIZE(ed25519_key),
				&dkey_id);
	zassert_equal(PSA_ERROR_ALREADY_EXISTS, status, "Expected %d got %d",
		      PSA_ERROR_ALREADY_EXISTS, status);
}

ZTEST(key_lock_test, test_psa_verify_message)
{
	psa_status_t status = psa_verify_message(DEFAULT_TEST_KEY, PSA_ALG_PURE_EDDSA,
		message, ARRAY_SIZE(message), signature, ARRAY_SIZE(signature));

	/* Note that the PSA_ERROR_INVALID_ARGUMENT, expected here, has been
	 * found out using comparison of behaviour against unlocked key; when
	 * key is not locked this verification is successful, as long as
	 * ed25519 key with sha mentioned above is provisioned to the key slot.
	 * Ref. NCSDK-31298.
	 */
	zassert_not_equal(status, PSA_SUCCESS, "Verification should fail");
	zassert_equal(PSA_ERROR_INVALID_ARGUMENT, status, "Expected %d got %d",
		      PSA_ERROR_INVALID_ARGUMENT, status);
}

ZTEST_SUITE(key_lock_test, NULL, &test_init, NULL, NULL, NULL);
