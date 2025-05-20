/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <unity.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <modem/modem_jwt.h>

#include "cmock_nrf_modem_at.h"


/* Return value to be returned by the stub for nrf_modem_at_cmd. */
static int at_cmd_stub_ret_val;

void setUp(void)
{
	at_cmd_stub_ret_val = 0;
}

void tearDown(void)
{
}

/* Our sample token:
 * {
 *   "typ": "JWT",
 *   "alg": "ES256",
 *   "kid": "e6c35cea1f541a4be485ff8db21f51dfe7242e7ca7b9456c90f69815e77ea61f"
 * } .
 * {
 *   "iss": "nRF9160.50363154-3930-4d96-80f9-13121cf35adc",
 *   "jti": "nRF9160.f72d9789-2e73-4780-9248-b3952f89b872.942d99b903aca2c3dd649b5d612322dc",
 *   "sub": "50363154-3930-4d96-80f9-13121cf35adc"
 * }
 */
static const char jwt_token[] =
	"%JWT: \""
	"eyJ0eXAiOiJKV1QiLCJhbGciOiJFUzI1NiIsImtpZCI6ImU2YzM1Y2VhMWY1NDFhNGJlNDg1ZmY4ZGIyMWY1MWRmZTcyNDJlN2NhN2I5NDU2YzkwZjY5ODE1ZTc3ZWE2MWYifSAg.eyJpc3MiOiJuUkY5MTYwLjUwMzYzMTU0LTM5MzAtNGQ5Ni04MGY5LTEzMTIxY2YzNWFkYyIsImp0aSI6Im5SRjkxNjAuZjcyZDk3ODktMmU3My00NzgwLTkyNDgtYjM5NTJmODliODcyLjk0MmQ5OWI5MDNhY2EyYzNkZDY0OWI1ZDYxMjMyMmRjIiwic3ViIjoiNTAzNjMxNTQtMzkzMC00ZDk2LTgwZjktMTMxMjFjZjM1YWRjIn0g.tW2wG-sSE01jtrcT9JmFvPVIvEB99jUGGHIUCCH65fG5HtBrisOD1cl0L49cuyo2OE6XXvI9wdKSASxDQPJ8lw"
	"\"";

int cmd_cb(void *buf, size_t len, const char *fmt, int cmock_num_calls)
{
	strncpy(buf, jwt_token, len - 1);
	((char *)buf)[len-1] = 0;
	return at_cmd_stub_ret_val;
}

void test_modem_jwt_generate(void)
{
	struct jwt_data jwt = {0};
	int rc;

	/* Few failure cases */
	rc = modem_jwt_generate(NULL);
	TEST_ASSERT_EQUAL_INT(-EINVAL, rc);

	jwt.jwt_buf = (char *) 0xDEADEEF;
	jwt.jwt_sz = 0;
	rc = modem_jwt_generate(&jwt);
	TEST_ASSERT_EQUAL_INT(-EMSGSIZE, rc);

	jwt.jwt_buf = NULL;

	__cmock_nrf_modem_at_cmd_Stub(cmd_cb);

	/* First call should fail, pass through the error code */
	at_cmd_stub_ret_val = -1;

	rc = modem_jwt_generate(&jwt);
	TEST_ASSERT_EQUAL_INT(-1, rc);

	/* Success case, default parameters */
	at_cmd_stub_ret_val = 0;

	rc = modem_jwt_generate(&jwt);
	TEST_ASSERT_EQUAL_INT(0, rc);

	modem_jwt_free(jwt.jwt_buf);
	jwt.jwt_buf = NULL;

	/* Success case */
	jwt.subject = "sub";
	jwt.audience = "aud";
	jwt.exp_delta_s = 100;
	jwt.sec_tag = 10;
	jwt.key = 2;
	rc = modem_jwt_generate(&jwt);
	TEST_ASSERT_EQUAL_INT(0, rc);
}

void test_modem_jwt_get_uuids(void)
{
	struct nrf_device_uuid dev = {0};
	struct nrf_modem_fw_uuid mfw = {0};
	int rc;

	at_cmd_stub_ret_val = 0;
	__cmock_nrf_modem_at_cmd_Stub(cmd_cb);

	rc = modem_jwt_get_uuids(&dev, &mfw);
	TEST_ASSERT_EQUAL_INT(0, rc);
	TEST_ASSERT_EQUAL_STRING("50363154-3930-4d96-80f9-13121cf35adc", dev.str);
	TEST_ASSERT_EQUAL_STRING("f72d9789-2e73-4780-9248-b3952f89b872", mfw.str);
}

/* It is required to be added to each test. That is because unity's
 * main may return nonzero, while zephyr's main currently must
 * return 0 in all cases (other values are reserved).
 */
extern int unity_main(void);

int main(void)
{
	(void)unity_main();

	return 0;
}
