/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * SPAKE2+, measured as the client, with the parameters of
 * samples/crypto/spake2p.
 *
 * The client's key is w0 || w1 and the server's the record w0 || L, both
 * constants here, so each key setup row is a plain import.
 *
 * psa_check_spake2p_sequence() fixes the order as shareP, shareV, confirmV,
 * confirmP, so the server can do nothing before the client's share arrives and
 * the client cannot confirm before the server's two messages have. Only those
 * three server steps stay measured; its setup runs in prepare, its reading of
 * the client's confirmation and its derivation in the check.
 */

#include <string.h>

#include "pake_test_logic.h"

/* Client is the prover, server the verifier; both hash the same pair. */
static const struct pake_side client_side = {PSA_PAKE_ROLE_CLIENT, "client", "server"};
static const struct pake_side server_side = {PSA_PAKE_ROLE_SERVER, "server", "client"};

/* KDF info string, as in samples/crypto/spake2p. */
static const uint8_t shared_key_info[] = "Info";

/* The client's half of the registration record: w0 || w1. */
static const uint8_t key_pair[] = {0x54, 0x8E, 0xC1, 0x42, 0xE2, 0x27, 0x90, 0x23, 0x7C, 0x67, 0xA8,
				   0x88, 0x49, 0xE8, 0x61, 0xD3, 0x77, 0x00, 0x5F, 0x0A, 0x5C, 0x33,
				   0x88, 0xF9, 0xAF, 0xA1, 0xC2, 0xFA, 0x58, 0xC7, 0xDA, 0x51, 0x35,
				   0x99, 0xF8, 0x67, 0x1D, 0xBB, 0x67, 0x04, 0xA2, 0xC6, 0x3A, 0x78,
				   0x4F, 0xC9, 0x5C, 0xD2, 0x8E, 0xBC, 0x55, 0x2E, 0xA4, 0x79, 0x98,
				   0xB9, 0x18, 0x69, 0x9A, 0xB9, 0x3F, 0x4F, 0x7A, 0xD7};

/* The server's half of the same record: w0 || L. */
static const uint8_t public_key[] = {
	0x54, 0x8E, 0xC1, 0x42, 0xE2, 0x27, 0x90, 0x23, 0x7C, 0x67, 0xA8, 0x88, 0x49, 0xE8,
	0x61, 0xD3, 0x77, 0x00, 0x5F, 0x0A, 0x5C, 0x33, 0x88, 0xF9, 0xAF, 0xA1, 0xC2, 0xFA,
	0x58, 0xC7, 0xDA, 0x51, 0x04, 0x81, 0x43, 0x3D, 0xC5, 0x93, 0xC9, 0x46, 0xC9, 0x37,
	0xD9, 0x90, 0x26, 0xDD, 0x42, 0x14, 0x40, 0xE1, 0xC8, 0x7D, 0x0E, 0xC4, 0x94, 0x8B,
	0xFF, 0x59, 0xEA, 0xF4, 0x77, 0xE3, 0x35, 0xE5, 0x52, 0x49, 0x66, 0xB2, 0x03, 0x31,
	0x37, 0xD8, 0x4C, 0x65, 0x56, 0xDE, 0x07, 0x31, 0x57, 0x5C, 0xD2, 0x95, 0xC9, 0x75,
	0x12, 0x4F, 0x52, 0x13, 0x25, 0xF7, 0x80, 0x01, 0xEC, 0xBE, 0x67, 0xE8, 0xB7};

/* The server's two answers go out together: written one at a time, the second
 * would overwrite the first in the store.
 */
static const psa_pake_step_t client_share_steps[] = {PSA_PAKE_STEP_KEY_SHARE};
static const psa_pake_step_t server_reply_steps[] = {PSA_PAKE_STEP_KEY_SHARE,
						     PSA_PAKE_STEP_CONFIRM};
static const psa_pake_step_t client_confirm_steps[] = {PSA_PAKE_STEP_CONFIRM};

static psa_pake_operation_t server = PSA_PAKE_OPERATION_INIT;
static psa_key_id_t server_public_key;
static uint8_t server_secret[PAKE_SECRET_SIZE];
static const struct pake_test_data *server_test;

static psa_key_id_t client_key_pair;
static uint8_t client_secret[PAKE_SECRET_SIZE];
static bool client_secret_valid;

static void cipher_suite(const struct pake_test_data *test, psa_pake_cipher_suite_t *suite)
{
	psa_pake_cs_set_algorithm(suite, test->algorithm);
	psa_pake_cs_set_primitive(suite, PSA_PAKE_PRIMITIVE(PSA_PAKE_PRIMITIVE_TYPE_ECC,
							    test->family, test->bits));
	psa_pake_cs_set_key_confirmation(suite, PSA_PAKE_CONFIRMED_KEY);
}

static psa_status_t import_key(const struct pake_test_data *test, psa_key_type_t type,
			       const uint8_t *data, size_t data_length, psa_key_id_t *key)
{
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_status_t status;

	psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_DERIVE);
	psa_set_key_algorithm(&attributes, test->algorithm);
	psa_set_key_type(&attributes, type);
	psa_set_key_bits(&attributes, test->bits);

	status = psa_import_key(&attributes, data, data_length, key);
	psa_reset_key_attributes(&attributes);

	return status;
}

/* Key setup, the server's half: the record a registration left it with. */
static psa_status_t server_import(const void *context)
{
	const struct pake_test_data *test = context;

	return import_key(test, PSA_KEY_TYPE_SPAKE2P_PUBLIC_KEY(test->family), public_key,
			  sizeof(public_key), &server_public_key);
}

/* Key setup, measured: the pair the client must hold before starting. */
static psa_status_t client_import(const void *context)
{
	const struct pake_test_data *test = context;

	return import_key(test, PSA_KEY_TYPE_SPAKE2P_KEY_PAIR(test->family), key_pair,
			  sizeof(key_pair), &client_key_pair);
}

/* All the server can do before the client's key share arrives. */
static psa_status_t server_prepare(const void *context)
{
	const struct pake_test_data *test = context;
	psa_pake_cipher_suite_t suite = PSA_PAKE_CIPHER_SUITE_INIT;
	psa_status_t status;

	cipher_suite(test, &suite);

	status = pake_setup_side(&server, &suite, server_public_key, &server_side);
	if (status != PSA_SUCCESS) {
		return status;
	}

	server_test = test;

	return PSA_SUCCESS;
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

	status = pake_setup_side(&client, &suite, client_key_pair, &client_side);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	status = pake_output_round(&client, client_share_steps, ARRAY_SIZE(client_share_steps));
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	status = pake_input_round(&server, client_share_steps, ARRAY_SIZE(client_share_steps));
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	status = pake_output_round(&server, server_reply_steps, ARRAY_SIZE(server_reply_steps));
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	status = pake_input_round(&client, server_reply_steps, ARRAY_SIZE(server_reply_steps));
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	status = pake_output_round(&client, client_confirm_steps, ARRAY_SIZE(client_confirm_steps));
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	status = pake_derive_secret(&client, test->kdf, shared_key_info,
				    sizeof(shared_key_info) - 1, client_secret);
	client_secret_valid = status == PSA_SUCCESS;

exit:
	(void)psa_pake_abort(&client);

	return status;
}

/* Whether the exchange really agreed on something. The client waits for neither
 * of these steps, so neither belongs in the measured row.
 */
int spake2p_check(void)
{
	psa_status_t status;

	if (!client_secret_valid || server_test == NULL) {
		return APP_ERROR;
	}

	status = pake_input_round(&server, client_confirm_steps,
				  ARRAY_SIZE(client_confirm_steps));
	if (status != PSA_SUCCESS) {
		return APP_ERROR;
	}

	status = pake_derive_secret(&server, server_test->kdf, shared_key_info,
				    sizeof(shared_key_info) - 1, server_secret);
	if (status != PSA_SUCCESS) {
		return APP_ERROR;
	}

	if (memcmp(client_secret, server_secret, PAKE_SECRET_SIZE) != 0) {
		return APP_ERROR;
	}

	return APP_SUCCESS;
}

void spake2p_cleanup(void)
{
	(void)psa_pake_abort(&server);
	server = psa_pake_operation_init();
	server_test = NULL;

	if (client_key_pair != 0) {
		(void)psa_destroy_key(client_key_pair);
		client_key_pair = 0;
	}

	if (server_public_key != 0) {
		(void)psa_destroy_key(server_public_key);
		server_public_key = 0;
	}

	client_secret_valid = false;
}

const struct op spake2p_keysetup_ops[] = {
	{.name = "import", .fn = client_import, .prepare = server_import},
};

const struct op spake2p_exchange_ops[] = {
	{.name = "exchange", .fn = client_exchange, .prepare = server_prepare},
};

BUILD_ASSERT(ARRAY_SIZE(spake2p_keysetup_ops) == PAKE_KEYSETUP_OP_COUNT,
	     "PAKE_KEYSETUP_OP_COUNT does not match the table");
BUILD_ASSERT(ARRAY_SIZE(spake2p_exchange_ops) == PAKE_EXCHANGE_OP_COUNT,
	     "PAKE_EXCHANGE_OP_COUNT does not match the table");
