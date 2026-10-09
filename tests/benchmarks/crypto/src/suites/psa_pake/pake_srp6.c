/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * SRP-6, measured as the client. There is no standalone sample to draw the
 * parameters from; Cracen's SRP-6 is hardwired to SHA-512 over the RFC 3526
 * 3072-bit MODP group and a confirmed key, rejecting anything else at setup and
 * at key import alike.
 *
 * The client's key is the private key x a password hashes to, computed only by
 * PSA_ALG_SRP_PASSWORD_HASH, so its key setup row is a derivation rather than
 * an import. The server's half is the verifier, constant for the reason given
 * at srp_verifier below.
 *
 * psa_check_srp_sequence() leaves the two key shares independent, so the
 * server's B is computed in prepare; but the proofs are client-first and the
 * client cannot finish without M2, which needs both A and M1. Only those two
 * server steps stay measured, and its derivation runs in the check.
 */

#include <string.h>

#include "pake_test_logic.h"

/* SRP binds the private key to the user name, so both sides present the same
 * one. The core rejects psa_pake_set_peer() here, and that aborts the operation.
 */
static const char srp_user[] = "client";
static const struct pake_side client_side = {PSA_PAKE_ROLE_CLIENT, srp_user, NULL};
static const struct pake_side server_side = {PSA_PAKE_ROLE_SERVER, srp_user, NULL};

/* Historical text, from a SPAKE2+ registration this folder no longer does. It
 * cannot change without recomputing srp_verifier below.
 */
static const uint8_t salt[] = "spake2p registration salt";

/* Info string of the KDF that turns the agreed secret into bytes. */
static const uint8_t shared_key_info[] = "crypto benchmarks pake secret";

/* A key share and the verifier are both full elements of the 3072-bit group. */
#define SRP_VERIFIER_SIZE PSA_BITS_TO_BYTES(3072)

/*
 * The verifier a server would hold from registration.
 *
 * Constant because the device cannot produce it: psa_export_public_key() on a
 * derived SRP key pair reports PSA_ERROR_NOT_SUPPORTED, and PSA exposes no
 * modular exponentiation. Handing the server the key pair instead fails too --
 * Cracen's x and v share a union, so the "derive v from x" path in
 * cracen_srp_set_role() never sees a zero v and never runs.
 *
 * v = g^x mod N, g = 5 and N the RFC 3526 3072-bit prime, over
 *     x = SHA-512(salt | SHA-512(srp_user | ":" | password))
 * Changing the user, salt or password makes this stale, and key confirmation
 * then fails.
 */
static const uint8_t srp_verifier[SRP_VERIFIER_SIZE] = {
	0x68, 0x7E, 0x4F, 0x8C, 0x4E, 0x5E, 0x12, 0x72, 0x5B, 0xD0, 0x7C, 0x2F, 0x28, 0x12, 0x94,
	0xDB, 0xEB, 0x95, 0xE3, 0x45, 0xA2, 0x15, 0x6B, 0xEB, 0x22, 0x48, 0xBC, 0x07, 0xE9, 0xA9,
	0x4C, 0xF2, 0xAC, 0x05, 0x10, 0xED, 0x77, 0x76, 0x64, 0x2C, 0xD8, 0x9C, 0x7D, 0xB2, 0x3A,
	0x07, 0xE2, 0x50, 0x97, 0x34, 0xA2, 0x03, 0x48, 0xFF, 0x95, 0x87, 0xBF, 0xFE, 0x0F, 0xA2,
	0x14, 0x9A, 0x19, 0xB2, 0x51, 0x01, 0x1E, 0x0A, 0xFF, 0x87, 0x66, 0x00, 0x80, 0xFE, 0x3E,
	0x6A, 0x3C, 0xA8, 0xC6, 0xF5, 0x1F, 0xC7, 0x04, 0x5A, 0x6A, 0x02, 0x4C, 0x41, 0x98, 0xA1,
	0x71, 0x41, 0x3B, 0xD3, 0x03, 0x06, 0x02, 0x2E, 0x67, 0x6D, 0xB9, 0xF3, 0x6B, 0x1F, 0x03,
	0xE4, 0xCC, 0x6E, 0xAC, 0xB7, 0x3D, 0x25, 0x56, 0xEE, 0x6D, 0x37, 0x83, 0x29, 0xB9, 0x85,
	0x40, 0xE8, 0xBA, 0x5E, 0x36, 0x45, 0x62, 0x49, 0xA0, 0xF6, 0x02, 0x89, 0x09, 0xAB, 0xCE,
	0x1E, 0x90, 0x40, 0xD7, 0x2F, 0x22, 0xBA, 0x8E, 0xC7, 0x49, 0x1D, 0xC6, 0xBE, 0x09, 0xF7,
	0xD5, 0x79, 0x10, 0xC7, 0xC0, 0x33, 0xF5, 0x3A, 0x74, 0xAA, 0x7A, 0x96, 0x3D, 0xFA, 0x1E,
	0x2F, 0xD0, 0x15, 0x31, 0x26, 0x31, 0x28, 0xEA, 0x82, 0xF1, 0x35, 0x1E, 0x3B, 0xD7, 0xEC,
	0x2B, 0x19, 0x9F, 0x3F, 0x31, 0x51, 0xB5, 0x50, 0x12, 0xEB, 0x63, 0x98, 0x49, 0x84, 0xF7,
	0x72, 0xE2, 0x85, 0xF7, 0xBB, 0x07, 0xE2, 0xB6, 0x9F, 0x5D, 0x39, 0x86, 0x03, 0x3B, 0xDF,
	0x62, 0x1C, 0x2E, 0x89, 0xA9, 0xC8, 0x05, 0xC3, 0x8C, 0xF2, 0xB6, 0x04, 0x32, 0x9E, 0xAA,
	0x5A, 0x1A, 0x39, 0x0E, 0xEE, 0x61, 0x5C, 0xCA, 0xE4, 0xE1, 0x25, 0x4E, 0xA3, 0xEE, 0x21,
	0x08, 0x9E, 0x6E, 0x0E, 0x55, 0x0A, 0xBE, 0x0F, 0x71, 0xC2, 0x33, 0x83, 0x61, 0x02, 0x08,
	0x6F, 0xF7, 0xA3, 0xBB, 0xD1, 0x34, 0x44, 0xF1, 0x2A, 0x7F, 0x16, 0x7B, 0x56, 0xFC, 0xE8,
	0x8D, 0x45, 0xF6, 0x76, 0x7A, 0x76, 0x3D, 0x8A, 0x83, 0x9F, 0x60, 0x75, 0x09, 0xDA, 0xF9,
	0x3B, 0xBA, 0x8F, 0xB5, 0xEE, 0x3D, 0x6A, 0xDE, 0x52, 0xE9, 0x87, 0x2E, 0xE9, 0x47, 0xD7,
	0x87, 0x2D, 0xFE, 0xBB, 0xF5, 0x93, 0xBA, 0x5D, 0x43, 0x1C, 0x98, 0xF9, 0xB1, 0xF4, 0x5A,
	0x64, 0x50, 0x93, 0xC7, 0x6B, 0xF3, 0x2A, 0xBA, 0x78, 0xBA, 0xAE, 0x69, 0x4E, 0x4E, 0x46,
	0xD5, 0x03, 0x75, 0x0E, 0x1C, 0x82, 0x92, 0xCC, 0xD3, 0xF6, 0x21, 0xB5, 0xEA, 0x04, 0x15,
	0x4B, 0x59, 0xEF, 0xD9, 0x42, 0xEC, 0x0B, 0xC5, 0x32, 0x8C, 0x4F, 0x0D, 0xCF, 0x1F, 0x1F,
	0x3F, 0xCA, 0x0A, 0x71, 0x01, 0x9C, 0x80, 0x9B, 0xE7, 0x43, 0x4B, 0x1C, 0xAB, 0xC9, 0x18,
	0x9D, 0x04, 0x90, 0xA2, 0x31, 0x95, 0x17, 0x74, 0xA5
};

static psa_pake_operation_t server = PSA_PAKE_OPERATION_INIT;
static psa_key_id_t server_verifier_key;
static uint8_t server_secret[PAKE_SECRET_SIZE];
static const struct pake_test_data *server_test;

static psa_key_id_t client_key;
static uint8_t client_secret[PAKE_SECRET_SIZE];
static bool client_secret_valid;

static void cipher_suite(const struct pake_test_data *test, psa_pake_cipher_suite_t *suite)
{
	psa_pake_cs_set_algorithm(suite, test->algorithm);
	psa_pake_cs_set_primitive(suite, PSA_PAKE_PRIMITIVE(PSA_PAKE_PRIMITIVE_TYPE_DH,
							    test->family, test->bits));
	psa_pake_cs_set_key_confirmation(suite, PSA_PAKE_CONFIRMED_KEY);
}

/* Key setup, the server's half: the verifier it holds from registration. */
static psa_status_t server_import(const void *context)
{
	const struct pake_test_data *test = context;
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_status_t status;

	psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_DERIVE);
	psa_set_key_algorithm(&attributes, test->algorithm);
	psa_set_key_type(&attributes, PSA_KEY_TYPE_SRP_PUBLIC_KEY(test->family));
	psa_set_key_bits(&attributes, test->bits);

	status = psa_import_key(&attributes, srp_verifier, sizeof(srp_verifier),
			       &server_verifier_key);
	psa_reset_key_attributes(&attributes);

	return status;
}

/*
 * Key setup, measured: x = H(salt | H(user | ":" | password)). Only
 * PSA_ALG_SRP_PASSWORD_HASH computes it, and it wants the user name, the
 * password and the salt in that order.
 */
static psa_status_t client_derive_key(const void *context)
{
	const struct pake_test_data *test = context;
	psa_key_derivation_operation_t derivation = PSA_KEY_DERIVATION_OPERATION_INIT;
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_algorithm_t kdf = PSA_ALG_SRP_PASSWORD_HASH(PSA_ALG_GET_HASH(test->algorithm));
	psa_key_id_t password_key = 0;
	psa_status_t status;

	/* Only a password passed as a key object unlocks deriving a key from it. */
	status = pake_import_password(kdf, &password_key);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	status = psa_key_derivation_setup(&derivation, kdf);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	status = psa_key_derivation_input_bytes(&derivation, PSA_KEY_DERIVATION_INPUT_INFO,
					       (const uint8_t *)srp_user, strlen(srp_user));
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	status = psa_key_derivation_input_key(&derivation, PSA_KEY_DERIVATION_INPUT_PASSWORD,
					      password_key);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	status = psa_key_derivation_input_bytes(&derivation, PSA_KEY_DERIVATION_INPUT_SALT, salt,
					       sizeof(salt) - 1);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_DERIVE);
	psa_set_key_algorithm(&attributes, test->algorithm);
	psa_set_key_type(&attributes, PSA_KEY_TYPE_SRP_KEY_PAIR(test->family));
	psa_set_key_bits(&attributes, test->bits);

	status = psa_key_derivation_output_key(&attributes, &derivation, &client_key);

exit:
	(void)psa_key_derivation_abort(&derivation);
	if (password_key != 0) {
		(void)psa_destroy_key(password_key);
	}
	psa_reset_key_attributes(&attributes);

	return status;
}

/* All the server can do before the client speaks: setup, the salt it already
 * holds, and B, which the schedule does not make wait for the client's A.
 */
static psa_status_t server_prepare(const void *context)
{
	const struct pake_test_data *test = context;
	psa_pake_cipher_suite_t suite = PSA_PAKE_CIPHER_SUITE_INIT;
	psa_status_t status;

	cipher_suite(test, &suite);

	status = pake_setup_side(&server, &suite, server_verifier_key, &server_side);
	if (status != PSA_SUCCESS) {
		return status;
	}

	/* Not a peer message, but still fed in explicitly, and before the proofs. */
	status = psa_pake_input(&server, PSA_PAKE_STEP_SALT, salt, sizeof(salt) - 1);
	if (status != PSA_SUCCESS) {
		return status;
	}

	server_test = test;

	return pake_output_step(&server, PSA_PAKE_STEP_KEY_SHARE);
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

	status = pake_setup_side(&client, &suite, client_key, &client_side);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	status = psa_pake_input(&client, PSA_PAKE_STEP_SALT, salt, sizeof(salt) - 1);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	/* B first, so the client's A does not overwrite it; either order is legal. */
	status = pake_input_step(&client, PSA_PAKE_STEP_KEY_SHARE);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	status = pake_output_step(&client, PSA_PAKE_STEP_KEY_SHARE);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	status = pake_input_step(&server, PSA_PAKE_STEP_KEY_SHARE);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	/* The proofs are client-first: the reverse order is a bad state. */
	status = pake_output_step(&client, PSA_PAKE_STEP_CONFIRM);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	status = pake_input_step(&server, PSA_PAKE_STEP_CONFIRM);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	status = pake_output_step(&server, PSA_PAKE_STEP_CONFIRM);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	status = pake_input_step(&client, PSA_PAKE_STEP_CONFIRM);
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

/* Whether the exchange really agreed on something. The server sent M2 as the
 * exchange's last step, so only its own derivation is left.
 */
int srp6_check(void)
{
	psa_status_t status;

	if (!client_secret_valid || server_test == NULL) {
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

void srp6_cleanup(void)
{
	(void)psa_pake_abort(&server);
	server = psa_pake_operation_init();
	server_test = NULL;

	if (client_key != 0) {
		(void)psa_destroy_key(client_key);
		client_key = 0;
	}

	if (server_verifier_key != 0) {
		(void)psa_destroy_key(server_verifier_key);
		server_verifier_key = 0;
	}

	client_secret_valid = false;
}

const struct op srp6_keysetup_ops[] = {
	{.name = "derive", .fn = client_derive_key, .prepare = server_import},
};

const struct op srp6_exchange_ops[] = {
	{.name = "exchange", .fn = client_exchange, .prepare = server_prepare},
};

BUILD_ASSERT(ARRAY_SIZE(srp6_keysetup_ops) == PAKE_KEYSETUP_OP_COUNT,
	     "PAKE_KEYSETUP_OP_COUNT does not match the table");
BUILD_ASSERT(ARRAY_SIZE(srp6_exchange_ops) == PAKE_EXCHANGE_OP_COUNT,
	     "PAKE_EXCHANGE_OP_COUNT does not match the table");
