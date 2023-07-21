/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <psa/crypto.h>
#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(ecjpake, LOG_LEVEL_DBG);

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

psa_status_t do_rounds(psa_pake_operation_t *server, psa_pake_operation_t *client)
{
	psa_pake_step_t round1_steps[] = {PSA_PAKE_STEP_KEY_SHARE, PSA_PAKE_STEP_ZK_PUBLIC,
					  PSA_PAKE_STEP_ZK_PROOF,  PSA_PAKE_STEP_KEY_SHARE,
					  PSA_PAKE_STEP_ZK_PUBLIC, PSA_PAKE_STEP_ZK_PROOF};
	uint8_t buffer[128];
	size_t outlen;
	psa_status_t status;

	/* Server provides data to client */
	for (uint32_t i = 0; i < ARRAY_SIZE(round1_steps); i++) {
		status = psa_pake_output(server, round1_steps[i], buffer, sizeof(buffer), &outlen);
		if (status != PSA_SUCCESS) {
			LOG_INF("psa_pake_output failed. (Error: %d)", status);
			return status;
		}

		status = psa_pake_input(client, round1_steps[i], buffer, outlen);
		if (status != PSA_SUCCESS) {
			LOG_INF("psa_pake_input failed. (Error: %d)", status);
			return status;
		}
	}

	/* Client provides data to server. */
	for (uint32_t i = 0; i < ARRAY_SIZE(round1_steps); i++) {
		status = psa_pake_output(client, round1_steps[i], buffer, sizeof(buffer), &outlen);
		if (status != PSA_SUCCESS) {
			LOG_INF("psa_pake_output failed. (Error: %d)", status);
			return status;
		}

		status = psa_pake_input(server, round1_steps[i], buffer, outlen);
		if (status != PSA_SUCCESS) {
			LOG_INF("psa_pake_input failed. (Error: %d)", status);
			return status;
		}
	}

	psa_pake_step_t round2_steps[] = {PSA_PAKE_STEP_KEY_SHARE, PSA_PAKE_STEP_ZK_PUBLIC,
					  PSA_PAKE_STEP_ZK_PROOF};

	/* Server provides data to client */
	for (uint32_t i = 0; i < ARRAY_SIZE(round2_steps); i++) {
		status = psa_pake_output(server, round2_steps[i], buffer, sizeof(buffer), &outlen);
		if (status != PSA_SUCCESS) {
			LOG_INF("psa_pake_output failed. (Error: %d)", status);
			return status;
		}

		status = psa_pake_input(client, round2_steps[i], buffer, outlen);
		if (status != PSA_SUCCESS) {
			LOG_INF("psa_pake_input failed. (Error: %d)", status);
			return status;
		}
	}

	/* Client provides data to server. */
	for (uint32_t i = 0; i < ARRAY_SIZE(round2_steps); i++) {
		status = psa_pake_output(client, round2_steps[i], buffer, sizeof(buffer), &outlen);
		if (status != PSA_SUCCESS) {
			LOG_INF("psa_pake_output failed. (Error: %d)", status);
			return status;
		}

		status = psa_pake_input(server, round2_steps[i], buffer, outlen);
		if (status != PSA_SUCCESS) {
			LOG_INF("psa_pake_input failed. (Error: %d)", status);
			return status;
		}
	}

	return status;
}

psa_status_t pake_setup(psa_pake_operation_t *op, psa_pake_cipher_suite_t *cs, const char *user,
			const char *peer, psa_key_id_t *password)
{
	psa_status_t status = psa_pake_setup(op, cs);

	if (status != PSA_SUCCESS) {
		LOG_INF("psa_pake_setup failed. (Error: %d)", status);
		return status;
	}

	status = psa_pake_set_user(op, user, strlen(user));
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_pake_set_user failed. (Error: %d)", status);
		return status;
	}

	status = psa_pake_set_peer(op, peer, strlen(peer));
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_pake_set_peer failed. (Error: %d)", status);
		return status;
	}

	status = psa_pake_set_password_key(op, *password);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_pake_set_password_key failed. (Error: %d)", status);
		return status;
	}

	return PSA_SUCCESS;
}

psa_status_t do_key_derivation(psa_pake_operation_t *op, uint8_t *key_buffer,
			       size_t key_buffer_size)
{
	psa_key_derivation_operation_t kdf = PSA_KEY_DERIVATION_OPERATION_INIT;

	psa_status_t status = psa_key_derivation_setup(&kdf, PSA_ALG_TLS12_ECJPAKE_TO_PMS);

	if (status != PSA_SUCCESS) {
		LOG_INF("psa_key_derivation_setup failed. (Error: %d)", status);
		return status;
	}

	status = psa_pake_get_implicit_key(op, &kdf);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_pake_get_implicit_key failed. (Error: %d)", status);
		psa_key_derivation_abort(&kdf);
		return status;
	}

	status = psa_key_derivation_output_bytes(&kdf, key_buffer, key_buffer_size);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_key_derivation_output_bytes failed. (Error: %d)", status);
		psa_key_derivation_abort(&kdf);
		return status;
	}

	return PSA_SUCCESS;
}

int main(void)
{
	psa_status_t status = psa_crypto_init();

	if (status != PSA_SUCCESS) {
		LOG_INF("psa_crypto_init failed. (Error: %d)", status);
	}

	psa_pake_cipher_suite_t cipher_suite = PSA_PAKE_CIPHER_SUITE_INIT;

	psa_pake_cs_set_algorithm(&cipher_suite, PSA_ALG_JPAKE);
	psa_pake_cs_set_primitive(&cipher_suite, PSA_PAKE_PRIMITIVE(PSA_PAKE_PRIMITIVE_TYPE_ECC,
								    PSA_ECC_FAMILY_SECP_R1, 256));
	psa_pake_cs_set_hash(&cipher_suite, PSA_ALG_SHA_256);

	psa_key_attributes_t key_attributes = PSA_KEY_ATTRIBUTES_INIT;

	psa_set_key_usage_flags(&key_attributes, PSA_KEY_USAGE_DERIVE);
	psa_set_key_algorithm(&key_attributes, PSA_ALG_JPAKE);
	psa_set_key_type(&key_attributes, PSA_KEY_TYPE_PASSWORD);

	psa_key_id_t key;

	status = psa_import_key(&key_attributes, "password", 8, &key);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_import_key failed. (Error: %d)", status);
		goto error;
	}

	/* Initialize PAKE operation object for the client.*/
	psa_pake_operation_t client = PSA_PAKE_OPERATION_INIT;

	status = pake_setup(&client, &cipher_suite, "client", "server", &key);
	if (status != PSA_SUCCESS) {
		goto error;
	}

	/* Initialize PAKE operation object for the server. */
	psa_pake_operation_t server = PSA_PAKE_OPERATION_INIT;

	status = pake_setup(&server, &cipher_suite, "server", "client", &key);
	if (status != PSA_SUCCESS) {
		goto error;
	}

	/* Perform the data exchange rounds */
	status = do_rounds(&server, &client);
	if (status != PSA_SUCCESS) {
		LOG_INF("EC J-PAKE rounds failed. (Error: %d)", status);
		goto error;
	}

	/* Retrieve keys from J-PAKE results. */
	uint8_t server_secret[32];
	uint8_t client_secret[32];

	status = do_key_derivation(&server, server_secret, sizeof(server_secret));
	if (status != PSA_SUCCESS) {
		goto error;
	}

	status = do_key_derivation(&client, client_secret, sizeof(client_secret));
	if (status != PSA_SUCCESS) {
		goto error;
	}

	PRINT_HEX("server_secret", server_secret, sizeof(server_secret));
	PRINT_HEX("client_secret", client_secret, sizeof(client_secret));

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
	psa_destroy_key(key);
	psa_pake_abort(&client);
	psa_pake_abort(&server);

	return APP_ERROR;
}
