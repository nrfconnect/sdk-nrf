/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>

#include "mac_test_logic.h"

#define MAC_SIZE PSA_MAC_MAX_SIZE

static const uint8_t text[TEXT_SIZE] = "Crypto benchmarks HMAC test data.";
static uint8_t mac[MAC_SIZE];
static size_t mac_length;
static psa_key_id_t key_id;
static bool key_created;

static psa_status_t generate_key(const void *context)
{
	const struct mac_test_data *test = context;
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_status_t status;

	psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_SIGN_HASH | PSA_KEY_USAGE_VERIFY_HASH);
	psa_set_key_algorithm(&attributes, test->algorithm);
	psa_set_key_type(&attributes, test->key_type);
	psa_set_key_bits(&attributes, test->key_bits);

	status = psa_generate_key(&attributes, &key_id);
	psa_reset_key_attributes(&attributes);

	key_created = status == PSA_SUCCESS;

	return status;
}

static psa_status_t sign(const void *context)
{
	const struct mac_test_data *test = context;
	psa_mac_operation_t operation = PSA_MAC_OPERATION_INIT;
	psa_status_t status;

	status = psa_mac_sign_setup(&operation, key_id, test->algorithm);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	status = psa_mac_update(&operation, text, TEXT_HALF_SIZE);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	status = psa_mac_update(&operation, &text[TEXT_HALF_SIZE], TEXT_HALF_SIZE);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	/* psa_mac_sign_finish() releases the operation on both success and failure. */
	return psa_mac_sign_finish(&operation, mac, sizeof(mac), &mac_length);

exit:
	(void)psa_mac_abort(&operation);

	return status;
}

static psa_status_t compute(const void *context)
{
	const struct mac_test_data *test = context;

	return psa_mac_compute(key_id, test->algorithm, text, sizeof(text), mac, sizeof(mac),
			       &mac_length);
}

static psa_status_t verify(const void *context)
{
	const struct mac_test_data *test = context;
	psa_mac_operation_t operation = PSA_MAC_OPERATION_INIT;
	psa_status_t status;

	status = psa_mac_verify_setup(&operation, key_id, test->algorithm);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	status = psa_mac_update(&operation, text, TEXT_HALF_SIZE);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	status = psa_mac_update(&operation, &text[TEXT_HALF_SIZE], TEXT_HALF_SIZE);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	/* psa_mac_verify_finish() releases the operation on both success and failure. */
	return psa_mac_verify_finish(&operation, mac, mac_length);

exit:
	(void)psa_mac_abort(&operation);

	return status;
}

void mac_cleanup(void)
{
	if (!key_created) {
		return;
	}

	(void)psa_destroy_key(key_id);
	key_created = false;
}

const struct op mac_keysetup_ops[] = {{"generate", generate_key, NULL}};
const struct op mac_singlepart_ops[] = {{"compute", compute, NULL}};
const struct op mac_operations[] = {{"sign", sign, NULL}, {"verify", verify, NULL}};
