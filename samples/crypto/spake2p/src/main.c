/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <psa/crypto.h>
#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(spake2p, LOG_LEVEL_DBG);

#define PRINT_HEX(p_label, p_text, len)                                                            \
	({                                                                                         \
		LOG_INF("---- %s (len: %u): ----", p_label, len);                                  \
		LOG_HEXDUMP_INF(p_text, len, "Content:");                                          \
		LOG_INF("---- %s end  ----", p_label);                                             \
	})

#define APP_SUCCESS	    (0)
#define APP_ERROR	    (-1)
#define APP_SUCCESS_MESSAGE "Example finished successfully!"
#define APP_ERROR_MESSAGE   "Example exited with error!"

/* w0 || w1 */
static const uint8_t key_pair[] = {0x54, 0x8E, 0xC1, 0x42, 0xE2, 0x27, 0x90, 0x23, 0x7C, 0x67, 0xA8,
				   0x88, 0x49, 0xE8, 0x61, 0xD3, 0x77, 0x00, 0x5F, 0x0A, 0x5C, 0x33,
				   0x88, 0xF9, 0xAF, 0xA1, 0xC2, 0xFA, 0x58, 0xC7, 0xDA, 0x51, 0x35,
				   0x99, 0xF8, 0x67, 0x1D, 0xBB, 0x67, 0x04, 0xA2, 0xC6, 0x3A, 0x78,
				   0x4F, 0xC9, 0x5C, 0xD2, 0x8E, 0xBC, 0x55, 0x2E, 0xA4, 0x79, 0x98,
				   0xB9, 0x18, 0x69, 0x9A, 0xB9, 0x3F, 0x4F, 0x7A, 0xD7};

/* w0 || L */
static const uint8_t public_key[] = {
	0x54, 0x8E, 0xC1, 0x42, 0xE2, 0x27, 0x90, 0x23, 0x7C, 0x67, 0xA8, 0x88, 0x49, 0xE8,
	0x61, 0xD3, 0x77, 0x00, 0x5F, 0x0A, 0x5C, 0x33, 0x88, 0xF9, 0xAF, 0xA1, 0xC2, 0xFA,
	0x58, 0xC7, 0xDA, 0x51, 0x04, 0x81, 0x43, 0x3D, 0xC5, 0x93, 0xC9, 0x46, 0xC9, 0x37,
	0xD9, 0x90, 0x26, 0xDD, 0x42, 0x14, 0x40, 0xE1, 0xC8, 0x7D, 0x0E, 0xC4, 0x94, 0x8B,
	0xFF, 0x59, 0xEA, 0xF4, 0x77, 0xE3, 0x35, 0xE5, 0x52, 0x49, 0x66, 0xB2, 0x03, 0x31,
	0x37, 0xD8, 0x4C, 0x65, 0x56, 0xDE, 0x07, 0x31, 0x57, 0x5C, 0xD2, 0x95, 0xC9, 0x75,
	0x12, 0x4F, 0x52, 0x13, 0x25, 0xF7, 0x80, 0x01, 0xEC, 0xBE, 0x67, 0xE8, 0xB7};

static psa_status_t send_message(psa_pake_operation_t *from, psa_pake_operation_t *to,
				 psa_pake_step_t step)
{
	uint8_t data[1024];
	size_t length;

	psa_status_t status = psa_pake_output(from, step, data, sizeof(data), &length);

	if (status) {
		return status;
	}
	PRINT_HEX("send_message", data, length);
	status = psa_pake_input(to, step, data, length);
	return status;
}

static psa_status_t pake_setup(psa_pake_operation_t *op, psa_pake_role_t role, psa_key_id_t key,
			       psa_pake_cipher_suite_t *cipher_suite)
{
	psa_status_t status;

	status = psa_pake_setup(op, key, cipher_suite);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_pake_setup failed. (Error: %d)", status);
		return status;
	}

	status = psa_pake_set_role(op, role);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_pake_set_role failed. (Error: %d)", status);
		return status;
	}

	if (role == PSA_PAKE_ROLE_SERVER) {
		status = psa_pake_set_user(op, "client", strlen("client"));
		if (status != PSA_SUCCESS) {
			LOG_INF("psa_pake_set_user failed. (Error: %d)", status);
			return status;
		}

		status = psa_pake_set_peer(op, "server", strlen("server"));
		if (status != PSA_SUCCESS) {
			LOG_INF("psa_pake_set_peer failed. (Error: %d)", status);
			return status;
		}
	} else {
		status = psa_pake_set_user(op, "server", strlen("server"));
		if (status != PSA_SUCCESS) {
			LOG_INF("psa_pake_set_user failed. (Error: %d)", status);
			return status;
		}

		status = psa_pake_set_peer(op, "client", strlen("client"));
		if (status != PSA_SUCCESS) {
			LOG_INF("psa_pake_set_peer failed. (Error: %d)", status);
			return status;
		}
	}

	return status;
}

int main(void)
{
	/* For Matter-compatible Spake2+, this will be PSA_ALG_SPAKE2P_MATTER */
	psa_algorithm_t psa_spake_alg = PSA_ALG_SPAKE2P_HMAC(PSA_ALG_SHA_256);
	psa_status_t status = psa_crypto_init();

	if (status != PSA_SUCCESS) {
		LOG_INF("psa_crypto_init failed. (Error: %d)", status);
	}

	psa_pake_cipher_suite_t cipher_suite = PSA_PAKE_CIPHER_SUITE_INIT;

	psa_pake_cs_set_algorithm(&cipher_suite, psa_spake_alg);
	psa_pake_cs_set_primitive(&cipher_suite, PSA_PAKE_PRIMITIVE(PSA_PAKE_PRIMITIVE_TYPE_ECC,
								    PSA_ECC_FAMILY_SECP_R1, 256));
	psa_pake_cs_set_key_confirmation(&cipher_suite, PSA_PAKE_CONFIRMED_KEY);

	psa_key_attributes_t key_attributes = PSA_KEY_ATTRIBUTES_INIT;

	psa_set_key_usage_flags(&key_attributes, PSA_KEY_USAGE_DERIVE);
	psa_set_key_algorithm(&key_attributes, psa_spake_alg);
	psa_set_key_type(&key_attributes, PSA_KEY_TYPE_SPAKE2P_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
	psa_set_key_bits(&key_attributes, 256);

	psa_key_id_t key;

	status = psa_import_key(&key_attributes, key_pair, sizeof(key_pair), &key);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_import_key failed. (Error: %d)", status);
		goto error;
	}

	psa_set_key_usage_flags(&key_attributes, PSA_KEY_USAGE_DERIVE);
	psa_set_key_algorithm(&key_attributes, psa_spake_alg);
	psa_set_key_type(&key_attributes, PSA_KEY_TYPE_SPAKE2P_PUBLIC_KEY(PSA_ECC_FAMILY_SECP_R1));
	psa_set_key_bits(&key_attributes, 256);

	psa_key_id_t key_server;

	status = psa_import_key(&key_attributes, public_key, sizeof(public_key), &key_server);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_import_key failed. (Error: %d)", status);
		goto error;
	}

	psa_reset_key_attributes(&key_attributes);

	psa_pake_operation_t client_op = PSA_PAKE_OPERATION_INIT;

	status = pake_setup(&client_op, PSA_PAKE_ROLE_CLIENT, key, &cipher_suite);
	if (status != PSA_SUCCESS) {
		goto error;
	}

	psa_pake_operation_t server_op = PSA_PAKE_OPERATION_INIT;

	status = pake_setup(&server_op, PSA_PAKE_ROLE_SERVER, key_server, &cipher_suite);
	if (status != PSA_SUCCESS) {
		goto error;
	}

	/* Share P */
	status = send_message(&client_op, &server_op, PSA_PAKE_STEP_KEY_SHARE);
	if (status != PSA_SUCCESS) {
		LOG_INF("send_message failed. (Error: %d, step: %d)", status,
			PSA_PAKE_STEP_KEY_SHARE);
		goto error;
	}

	/* Share V */
	status = send_message(&server_op, &client_op, PSA_PAKE_STEP_KEY_SHARE);
	if (status != PSA_SUCCESS) {
		LOG_INF("send_message failed. (Error: %d, step: %d)", status,
			PSA_PAKE_STEP_KEY_SHARE);
		goto error;
	}

	/* Confirm P */
	status = send_message(&server_op, &client_op, PSA_PAKE_STEP_CONFIRM);
	if (status != PSA_SUCCESS) {
		LOG_INF("send_message failed. (Error: %d, step: %d)", status,
			PSA_PAKE_STEP_CONFIRM);
		goto error;
	}

	/* Confirm V */
	status = send_message(&client_op, &server_op, PSA_PAKE_STEP_CONFIRM);
	if (status != PSA_SUCCESS) {
		LOG_INF("send_message failed. (Error: %d, step: %d)", status,
			PSA_PAKE_STEP_CONFIRM);
		goto error;
	}

	psa_key_derivation_operation_t kdf = PSA_KEY_DERIVATION_OPERATION_INIT;
	uint8_t client_secret[32] = {0};
	uint8_t server_secret[32] = {0};

	psa_key_attributes_t shared_key_attributes = PSA_KEY_ATTRIBUTES_INIT;

	psa_set_key_type(&shared_key_attributes, PSA_KEY_TYPE_DERIVE);
	psa_set_key_usage_flags(&shared_key_attributes, PSA_KEY_USAGE_DERIVE);
	psa_set_key_algorithm(&shared_key_attributes, PSA_ALG_HKDF(PSA_ALG_SHA_256));

	struct {
		psa_pake_operation_t *op;
		uint8_t *secret;
	} client_server[] = {{&client_op, client_secret}, {&server_op, server_secret}};

	for (size_t i = 0; i < ARRAY_SIZE(client_server); i++) {
		psa_key_id_t key;

		status = psa_pake_get_shared_key(client_server[i].op, &shared_key_attributes, &key);
		if (status != PSA_SUCCESS) {
			LOG_INF("psa_pake_get_shared_key failed. (Error: %d)", status);
			goto error;
		}

		status = psa_key_derivation_setup(&kdf, PSA_ALG_HKDF(PSA_ALG_SHA_256));
		if (status != PSA_SUCCESS) {
			LOG_INF("psa_key_derivation_setup failed. (Error: %d)", status);
			goto error;
		}

		status = psa_key_derivation_input_key(&kdf, PSA_KEY_DERIVATION_INPUT_SECRET, key);
		if (status != PSA_SUCCESS) {
			LOG_INF("psa_key_derivation_input_key failed. (Error: %d)", status);
			goto error;
		}

		status = psa_key_derivation_input_bytes(&kdf, PSA_KEY_DERIVATION_INPUT_INFO, "Info",
							4);
		if (status != PSA_SUCCESS) {
			LOG_INF("psa_key_derivation_input_bytes failed. (Error: %d)", status);
			goto error;
		}

		status = psa_key_derivation_output_bytes(&kdf, client_server[i].secret,
							 sizeof(client_secret));
		if (status != PSA_SUCCESS) {
			LOG_INF("psa_key_derivation_output_bytes failed. (Error: %d)", status);
			goto error;
		}

		status = psa_key_derivation_abort(&kdf);
		if (status != PSA_SUCCESS) {
			LOG_INF("psa_key_derivation_abort failed. (Error: %d)", status);
			goto error;
		}

		status = psa_destroy_key(key);
		if (status != PSA_SUCCESS) {
			LOG_INF("psa_destroy_key failed. (Error: %d)", status);
			goto error;
		}
	}

	psa_reset_key_attributes(&shared_key_attributes);
	PRINT_HEX("server_secret", client_secret, sizeof(client_secret));
	PRINT_HEX("client_secret", server_secret, sizeof(server_secret));

	bool compare_eq = true;

	for (size_t i = 0; i < sizeof(server_secret); i++) {
		if (server_secret[i] != client_secret[i]) {
			compare_eq = false;
		}
	}

	if (!compare_eq) {
		LOG_ERR("Derived keys for server and client are not equal.");
		goto error;
	}

	LOG_INF(APP_SUCCESS_MESSAGE);
	return APP_SUCCESS;

error:
	LOG_INF(APP_ERROR_MESSAGE);
	return APP_ERROR;
}
