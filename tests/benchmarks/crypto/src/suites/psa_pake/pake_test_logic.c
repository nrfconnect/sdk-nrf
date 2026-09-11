/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * What the three PAKE protocols here have in common. Each protocol's own file
 * holds its schedule and its measured/unmeasured split; see pake_test_logic.h.
 */

#include <string.h>

#include "pake_test_logic.h"

/* Changing this makes the srp_verifier constant in pake_srp6.c stale, and that
 * exchange then fails its key confirmation.
 */
static const uint8_t password[] = "password";

/*
 * The store the two sides pass messages through, sized for the widest message
 * (an SRP-6 key share, a full 3072-bit element) and for the most messages
 * written before any are read (J-PAKE's first round, two triples).
 *
 * One store serves every suite and both directions: suites run one at a time,
 * and within an exchange the schedule strictly alternates.
 */
#define MESSAGE_MAX_SIZE  PSA_BITS_TO_BYTES(3072)
#define MESSAGE_MAX_COUNT 6

static struct {
	uint8_t data[MESSAGE_MAX_SIZE];
	size_t length;
} messages[MESSAGE_MAX_COUNT];

psa_status_t pake_import_password(psa_algorithm_t algorithm, psa_key_id_t *key)
{
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_status_t status;

	psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_DERIVE);
	psa_set_key_algorithm(&attributes, algorithm);
	psa_set_key_type(&attributes, PSA_KEY_TYPE_PASSWORD);

	status = psa_import_key(&attributes, password, sizeof(password) - 1, key);
	psa_reset_key_attributes(&attributes);

	return status;
}

psa_status_t pake_setup_side(psa_pake_operation_t *operation,
			     const psa_pake_cipher_suite_t *suite, psa_key_id_t key,
			     const struct pake_side *side)
{
	psa_status_t status;

	status = psa_pake_setup(operation, key, suite);
	if (status != PSA_SUCCESS) {
		return status;
	}

	/* Before the identities: the core wants a role set when one arrives, and
	 * the role decides which side of the schedule the operation is held to.
	 */
	if (side->role != PSA_PAKE_ROLE_NONE) {
		status = psa_pake_set_role(operation, side->role);
		if (status != PSA_SUCCESS) {
			return status;
		}
	}

	status = psa_pake_set_user(operation, side->user, strlen(side->user));
	if (status != PSA_SUCCESS) {
		return status;
	}

	if (side->peer == NULL) {
		return PSA_SUCCESS;
	}

	return psa_pake_set_peer(operation, side->peer, strlen(side->peer));
}

psa_status_t pake_output_round(psa_pake_operation_t *operation, const psa_pake_step_t *steps,
			       size_t step_cnt)
{
	if (step_cnt > ARRAY_SIZE(messages)) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	for (size_t i = 0; i < step_cnt; i++) {
		psa_status_t status =
			psa_pake_output(operation, steps[i], messages[i].data,
					sizeof(messages[i].data), &messages[i].length);

		if (status != PSA_SUCCESS) {
			return status;
		}
	}

	return PSA_SUCCESS;
}

psa_status_t pake_input_round(psa_pake_operation_t *operation, const psa_pake_step_t *steps,
			      size_t step_cnt)
{
	if (step_cnt > ARRAY_SIZE(messages)) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	for (size_t i = 0; i < step_cnt; i++) {
		psa_status_t status = psa_pake_input(operation, steps[i], messages[i].data,
						     messages[i].length);

		if (status != PSA_SUCCESS) {
			return status;
		}
	}

	return PSA_SUCCESS;
}

psa_status_t pake_output_step(psa_pake_operation_t *operation, psa_pake_step_t step)
{
	return pake_output_round(operation, &step, 1);
}

psa_status_t pake_input_step(psa_pake_operation_t *operation, psa_pake_step_t step)
{
	return pake_input_round(operation, &step, 1);
}

psa_status_t pake_derive_secret(psa_pake_operation_t *operation, psa_algorithm_t kdf,
				const uint8_t *info, size_t info_length, uint8_t *secret)
{
	psa_key_derivation_operation_t derivation = PSA_KEY_DERIVATION_OPERATION_INIT;
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_key_id_t shared_key = 0;
	psa_status_t status;

	/* Policy checked when the key goes into the derivation below. */
	psa_set_key_type(&attributes, PSA_KEY_TYPE_DERIVE);
	psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_DERIVE);
	psa_set_key_algorithm(&attributes, kdf);

	status = psa_pake_get_shared_key(operation, &attributes, &shared_key);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	status = psa_key_derivation_setup(&derivation, kdf);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	status = psa_key_derivation_input_key(&derivation, PSA_KEY_DERIVATION_INPUT_SECRET,
					      shared_key);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	if (info != NULL) {
		status = psa_key_derivation_input_bytes(&derivation, PSA_KEY_DERIVATION_INPUT_INFO,
						       info, info_length);
		if (status != PSA_SUCCESS) {
			goto exit;
		}
	}

	status = psa_key_derivation_output_bytes(&derivation, secret, PAKE_SECRET_SIZE);

exit:
	(void)psa_key_derivation_abort(&derivation);
	if (shared_key != 0) {
		(void)psa_destroy_key(shared_key);
	}
	psa_reset_key_attributes(&attributes);

	return status;
}
