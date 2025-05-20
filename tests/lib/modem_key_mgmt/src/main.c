/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>

#include <modem/modem_key_mgmt.h>

#include <zephyr/fff.h>

#include <nrf_modem_at.h>
#include <nrf_errno.h>

DEFINE_FFF_GLOBALS;

FAKE_VOID_FUNC(key_list_cb, nrf_sec_tag_t, enum modem_key_mgmt_cred_type);
FAKE_VALUE_FUNC_VARARG(int, nrf_modem_at_printf, const char *, ...);
FAKE_VALUE_FUNC_VARARG(int, nrf_modem_at_scanf, const char *, const char *, ...);
FAKE_VALUE_FUNC_VARARG(int, nrf_modem_at_cmd, void *, size_t, const char *, ...);

static const char test_cmee_enabled[] = "+CMEE: 1";
static const char *test_data_empty_list = "OK\r\n";

static int nrf_modem_at_scanf_test_digest_empty(const char *cmd, const char *fmt, va_list args)
{
	switch (nrf_modem_at_scanf_fake.call_count) {
	case 1:
		/* For the purpose of this test, simplify by having the cmee already enabled. */
		return vsscanf(test_cmee_enabled, fmt, args);
	case 2:
		zassert_equal(0, strcmp("AT%CMNG=1,0,0", cmd));
		return vsscanf(test_data_empty_list, fmt, args);
	default:
		zassert_true(false);
	}

	return 0;
}

ZTEST(suite_modem_key_mgmt, test_digest_no_entry)
{
	char digest_buf[32];
	int err;

	nrf_modem_at_scanf_fake.custom_fake = nrf_modem_at_scanf_test_digest_empty;

	err = modem_key_mgmt_digest(0, MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN,
				    digest_buf, sizeof(digest_buf));
	zassert_equal(err, -ENOENT);
}

ZTEST(suite_modem_key_mgmt, test_digest_size_too_small)
{
	char digest_buf[32];
	int err;

	err = modem_key_mgmt_digest(0, MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN,
				    digest_buf, sizeof(digest_buf) - 1);
	zassert_equal(err, -ENOMEM);
}

static const char *test_data_digest =
"%CMNG: 16842753,1,\"B9BAC15641653CAE2B5151DCBD0C40DDCDF19125950FA2475977437EA35F7A45\"\r\n"
"OK\r\n";

static const uint8_t expected_digest[] = {
	0xB9, 0xBA, 0xC1, 0x56,  0x41, 0x65, 0x3C, 0xAE,
	0x2B, 0x51, 0x51, 0xDC,  0xBD, 0x0C, 0x40, 0xDD,
	0xCD, 0xF1, 0x91, 0x25,  0x95, 0x0F, 0xA2, 0x47,
	0x59, 0x77, 0x43, 0x7E,  0xA3, 0x5F, 0x7A, 0x45
};

static int nrf_modem_at_scanf_test_digest(const char *cmd, const char *fmt, va_list args)
{
	switch (nrf_modem_at_scanf_fake.call_count) {
	case 1:
		/* For the purpose of this test, simplify by having the cmee already enabled. */
		return vsscanf(test_cmee_enabled, fmt, args);
	case 2:
		zassert_equal(0, strcmp("AT%CMNG=1,16842753,1", cmd));
		return vsscanf(test_data_digest, fmt, args);
	default:
		zassert_true(false);
	}

	return 0;
}

ZTEST(suite_modem_key_mgmt, test_digest_public_cert)
{
	char digest_buf[32];
	int err;

	nrf_modem_at_scanf_fake.custom_fake = nrf_modem_at_scanf_test_digest;

	err = modem_key_mgmt_digest(16842753, MODEM_KEY_MGMT_CRED_TYPE_PUBLIC_CERT,
				    digest_buf, sizeof(digest_buf));
	zassert_ok(err);
	zassert_mem_equal(digest_buf, expected_digest, sizeof(expected_digest));
}

/* Invalid because the digest is one byte short. */
static const char *test_data_invalid_digest =
"%CMNG: 16842753,1,\"B9BAC15641653CAE2B5151DCBD0C40DDCDF19125950FA2475977437EA35F7A\"\r\n"
"OK\r\n";

static int nrf_modem_at_scanf_test_invalid_digest(const char *cmd, const char *fmt, va_list args)
{
	switch (nrf_modem_at_scanf_fake.call_count) {
	case 1:
		/* For the purpose of this test, simplify by having the cmee already enabled. */
		return vsscanf(test_cmee_enabled, fmt, args);
	case 2:
		zassert_equal(0, strcmp("AT%CMNG=1,16842753,1", cmd));
		return vsscanf(test_data_invalid_digest, fmt, args);
	default:
		zassert_true(false);
	}

	return 0;
}

ZTEST(suite_modem_key_mgmt, test_digest_invalid_response)
{
	char digest_buf[32];
	int err;

	nrf_modem_at_scanf_fake.custom_fake = nrf_modem_at_scanf_test_invalid_digest;

	err = modem_key_mgmt_digest(16842753, MODEM_KEY_MGMT_CRED_TYPE_PUBLIC_CERT,
				    digest_buf, sizeof(digest_buf));
	zassert_equal(err, -EINVAL);
}


ZTEST(suite_modem_key_mgmt, test_list_response_too_big)
{
	int err;

	nrf_modem_at_cmd_fake.return_val = -ENOBUFS;

	err = modem_key_mgmt_list(key_list_cb);
	zassert_equal(err, -ENOBUFS);

	zassert_equal(nrf_modem_at_cmd_fake.call_count, 1);
	zassert_equal(key_list_cb_fake.call_count, 0);
}

static int nrf_modem_at_cmd_test_list_empty(void *buf, size_t len, const char *cmd, va_list args)
{
	char formatted_cmd[20];

	vsnprintk(formatted_cmd, sizeof(formatted_cmd), cmd, args);
	zassert_equal(0, strcmp("AT%CMNG=1", formatted_cmd));

	zassert_true(len > strlen(test_data_empty_list));
	strcpy(buf, test_data_empty_list);

	return 0;
}

ZTEST(suite_modem_key_mgmt, test_list_empty)
{
	int err;

	nrf_modem_at_cmd_fake.custom_fake = nrf_modem_at_cmd_test_list_empty;

	err = modem_key_mgmt_list(key_list_cb);
	zassert_ok(err);

	zassert_equal(nrf_modem_at_cmd_fake.call_count, 1);

	zassert_equal(key_list_cb_fake.call_count, 0);
}

static const char *test_data_list =
"%CMNG: 42,0,\"2C43952EE9E000FF2ACC4E2ED0897C0A72AD5FA72C3D934E81741CBD54F05BD1\"\r\n"
"%CMNG: 1001,0,\"39FDCF28AEFFE08D03251FCCAF645E3C5DE19FA4EBBAFC89B4EDE2A422148BAB\"\r\n"
"%CMNG: 16842753,0,\"2C43952EE9E000FF2ACC4E2ED0897C0A72AD5FA72C3D934E81741CBD54F05BD1\"\r\n"
"%CMNG: 16842753,1,\"B9BAC15641653CAE2B5151DCBD0C40DDCDF19125950FA2475977437EA35F7A45\"\r\n"
"%CMNG: 16842753,2,\"3EB33ABEB054A64578ED0ACF2BC13D570557FDEB413566D3D4678A2B89F65695\"\r\n"
"%CMNG: 4294967293,10,\"2C43952EE9E000FF2ACC4E2ED0897C0A72AD5FA72C3D934E81741CBD54F05BD1\"\r\n"
"%CMNG: 4294967294,6,\"A3CC4121A62D4BB103AE92F70A77A456886AE3CD5B2D156A200A1B90D9E92E05\"\r\n"
"%CMNG: 4294967292,11,\"2027C4699EAA90A414D33FA81B975C0FDEDEFB04A19CEA1ED43A8876CAD31E89\"\r\n"
"OK\r\n";

static int nrf_modem_at_cmd_test_list(void *buf, size_t len, const char *cmd, va_list args)
{
	char formatted_cmd[20];

	vsnprintk(formatted_cmd, sizeof(formatted_cmd), cmd, args);
	zassert_equal(0, strcmp("AT%CMNG=1", formatted_cmd));

	zassert_true(len > strlen(test_data_list));
	strcpy(buf, test_data_list);

	return 0;
}

ZTEST(suite_modem_key_mgmt, test_list_multiple_entries)
{
	int err;

	nrf_modem_at_cmd_fake.custom_fake = nrf_modem_at_cmd_test_list;

	err = modem_key_mgmt_list(key_list_cb);
	zassert_ok(err);

	zassert_equal(nrf_modem_at_cmd_fake.call_count, 1);

	zassert_equal(key_list_cb_fake.call_count, 5);

	zassert_equal(key_list_cb_fake.arg0_history[0], 42);
	zassert_equal(key_list_cb_fake.arg1_history[0], MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN);

	zassert_equal(key_list_cb_fake.arg0_history[1], 1001);
	zassert_equal(key_list_cb_fake.arg1_history[1], MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN);

	zassert_equal(key_list_cb_fake.arg0_history[2], 16842753);
	zassert_equal(key_list_cb_fake.arg1_history[2], MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN);

	zassert_equal(key_list_cb_fake.arg0_history[3], 16842753);
	zassert_equal(key_list_cb_fake.arg1_history[3], MODEM_KEY_MGMT_CRED_TYPE_PUBLIC_CERT);

	zassert_equal(key_list_cb_fake.arg0_history[4], 16842753);
	zassert_equal(key_list_cb_fake.arg1_history[4], MODEM_KEY_MGMT_CRED_TYPE_PRIVATE_CERT);
}

static void _test_setup(void *fixture)
{
	RESET_FAKE(nrf_modem_at_printf);
	RESET_FAKE(nrf_modem_at_scanf);
	RESET_FAKE(nrf_modem_at_cmd);
	RESET_FAKE(key_list_cb);
}

ZTEST_SUITE(suite_modem_key_mgmt, NULL, NULL, _test_setup, NULL, NULL);
