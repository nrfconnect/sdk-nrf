/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * EC J-PAKE, measured as the client, with the parameters of
 * samples/crypto/ecjpake.
 *
 * Of the server's steps only reading round 1 and answering round 2 stay inside
 * the measured region: it cannot produce round 2 before reading the client's
 * round 1, and the client cannot finish without round 2. Its setup and first
 * round run in prepare; its last step and its derivation in the check.
 */

#include <string.h>

#include "pake_test_logic.h"

/* J-PAKE is symmetric, so the identities are all that tells the sides apart. */
static const struct pake_side client_side = {PSA_PAKE_ROLE_NONE, "client", "server"};
static const struct pake_side server_side = {PSA_PAKE_ROLE_NONE, "server", "client"};

/*
 * Round 1 is two (key share, ZK public, ZK proof) triples per side, round 2 one
 * more; only then can an operation hand out a key.
 *
 * psa_check_jpake_sequence() holds a side to all six steps of one direction
 * before the six of the other, counting steps rather than messages, so neither
 * side can tell how the directions interleaved in time. That is what lets the
 * server write its whole first round in prepare. The server writes first, as in
 * samples/crypto/ecjpake.
 */
static const psa_pake_step_t round1_steps[] = {
	PSA_PAKE_STEP_KEY_SHARE, PSA_PAKE_STEP_ZK_PUBLIC, PSA_PAKE_STEP_ZK_PROOF,
	PSA_PAKE_STEP_KEY_SHARE, PSA_PAKE_STEP_ZK_PUBLIC, PSA_PAKE_STEP_ZK_PROOF};
static const psa_pake_step_t round2_steps[] = {PSA_PAKE_STEP_KEY_SHARE, PSA_PAKE_STEP_ZK_PUBLIC,
					       PSA_PAKE_STEP_ZK_PROOF};

/* Static: none of the server's state belongs on the client's stack. The context
 * comes from prepare, because the check takes no argument of its own.
 */
static psa_pake_operation_t server = PSA_PAKE_OPERATION_INIT;
static psa_key_id_t server_password_key;
static uint8_t server_secret[PAKE_SECRET_SIZE];
static const struct pake_test_data *server_test;

/* Outlives the row that imports it; a key id is a handle, so it costs no stack. */
static psa_key_id_t client_password_key;
static uint8_t client_secret[PAKE_SECRET_SIZE];
static bool client_secret_valid;

static void cipher_suite(const struct pake_test_data *test, psa_pake_cipher_suite_t *suite)
{
	psa_pake_cs_set_algorithm(suite, test->algorithm);
	psa_pake_cs_set_primitive(suite, PSA_PAKE_PRIMITIVE(PSA_PAKE_PRIMITIVE_TYPE_ECC,
							    test->family, test->bits));
	psa_pake_cs_set_key_confirmation(suite, PSA_PAKE_UNCONFIRMED_KEY);
}

/* Key setup, the server's half: its own key object, from the same password. */
static psa_status_t server_import(const void *context)
{
	const struct pake_test_data *test = context;

	return pake_import_password(test->algorithm, &server_password_key);
}

/* Key setup, measured: the key the client has to hold before it can start. */
static psa_status_t client_import(const void *context)
{
	const struct pake_test_data *test = context;

	return pake_import_password(test->algorithm, &client_password_key);
}

/* All the server can do before the client speaks: setup and its first round. */
static psa_status_t server_prepare(const void *context)
{
	const struct pake_test_data *test = context;
	psa_pake_cipher_suite_t suite = PSA_PAKE_CIPHER_SUITE_INIT;
	psa_status_t status;

	cipher_suite(test, &suite);

	status = pake_setup_side(&server, &suite, server_password_key, &server_side);
	if (status != PSA_SUCCESS) {
		return status;
	}

	server_test = test;

	return pake_output_round(&server, round1_steps, ARRAY_SIZE(round1_steps));
}

/* The client's exchange, from its own setup to its derived secret. */
static psa_status_t client_exchange(const void *context)
{
	const struct pake_test_data *test = context;
	psa_pake_cipher_suite_t suite = PSA_PAKE_CIPHER_SUITE_INIT;
	/* Local, so the operation's own footprint lands in this row's stack figure. */
	psa_pake_operation_t client = PSA_PAKE_OPERATION_INIT;
	psa_status_t status;

	cipher_suite(test, &suite);

	status = pake_setup_side(&client, &suite, client_password_key, &client_side);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	status = pake_input_round(&client, round1_steps, ARRAY_SIZE(round1_steps));
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	status = pake_output_round(&client, round1_steps, ARRAY_SIZE(round1_steps));
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	status = pake_input_round(&server, round1_steps, ARRAY_SIZE(round1_steps));
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	status = pake_output_round(&server, round2_steps, ARRAY_SIZE(round2_steps));
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	status = pake_input_round(&client, round2_steps, ARRAY_SIZE(round2_steps));
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	status = pake_output_round(&client, round2_steps, ARRAY_SIZE(round2_steps));
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	/* The secret is a curve point; TLS12_ECJPAKE_TO_PMS is all PSA will do
	 * with it, and it takes the secret alone.
	 */
	status = pake_derive_secret(&client, test->kdf, NULL, 0, client_secret);
	client_secret_valid = status == PSA_SUCCESS;

exit:
	(void)psa_pake_abort(&client);

	return status;
}

/*
 * Whether the exchange really agreed on something. The server's last steps run
 * here because the client never waits for them: its round 2 is still in the
 * store, which is all the server needs.
 */
int ecjpake_check(void)
{
	psa_status_t status;

	/* A failed exchange leaves these clear, so a stale secret cannot pass. */
	if (!client_secret_valid || server_test == NULL) {
		return APP_ERROR;
	}

	status = pake_input_round(&server, round2_steps, ARRAY_SIZE(round2_steps));
	if (status != PSA_SUCCESS) {
		return APP_ERROR;
	}

	status = pake_derive_secret(&server, server_test->kdf, NULL, 0, server_secret);
	if (status != PSA_SUCCESS) {
		return APP_ERROR;
	}

	if (memcmp(client_secret, server_secret, PAKE_SECRET_SIZE) != 0) {
		return APP_ERROR;
	}

	return APP_SUCCESS;
}

void ecjpake_cleanup(void)
{
	/* Reinitialized, not just aborted: the suite may have stopped anywhere. */
	(void)psa_pake_abort(&server);
	server = psa_pake_operation_init();
	server_test = NULL;

	if (client_password_key != 0) {
		(void)psa_destroy_key(client_password_key);
		client_password_key = 0;
	}

	if (server_password_key != 0) {
		(void)psa_destroy_key(server_password_key);
		server_password_key = 0;
	}

	client_secret_valid = false;
}

const struct op ecjpake_keysetup_ops[] = {
	{.name = "import", .fn = client_import, .prepare = server_import},
};

const struct op ecjpake_exchange_ops[] = {
	{.name = "exchange", .fn = client_exchange, .prepare = server_prepare},
};

/* Nothing but this ties the tables to the counts declared in the header. */
BUILD_ASSERT(ARRAY_SIZE(ecjpake_keysetup_ops) == PAKE_KEYSETUP_OP_COUNT,
	     "PAKE_KEYSETUP_OP_COUNT does not match the table");
BUILD_ASSERT(ARRAY_SIZE(ecjpake_exchange_ops) == PAKE_EXCHANGE_OP_COUNT,
	     "PAKE_EXCHANGE_OP_COUNT does not match the table");
