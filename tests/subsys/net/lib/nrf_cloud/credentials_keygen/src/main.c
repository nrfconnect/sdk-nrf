/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/fff.h>
#include <zephyr/sys/util.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_dummy.h>
#include <zephyr/net/tls_credentials.h>
#include <zephyr/sys/base64.h>

#include <psa/crypto.h>
#include <mbedtls/x509_csr.h>

#include <net/nrf_cloud.h>
#include <net/nrf_cloud_credentials_keygen.h>
#include "nrf_cloud_credentials_keygen_internal.h"

DEFINE_FFF_GLOBALS;

/* Sec tags map directly to PSA user key ids (must be within
 * PSA_KEY_ID_USER_MIN..PSA_KEY_ID_USER_MAX).
 */
#define TEST_SEC_TAG	 42
#define TEST_SEC_TAG_2	 43
#define TEST_DEVICE_ID	 "test-device-id"

/* The only nRF Cloud dependencies of the units under test. Everything else
 * (PSA crypto, mbedTLS, TLS credentials) is linked for real.
 */
sec_tag_t nrf_cloud_sec_tag_get(void)
{
	return TEST_SEC_TAG;
}

FAKE_VALUE_FUNC(int, nrf_cloud_client_id_get, char *, size_t);

static int client_id_get_custom(char *buf, size_t sz)
{
	strncpy(buf, TEST_DEVICE_ID, sz);
	buf[sz - 1] = '\0';
	return 0;
}

/* Bring the module and PSA back to a known state before each test. Deleting
 * both known sec tags destroys any generated key and frees the module's single
 * registration slot regardless of which tag was registered.
 */
static void reset_state(void)
{
	(void)nrf_cloud_credentials_key_delete(TEST_SEC_TAG);
	(void)nrf_cloud_credentials_key_delete(TEST_SEC_TAG_2);

	RESET_FAKE(nrf_cloud_client_id_get);
	nrf_cloud_client_id_get_fake.custom_fake = client_id_get_custom;
}

/* ---------------------------------------------------------------------------
 * API tests (real PSA + real TLS credentials, one faked nRF Cloud helper).
 * ---------------------------------------------------------------------------
 */

static void api_before(void *fixture)
{
	ARG_UNUSED(fixture);
	reset_state();
}

ZTEST_SUITE(keygen_api, NULL, NULL, api_before, NULL, NULL);

ZTEST(keygen_api, test_generate_and_exists)
{
	zassert_false(nrf_cloud_credentials_keygen_key_exists(TEST_SEC_TAG),
		      "key should not exist before generation");

	zassert_ok(nrf_cloud_credentials_key_generate(TEST_SEC_TAG), "generate failed");

	zassert_true(nrf_cloud_credentials_keygen_key_exists(TEST_SEC_TAG),
		     "key should exist after generation");
	zassert_equal(nrf_cloud_credentials_keygen_key_id(TEST_SEC_TAG),
		      (psa_key_id_t)TEST_SEC_TAG, "unexpected key id mapping");
}

ZTEST(keygen_api, test_generate_twice_is_ealready)
{
	zassert_ok(nrf_cloud_credentials_key_generate(TEST_SEC_TAG), "first generate failed");
	zassert_equal(nrf_cloud_credentials_key_generate(TEST_SEC_TAG), -EALREADY,
		      "second generate should report -EALREADY");
}

ZTEST(keygen_api, test_invalid_sec_tag)
{
	/* sec tag 0 maps to PSA key id 0, below PSA_KEY_ID_USER_MIN. */
	zassert_equal(nrf_cloud_credentials_key_generate(0), -EINVAL,
		      "out-of-range sec tag should be rejected");
}

ZTEST(keygen_api, test_delete)
{
	zassert_ok(nrf_cloud_credentials_key_generate(TEST_SEC_TAG), "generate failed");
	zassert_ok(nrf_cloud_credentials_key_delete(TEST_SEC_TAG), "delete failed");
	zassert_false(nrf_cloud_credentials_keygen_key_exists(TEST_SEC_TAG),
		      "key should be gone after delete");
}

ZTEST(keygen_api, test_single_sec_tag_conflict)
{
	zassert_ok(nrf_cloud_credentials_key_generate(TEST_SEC_TAG), "generate failed");

	/* A second, different sec tag must be rejected without side effects. */
	zassert_equal(nrf_cloud_credentials_key_generate(TEST_SEC_TAG_2), -ENOTSUP,
		      "second sec tag should be rejected with -ENOTSUP");
	zassert_false(nrf_cloud_credentials_keygen_key_exists(TEST_SEC_TAG_2),
		      "rejected request must not leave a key behind");
}

ZTEST(keygen_api, test_delete_then_switch_sec_tag)
{
	zassert_ok(nrf_cloud_credentials_key_generate(TEST_SEC_TAG), "generate failed");
	zassert_ok(nrf_cloud_credentials_key_delete(TEST_SEC_TAG), "delete failed");

	/* Slot freed, so a different sec tag can now be used. */
	zassert_ok(nrf_cloud_credentials_key_generate(TEST_SEC_TAG_2),
		   "generate for a new sec tag after delete failed");
}

ZTEST(keygen_api, test_pubkey_get)
{
	uint8_t pub[65];
	size_t len = 0;

	zassert_ok(nrf_cloud_credentials_key_generate(TEST_SEC_TAG), "generate failed");

	zassert_equal(nrf_cloud_credentials_pubkey_get(TEST_SEC_TAG, NULL, sizeof(pub), &len),
		      -EINVAL, "NULL output must be rejected");

	zassert_ok(nrf_cloud_credentials_pubkey_get(TEST_SEC_TAG, pub, sizeof(pub), &len),
		   "pubkey export failed");
	zassert_equal(len, 65, "P-256 SEC1 public key must be 65 bytes");
	zassert_equal(pub[0], 0x04, "uncompressed point must start with 0x04");
}

ZTEST(keygen_api, test_csr_generate)
{
	uint8_t der[512];
	size_t len = 0;

	zassert_ok(nrf_cloud_credentials_key_generate(TEST_SEC_TAG), "generate failed");

	zassert_equal(nrf_cloud_credentials_csr_generate(TEST_SEC_TAG, NULL, der, sizeof(der),
							 &len),
		      -EINVAL, "NULL subject must be rejected");

	zassert_ok(nrf_cloud_credentials_csr_generate(TEST_SEC_TAG, "CN=test", der, sizeof(der),
						      &len),
		   "CSR generation failed");
	zassert_true(len > 0, "CSR length must be positive");
	zassert_equal(der[0], 0x30, "DER CSR must begin with a SEQUENCE tag");

	/* Re-parse the CSR: this validates the DER structure, the requested
	 * subject and, crucially, the self-signature made through PSA with the
	 * on-device key.
	 */
	mbedtls_x509_csr csr;
	char subj[64];

	mbedtls_x509_csr_init(&csr);
	zassert_equal(mbedtls_x509_csr_parse_der(&csr, der, len), 0,
		      "generated CSR does not parse");
	zassert_true(mbedtls_x509_dn_gets(subj, sizeof(subj), &csr.subject) > 0,
		     "could not read CSR subject");
	zassert_not_null(strstr(subj, "CN=test"), "unexpected CSR subject: %s", subj);
	mbedtls_x509_csr_free(&csr);
}

ZTEST(keygen_api, test_rollback_on_registration_failure)
{
	static const uint8_t dummy[] = {0xAA};
	const sec_tag_t fill_tags[] = {9001, 9002, 9003, 9004};

	BUILD_ASSERT(ARRAY_SIZE(fill_tags) >= CONFIG_TLS_MAX_CREDENTIALS_NUMBER,
		     "must be able to fill the whole TLS credentials store");

	/* Fill the TLS credentials store so the registration inside key_generate()
	 * fails after the PSA key has been generated.
	 */
	for (int i = 0; i < CONFIG_TLS_MAX_CREDENTIALS_NUMBER; i++) {
		zassert_ok(tls_credential_add(fill_tags[i], TLS_CREDENTIAL_CA_CERTIFICATE,
					      dummy, sizeof(dummy)),
			   "failed to fill TLS credentials store");
	}

	zassert_not_equal(nrf_cloud_credentials_key_generate(TEST_SEC_TAG), 0,
			  "generate should fail when registration cannot complete");
	zassert_false(nrf_cloud_credentials_keygen_key_exists(TEST_SEC_TAG),
		      "failed registration must roll back the generated key");

	for (int i = 0; i < CONFIG_TLS_MAX_CREDENTIALS_NUMBER; i++) {
		(void)tls_credential_delete(fill_tags[i], TLS_CREDENTIAL_CA_CERTIFICATE);
	}
}

/* ---------------------------------------------------------------------------
 * Shell tests. Host tooling (nrfcredstore / device_credentials_installer)
 * scrapes the command output, so the exact markers below are a contract.
 * ---------------------------------------------------------------------------
 */

static const struct shell *dummy;

static void *shell_setup(void)
{
	dummy = shell_backend_dummy_get_ptr();
	WAIT_FOR(shell_ready(dummy), 20000, k_msleep(1));
	zassert_true(shell_ready(dummy), "timed out waiting for dummy shell backend");
	return NULL;
}

static void shell_before(void *fixture)
{
	ARG_UNUSED(fixture);
	reset_state();
	shell_backend_dummy_clear_output(dummy);
}

/* Execute a command and return the captured output. */
static const char *run(const char *cmd, int *ret)
{
	size_t size;
	int r;

	shell_backend_dummy_clear_output(dummy);
	r = shell_execute_cmd(dummy, cmd);
	if (ret != NULL) {
		*ret = r;
	}
	return shell_backend_dummy_get_output(dummy, &size);
}

/* Copy the whitespace-delimited token that follows @p prefix in @p out into
 * @p tok. Returns the token length, or 0 if @p prefix is not found.
 */
static size_t token_after(const char *out, const char *prefix, char *tok, size_t tok_size)
{
	const char *p = strstr(out, prefix);
	size_t i = 0;

	if (p == NULL) {
		return 0;
	}
	p += strlen(prefix);

	while (p[i] != '\0' && p[i] != '\r' && p[i] != '\n' && p[i] != ' ' && i < tok_size - 1) {
		tok[i] = p[i];
		i++;
	}
	tok[i] = '\0';

	return i;
}

/* True if every character is in the standard Base64 alphabet. */
static bool is_base64(const char *s)
{
	for (; *s != '\0'; s++) {
		char c = *s;

		if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
		      (c >= '0' && c <= '9') || c == '+' || c == '/' || c == '=')) {
			return false;
		}
	}

	return true;
}

ZTEST_SUITE(keygen_shell, NULL, shell_setup, shell_before, NULL, NULL);

ZTEST(keygen_shell, test_shell_keygen_output)
{
	int ret;
	const char *out = run("nrf_cloud_cred keygen 42", &ret);

	zassert_ok(ret, "keygen command returned %d", ret);
	zassert_not_null(strstr(out, "Generated device key in sec tag 42"),
			 "missing success line, got: %s", out);
	zassert_true(nrf_cloud_credentials_keygen_key_exists(TEST_SEC_TAG),
		     "key not created by shell command");
}

ZTEST(keygen_shell, test_shell_keygen_already_exists)
{
	int ret;
	const char *out;

	(void)run("nrf_cloud_cred keygen 42", NULL);
	out = run("nrf_cloud_cred keygen 42", &ret);

	zassert_not_equal(ret, 0, "second keygen should fail");
	zassert_not_null(strstr(out, "Key already exists"), "missing warning, got: %s", out);
	zassert_not_null(strstr(out, "delete"), "should point user to 'delete', got: %s", out);
}

ZTEST(keygen_shell, test_shell_csr_output)
{
	int ret;
	const char *out;

	char tok[1024];
	uint8_t der[512];
	size_t tok_len;
	size_t der_len = 0;
	mbedtls_x509_csr csr;

	(void)run("nrf_cloud_cred keygen 42", NULL);
	out = run("nrf_cloud_cred csr 42 CN=test", &ret);

	zassert_ok(ret, "csr command returned %d", ret);
	/* Host tooling captures the Base64 DER from the "CSR: " prefix and uses
	 * the completion marker to know the output is done.
	 */
	zassert_not_null(strstr(out, "CSR: "), "missing 'CSR: ' prefix, got: %s", out);
	zassert_not_null(strstr(out, "CSR generation complete"),
			 "missing completion marker, got: %s", out);

	/* The printed CSR must be valid, complete Base64. Decoding it and parsing
	 * the result as a CSR proves the line is real Base64 and was not
	 * truncated: a cut-off line would decode to a malformed DER that fails to
	 * parse.
	 */
	tok_len = token_after(out, "CSR: ", tok, sizeof(tok));
	zassert_true(tok_len > 0, "no CSR token in output: %s", out);
	zassert_true(is_base64(tok), "CSR is not Base64: %s", tok);
	zassert_ok(base64_decode(der, sizeof(der), &der_len, tok, tok_len),
		   "CSR Base64 does not decode (truncated?)");
	zassert_true(der_len > 0, "decoded CSR is empty");

	mbedtls_x509_csr_init(&csr);
	zassert_equal(mbedtls_x509_csr_parse_der(&csr, der, der_len), 0,
		      "decoded CSR does not parse (truncated?)");
	mbedtls_x509_csr_free(&csr);
}

ZTEST(keygen_shell, test_shell_csr_default_subject)
{
	int ret;
	const char *out;

	/* No sec tag and no subject: defaults to the nRF Cloud sec tag and
	 * CN=<device id>, which requires the client-id lookup.
	 */
	(void)run("nrf_cloud_cred keygen", NULL);
	out = run("nrf_cloud_cred csr", &ret);

	zassert_ok(ret, "csr with defaults returned %d", ret);
	zassert_true(nrf_cloud_client_id_get_fake.call_count > 0,
		     "default subject should query the device id");
	zassert_not_null(strstr(out, "CSR: "), "missing 'CSR: ' prefix, got: %s", out);
}

ZTEST(keygen_shell, test_shell_pubkey_output)
{
	int ret;
	const char *out;

	char tok[128];
	uint8_t dec[80];
	size_t tok_len;
	size_t dec_len = 0;

	(void)run("nrf_cloud_cred keygen 42", NULL);
	out = run("nrf_cloud_cred pubkey 42", &ret);

	zassert_ok(ret, "pubkey command returned %d", ret);
	zassert_not_null(strstr(out, "PUBKEY: "), "missing 'PUBKEY: ' prefix, got: %s", out);
	zassert_not_null(strstr(out, "Public key export complete"),
			 "missing completion marker, got: %s", out);

	/* The printed key must be valid, complete Base64. A 65-byte SEC1 point
	 * encodes to exactly 88 Base64 characters; decoding back to 65 bytes
	 * starting with 0x04 proves it is neither malformed nor truncated.
	 */
	tok_len = token_after(out, "PUBKEY: ", tok, sizeof(tok));
	zassert_equal(tok_len, 88, "public key Base64 length %zu != 88 (truncated?)", tok_len);
	zassert_true(is_base64(tok), "public key is not Base64: %s", tok);
	zassert_ok(base64_decode(dec, sizeof(dec), &dec_len, tok, tok_len),
		   "public key Base64 does not decode");
	zassert_equal(dec_len, 65, "decoded public key is %zu bytes, expected 65", dec_len);
	zassert_equal(dec[0], 0x04, "decoded public key is not an uncompressed point");
}

ZTEST(keygen_shell, test_shell_delete_output)
{
	int ret;
	const char *out;

	(void)run("nrf_cloud_cred keygen 42", NULL);
	out = run("nrf_cloud_cred delete 42", &ret);

	zassert_ok(ret, "delete command returned %d", ret);
	zassert_not_null(strstr(out, "Deleted device key in sec tag 42"),
			 "missing delete confirmation, got: %s", out);
	zassert_false(nrf_cloud_credentials_keygen_key_exists(TEST_SEC_TAG),
		      "key still present after shell delete");
}

ZTEST(keygen_shell, test_shell_invalid_sec_tag)
{
	int ret;
	const char *out = run("nrf_cloud_cred keygen notanumber", &ret);

	zassert_not_equal(ret, 0, "invalid sec tag should fail");
	zassert_not_null(strstr(out, "Invalid sec tag"), "missing error, got: %s", out);
}
