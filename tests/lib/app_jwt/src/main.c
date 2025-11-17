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

#define EXPECTED_JWT_MIN                                                                           \
	"eyJ0eXAiOiJKV1QiLCJhbGciOiJFUzI1NiJ9."                                                    \
	"eyJzdWIiOiJucmYtMzU4Mjk5ODQwMTIzNDU2IiwiZXhwIjoxNzM5ODgxODg5fQ."                          \
	"duytcUdESlbKf6OnpTl683GcX0rPuxF2WRn9bHJsfyEbi9UEnx9m9t1JJfCHwshvSce8Sy_0BiTS6xqOEtOWRg"

#define EXPECTED_JWT_MIN_UUID                                                                      \
	"eyJ0eXAiOiJKV1QiLCJhbGciOiJFUzI1NiJ9."                                                    \
	"eyJzdWIiOiI2YzBmMTIyNi1mNTA2LTExZWYtYTViYi05Mzc2M2I0YmRmMTEiLCJleHAiOjE3Mzk4ODE4ODl9."    \
	"4JrHsAPnqzYoLnnhHroJx8B8ui8uoSpzyV6hJjQhcDPcFyRa6JlI5lRPnCnzczhvlIQGwOzjxA-S1D7en3--XQ"

#define EXPECTED_JWT_NRF91                                                                         \
	"eyJ0eXAiOiJKV1QiLCJhbGciOiJFUzI1NiIsImtpZCI6IjJhNTNkMjBmODcxYjFkZWVhYjUzYTFiZmNlMDJhZTVk" \
	"NWVmYzEyZjFlMGY0YjVjZTY2YzRhMWMyOGRhZDExMmMifQ."                                          \
	"eyJpYXQiOjE3Mzk4ODEyODksImp0aSI6Im5SRjkxNjAuNTY0YWIzNTYtZjUwNS0xMWVmLWIyODctYjc2OTAyMWQ1" \
	"ZmQ2LjlmZjE3NTI0OWFmZTQ4OTQxMiIsImlzcyI6Im5SRjkxNjAuNTY0YWIzNTYtZjUwNS0xMWVmLWIyODctYjc2" \
	"OTAyMWQ1ZmQ2Iiwic3ViIjoibnJmLTM1ODI5OTg0MDEyMzQ1NiIsImV4cCI6MTczOTg4MTg4OX0."             \
	"icQWyXMrqInwmDOnycuJ3mCre7T32RPWxFCM95iL9Y_dZTZGpODiopaTfGuu8KwSvTu32Wbmwsxiw-x2szcAfQ"

#define EC_PRV_KEY                                                                                 \
	"-----BEGIN PRIVATE KEY-----\n"                                                            \
	"MIGHAgEAMBMGByqGSM49AgEGCCqGSM49AwEHBG0wawIBAQQgyw3wVT7Uo05WHT4t\n"                       \
	"w8AgAo6eXMm0Pj8jqnAoABVtflmhRANCAAQ2ofrLZw+Fve9PzQ4z9COJE8IFtFnH\n"                       \
	"V0ey+tw/yXcqWROTXMauO4bJ4TC7CrmpM8IBXkKQm/gEHdQQCZXYAn68\n"                               \
	"-----END PRIVATE KEY-----\n"

#define EC_PUB_KEY                                                                                 \
	"-----BEGIN PUBLIC KEY-----\n"                                                             \
	"MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAENqH6y2cPhb3vT80OM/QjiRPCBbRZ\n"                       \
	"x1dHsvrcP8l3KlkTk1zGrjuGyeEwuwq5qTPCAV5CkJv4BB3UEAmV2AJ+vA==\n"                           \
	"-----END PUBLIC KEY-----\n"

static int import_ecdsa_keypair(uint32_t *user_keypair_id)
{
	int err = 0;
	const uint8_t priv_key[] = {0xcb, 0x0d, 0xf0, 0x55, 0x3e, 0xd4, 0xa3, 0x4e,
				    0x56, 0x1d, 0x3e, 0x2d, 0xc3, 0xc0, 0x20, 0x02,
				    0x8e, 0x9e, 0x5c, 0xc9, 0xb4, 0x3e, 0x3f, 0x23,
				    0xaa, 0x70, 0x28, 0x00, 0x15, 0x6d, 0x7e, 0x59};

	if (user_keypair_id != NULL) {
		psa_key_attributes_t key_attributes = PSA_KEY_ATTRIBUTES_INIT;

		/* Initialize PSA Crypto */
		err = psa_crypto_init();
		if (err != PSA_SUCCESS) {
			printk("psa_crypto_init failed! (Error: %d)\n", err);
			return -1;
		}

		psa_set_key_usage_flags(&key_attributes,
					PSA_KEY_USAGE_SIGN_MESSAGE | PSA_KEY_USAGE_VERIFY_MESSAGE);
		psa_set_key_lifetime(&key_attributes, PSA_KEY_LIFETIME_VOLATILE);
		psa_set_key_algorithm(&key_attributes, PSA_ALG_ECDSA(PSA_ALG_SHA_256));
		psa_set_key_type(&key_attributes,
				 PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
		psa_set_key_bits(&key_attributes, 256);

		/* Import key into PSA */
		err = psa_import_key(&key_attributes, priv_key, sizeof(priv_key), &kid);
		if (err != PSA_SUCCESS) {
			LOG_ERR("psa_import_key failed! (Error: %d)", err);
			return err;
		}

		LOG_INF("Public key in PEM format:");
		printk(EC_PUB_KEY "\n");
	}
	return err;
}

/* Construct minimal JWT that still contains enough information to be accepted by nRF Cloud. */
ZTEST(app_jwt, test_minimal)
{
	uint8_t buf[APP_JWT_STR_MAX_LEN];
	int err = 0;
	const char *expected_jwt = EXPECTED_JWT_MIN;

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

/* Construct minimal JWT using a UUID as subject. */
ZTEST(app_jwt, test_minimal_uuid)
{
	uint8_t buf[APP_JWT_STR_MAX_LEN];
	int err = 0;
	const char *expected_jwt = EXPECTED_JWT_MIN_UUID;

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

/* Construct JWT similar to how the nRF91 modem makes them. */
ZTEST(app_jwt, test_nrf91_like)
{
	uint8_t buf[APP_JWT_STR_MAX_LEN];
	int err = 0;
	const char *expected_jwt = EXPECTED_JWT_NRF91;

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
	int err = import_ecdsa_keypair(&kid);

	zassert_equal(err, 0, "err: %d", err);

	return NULL;
}

ZTEST_SUITE(app_jwt, NULL, setup, NULL, NULL, NULL);
