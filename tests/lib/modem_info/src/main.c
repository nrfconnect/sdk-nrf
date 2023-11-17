/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <unity.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>

#include "modem_info.h"

#include <zephyr/fff.h>

#include <nrf_modem_at.h>
#include <nrf_errno.h>

DEFINE_FFF_GLOBALS;

FAKE_VALUE_FUNC(int, nrf_modem_at_notif_handler_set, nrf_modem_at_notif_handler_t);
FAKE_VALUE_FUNC(int, at_params_list_init, struct at_param_list *, size_t);
FAKE_VALUE_FUNC_VARARG(int, nrf_modem_at_scanf, const char *, const char *, ...);
FAKE_VALUE_FUNC_VARARG(int, nrf_modem_at_cmd, void *, size_t, const char *, ...);

#define FW_UUID_SIZE 37
#define SVN_SIZE 3
#define EXAMPLE_UUID "48d7cbc0-3354-11ed-9a95-e334dfb8a3cb"
#define EXAMPLE_FWVER "nrf9160_1.3.2"
#define EXAMPLE_SVN "25"
#define EXAMPLE_VBAT 5054
#define EXAMPLE_TEMP 24
#define EXAMPLE_RSRP_INVALID 255
#define EXAMPLE_RSRP_VALID 160
#define RSRP_OFFSET 140
#define EXAMPLE_BAND 13
#define EXAMPLE_BAND_MAX_VAL 71
#define EXAMPLE_ONE_LETTER_OPERATOR_NAME "O"
#define EXAMPLE_SHORT_OPERATOR_NAME "OP"
#define EXAMPLE_TOO_LARGE_OPERATOR_NAME "TOO_LARGE_OPERATOR_NAME"

// TODO: Confirm - this is an educated guess at the moment, based upon various
// #define values in the library. Get input from Nordic on this later.
#define XMONITOR_CMD_MAX_RESPONSE_LEN 300

struct at_param at_params[10] = {};
static struct at_param_list m_param_list = {
	.param_count = 0,
	.params = at_params,
};

static int nrf_modem_at_scanf_custom_no_match(const char *cmd, const char *fmt, va_list args)
{
	return 0;
}

static int nrf_modem_at_scanf_custom_modemuuid(const char *cmd, const char *fmt, va_list args)
{
	TEST_ASSERT_EQUAL_STRING("AT%XMODEMUUID", cmd);
	TEST_ASSERT_EQUAL_STRING("%%XMODEMUUID: %" STRINGIFY(FW_UUID_SIZE) "[^\r\n]", fmt);

	char *buf = va_arg(args, char *);

	strncpy(buf, EXAMPLE_UUID, FW_UUID_SIZE);

	return 1;
}

static int nrf_modem_at_scanf_custom_shortswver(const char *cmd, const char *fmt, va_list args)
{
	TEST_ASSERT_EQUAL_STRING("AT%SHORTSWVER", cmd);
	TEST_ASSERT_EQUAL_STRING("%%SHORTSWVER: %" STRINGIFY(MODEM_INFO_FWVER_SIZE) "[^\r\n]", fmt);

	char *buf = va_arg(args, char *);

	strncpy(buf, EXAMPLE_FWVER, MODEM_INFO_FWVER_SIZE);

	return 1;
}

static int nrf_modem_at_scanf_custom_cgsn(const char *cmd, const char *fmt, va_list args)
{
	TEST_ASSERT_EQUAL_STRING("AT+CGSN=3", cmd);
	TEST_ASSERT_EQUAL_STRING("+CGSN: \"%" STRINGIFY(SVN_SIZE) "[^\"]", fmt);

	char *buf = va_arg(args, char *);

	strncpy(buf, EXAMPLE_SVN, SVN_SIZE);

	return 1;
}

static int nrf_modem_at_scanf_custom_vbat(const char *cmd, const char *fmt, va_list args)
{
	TEST_ASSERT_EQUAL_STRING("AT%XVBAT", cmd);
	TEST_ASSERT_EQUAL_STRING("%%XVBAT: %d", fmt);

	int *val = va_arg(args, int *);

	*val = EXAMPLE_VBAT;

	return 1;
}

static int nrf_modem_at_scanf_custom_temp(const char *cmd, const char *fmt, va_list args)
{
	TEST_ASSERT_EQUAL_STRING("AT%XTEMP?", cmd);
	TEST_ASSERT_EQUAL_STRING("%%XTEMP: %d", fmt);

	int *val = va_arg(args, int *);

	*val = EXAMPLE_TEMP;

	return 1;
}

static int nrf_modem_at_scanf_custom_cesq(const char *cmd, const char *fmt, va_list args)
{
	TEST_ASSERT_EQUAL_STRING("AT+CESQ", cmd);
	TEST_ASSERT_EQUAL_STRING("+CESQ: %*d,%*d,%*d,%*d,%*d,%d", fmt);

	int *val = va_arg(args, int *);

	*val = EXAMPLE_RSRP_VALID;

	return 1;
}

static int nrf_modem_at_scanf_custom_cesq_invalid(const char *cmd, const char *fmt, va_list args)
{
	TEST_ASSERT_EQUAL_STRING("AT+CESQ", cmd);
	TEST_ASSERT_EQUAL_STRING("+CESQ: %*d,%*d,%*d,%*d,%*d,%d", fmt);

	int *val = va_arg(args, int *);

	*val = EXAMPLE_RSRP_INVALID;

	return 1;
}

static int nrf_modem_at_scanf_custom_xcband(const char *cmd, const char *fmt, va_list args)
{
	TEST_ASSERT_EQUAL_STRING("AT%XCBAND", cmd);
	TEST_ASSERT_EQUAL_STRING("%%XCBAND: %u", fmt);

	uint8_t *val = va_arg(args, uint8_t *);
	*val = EXAMPLE_BAND;

	return 1;
}

static int nrf_modem_at_scanf_custom_xcband_max_val(const char *cmd, const char *fmt, va_list args)
{
	TEST_ASSERT_EQUAL_STRING("AT%XCBAND", cmd);
	TEST_ASSERT_EQUAL_STRING("%%XCBAND: %u", fmt);

	uint8_t *val = va_arg(args, uint8_t *);
	*val = EXAMPLE_BAND_MAX_VAL;

	return 1;
}

static int nrf_modem_at_scanf_custom_xcband_unavailable(const char *cmd, const char *fmt,
							va_list args)
{
	TEST_ASSERT_EQUAL_STRING("AT%XCBAND", cmd);
	TEST_ASSERT_EQUAL_STRING("%%XCBAND: %u", fmt);

	uint8_t *val = va_arg(args, uint8_t *);
	*val = BAND_UNAVAILABLE;

	return 1;
}

static int nrf_modem_at_scanf_custom_xcband_at_cmd_error(const char *cmd, const char *fmt,
							 va_list args)
{
	TEST_ASSERT_EQUAL_STRING("AT%XCBAND", cmd);
	TEST_ASSERT_EQUAL_STRING("%%XCBAND: %u", fmt);

	return -NRF_EBADMSG; // no arguments matched
}

static int nrf_modem_at_cmd_custom_xmonitor_no_operator(void *buf, size_t len, const char *fmt,
							va_list args)
{
	TEST_ASSERT_EQUAL_STRING("AT%%XMONITOR", fmt);
	char xmonitor_resp[XMONITOR_CMD_MAX_RESPONSE_LEN] = "%XMONITOR: 2\r\nOK\r\n";
	char *response = (char *)buf;
	memcpy(response, xmonitor_resp, sizeof(xmonitor_resp));
	return 0;
}

static int nrf_modem_at_cmd_custom_xmonitor_empty_operator(void *buf, size_t len, const char *fmt,
							   va_list args)
{
	TEST_ASSERT_EQUAL_STRING("AT%%XMONITOR", fmt);
	char xmonitor_resp[XMONITOR_CMD_MAX_RESPONSE_LEN] =
		"%XMONITOR: 1,\"Operator\",\""
		"\",\"20065\",\"4321\",7,20,\"12345678\",334,6200,66,44,\"\","
		"\"11100000\",\"00010011\",\"01001001\"";
	char *response = (char *)buf;
	memcpy(response, xmonitor_resp, sizeof(xmonitor_resp));
	return 0;
}

static int nrf_modem_at_cmd_custom_xmonitor_too_large_operator(void *buf, size_t len,
							       const char *fmt, va_list args)
{
	TEST_ASSERT_EQUAL_STRING("AT%%XMONITOR", fmt);

	// Construct the response
	char xmonitor_resp[XMONITOR_CMD_MAX_RESPONSE_LEN] = "%XMONITOR: 1,\"Operator\",\"";
	strcat(xmonitor_resp, EXAMPLE_TOO_LARGE_OPERATOR_NAME);
	strcat(xmonitor_resp, "\",\"20065\",\"4321\",7,20,\"12345678\",334,6200,66,44,\"\","
			      "\"11100000\",\"00010011\",\"01001001\"");

	char *response = (char *)buf;
	memcpy(response, xmonitor_resp, sizeof(xmonitor_resp));
	return 0;
}

static int nrf_modem_at_cmd_custom_xmonitor_one_letter_success(void *buf, size_t len,
							       const char *fmt, va_list args)
{
	TEST_ASSERT_EQUAL_STRING("AT%%XMONITOR", fmt);

	// Construct the response
	char xmonitor_resp[XMONITOR_CMD_MAX_RESPONSE_LEN] = "%XMONITOR: 1,\"Operator\",\"";
	strcat(xmonitor_resp, EXAMPLE_ONE_LETTER_OPERATOR_NAME);
	strcat(xmonitor_resp, "\",\"20065\",\"4321\",7,20,\"12345678\",334,6200,66,44,\"\","
			      "\"11100000\",\"00010011\",\"01001001\"");

	char *response = (char *)buf;
	memcpy(response, xmonitor_resp, sizeof(xmonitor_resp));
	return 0;
}

static int nrf_modem_at_cmd_custom_xmonitor_shortname_success(void *buf, size_t len,
							      const char *fmt, va_list args)
{
	TEST_ASSERT_EQUAL_STRING("AT%%XMONITOR", fmt);

	// Construct the response
	char xmonitor_resp[XMONITOR_CMD_MAX_RESPONSE_LEN] = "%XMONITOR: 1,\"Operator\",\"";
	strcat(xmonitor_resp, EXAMPLE_SHORT_OPERATOR_NAME);
	strcat(xmonitor_resp, "\",\"20065\",\"4321\",7,20,\"12345678\",334,6200,66,44,\"\","
			      "\"11100000\",\"00010011\",\"01001001\"");

	char *response = (char *)buf;
	memcpy(response, xmonitor_resp, sizeof(xmonitor_resp));
	return 0;
}

void setUp(void)
{
	RESET_FAKE(nrf_modem_at_notif_handler_set);
	RESET_FAKE(at_params_list_init);
	RESET_FAKE(nrf_modem_at_scanf);
	RESET_FAKE(nrf_modem_at_cmd);
}

void tearDown(void)
{
}

int at_params_list_init_custom(struct at_param_list *list, size_t max_params_count)
{
	*list = m_param_list;
	return EXIT_SUCCESS;
}

void test_modem_info_init_success(void)
{
	int ret;

	at_params_list_init_fake.custom_fake = at_params_list_init_custom;
	ret = modem_info_init();
	TEST_ASSERT_EQUAL(0, ret);
	TEST_ASSERT_EQUAL(1, at_params_list_init_fake.call_count);
}

void test_modem_info_get_fw_uuid_null(void)
{
	int ret;

	ret = modem_info_get_fw_uuid(NULL, FW_UUID_SIZE);
	TEST_ASSERT_EQUAL(-EINVAL, ret);
	TEST_ASSERT_EQUAL(0, nrf_modem_at_scanf_fake.call_count);
}

void test_modem_info_get_fw_uuid_buf_too_small(void)
{
	int ret;
	char buf[FW_UUID_SIZE-1];

	ret = modem_info_get_fw_uuid(buf, ARRAY_SIZE(buf));
	TEST_ASSERT_EQUAL(-EINVAL, ret);
	TEST_ASSERT_EQUAL(0, nrf_modem_at_scanf_fake.call_count);
}

void test_modem_info_get_fw_uuid_at_returns_error(void)
{
	int ret;
	char buf[FW_UUID_SIZE];

	nrf_modem_at_scanf_fake.custom_fake = nrf_modem_at_scanf_custom_no_match;

	ret = modem_info_get_fw_uuid(buf, ARRAY_SIZE(buf));
	TEST_ASSERT_EQUAL(-EIO, ret);
	TEST_ASSERT_EQUAL(1, nrf_modem_at_scanf_fake.call_count);
}

void test_modem_info_get_fw_uuid_success(void)
{
	int ret;
	char buf[FW_UUID_SIZE];

	nrf_modem_at_scanf_fake.custom_fake = nrf_modem_at_scanf_custom_modemuuid;

	ret = modem_info_get_fw_uuid(buf, ARRAY_SIZE(buf));
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	TEST_ASSERT_EQUAL_STRING(EXAMPLE_UUID, buf);
	TEST_ASSERT_EQUAL(1, nrf_modem_at_scanf_fake.call_count);
}

void test_modem_info_get_fw_version_null(void)
{
	int ret;

	ret = modem_info_get_fw_version(NULL, 0);
	TEST_ASSERT_EQUAL(-EINVAL, ret);
	TEST_ASSERT_EQUAL(0, nrf_modem_at_scanf_fake.call_count);
}

void test_modem_info_get_fw_version_buf_too_small(void)
{
	int ret;
	char buf[MODEM_INFO_FWVER_SIZE-1];

	nrf_modem_at_scanf_fake.custom_fake = nrf_modem_at_scanf_custom_shortswver;

	ret = modem_info_get_fw_version(buf, ARRAY_SIZE(buf));
	TEST_ASSERT_EQUAL(-EINVAL, ret);
	TEST_ASSERT_EQUAL(0, nrf_modem_at_scanf_fake.call_count);
}

void test_modem_info_get_fw_version_at_returns_error(void)
{
	int ret;
	char buf[MODEM_INFO_FWVER_SIZE];

	nrf_modem_at_scanf_fake.custom_fake = nrf_modem_at_scanf_custom_no_match;

	ret = modem_info_get_fw_version(buf, ARRAY_SIZE(buf));
	TEST_ASSERT_EQUAL(-EIO, ret);
	TEST_ASSERT_EQUAL(1, nrf_modem_at_scanf_fake.call_count);
}

void test_modem_info_get_fw_version_success(void)
{
	int ret;
	char buf[MODEM_INFO_FWVER_SIZE];

	nrf_modem_at_scanf_fake.custom_fake = nrf_modem_at_scanf_custom_shortswver;

	ret = modem_info_get_fw_version(buf, ARRAY_SIZE(buf));
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	TEST_ASSERT_EQUAL_STRING(EXAMPLE_FWVER, buf);
	TEST_ASSERT_EQUAL(1, nrf_modem_at_scanf_fake.call_count);
}

void test_modem_info_get_svn_null(void)
{
	int ret;

	ret = modem_info_get_svn(NULL, SVN_SIZE);
	TEST_ASSERT_EQUAL(-EINVAL, ret);
	TEST_ASSERT_EQUAL(0, nrf_modem_at_scanf_fake.call_count);
}

void test_modem_info_get_svn_buf_too_small(void)
{
	int ret;
	char buf[SVN_SIZE-1];

	ret = modem_info_get_svn(buf, ARRAY_SIZE(buf));
	TEST_ASSERT_EQUAL(-EINVAL, ret);
	TEST_ASSERT_EQUAL(0, nrf_modem_at_scanf_fake.call_count);
}

void test_modem_info_get_svn_at_returns_error(void)
{
	int ret;
	char buf[SVN_SIZE];

	nrf_modem_at_scanf_fake.custom_fake = nrf_modem_at_scanf_custom_no_match;

	ret = modem_info_get_svn(buf, ARRAY_SIZE(buf));
	TEST_ASSERT_EQUAL(-EIO, ret);
	TEST_ASSERT_EQUAL(1, nrf_modem_at_scanf_fake.call_count);
}

void test_modem_info_get_svn_success(void)
{
	int ret;
	char buf[SVN_SIZE];

	nrf_modem_at_scanf_fake.custom_fake = nrf_modem_at_scanf_custom_cgsn;

	ret = modem_info_get_svn(buf, ARRAY_SIZE(buf));
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	TEST_ASSERT_EQUAL_STRING(EXAMPLE_SVN, buf);
	TEST_ASSERT_EQUAL(1, nrf_modem_at_scanf_fake.call_count);
}

void test_modem_info_get_batt_voltage_null(void)
{
	int ret;

	ret = modem_info_get_batt_voltage(NULL);
	TEST_ASSERT_EQUAL(-EINVAL, ret);
	TEST_ASSERT_EQUAL(0, nrf_modem_at_scanf_fake.call_count);
}

void test_modem_info_get_batt_voltage_at_returns_error(void)
{
	int ret;
	int val;

	nrf_modem_at_scanf_fake.custom_fake = nrf_modem_at_scanf_custom_no_match;

	ret = modem_info_get_batt_voltage(&val);
	TEST_ASSERT_EQUAL(-EIO, ret);
	TEST_ASSERT_EQUAL(1, nrf_modem_at_scanf_fake.call_count);
}

void test_modem_info_get_batt_voltage_success(void)
{
	int ret;
	int val;

	nrf_modem_at_scanf_fake.custom_fake = nrf_modem_at_scanf_custom_vbat;

	ret = modem_info_get_batt_voltage(&val);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	TEST_ASSERT_EQUAL(EXAMPLE_VBAT, val);
	TEST_ASSERT_EQUAL(1, nrf_modem_at_scanf_fake.call_count);
}

void test_modem_info_get_temperature_null(void)
{
	int ret;

	ret = modem_info_get_temperature(NULL);
	TEST_ASSERT_EQUAL(-EINVAL, ret);
	TEST_ASSERT_EQUAL(0, nrf_modem_at_scanf_fake.call_count);
}

void test_modem_info_get_temperature_at_returns_error(void)
{
	int ret;
	int val;

	nrf_modem_at_scanf_fake.custom_fake = nrf_modem_at_scanf_custom_no_match;

	ret = modem_info_get_temperature(&val);
	TEST_ASSERT_EQUAL(-EIO, ret);
	TEST_ASSERT_EQUAL(1, nrf_modem_at_scanf_fake.call_count);
}

void test_modem_info_get_temperature_success(void)
{
	int ret;
	int val;

	nrf_modem_at_scanf_fake.custom_fake = nrf_modem_at_scanf_custom_temp;

	ret = modem_info_get_temperature(&val);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	TEST_ASSERT_EQUAL(EXAMPLE_TEMP, val);
	TEST_ASSERT_EQUAL(1, nrf_modem_at_scanf_fake.call_count);
}

void test_modem_info_get_rsrp_null(void)
{
	int ret;

	ret = modem_info_get_rsrp(NULL);
	TEST_ASSERT_EQUAL(-EINVAL, ret);
	TEST_ASSERT_EQUAL(0, nrf_modem_at_scanf_fake.call_count);
}

void test_modem_info_get_rsrp_at_returns_error(void)
{
	int ret;
	int val;

	nrf_modem_at_scanf_fake.custom_fake = nrf_modem_at_scanf_custom_no_match;

	ret = modem_info_get_rsrp(&val);
	TEST_ASSERT_EQUAL(-EIO, ret);
	TEST_ASSERT_EQUAL(1, nrf_modem_at_scanf_fake.call_count);
}

void test_modem_info_get_rsrp_invalid_rsrp(void)
{
	int ret;
	int val = 0;

	nrf_modem_at_scanf_fake.custom_fake = nrf_modem_at_scanf_custom_cesq_invalid;

	ret = modem_info_get_rsrp(&val);
	TEST_ASSERT_EQUAL(-ENOENT, ret);
	TEST_ASSERT_EQUAL(EXAMPLE_RSRP_INVALID, val);
	TEST_ASSERT_EQUAL(1, nrf_modem_at_scanf_fake.call_count);
}

void test_modem_info_get_rsrp_success(void)
{
	int ret;
	int val = 0;

	nrf_modem_at_scanf_fake.custom_fake = nrf_modem_at_scanf_custom_cesq;

	ret = modem_info_get_rsrp(&val);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	TEST_ASSERT_EQUAL(EXAMPLE_RSRP_VALID-RSRP_OFFSET, val);
	TEST_ASSERT_EQUAL(1, nrf_modem_at_scanf_fake.call_count);
}

void test_modem_info_get_current_band_null(void)
{
	int ret = modem_info_get_current_band(NULL);
	TEST_ASSERT_EQUAL(-EINVAL, ret);
	TEST_ASSERT_EQUAL(0, nrf_modem_at_scanf_fake.call_count);
}

void test_modem_info_get_current_band_success(void)
{
	uint8_t band;

	nrf_modem_at_scanf_fake.custom_fake = nrf_modem_at_scanf_custom_xcband;

	int ret = modem_info_get_current_band(&band);
	TEST_ASSERT_EQUAL(0, ret);
	TEST_ASSERT_EQUAL(EXAMPLE_BAND, band);
	TEST_ASSERT_EQUAL(1, nrf_modem_at_scanf_fake.call_count);
}

void test_modem_info_get_current_band_success_max_val(void)
{
	uint8_t band;

	nrf_modem_at_scanf_fake.custom_fake = nrf_modem_at_scanf_custom_xcband_max_val;

	int ret = modem_info_get_current_band(&band);
	TEST_ASSERT_EQUAL(0, ret);
	TEST_ASSERT_EQUAL(EXAMPLE_BAND_MAX_VAL, band);
	TEST_ASSERT_EQUAL(1, nrf_modem_at_scanf_fake.call_count);
}

void test_modem_info_get_current_band_unavailable(void)
{
	uint8_t band;
	nrf_modem_at_scanf_fake.custom_fake = nrf_modem_at_scanf_custom_xcband_unavailable;

	int ret = modem_info_get_current_band(&band);
	TEST_ASSERT_EQUAL(-ENOMSG, ret);
	TEST_ASSERT_EQUAL(1, nrf_modem_at_scanf_fake.call_count);
}

void test_modem_info_get_current_band_at_cmd_error(void)
{
	uint8_t band;
	nrf_modem_at_scanf_fake.custom_fake = nrf_modem_at_scanf_custom_xcband_at_cmd_error;

	int ret = modem_info_get_current_band(&band);
	TEST_ASSERT_EQUAL(-EBADMSG, ret); // the generic posix error should get returned
	TEST_ASSERT_EQUAL(1, nrf_modem_at_scanf_fake.call_count);
}

void test_modem_info_get_operator_invalid_null_buf(void)
{
	int ret = modem_info_get_operator(NULL, 0);
	TEST_ASSERT_EQUAL(-EINVAL, ret);
	TEST_ASSERT_EQUAL(0, nrf_modem_at_cmd_fake.call_count);
}

void test_modem_info_get_operator_invalid_buffer_len(void)
{
	char buffer[MODEM_INFO_MAX_SHORT_OP_NAME_SIZE - 1];
	int ret = modem_info_get_operator(buffer, sizeof(buffer));
	TEST_ASSERT_EQUAL(-EINVAL, ret);
	TEST_ASSERT_EQUAL(0, nrf_modem_at_cmd_fake.call_count);
}

void test_modem_info_get_operator_none(void)
{
	char buffer[MODEM_INFO_MAX_SHORT_OP_NAME_SIZE];

	nrf_modem_at_cmd_fake.custom_fake = nrf_modem_at_cmd_custom_xmonitor_no_operator;

	int ret = modem_info_get_operator(buffer, sizeof(buffer));
	TEST_ASSERT_EQUAL(-ENOMSG, ret);
	TEST_ASSERT_EQUAL(1, nrf_modem_at_cmd_fake.call_count);
}

void test_modem_info_get_operator_empty(void)
{
	char buffer[MODEM_INFO_MAX_SHORT_OP_NAME_SIZE];

	nrf_modem_at_cmd_fake.custom_fake = nrf_modem_at_cmd_custom_xmonitor_empty_operator;

	int ret = modem_info_get_operator(buffer, sizeof(buffer));
	TEST_ASSERT_EQUAL(-ENOMSG, ret);
	TEST_ASSERT_EQUAL(1, nrf_modem_at_cmd_fake.call_count);
}

void test_modem_info_get_operator_too_large(void)
{
	char buffer[MODEM_INFO_MAX_SHORT_OP_NAME_SIZE];
	char at_cmd_response[] = EXAMPLE_TOO_LARGE_OPERATOR_NAME;
	char expected_substr[MODEM_INFO_MAX_SHORT_OP_NAME_SIZE];
	strncpy(expected_substr, at_cmd_response, MODEM_INFO_MAX_SHORT_OP_NAME_SIZE - 1);
	expected_substr[MODEM_INFO_MAX_SHORT_OP_NAME_SIZE - 1] = '\0';

	nrf_modem_at_cmd_fake.custom_fake = nrf_modem_at_cmd_custom_xmonitor_too_large_operator;

	int ret = modem_info_get_operator(buffer, sizeof(buffer));
	TEST_ASSERT_EQUAL(0, ret);
	TEST_ASSERT_EQUAL_STRING(expected_substr, buffer);
	TEST_ASSERT_EQUAL(1, nrf_modem_at_cmd_fake.call_count);
}

void test_modem_info_get_operator_one_letter(void)
{
	char buffer[MODEM_INFO_MAX_SHORT_OP_NAME_SIZE];

	nrf_modem_at_cmd_fake.custom_fake = nrf_modem_at_cmd_custom_xmonitor_one_letter_success;

	int ret = modem_info_get_operator(buffer, sizeof(buffer));
	TEST_ASSERT_EQUAL(0, ret);
	TEST_ASSERT_EQUAL_STRING(EXAMPLE_ONE_LETTER_OPERATOR_NAME, buffer);
	TEST_ASSERT_EQUAL(1, nrf_modem_at_cmd_fake.call_count);
}

void test_modem_info_get_operator_shortname_success(void)
{
	char buffer[MODEM_INFO_MAX_SHORT_OP_NAME_SIZE];

	nrf_modem_at_cmd_fake.custom_fake = nrf_modem_at_cmd_custom_xmonitor_shortname_success;

	int ret = modem_info_get_operator(buffer, sizeof(buffer));
	TEST_ASSERT_EQUAL(0, ret);
	TEST_ASSERT_EQUAL_STRING(EXAMPLE_SHORT_OPERATOR_NAME, buffer);
	TEST_ASSERT_EQUAL(1, nrf_modem_at_cmd_fake.call_count);
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
