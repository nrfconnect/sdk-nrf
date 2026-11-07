/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>

#include "key_derivation_test_logic.h"

#define MAX_OUTPUT_SIZE 64

/*
 * One salt and one info string for every suite. The check only re-derives and
 * compares, so no value here has to be any particular one.
 *
 * The cost is in the two TLS 1.2 suites, where PSA feeds these as SEED and
 * LABEL: real TLS 1.2 would send the two handshake randoms and a label from RFC
 * 5246. Same work, so the timings hold; the inputs are just not a handshake's.
 */
static const uint8_t derivation_salt[] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c,
};
static const uint8_t derivation_info[] = "crypto benchmarks key derivation";

static psa_key_id_t input_key_id;
static bool key_created;
static uint8_t output[MAX_OUTPUT_SIZE];
/* A second derivation from the same inputs, for the check. Produced in the
 * prepare callback, so it costs the measured derivation nothing.
 */
static uint8_t repeated[MAX_OUTPUT_SIZE];
/* Bytes produced, and so compared. Zero until a derivation has run. */
static size_t derived_length;

static psa_status_t generate_key(const void *context)
{
	const struct key_derivation_test_data *test = context;
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_status_t status;

	psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_DERIVE);
	psa_set_key_algorithm(&attributes, test->algorithm);
	psa_set_key_type(&attributes, test->key_type);
	psa_set_key_bits(&attributes, test->key_bits);

	status = psa_generate_key(&attributes, &input_key_id);
	psa_reset_key_attributes(&attributes);

	key_created = status == PSA_SUCCESS;

	return status;
}

static psa_status_t derive_into(const struct key_derivation_test_data *test, uint8_t *out)
{
	psa_key_derivation_operation_t operation = PSA_KEY_DERIVATION_OPERATION_INIT;
	/* TLS 1.2 takes SEED/SECRET/LABEL, not HKDF's SALT/SECRET/INFO. */
	bool is_tls12 = PSA_ALG_IS_TLS12_PRF(test->algorithm) ||
			PSA_ALG_IS_TLS12_PSK_TO_MS(test->algorithm);
	/* PBKDF2 has no third input: the other three take an info or a label. */
	bool takes_info = !PSA_ALG_IS_PBKDF2_HMAC(test->algorithm);
	psa_status_t status;

	status = psa_key_derivation_setup(&operation, test->algorithm);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	if (test->cost != 0) {
		status = psa_key_derivation_input_integer(&operation,
							 PSA_KEY_DERIVATION_INPUT_COST,
							 test->cost);
		if (status != PSA_SUCCESS) {
			goto exit;
		}
	}

	status = psa_key_derivation_input_bytes(&operation,
						is_tls12 ? PSA_KEY_DERIVATION_INPUT_SEED
							 : PSA_KEY_DERIVATION_INPUT_SALT,
						derivation_salt, sizeof(derivation_salt));
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	status = psa_key_derivation_input_key(&operation,
					      test->key_type == PSA_KEY_TYPE_PASSWORD
						      ? PSA_KEY_DERIVATION_INPUT_PASSWORD
						      : PSA_KEY_DERIVATION_INPUT_SECRET,
					      input_key_id);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	if (takes_info) {
		status = psa_key_derivation_input_bytes(&operation,
							is_tls12 ? PSA_KEY_DERIVATION_INPUT_LABEL
								 : PSA_KEY_DERIVATION_INPUT_INFO,
							derivation_info,
							sizeof(derivation_info) - 1);
		if (status != PSA_SUCCESS) {
			goto exit;
		}
	}

	status = psa_key_derivation_output_bytes(&operation, out, test->output_size);
	if (status == PSA_SUCCESS) {
		derived_length = test->output_size;
	}

exit:
	(void)psa_key_derivation_abort(&operation);

	return status;
}

static psa_status_t derive_repeat(const void *context)
{
	return derive_into(context, repeated);
}

static psa_status_t derive(const void *context)
{
	return derive_into(context, output);
}

/* The input key is random, so the only expectation is that the same inputs
 * derive the same bytes.
 */
int key_derivation_check(void)
{
	if (derived_length == 0) {
		return APP_ERROR;
	}

	if (memcmp(output, repeated, derived_length) != 0) {
		return APP_ERROR;
	}

	return APP_SUCCESS;
}

void key_derivation_cleanup(void)
{
	/* So a suite that never derived cannot be checked against stale bytes. */
	derived_length = 0;

	if (!key_created) {
		return;
	}

	(void)psa_destroy_key(input_key_id);
	key_created = false;
}

const struct op key_derivation_keysetup_ops[] = {{"generate", generate_key, NULL}};
const struct op key_derivation_operations[] = {
	{.name = "derive", .fn = derive, .prepare = derive_repeat},
};
