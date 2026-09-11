/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>

#include "key_agreement_test_logic.h"

#define MAX_PUBLIC_KEY_SIZE 133
#define MAX_SECRET_SIZE 66

/*
 * Both sides cost the same, so measuring both would report one figure twice.
 * The rows measure Alice; Bob's keypair, public key and agreement run in a
 * prepare callback. His secret is still computed, because comparing the two is
 * the only thing that shows the agreement really agreed.
 */
static psa_key_id_t alice_key;
static psa_key_id_t bob_key;
static bool alice_created;
static bool bob_created;
static uint8_t alice_public_key[MAX_PUBLIC_KEY_SIZE];
static uint8_t bob_public_key[MAX_PUBLIC_KEY_SIZE];
static uint8_t alice_secret[MAX_SECRET_SIZE];
static uint8_t bob_secret[MAX_SECRET_SIZE];
static size_t alice_secret_length;
static size_t bob_secret_length;

static psa_status_t generate_key(const struct key_agreement_test_data *test, psa_key_id_t *key_id)
{
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_status_t status;

	psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_DERIVE);
	psa_set_key_algorithm(&attributes, PSA_ALG_ECDH);
	psa_set_key_type(&attributes, PSA_KEY_TYPE_ECC_KEY_PAIR(test->family));
	psa_set_key_bits(&attributes, test->bits);
	status = psa_generate_key(&attributes, key_id);
	psa_reset_key_attributes(&attributes);

	return status;
}

static psa_status_t generate(const void *context)
{
	psa_status_t status = generate_key(context, &alice_key);

	alice_created = status == PSA_SUCCESS;

	return status;
}

static psa_status_t export_public_key(const void *context)
{
	const struct key_agreement_test_data *test = context;
	size_t length;

	return psa_export_public_key(alice_key, alice_public_key, test->public_key_size, &length);
}

/* Bob, in full, unmeasured, so the row that follows is one
 * psa_raw_key_agreement() and nothing else.
 */
static psa_status_t prepare_peer(const void *context)
{
	const struct key_agreement_test_data *test = context;
	psa_status_t status;
	size_t length;

	status = generate_key(test, &bob_key);
	if (status != PSA_SUCCESS) {
		return status;
	}
	bob_created = true;

	status = psa_export_public_key(bob_key, bob_public_key, test->public_key_size, &length);
	if (status != PSA_SUCCESS) {
		return status;
	}

	return psa_raw_key_agreement(PSA_ALG_ECDH, bob_key, alice_public_key,
				     test->public_key_size, bob_secret, test->secret_size,
				     &bob_secret_length);
}

static psa_status_t agree(const void *context)
{
	const struct key_agreement_test_data *test = context;

	return psa_raw_key_agreement(PSA_ALG_ECDH, alice_key, bob_public_key,
				     test->public_key_size, alice_secret, test->secret_size,
				     &alice_secret_length);
}

int key_agreement_check(void)
{
	if (alice_secret_length == 0 || alice_secret_length != bob_secret_length) {
		return APP_ERROR;
	}

	if (memcmp(alice_secret, bob_secret, alice_secret_length) != 0) {
		return APP_ERROR;
	}

	return APP_SUCCESS;
}

void key_agreement_cleanup(void)
{
	/* Tracked apart, so one keypair is destroyed even if the other failed. */
	if (alice_created) {
		(void)psa_destroy_key(alice_key);
		alice_created = false;
	}

	if (bob_created) {
		(void)psa_destroy_key(bob_key);
		bob_created = false;
	}

	/* So a suite that never agreed cannot be checked against stale secrets. */
	alice_secret_length = 0;
	bob_secret_length = 0;
}

const struct op key_agreement_keysetup_ops[] = {
	{.name = "generate", .fn = generate},
	/* Bob agrees against this, so exporting it gates the rest of the suite. */
	{.name = "export", .fn = export_public_key},
};
const struct op key_agreement_operations[] = {
	{.name = "agree", .fn = agree, .prepare = prepare_peer},
};
