/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <zephyr/fff.h>

#include <zephyr/net/tls_credentials.h>
#include "tls_internal.h"

#include <modem/modem_key_mgmt.h>

DEFINE_FFF_GLOBALS;

FAKE_VALUE_FUNC(int, modem_key_mgmt_write, nrf_sec_tag_t, enum modem_key_mgmt_cred_type,
					   const void *, size_t);
FAKE_VALUE_FUNC(int, modem_key_mgmt_read, nrf_sec_tag_t, enum modem_key_mgmt_cred_type,
					  void *, size_t *);
FAKE_VALUE_FUNC(int, modem_key_mgmt_delete, nrf_sec_tag_t, enum modem_key_mgmt_cred_type);
FAKE_VALUE_FUNC(int, modem_key_mgmt_clear, nrf_sec_tag_t);
FAKE_VALUE_FUNC(int, modem_key_mgmt_cmp, nrf_sec_tag_t, enum modem_key_mgmt_cred_type,
					 const void *, size_t);
FAKE_VALUE_FUNC(int, modem_key_mgmt_digest, nrf_sec_tag_t, enum modem_key_mgmt_cred_type,
					    void *, size_t);
FAKE_VALUE_FUNC(int, modem_key_mgmt_exists, nrf_sec_tag_t, enum modem_key_mgmt_cred_type,
					    bool *);
FAKE_VALUE_FUNC(int, modem_key_mgmt_list, modem_key_mgmt_list_cb_t);

static const char pem_buf[] = "--- begin --- base64-encode(some-bytes) --- end ----";

/* These tests should be run first, to verify the credentials loaded on modem init. */
ZTEST(tls_credentials, test_00_init_import_list_public_api)
{
	uint8_t cred[1];
	size_t len = sizeof(cred);

	/* The CA chain can be accessed. */
	modem_key_mgmt_read_fake.return_val = 0;

	zassert_equal(tls_credential_get(100, TLS_CREDENTIAL_CA_CERTIFICATE, cred, &len), 0);
	zassert_equal(tls_credential_get(101, TLS_CREDENTIAL_CA_CERTIFICATE, cred, &len), 0);
	zassert_equal(tls_credential_get(103, TLS_CREDENTIAL_CA_CERTIFICATE, cred, &len), 0);

	/* The server certificate and private key cannot be accessed, instead returning EACCESS. */
	modem_key_mgmt_read_fake.return_val = -EACCES;

	zassert_equal(tls_credential_get(1, TLS_CREDENTIAL_SERVER_CERTIFICATE, cred, &len),
		      -EACCES);
	zassert_equal(tls_credential_get(1, TLS_CREDENTIAL_PRIVATE_KEY, cred, &len), -EACCES);

	zassert_equal(tls_credential_get(2, TLS_CREDENTIAL_SERVER_CERTIFICATE, cred, &len),
		      -EACCES);
	zassert_equal(tls_credential_get(2, TLS_CREDENTIAL_PRIVATE_KEY, cred, &len), -EACCES);
}

ZTEST(tls_credentials, test_00_init_import_list_internal_api)
{
	zassert_not_null(credential_get(100, TLS_CREDENTIAL_CA_CERTIFICATE));
	zassert_not_null(credential_get(101, TLS_CREDENTIAL_CA_CERTIFICATE));
	zassert_not_null(credential_get(103, TLS_CREDENTIAL_CA_CERTIFICATE));

	zassert_not_null(credential_get(1, TLS_CREDENTIAL_SERVER_CERTIFICATE));
	zassert_not_null(credential_get(1, TLS_CREDENTIAL_PRIVATE_KEY));

	zassert_not_null(credential_get(2, TLS_CREDENTIAL_SERVER_CERTIFICATE));
	zassert_not_null(credential_get(2, TLS_CREDENTIAL_PRIVATE_KEY));
}

ZTEST(tls_credentials, test_00_init_import_list_internal_api_iterator)
{
	struct tls_credential *cred;

	cred = credential_next_get(100, NULL);
	zassert_not_null(cred);
	zassert_equal(cred->tag, 100);
	zassert_equal(cred->type, TLS_CREDENTIAL_CA_CERTIFICATE);
	zassert_is_null(credential_next_get(100, cred));

	cred = credential_next_get(101, NULL);
	zassert_not_null(cred);
	zassert_equal(cred->tag, 101);
	zassert_equal(cred->type, TLS_CREDENTIAL_CA_CERTIFICATE);
	zassert_is_null(credential_next_get(100, cred));

	cred = credential_next_get(103, NULL);
	zassert_not_null(cred);
	zassert_equal(cred->tag, 103);
	zassert_equal(cred->type, TLS_CREDENTIAL_CA_CERTIFICATE);
	zassert_is_null(credential_next_get(100, cred));

	/* We assert the type of the iterator based on the order they were added by the callback. */
	cred = credential_next_get(1, NULL);
	zassert_not_null(cred);
	zassert_equal(cred->tag, 1);
	zassert_equal(cred->type, TLS_CREDENTIAL_SERVER_CERTIFICATE);
	cred = credential_next_get(1, cred);
	zassert_not_null(cred);
	zassert_equal(cred->tag, 1);
	zassert_equal(cred->type, TLS_CREDENTIAL_PRIVATE_KEY);
	zassert_is_null(credential_next_get(1, cred));

	cred = credential_next_get(2, NULL);
	zassert_not_null(cred);
	zassert_equal(cred->tag, 2);
	zassert_equal(cred->type, TLS_CREDENTIAL_PRIVATE_KEY);
	cred = credential_next_get(2, cred);
	zassert_not_null(cred);
	zassert_equal(cred->tag, 2);
	zassert_equal(cred->type, TLS_CREDENTIAL_SERVER_CERTIFICATE);
}

/* base64(bin("B9BAC15641653CAE2B5151DCBD0C40DDCDF19125950FA2475977437EA35F7A45")) */
const char expected_digest_str[] =
	"ubrBVkFlPK4rUVHcvQxA3c3xkSWVD6JHWXdDfqNfekU=";

static const uint8_t digest[] = {
	0xB9, 0xBA, 0xC1, 0x56,  0x41, 0x65, 0x3C, 0xAE,
	0x2B, 0x51, 0x51, 0xDC,  0xBD, 0x0C, 0x40, 0xDD,
	0xCD, 0xF1, 0x91, 0x25,  0x95, 0x0F, 0xA2, 0x47,
	0x59, 0x77, 0x43, 0x7E,  0xA3, 0x5F, 0x7A, 0x45
};

int modem_key_mgmt_digest_custom(nrf_sec_tag_t sec_tag,
				 enum modem_key_mgmt_cred_type cred_type,
				 void *buf, size_t len)
{
	zassert_equal(sec_tag, 100);
	zassert_equal(cred_type, MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN);

	memcpy(buf, digest, sizeof(digest));
	return 0;
}

ZTEST(tls_credentials, test_credential_digest)
{
	struct tls_credential *cred;
	uint8_t digest_base64_buf[49];
	size_t digest_base64_len = sizeof(digest_base64_buf);

	modem_key_mgmt_digest_fake.custom_fake = modem_key_mgmt_digest_custom;

	cred = credential_next_get(100, NULL);

	zassert_ok(credential_digest(cred, digest_base64_buf, &digest_base64_len));
	zassert_mem_equal(digest_base64_buf, expected_digest_str, sizeof(expected_digest_str));
}

ZTEST(tls_credentials, test_tls_credential_add)
{
	size_t len = strlen(pem_buf);

	modem_key_mgmt_write_fake.return_val = 0;

	zassert_ok(tls_credential_add(10, TLS_CREDENTIAL_CA_CERTIFICATE, (uint8_t *)pem_buf, len));

	zassert_equal(modem_key_mgmt_write_fake.call_count, 1);

	zassert_equal(modem_key_mgmt_write_fake.arg0_val, 10);
	zassert_equal(modem_key_mgmt_write_fake.arg1_val, MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN);
	zassert_equal(modem_key_mgmt_write_fake.arg2_val, (uint8_t *)pem_buf);
	zassert_equal(modem_key_mgmt_write_fake.arg3_val, len);

	zassert_ok(tls_credential_delete(10, TLS_CREDENTIAL_CA_CERTIFICATE));

	zassert_equal(modem_key_mgmt_delete_fake.call_count, 1);

	zassert_equal(modem_key_mgmt_delete_fake.arg0_val, 10);
	zassert_equal(modem_key_mgmt_delete_fake.arg1_val, MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN);
}

ZTEST(tls_credentials, test_tls_credential_add_existing_equal)
{
	size_t len = strlen(pem_buf);

	modem_key_mgmt_cmp_fake.return_val = 0;

	zassert_ok(tls_credential_add(101, TLS_CREDENTIAL_CA_CERTIFICATE, (uint8_t *)pem_buf, len));

	zassert_equal(modem_key_mgmt_cmp_fake.call_count, 1);
	zassert_equal(modem_key_mgmt_cmp_fake.arg0_val, 101);
	zassert_equal(modem_key_mgmt_cmp_fake.arg1_val, MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN);
	zassert_equal(modem_key_mgmt_cmp_fake.arg2_val, (uint8_t *)pem_buf);
	zassert_equal(modem_key_mgmt_cmp_fake.arg3_val, len);
}

ZTEST(tls_credentials, test_tls_credential_add_existing_not_equal)
{
	size_t len = strlen(pem_buf);

	modem_key_mgmt_cmp_fake.return_val = 1;

	zassert_equal(tls_credential_add(101, TLS_CREDENTIAL_CA_CERTIFICATE,
					 (uint8_t *)pem_buf, len),
		      -EEXIST);

	zassert_equal(modem_key_mgmt_cmp_fake.call_count, 1);
	zassert_equal(modem_key_mgmt_cmp_fake.arg0_val, 101);
	zassert_equal(modem_key_mgmt_cmp_fake.arg1_val, MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN);
	zassert_equal(modem_key_mgmt_cmp_fake.arg2_val, (uint8_t *)pem_buf);
	zassert_equal(modem_key_mgmt_cmp_fake.arg3_val, len);

	/* Should not have been written */
	zassert_equal(modem_key_mgmt_write_fake.call_count, 0);
}

/* Verify that if we have an entry, but the modem for some reason has lost it, we write it. */
ZTEST(tls_credentials, test_tls_credential_add_existing_but_not_existing)
{
	size_t len = strlen(pem_buf);

	modem_key_mgmt_cmp_fake.return_val = -ENOENT;
	modem_key_mgmt_write_fake.return_val = 0;

	zassert_ok(tls_credential_add(101, TLS_CREDENTIAL_CA_CERTIFICATE, (uint8_t *)pem_buf, len));

	zassert_equal(modem_key_mgmt_cmp_fake.call_count, 1);

	/* Write it if it does not exist. */
	zassert_equal(modem_key_mgmt_write_fake.call_count, 1);
	zassert_equal(modem_key_mgmt_write_fake.arg0_val, 101);
	zassert_equal(modem_key_mgmt_write_fake.arg1_val, MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN);
	zassert_equal(modem_key_mgmt_write_fake.arg2_val, (uint8_t *)pem_buf);
	zassert_equal(modem_key_mgmt_write_fake.arg3_val, len);
}


static void _reset_fake(const struct ztest_unit_test *test, void *fixture)
{
	RESET_FAKE(modem_key_mgmt_write);
	RESET_FAKE(modem_key_mgmt_read);
	RESET_FAKE(modem_key_mgmt_cmp);
	RESET_FAKE(modem_key_mgmt_exists);
	RESET_FAKE(modem_key_mgmt_delete);
	RESET_FAKE(modem_key_mgmt_list);
}

ZTEST_RULE(reset_fake_rule, _reset_fake, NULL);

/* NRF_MODEM_LIB_ON_INIT handler. */
extern void credentials_on_modem_init(int ret, void *ctx);

int modem_key_mgmt_list_setup(modem_key_mgmt_list_cb_t list_cb)
{
	list_cb(100, MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN);
	list_cb(101, MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN);
	list_cb(103, MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN);

	list_cb(1, MODEM_KEY_MGMT_CRED_TYPE_PUBLIC_CERT);
	list_cb(1, MODEM_KEY_MGMT_CRED_TYPE_PRIVATE_CERT);

	list_cb(2, MODEM_KEY_MGMT_CRED_TYPE_PRIVATE_CERT);
	list_cb(2, MODEM_KEY_MGMT_CRED_TYPE_PUBLIC_CERT);

	return 0;
}

static void *tls_credentials_setup(void)
{
	modem_key_mgmt_list_fake.custom_fake = modem_key_mgmt_list_setup;

	credentials_on_modem_init(0, NULL);
	return NULL;
}

ZTEST_SUITE(tls_credentials, NULL, tls_credentials_setup, NULL, NULL, NULL);
