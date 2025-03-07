/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/base64.h>

#include <psa/crypto.h>

#include <app_jwt.h>

LOG_MODULE_REGISTER(test_main, LOG_LEVEL_DBG);

int date_time_now(int64_t *timestamp)
{
	*timestamp = 1739881289000;
	return 0;
}

static psa_key_id_t kid;

#define EC_PUB_KEY_RAW_LEN     65
#define EC_PUB_KEY_HEADER_LEN  26
#define EC_PUB_KEY_CONTENT_LEN (EC_PUB_KEY_RAW_LEN + EC_PUB_KEY_HEADER_LEN)
#define PEM_LINE_LENGTH	       64

static int generate_ecdsa_keypair(uint32_t *user_keypair_id)
{
	int err = 0;
	uint8_t pub_key[EC_PUB_KEY_CONTENT_LEN] = {
		0x30, 0x59, 0x30, 0x13, 0x06, 0x07, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x02, 0x01,
		0x06, 0x08, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x03, 0x01, 0x07, 0x03, 0x42, 0x00};
	char pub_key_base64[125];
	size_t olen;

	if (user_keypair_id != NULL) {
		psa_key_attributes_t key_attributes = PSA_KEY_ATTRIBUTES_INIT;

		/* Initialize PSA Crypto */
		err = psa_crypto_init();
		if (err != PSA_SUCCESS) {
			printk("psa_crypto_init failed! (Error: %d)\n", err);
			return -1;
		}

		/* Configure the key attributes */
		psa_set_key_usage_flags(&key_attributes,
					PSA_KEY_USAGE_SIGN_MESSAGE | PSA_KEY_USAGE_VERIFY_MESSAGE);
		psa_set_key_lifetime(&key_attributes, PSA_KEY_LIFETIME_VOLATILE);
		psa_set_key_algorithm(&key_attributes, PSA_ALG_ECDSA(PSA_ALG_SHA_256));
		psa_set_key_type(&key_attributes,
				 PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
		psa_set_key_bits(&key_attributes, 256);

		/**
		 * Generate a random keypair. The keypair is not exposed to the application,
		 * we can use it to sign messages.
		 */
		err = psa_generate_key(&key_attributes, user_keypair_id);
		if (err != PSA_SUCCESS) {
			printk("psa_generate_key failed! (Error: %d)\n", err);
			return err;
		}

		/* Export pub key in raw format */
		err = psa_export_public_key(*user_keypair_id, pub_key + EC_PUB_KEY_HEADER_LEN,
					    EC_PUB_KEY_RAW_LEN, &olen);

		if ((err != PSA_SUCCESS) || (olen != EC_PUB_KEY_RAW_LEN)) {
			LOG_INF("psa_export_public_key failed! (Error: %d). Exiting!", err);
			return err;
		}

		/* base64-encode and print pub key in PEM format */
		err = base64_encode(pub_key_base64, sizeof(pub_key_base64), &olen, pub_key,
				    sizeof(pub_key));
		if (err) {
			LOG_INF("base64_encode failed, error: %d", err);
			return err;
		}
		LOG_INF("Public key in PEM format:");
		printk("-----BEGIN PUBLIC KEY-----\n");
		for (size_t i = 0; i < sizeof(pub_key_base64); i += PEM_LINE_LENGTH) {
			printk("%.*s\n", PEM_LINE_LENGTH, pub_key_base64 + i);
		}
		printk("-----END PUBLIC KEY-----\n\n");
	}
	return err;
}

ZTEST(jwt_app_test, minimal)
{
	uint8_t buf[APP_JWT_STR_MAX_LEN];
	int err = 0;
	const char *expected_jwt =
		"eyJ0eXAiOiJKV1QiLCJhbGciOiJFUzI1NiJ9."
		"eyJzdWIiOiJucmYtMzU4Mjk5ODQwMTIzNDU2IiwiZXhwIjoxNzM5ODgxODg5fQ.QrHn_"
		"M9POqkjKIXl8s0pfwiN7WFSsYjuliZEGvRnum3KzSwfRZTChd_o1TyAZC4NYhR4W2D2Rum4oFFf2gjRew";

	struct app_jwt_data jwt = {
		.sec_tag = kid,
		.key_type = JWT_KEY_TYPE_CLIENT_PRIV,
		.alg = JWT_ALG_TYPE_ES256,
		.add_keyid_to_header = false,
		.validity_s = APP_JWT_VALID_TIME_S_DEF,
		.jwt_buf = buf,
		.jwt_sz = sizeof(buf),
		.subject = "nrf-358299840123456",
	};

	err = app_jwt_generate(&jwt);
	zassert_equal(err, 0, "err: %d", err);
	zassert_equal(strcmp(jwt.jwt_buf, expected_jwt), 0, "unexpected JWT: %s", jwt.jwt_buf);
	LOG_INF("minimal JWT: %s", jwt.jwt_buf);
}

ZTEST(jwt_app_test, minimal_uuid)
{
	uint8_t buf[APP_JWT_STR_MAX_LEN];
	int err = 0;
	const char *expected_jwt = "eyJ0eXAiOiJKV1QiLCJhbGciOiJFUzI1NiJ9."
				   "eyJzdWIiOiI2YzBmMTIyNi1mNTA2LTExZWYtYTViYi05Mzc2M2I0YmRmMTEiLCJ"
				   "leHAiOjE3Mzk4ODE4ODl9.Yjaj765W363ImhelkCwzucwWU6R90PA-fTrQRYDT-"
				   "vp7vioZ_132ukuY6JvfCi7naeC8ZkOYdK6wqnjFbgVqHg";

	struct app_jwt_data jwt = {
		.sec_tag = kid,
		.key_type = JWT_KEY_TYPE_CLIENT_PRIV,
		.alg = JWT_ALG_TYPE_ES256,
		.add_keyid_to_header = false,
		.validity_s = APP_JWT_VALID_TIME_S_DEF,
		.jwt_buf = buf,
		.jwt_sz = sizeof(buf),
		.subject = "6c0f1226-f506-11ef-a5bb-93763b4bdf11",
	};

	err = app_jwt_generate(&jwt);
	zassert_equal(err, 0, "err: %d", err);
	zassert_equal(strcmp(jwt.jwt_buf, expected_jwt), 0, "unexpected JWT: %s", jwt.jwt_buf);
	LOG_INF("minimal JWT using UUID: %s", jwt.jwt_buf);
}

ZTEST(jwt_app_test, nrf91_like)
{
	uint8_t buf[APP_JWT_STR_MAX_LEN];
	int err = 0;
	const char *expected_jwt =
		"eyJ0eXAiOiJKV1QiLCJhbGciOiJFUzI1NiIsImtpZCI6IjU1NTFhYzBjOGIwYWExMjNhOGI4MTExNTJkYj"
		"E0OWRiNzFlZDA5N2MyZWRhNzM1YWJjNzYxNjkwMGEzNDJhNTAifQ."
		"eyJpYXQiOjE3Mzk4ODEyODksImp0aSI6Im5SRjkxNjAuNTY0YWIzNTYtZjUwNS0xMWVmLWIyODctYjc2OT"
		"AyMWQ1ZmQ2LjlmZjE3NTI0OWFmZTQ4OTQxMiIsImlzcyI6Im5SRjkxNjAuNTY0YWIzNTYtZjUwNS0xMWVm"
		"LWIyODctYjc2OTAyMWQ1ZmQ2Iiwic3ViIjoibnJmLTM1ODI5OTg0MDEyMzQ1NiIsImV4cCI6MTczOTg4MT"
		"g4OX0.hbyKcSdJ6A08vScTD2nufWufZD3GuuhEj9kz0yKKZuknDQFMXX25NYsuLO2ibDCs06u2n-"
		"Hql3CfF0OM4X94sQ";

	struct app_jwt_data jwt = {
		.sec_tag = kid,
		.key_type = JWT_KEY_TYPE_CLIENT_PRIV,
		.alg = JWT_ALG_TYPE_ES256,
		.add_keyid_to_header = true,
		.add_timestamp = true,
		.validity_s = APP_JWT_VALID_TIME_S_DEF,
		.jwt_buf = buf,
		.jwt_sz = sizeof(buf),
		.issuer = "nRF9160.564ab356-f505-11ef-b287-b769021d5fd6",
		.json_token_id = "nRF9160.564ab356-f505-11ef-b287-b769021d5fd6.9ff175249afe489412",
		.subject = "nrf-358299840123456",
	};

	err = app_jwt_generate(&jwt);
	zassert_equal(err, 0, "err: %d", err);
	zassert_equal(strcmp(jwt.jwt_buf, expected_jwt), 0, "unexpected JWT: %s", jwt.jwt_buf);
	LOG_INF("nrf91-like JWT: %s", jwt.jwt_buf);
}

static void *setup(void)
{
	int err = generate_ecdsa_keypair(&kid);

	zassert_equal(err, 0, "err: %d", err);

	return NULL;
}

ZTEST_SUITE(jwt_app_test, NULL, setup, NULL, NULL, NULL);
