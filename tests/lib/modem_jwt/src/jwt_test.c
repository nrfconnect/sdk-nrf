/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <unity.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <kernel.h>
#include <modem/modem_jwt.h>
#include <mock_at_cmd.h>


void setUp(void)
{
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

int cmd_cb(const char * const cmd, char *buf, size_t buf_len, enum at_cmd_state *state,
	   int cmock_num_calls)
{
	if (cmock_num_calls == 0) {
		/* Fail first call */
		return AT_CMD_ERROR;
	} else if (cmock_num_calls == 1) {
		TEST_ASSERT_EQUAL_STRING("AT%JWT=0,0,\"\",\"\"", cmd);
	} else if (cmock_num_calls == 2) {
		TEST_ASSERT_EQUAL_STRING("AT%JWT=0,100,\"sub\",\"aud\",10,2", cmd);
	}
	strncpy(buf, jwt_token, buf_len - 1);
	buf[buf_len-1] = 0;
	return 0;
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

	__wrap_at_cmd_init_ExpectAndReturn(-1);
	rc = modem_jwt_generate(&jwt);
	TEST_ASSERT_EQUAL_INT(-EPIPE, rc);

	__wrap_at_cmd_init_ExpectAndReturn(0);
	__wrap_at_cmd_write_Stub(cmd_cb);
	rc = modem_jwt_generate(&jwt);
	TEST_ASSERT_EQUAL_INT(-EBADMSG, rc);

	/* Success case, default parameters */
	__wrap_at_cmd_init_ExpectAndReturn(0);
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
	__wrap_at_cmd_init_ExpectAndReturn(0);
	rc = modem_jwt_generate(&jwt);
	TEST_ASSERT_EQUAL_INT(0, rc);
}

void test_modem_jwt_get_uuids(void)
{
	struct nrf_device_uuid dev = {0};
	struct nrf_modem_fw_uuid mfw = {0};
	int rc;

	__wrap_at_cmd_init_IgnoreAndReturn(0);
	rc = modem_jwt_get_uuids(&dev, &mfw);
	TEST_ASSERT_EQUAL_INT(0, rc);
	TEST_ASSERT_EQUAL_STRING("50363154-3930-4d96-80f9-13121cf35adc", dev.str);
	TEST_ASSERT_EQUAL_STRING("f72d9789-2e73-4780-9248-b3952f89b872", mfw.str);
}

extern int unity_main(void);

void main(void)
{
	(void)unity_main();
}
