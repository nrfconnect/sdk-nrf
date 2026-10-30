/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "xof_test_logic.h"

#define OUTPUT_SIZE 64

static const uint8_t text[TEXT_SIZE] = "Crypto benchmarks XOF test data.";
static uint8_t output[OUTPUT_SIZE];

/* Absorbs in two updates of half the message each, then squeezes. */
static psa_status_t squeeze(psa_algorithm_t algorithm, uint8_t *out, size_t out_size)
{
	psa_xof_operation_t operation = PSA_XOF_OPERATION_INIT;
	psa_status_t status;

	status = psa_xof_setup(&operation, algorithm);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	status = psa_xof_update(&operation, text, TEXT_HALF_SIZE);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	status = psa_xof_update(&operation, &text[TEXT_HALF_SIZE], TEXT_HALF_SIZE);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	status = psa_xof_output(&operation, out, out_size);

exit:
	/* A XOF stays live after a successful squeeze, so it must be released. */
	(void)psa_xof_abort(&operation);

	return status;
}

static psa_status_t compute(const void *context)
{
	const struct xof_test_data *test = context;

	return squeeze(test->algorithm, output, sizeof(output));
}

/*
 * No check callback: squeezing twice and comparing would catch only driver state
 * corruption, never a wrong answer, and a known-answer vector would have to pin
 * both TEXT_SIZE and the message bytes, which are meant to stay adjustable.
 */
const struct op xof_multipart_ops[] = {{"compute", compute, NULL}};
