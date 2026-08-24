/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>

#include "hash_test_logic.h"

#define MAX_DIGEST_SIZE PSA_HASH_LENGTH(PSA_ALG_SHA_512)

static const uint8_t text[TEXT_SIZE] = "Crypto benchmarks hash test data.";
static uint8_t digest[MAX_DIGEST_SIZE];
static size_t digest_length;
/* For hash_check(), which takes no context. The operations read their own. */
static psa_algorithm_t digest_algorithm;

static psa_status_t compute_single(const void *context)
{
	const struct hash_test_data *test = context;

	digest_algorithm = test->algorithm;

	return psa_hash_compute(test->algorithm, text, sizeof(text), digest,
				test->digest_length, &digest_length);
}

static psa_status_t compute_multipart(const void *context)
{
	const struct hash_test_data *test = context;
	psa_hash_operation_t operation = PSA_HASH_OPERATION_INIT;
	psa_status_t status;

	digest_algorithm = test->algorithm;

	status = psa_hash_setup(&operation, test->algorithm);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	status = psa_hash_update(&operation, text, TEXT_HALF_SIZE);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	status = psa_hash_update(&operation, &text[TEXT_HALF_SIZE], TEXT_HALF_SIZE);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	/* psa_hash_finish() releases the operation on both success and failure. */
	return psa_hash_finish(&operation, digest, test->digest_length, &digest_length);

exit:
	(void)psa_hash_abort(&operation);

	return status;
}

int hash_check(void)
{
	if (digest_algorithm == 0) {
		return APP_ERROR;
	}

	if (psa_hash_compare(digest_algorithm, text, sizeof(text), digest,
			     digest_length) != PSA_SUCCESS) {
		return APP_ERROR;
	}

	return APP_SUCCESS;
}

const struct op hash_singlepart_ops[] = {{"compute", compute_single, NULL}};
const struct op hash_multipart_ops[] = {{"compute", compute_multipart, NULL}};
