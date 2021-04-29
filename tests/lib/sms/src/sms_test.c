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
#include <modem/sms.h>
#include <mock_at_cmd.h>


static struct sms_data test_sms_data = {0};
static struct sms_deliver_header test_sms_header = {0};
static bool test_sms_header_exists = true;
static int test_handle;
static bool sms_callback_called_occurred;
static bool sms_callback_called_expected;

static void sms_callback(struct sms_data *const data, void *context);

/* sms_at_handler() is implemented in the library and we'll call it directly
 * to fake received SMS message
 */
extern void sms_at_handler(void *context, const char *at_notif);

static void helper_sms_data_clear(void)
{
	memset(&test_sms_data, 0, sizeof(test_sms_data));

	test_sms_data.type = SMS_TYPE_DELIVER;
	memset(test_sms_data.payload, 0, SMS_MAX_PAYLOAD_LEN_CHARS);

	memset(&test_sms_header, 0, sizeof(test_sms_header));

	test_sms_header_exists = true;
}

void setUp(void)
{
	helper_sms_data_clear();
	test_handle = 0;
	sms_callback_called_occurred = false;
	sms_callback_called_expected = false;
}

void tearDown(void)
{
	TEST_ASSERT_EQUAL(sms_callback_called_expected, sms_callback_called_occurred);
}

static void sms_reg_helper(void)
{
	char resp[] = "+CNMI: 0,0,0,0,1\r\n";

	__wrap_at_cmd_write_ExpectAndReturn("AT+CNMI?", NULL, 0, NULL, 0);
	__wrap_at_cmd_write_IgnoreArg_buf();
	__wrap_at_cmd_write_IgnoreArg_buf_len();
	__wrap_at_cmd_write_ReturnArrayThruPtr_buf(resp, sizeof(resp));

	__wrap_at_cmd_write_ExpectAndReturn("AT+CNMI=3,2,0,1", NULL, 0, NULL, 0);

	test_handle = sms_register_listener(sms_callback, NULL);
	TEST_ASSERT_EQUAL(0, test_handle);
}

static void sms_unreg_helper(void)
{
	__wrap_at_cmd_write_ExpectAndReturn("AT+CNMI=0,0,0,0", NULL, 0, NULL, 0);
	__wrap_at_cmd_write_IgnoreArg_buf();
	__wrap_at_cmd_write_IgnoreArg_buf_len();

	sms_unregister_listener(test_handle);
	test_handle = -1;
}

/********* SMS INIT/UNINIT TESTS ***********************/

void test_sms_init_uninit(void)
{
	sms_reg_helper();
	sms_unreg_helper();
}

void test_sms_init_fail_register_null(void)
{
	int handle = sms_register_listener(NULL, NULL);

	TEST_ASSERT_EQUAL(-EINVAL, handle);
}

void test_sms_init_fail_register_too_many(void)
{
	sms_reg_helper();

	/* Register listener with context pointer */
	int value2 = 2;
	int handle2 = sms_register_listener(sms_callback, &value2);
	int handle3 = sms_register_listener(sms_callback, NULL);

	TEST_ASSERT_EQUAL(1, handle2);
	TEST_ASSERT_EQUAL(-ENOSPC, handle3);

	/* Unregistering test_handle to get branch coverage */
	sms_unregister_listener(test_handle);
	test_handle = -1;

	/* Register test_handle again */
	test_handle = sms_register_listener(sms_callback, NULL);

	sms_unregister_listener(handle2);

	sms_unreg_helper();
}

/** Test error return value for AT+CNMI? */
void test_sms_init_fail_cnmi_query_ret_err(void)
{
	__wrap_at_cmd_write_ExpectAndReturn("AT+CNMI?", NULL, 0, NULL, -EINVAL);
	__wrap_at_cmd_write_IgnoreArg_buf();
	__wrap_at_cmd_write_IgnoreArg_buf_len();
	int handle = sms_register_listener(sms_callback, NULL);

	TEST_ASSERT_EQUAL(-EINVAL, handle);
}

/** Test unexpected response for AT+CNMI? */
void test_sms_init_fail_cnmi_resp_unexpected_value(void)
{
	char resp[] = "+CNMI: 0,0,5,0,1\r\n";

	__wrap_at_cmd_write_ExpectAndReturn("AT+CNMI?", NULL, 0, NULL, 0);
	__wrap_at_cmd_write_IgnoreArg_buf();
	__wrap_at_cmd_write_IgnoreArg_buf_len();
	__wrap_at_cmd_write_ReturnArrayThruPtr_buf(resp, sizeof(resp));
	int handle = sms_register_listener(sms_callback, NULL);

	TEST_ASSERT_EQUAL(-EBUSY, handle);
}

/** Test erroneous response for AT+CNMI? */
void test_sms_init_fail_cnmi_resp_erroneous(void)
{
	/* Make at_parser_max_params_from_str() fail */
	char resp[] = "+CNMI: 0,0,\"moi\",0,1,\r\n";

	__wrap_at_cmd_write_ExpectAndReturn("AT+CNMI?", NULL, 0, NULL, 0);
	__wrap_at_cmd_write_IgnoreArg_buf();
	__wrap_at_cmd_write_IgnoreArg_buf_len();
	__wrap_at_cmd_write_ReturnArrayThruPtr_buf(resp, sizeof(resp));
	int handle = sms_register_listener(sms_callback, NULL);

	TEST_ASSERT_EQUAL(-E2BIG, handle);
}

/** Test erroneous response for AT+CNMI? */
void test_sms_init_fail_cnmi_set_ret_err(void)
{
	char resp[] = "+CNMI: 0,0,0,0,1\r\n";

	__wrap_at_cmd_write_ExpectAndReturn("AT+CNMI?", NULL, 0, NULL, 0);
	__wrap_at_cmd_write_IgnoreArg_buf();
	__wrap_at_cmd_write_IgnoreArg_buf_len();
	__wrap_at_cmd_write_ReturnArrayThruPtr_buf(resp, sizeof(resp));
	__wrap_at_cmd_write_ExpectAndReturn("AT+CNMI=3,2,0,1", NULL, 0, NULL, -EIO);
	int handle = sms_register_listener(sms_callback, NULL);

	TEST_ASSERT_EQUAL(-EIO, handle);
}

/**
 * Test invalid SMS handles in unregistration.
 * There is no error to the client in unregistration failures so essentially
 * we just run through the code and we can see in coverage metrics whether
 * particular branches are executed.
 */
void test_sms_uninit_fail_unregister_invalid_handle(void)
{
	sms_unregister_listener(0);
	sms_unregister_listener(-1);

	sms_reg_helper();
	/* Unregister number higher than registered handle */
	sms_unregister_listener(CONFIG_SMS_SUBSCRIBERS_MAX_CNT + 1);

	/* Unregister above handle */
	sms_unreg_helper();
}

/** Test error code for AT command AT+CNMI=0,0,0,0 */
void test_sms_uninit_cnmi_ret_err(void)
{
	sms_reg_helper();

	__wrap_at_cmd_write_ExpectAndReturn("AT+CNMI=0,0,0,0", NULL, 0, NULL, -ENOEXEC);
	__wrap_at_cmd_write_IgnoreArg_buf();
	__wrap_at_cmd_write_IgnoreArg_buf_len();

	sms_unregister_listener(test_handle);

	/* Unregister to avoid leaving an open registration */
	sms_unreg_helper();
	test_handle = -1;
}

/********* SMS SEND TESTS ***********************/
/* The following site used as a reference for generated messages:
 * http://smstools3.kekekasvi.com/topic.php?id=288
 */

/**
 * Test sending using phone number with 10 characters and preceded by '+' sign.
 */
void test_send_len3_number10plus(void)
{
	sms_reg_helper();

	enum at_cmd_state state = 0;

	__wrap_at_cmd_write_ExpectAndReturn("AT+CMGS=15\r0021000A912143658709000003CD771A\x1A",
		NULL, 0, &state, 0);
	__wrap_at_cmd_write_IgnoreArg_buf();
	__wrap_at_cmd_write_IgnoreArg_buf_len();

	int ret = sms_send_text("+1234567890", "Moi");

	TEST_ASSERT_EQUAL(0, ret);
	TEST_ASSERT_EQUAL(AT_CMD_OK, state);

	/* Receive SMS-STATUS-REPORT */
	test_sms_data.type = SMS_TYPE_STATUS_REPORT;

	__wrap_at_cmd_write_ExpectAndReturn("AT+CNMA=1", NULL, 0, NULL, 0);
	sms_callback_called_expected = true;
	test_sms_header_exists = false;
	sms_at_handler(NULL, "+CDS: 24\r\n06550A912143658709122022118314801220221183148000\r\n");

	sms_unreg_helper();
}

/**
 * Test sending using phone number with 20 characters and preceded by '+' sign.
 */
void test_send_len1_number20plus(void)
{
	sms_reg_helper();

	enum at_cmd_state state = 0;

	__wrap_at_cmd_write_ExpectAndReturn(
		"AT+CMGS=18\r00210014912143658709214365870900000131\x1A", NULL, 0, &state, 0);
	__wrap_at_cmd_write_IgnoreArg_buf();
	__wrap_at_cmd_write_IgnoreArg_buf_len();

	int ret = sms_send_text("+12345678901234567890", "1");

	TEST_ASSERT_EQUAL(0, ret);
	TEST_ASSERT_EQUAL(AT_CMD_OK, state);

	/* Receive SMS-STATUS-REPORT */
	test_sms_data.type = SMS_TYPE_STATUS_REPORT;

	__wrap_at_cmd_write_ExpectAndReturn("AT+CNMA=1", NULL, 0, NULL, 0);
	sms_callback_called_expected = true;
	test_sms_header_exists = false;
	sms_at_handler(NULL, "+CDS: 24\r\n06550A912143658709122022118314801220221183148000\r\n");

	sms_unreg_helper();
}

/**
 * Test message length 7 to see that alignment with 7bit GSM encoding works.
 * Also, use phone number with 11 characters to have even count of characters.
 */
void test_send_len7_number11(void)
{
	enum at_cmd_state state = 0;

	__wrap_at_cmd_write_ExpectAndReturn(
		"AT+CMGS=20\r0021000B912143658709F100000731D98C56B3DD00\x1A", NULL, 0, &state, 0);
	__wrap_at_cmd_write_IgnoreArg_buf();
	__wrap_at_cmd_write_IgnoreArg_buf_len();

	int ret = sms_send_text("12345678901", "1234567");

	TEST_ASSERT_EQUAL(0, ret);
	TEST_ASSERT_EQUAL(AT_CMD_OK, state);
}

/**
 * Test message length 8 to see that alignment with 7bit GSM encoding works.
 * Also, use phone number with 1 character to see that everything works with
 * very short number.
 */
void test_send_len8_number1(void)
{
	enum at_cmd_state state = 0;

	__wrap_at_cmd_write_ExpectAndReturn(
		"AT+CMGS=15\r0021000191F100000831D98C56B3DD70\x1A", NULL, 0, &state, 0);
	__wrap_at_cmd_write_IgnoreArg_buf();
	__wrap_at_cmd_write_IgnoreArg_buf_len();

	int ret = sms_send_text("1", "12345678");

	TEST_ASSERT_EQUAL(0, ret);
	TEST_ASSERT_EQUAL(AT_CMD_OK, state);
}

/**
 * Test message length 9 to see that alignment with 7bit GSM encoding works.
 * Also, use phone number with 5 characters.
 */
void test_send_len9_number5(void)
{
	enum at_cmd_state state = 0;

	__wrap_at_cmd_write_ExpectAndReturn(
		"AT+CMGS=18\r00210005912143F500000931D98C56B3DD7039\x1A", NULL, 0, &state, 0);
	__wrap_at_cmd_write_IgnoreArg_buf();
	__wrap_at_cmd_write_IgnoreArg_buf_len();

	int ret = sms_send_text("12345", "123456789");

	TEST_ASSERT_EQUAL(0, ret);
	TEST_ASSERT_EQUAL(AT_CMD_OK, state);
}

/**
 * Send concatenated SMS of length 220 characters.
 * which is split into 2 messages.
 */
void test_send_concat_220chars_2msgs(void)
{
	enum at_cmd_state state1 = 0;
	enum at_cmd_state state2 = 0;

	__wrap_at_cmd_write_ExpectAndReturn(
		"AT+CMGS=153\r0061010C912143658709210000A005000301020162B219AD66BBE172B0986C46ABD96EB81C2C269BD16AB61B2E078BC966B49AED86CBC162B219AD66BBE172B0986C46ABD96EB81C2C269BD16AB61B2E078BC966B49AED86CBC162B219AD66BBE172B0986C46ABD96EB81C2C269BD16AB61B2E078BC966B49AED86CBC162B219AD66BBE172B0986C46ABD96EB81C2C269BD16AB61B2E078BC966\x1A",
		NULL, 0, &state1, 0);
	__wrap_at_cmd_write_IgnoreArg_buf();
	__wrap_at_cmd_write_IgnoreArg_buf_len();

	__wrap_at_cmd_write_ExpectAndReturn(
		"AT+CMGS=78\r0061020C9121436587092100004A0500030102026835DB0D9783C564335ACD76C3E56031D98C56B3DD7039584C36A3D56C375C0E1693CD6835DB0D9783C564335ACD76C3E56031D98C56B3DD703918\x1A",
		NULL, 0, &state2, 0);
	__wrap_at_cmd_write_IgnoreArg_buf();
	__wrap_at_cmd_write_IgnoreArg_buf_len();

	int ret = sms_send_text("+123456789012",
		"1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890");

	TEST_ASSERT_EQUAL(0, ret);
	TEST_ASSERT_EQUAL(AT_CMD_OK, state1);
	TEST_ASSERT_EQUAL(AT_CMD_OK, state2);
}

/**
 * Send concatenated SMS of length 291 characters."
 * which is split into 2 messages.
 */
void test_send_concat_291chars_2msgs(void)
{
	enum at_cmd_state state1 = 0;
	enum at_cmd_state state2 = 0;

	__wrap_at_cmd_write_ExpectAndReturn(
		"AT+CMGS=153\r0061030C912143658709210000A005000302020162B219AD66BBE172B0986C46ABD96EB81C2C269BD16AB61B2E078BC966B49AED86CBC162B219AD66BBE172B0986C46ABD96EB81C2C269BD16AB61B2E078BC966B49AED86CBC162B219AD66BBE172B0986C46ABD96EB81C2C269BD16AB61B2E078BC966B49AED86CBC162B219AD66BBE172B0986C46ABD96EB81C2C269BD16AB61B2E078BC966\x1A",
		NULL, 0, &state1, 0);
	__wrap_at_cmd_write_IgnoreArg_buf();
	__wrap_at_cmd_write_IgnoreArg_buf_len();

	__wrap_at_cmd_write_ExpectAndReturn(
		"AT+CMGS=140\r0061040C912143658709210000910500030202026835DB0D9783C564335ACD76C3E56031D98C56B3DD7039584C36A3D56C375C0E1693CD6835DB0D9783C564335ACD76C3E56031D98C56B3DD7039584C36A3D56C375C0E1693CD6835DB0D9783C564335ACD76C3E56031D98C56B3DD7039584C36A3D56C375C0E1693CD6835DB0D9783C564335ACD76C3E56031\x1A",
		NULL, 0, &state2, 0);
	__wrap_at_cmd_write_IgnoreArg_buf();
	__wrap_at_cmd_write_IgnoreArg_buf_len();

	int ret = sms_send_text("+123456789012",
		"123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901");

	TEST_ASSERT_EQUAL(0, ret);
	TEST_ASSERT_EQUAL(AT_CMD_OK, state1);
	TEST_ASSERT_EQUAL(AT_CMD_OK, state2);
}

/**
 * Send concatenated SMS of length 700 characters
 * which is split into 5 messages.
 */
void test_send_concat_700chars_5msgs(void)
{
	enum at_cmd_state state1 = 0;
	enum at_cmd_state state2 = 0;
	enum at_cmd_state state3 = 0;
	enum at_cmd_state state4 = 0;
	enum at_cmd_state state5 = 0;

	__wrap_at_cmd_write_ExpectAndReturn(
		"AT+CMGS=153\r0061050C912143658709210000A005000303050162B219AD66BBE172B0986C46ABD96EB81C2C269BD16AB61B2E078BC966B49AED86CBC162B219AD66BBE172B0986C46ABD96EB81C2C269BD16AB61B2E078BC966B49AED86CBC162B219AD66BBE172B0986C46ABD96EB81C2C269BD16AB61B2E078BC966B49AED86CBC162B219AD66BBE172B0986C46ABD96EB81C2C269BD16AB61B2E078BC966\x1A",
		NULL, 0, &state1, 0);
	__wrap_at_cmd_write_IgnoreArg_buf();
	__wrap_at_cmd_write_IgnoreArg_buf_len();

	__wrap_at_cmd_write_ExpectAndReturn(
		"AT+CMGS=153\r0061060C912143658709210000A00500030305026835DB0D9783C564335ACD76C3E56031D98C56B3DD7039584C36A3D56C375C0E1693CD6835DB0D9783C564335ACD76C3E56031D98C56B3DD7039584C36A3D56C375C0E1693CD6835DB0D9783C564335ACD76C3E56031D98C56B3DD7039584C36A3D56C375C0E1693CD6835DB0D9783C564335ACD76C3E56031D98C56B3DD7039584C36A3D56C\x1A",
		NULL, 0, &state2, 0);
	__wrap_at_cmd_write_IgnoreArg_buf();
	__wrap_at_cmd_write_IgnoreArg_buf_len();

	__wrap_at_cmd_write_ExpectAndReturn(
		"AT+CMGS=153\r0061070C912143658709210000A00500030305036EB81C2C269BD16AB61B2E078BC966B49AED86CBC162B219AD66BBE172B0986C46ABD96EB81C2C269BD16AB61B2E078BC966B49AED86CBC162B219AD66BBE172B0986C46ABD96EB81C2C269BD16AB61B2E078BC966B49AED86CBC162B219AD66BBE172B0986C46ABD96EB81C2C269BD16AB61B2E078BC966B49AED86CBC162B219AD66BBE172\x1A",
		NULL, 0, &state3, 0);
	__wrap_at_cmd_write_IgnoreArg_buf();
	__wrap_at_cmd_write_IgnoreArg_buf_len();

	__wrap_at_cmd_write_ExpectAndReturn(
		"AT+CMGS=153\r0061080C912143658709210000A00500030305046031D98C56B3DD7039584C36A3D56C375C0E1693CD6835DB0D9783C564335ACD76C3E56031D98C56B3DD7039584C36A3D56C375C0E1693CD6835DB0D9783C564335ACD76C3E56031D98C56B3DD7039584C36A3D56C375C0E1693CD6835DB0D9783C564335ACD76C3E56031D98C56B3DD7039584C36A3D56C375C0E1693CD6835DB0D9783C564\x1A",
		NULL, 0, &state4, 0);
	__wrap_at_cmd_write_IgnoreArg_buf();
	__wrap_at_cmd_write_IgnoreArg_buf_len();

	__wrap_at_cmd_write_ExpectAndReturn(
		"AT+CMGS=97\r0061090C9121436587092100005F05000303050566B49AED86CBC162B219AD66BBE172B0986C46ABD96EB81C2C269BD16AB61B2E078BC966B49AED86CBC162B219AD66BBE172B0986C46ABD96EB81C2C269BD16AB61B2E078BC966B49AED86CBC100\x1A",
		NULL, 0, &state5, 0);
	__wrap_at_cmd_write_IgnoreArg_buf();
	__wrap_at_cmd_write_IgnoreArg_buf_len();

	int ret = sms_send_text("+123456789012",
		"1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890");

	TEST_ASSERT_EQUAL(0, ret);
	TEST_ASSERT_EQUAL(AT_CMD_OK, state1);
	TEST_ASSERT_EQUAL(AT_CMD_OK, state2);
	TEST_ASSERT_EQUAL(AT_CMD_OK, state3);
	TEST_ASSERT_EQUAL(AT_CMD_OK, state4);
	TEST_ASSERT_EQUAL(AT_CMD_OK, state5);
}

/**
 * Test sending of special characters.
 */
void test_send_special_characters(void)
{
	enum at_cmd_state state = 0;

	__wrap_at_cmd_write_ExpectAndReturn(
		"AT+CMGS=49\r00210005912143F500002C5378799C0EB3416374581E1ED3CBF2B90EB4A1803628D02605DAF0401B1F68F3026D7AA00DD005\x1A",
		NULL, 0, &state, 0);
	__wrap_at_cmd_write_IgnoreArg_buf();
	__wrap_at_cmd_write_IgnoreArg_buf_len();

	int ret = sms_send_text("12345", "Special characters: ^ { } [ ] \\ ~ |.");

	TEST_ASSERT_EQUAL(0, ret);
	TEST_ASSERT_EQUAL(AT_CMD_OK, state);
}

/**
 * Test sending of special character so that escape char would go into
 * first concatenated message and next byte to second message.
 * Checks that implementation puts also escape code to second message.
 *
 * Expected AT commands have been obtained by checking what the library
 * returns and using them. Concatenation User Data Header makes it complex
 * to generate it yourself manually and no web tools exist for encoding
 * concatenated messages. Decoding of expected results have been done with
 * web tools though.
 */
void test_send_concat_special_character_split(void)
{
	enum at_cmd_state state1 = 0;
	enum at_cmd_state state2 = 0;

	__wrap_at_cmd_write_ExpectAndReturn(
		"AT+CMGS=150\r00610A05912143F500009F05000304020162B219AD66BBE172B0986C46ABD96EB81C2C269BD16AB61B2E078BC966B49AED86CBC162B219AD66BBE172B0986C46ABD96EB81C2C269BD16AB61B2E078BC966B49AED86CBC162B219AD66BBE172B0986C46ABD96EB81C2C269BD16AB61B2E078BC966B49AED86CBC162B219AD66BBE172B0986C46ABD96EB81C2C269BD16AB61B2E078BC900\x1A",
		NULL, 0, &state1, 0);
	__wrap_at_cmd_write_IgnoreArg_buf();
	__wrap_at_cmd_write_IgnoreArg_buf_len();

	__wrap_at_cmd_write_ExpectAndReturn(
		"AT+CMGS=27\r00610B05912143F500001305000304020236E5373C2E9FD3EBF63B1E\x1A",
		NULL, 0, &state2, 0);
	__wrap_at_cmd_write_IgnoreArg_buf();
	__wrap_at_cmd_write_IgnoreArg_buf_len();

	int ret = sms_send_text("12345",
		"12345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012\xA4opqrstuvwx");

	TEST_ASSERT_EQUAL(0, ret);
	TEST_ASSERT_EQUAL(AT_CMD_OK, state1);
	TEST_ASSERT_EQUAL(AT_CMD_OK, state2);
}

/** Text is empty. Message will be sent successfully. */
void test_send_text_empty(void)
{
	enum at_cmd_state state = 0;

	__wrap_at_cmd_write_ExpectAndReturn(
		"AT+CMGS=12\r002100099121436587F9000000\x1A", NULL, 0, &state, 0);
	__wrap_at_cmd_write_IgnoreArg_buf();
	__wrap_at_cmd_write_IgnoreArg_buf_len();

	int ret = sms_send_text("123456789", "");

	TEST_ASSERT_EQUAL(0, ret);
	TEST_ASSERT_EQUAL(AT_CMD_OK, state);
}

/** Text is NULL. Message will be sent successfully. */
void test_send_text_null(void)
{
	enum at_cmd_state state = 0;

	__wrap_at_cmd_write_ExpectAndReturn(
		"AT+CMGS=12\r002100099121436587F9000000\x1A", NULL, 0, &state, 0);
	__wrap_at_cmd_write_IgnoreArg_buf();
	__wrap_at_cmd_write_IgnoreArg_buf_len();

	int ret = sms_send_text("123456789", NULL);

	TEST_ASSERT_EQUAL(0, ret);
	TEST_ASSERT_EQUAL(AT_CMD_OK, state);
}

/********* SMS SEND FAIL TESTS ******************/

/** Phone number is empty. */
void test_send_fail_number_empty(void)
{
	int ret = sms_send_text("", "123456789");

	TEST_ASSERT_EQUAL(-EINVAL, ret);
}

/** Phone number is NULL. */
void test_send_fail_number_null(void)
{
	int ret = sms_send_text(NULL, "123456789");

	TEST_ASSERT_EQUAL(-EINVAL, ret);
}

/** Failing AT command response to CMGS command. */
void test_send_fail_atcmd(void)
{
	enum at_cmd_state state = AT_CMD_ERROR_CMS;

	__wrap_at_cmd_write_ExpectAndReturn(
		"AT+CMGS=15\r0021000A912143658709000003CD771A\x1A", NULL, 0, &state, 205);
	__wrap_at_cmd_write_IgnoreArg_buf();
	__wrap_at_cmd_write_IgnoreArg_buf_len();
	__wrap_at_cmd_write_IgnoreArg_state();

	int ret = sms_send_text("+1234567890", "Moi");

	TEST_ASSERT_EQUAL(205, ret);
	TEST_ASSERT_EQUAL(AT_CMD_ERROR_CMS, state);
}

/** Failing AT command response to CMGS command when sending concatenated message. */
void test_send_fail_atcmd_concat(void)
{
	enum at_cmd_state state = AT_CMD_ERROR_CME;

	__wrap_at_cmd_write_ExpectAndReturn(
		"AT+CMGS=153\r00610C0C912143658709210000A005000305020162B219AD66BBE172B0986C46ABD96EB81C2C269BD16AB61B2E078BC966B49AED86CBC162B219AD66BBE172B0986C46ABD96EB81C2C269BD16AB61B2E078BC966B49AED86CBC162B219AD66BBE172B0986C46ABD96EB81C2C269BD16AB61B2E078BC966B49AED86CBC162B219AD66BBE172B0986C46ABD96EB81C2C269BD16AB61B2E078BC966\x1A",
		NULL, 0, &state, 304);
	__wrap_at_cmd_write_IgnoreArg_buf();
	__wrap_at_cmd_write_IgnoreArg_buf_len();
	__wrap_at_cmd_write_IgnoreArg_state();

	int ret = sms_send_text("+123456789012",
		"1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890");

	TEST_ASSERT_EQUAL(304, ret);
	TEST_ASSERT_EQUAL(AT_CMD_ERROR_CME, state);
}

/**
 * Test receiving of erroneous CDS command with wrong number of arguments.
 */
void test_send_cds_erroneous(void)
{
	sms_reg_helper();

	enum at_cmd_state state = 0;

	__wrap_at_cmd_write_ExpectAndReturn(
		"AT+CMGS=15\r0021000A912143658709000003CD771A\x1A", NULL, 0, &state, 0);
	__wrap_at_cmd_write_IgnoreArg_buf();
	__wrap_at_cmd_write_IgnoreArg_buf_len();

	int ret = sms_send_text("+1234567890", "Moi");

	TEST_ASSERT_EQUAL(0, ret);
	TEST_ASSERT_EQUAL(AT_CMD_OK, state);

	/* Fail to receive SMS-STATUS-REPORT */
	sms_callback_called_expected = false;
	/* Removed length (24) from CDS */
	sms_at_handler(NULL, "+CDS: \r\n06550A912143658709122022118314801220221183148000\r\n");

	sms_unreg_helper();
}

/********* SMS RECV TESTS ***********************/

/** Callback that SMS library will call when a message is received. */
static void sms_callback(struct sms_data *const data, void *context)
{
	TEST_ASSERT_EQUAL(sms_callback_called_expected, true);

	sms_callback_called_occurred = true;
	TEST_ASSERT_EQUAL(test_sms_data.type, data->type);

	TEST_ASSERT_EQUAL_STRING(test_sms_data.payload, data->payload);
	TEST_ASSERT_EQUAL(test_sms_data.payload_len, data->payload_len);

	struct sms_deliver_header *sms_header = &data->header.deliver;

	if (!test_sms_header_exists) {
		return;
	}

	TEST_ASSERT_EQUAL_STRING(test_sms_header.originating_address.address_str,
		sms_header->originating_address.address_str);
	TEST_ASSERT_EQUAL(test_sms_header.originating_address.length,
		sms_header->originating_address.length);
	TEST_ASSERT_EQUAL(test_sms_header.originating_address.type,
		sms_header->originating_address.type);

	TEST_ASSERT_EQUAL(test_sms_header.time.year, sms_header->time.year);
	TEST_ASSERT_EQUAL(test_sms_header.time.month, sms_header->time.month);
	TEST_ASSERT_EQUAL(test_sms_header.time.day, sms_header->time.day);
	TEST_ASSERT_EQUAL(test_sms_header.time.hour, sms_header->time.hour);
	TEST_ASSERT_EQUAL(test_sms_header.time.minute, sms_header->time.minute);
	TEST_ASSERT_EQUAL(test_sms_header.time.second, sms_header->time.second);

	TEST_ASSERT_EQUAL(test_sms_header.app_port.present, sms_header->app_port.present);
	TEST_ASSERT_EQUAL(test_sms_header.app_port.dest_port, sms_header->app_port.dest_port);
	TEST_ASSERT_EQUAL(test_sms_header.app_port.src_port, sms_header->app_port.src_port);

	TEST_ASSERT_EQUAL(test_sms_header.concatenated.present, sms_header->concatenated.present);
	TEST_ASSERT_EQUAL(test_sms_header.concatenated.ref_number,
		sms_header->concatenated.ref_number);
	TEST_ASSERT_EQUAL(test_sms_header.concatenated.seq_number,
		sms_header->concatenated.seq_number);
	TEST_ASSERT_EQUAL(test_sms_header.concatenated.total_msgs,
		sms_header->concatenated.total_msgs);
}

/**
 * Tests:
 * - 3 bytes long user data
 * - 13 characters long number (odd number of characters)
 */
void test_recv_len3_number13(void)
{
	sms_reg_helper();

	strcpy(test_sms_header.originating_address.address_str, "1234567890123");
	test_sms_header.originating_address.length = 13;
	test_sms_header.originating_address.type = 0x91;
	test_sms_data.payload_len = 3;
	strcpy(test_sms_data.payload, "Moi");
	test_sms_header.time.year = 21;
	test_sms_header.time.month = 2;
	test_sms_header.time.day = 9;
	test_sms_header.time.hour = 20;
	test_sms_header.time.minute = 58;
	test_sms_header.time.second = 34;

	__wrap_at_cmd_write_ExpectAndReturn("AT+CNMA=1", NULL, 0, NULL, 0);
	sms_callback_called_expected = true;
	sms_at_handler(NULL, "+CMT: \"+1234567890123\",22\r\n"
		"0791534874894320040D91214365870921F300001220900285438003CD771A\r\n");

	sms_unreg_helper();
}

/**
 * Tests:
 * - 1 byte long user data
 * - 9 characters long number
 * - different number in AT command alpha parameter and SMS-DELIVER message
 *   TP-OA field.
 */
void test_recv_len1_number9(void)
{
	sms_reg_helper();

	strcpy(test_sms_header.originating_address.address_str, "123456789");
	test_sms_header.originating_address.length = 9;
	test_sms_header.originating_address.type = 0x91;
	test_sms_data.payload_len = 1;
	strcpy(test_sms_data.payload, "1");
	test_sms_header.time.year = 21;
	test_sms_header.time.month = 2;
	test_sms_header.time.day = 9;
	test_sms_header.time.hour = 20;
	test_sms_header.time.minute = 58;
	test_sms_header.time.second = 34;

	__wrap_at_cmd_write_ExpectAndReturn("AT+CNMA=1", NULL, 0, NULL, 0);
	sms_callback_called_expected = true;
	sms_at_handler(NULL, "+CMT: \"+123456789012\",20\r\n"
		"079153487489432004099121436587F90000122090028543800131\r\n");

	sms_unreg_helper();
}

/**
 * Tests:
 * - 8 bytes long user data
 * - 20 characters long number, which is maximum number length
 */
void test_recv_len8_number20(void)
{
	sms_reg_helper();

	strcpy(test_sms_header.originating_address.address_str, "12345678901234567890");
	test_sms_header.originating_address.length = 20;
	test_sms_header.originating_address.type = 0x91;
	test_sms_data.payload_len = 8;
	strcpy(test_sms_data.payload, "12345678");
	test_sms_header.time.year = 21;
	test_sms_header.time.month = 2;
	test_sms_header.time.day = 9;
	test_sms_header.time.hour = 20;
	test_sms_header.time.minute = 58;
	test_sms_header.time.second = 34;

	__wrap_at_cmd_write_ExpectAndReturn("AT+CNMA=1", NULL, 0, NULL, 0);
	sms_callback_called_expected = true;
	sms_at_handler(NULL, "+CMT: \"+12345678901234567890\",30\r\n"
		"0791534874894320041491214365870921436587090000122090028543800831D98C56B3DD70\r\n");

	sms_unreg_helper();
}

/** Receive concatenated SMS with 291 characters that are split into 2 messages. */
void test_recv_concat_len291_msgs2(void)
{
	sms_reg_helper();

	strcpy(test_sms_header.originating_address.address_str, "1234567890");
	test_sms_header.originating_address.length = 10;
	test_sms_header.originating_address.type = 0x91;
	test_sms_header.time.year = 21;
	test_sms_header.time.month = 2;
	test_sms_header.time.day = 21;
	test_sms_header.time.hour = 23;
	test_sms_header.time.minute = 50;
	test_sms_header.time.second = 44;

	test_sms_header.concatenated.present = true;
	test_sms_header.concatenated.total_msgs = 2;
	test_sms_header.concatenated.ref_number = 126;

	/* Part 1 */
	test_sms_data.payload_len = 153;
	strcpy(test_sms_data.payload,
		"123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123");
	test_sms_header.concatenated.seq_number = 1;

	__wrap_at_cmd_write_ExpectAndReturn("AT+CNMA=1", NULL, 0, NULL, 0);
	sms_callback_called_expected = true;
	sms_callback_called_occurred = false;
	sms_at_handler(NULL, "+CMT: \"+1234567890\",22\r\n"
		"0791534874894310440A912143658709000012201232054480A00500037E020162B219AD66BBE172B0986C46ABD96EB81C2C269BD16AB61B2E078BC966B49AED86CBC162B219AD66BBE172B0986C46ABD96EB81C2C269BD16AB61B2E078BC966B49AED86CBC162B219AD66BBE172B0986C46ABD96EB81C2C269BD16AB61B2E078BC966B49AED86CBC162B219AD66BBE172B0986C46ABD96EB81C2C269BD16AB61B2E078BC966\r\n");
	TEST_ASSERT_EQUAL(sms_callback_called_expected, sms_callback_called_occurred);

	/* Part 2 */
	test_sms_data.payload_len = 138;
	strcpy(test_sms_data.payload,
		"456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901");
	test_sms_header.concatenated.seq_number = 2;

	__wrap_at_cmd_write_ExpectAndReturn("AT+CNMA=1", NULL, 0, NULL, 0);
	sms_callback_called_occurred = false;
	sms_at_handler(NULL, "+CMT: \"+1234567890\",22\r\n"
		"0791534874894320440A912143658709000012201232054480910500037E02026835DB0D9783C564335ACD76C3E56031D98C56B3DD7039584C36A3D56C375C0E1693CD6835DB0D9783C564335ACD76C3E56031D98C56B3DD7039584C36A3D56C375C0E1693CD6835DB0D9783C564335ACD76C3E56031D98C56B3DD7039584C36A3D56C375C0E1693CD6835DB0D9783C564335ACD76C3E56031\r\n");
	TEST_ASSERT_EQUAL(sms_callback_called_expected, sms_callback_called_occurred);

	sms_unreg_helper();
}

/**
 * Receive concatenated SMS with 755 characters that are split into 5 messages,
 * which is maximum configured for tests.
 */
void test_recv_concat_len755_msgs5(void)
{
	sms_reg_helper();

	strcpy(test_sms_header.originating_address.address_str, "1234567890");
	test_sms_header.originating_address.length = 10;
	test_sms_header.originating_address.type = 0x91;
	test_sms_header.time.year = 21;
	test_sms_header.time.month = 2;
	test_sms_header.time.day = 22;
	test_sms_header.time.hour = 8;
	test_sms_header.time.minute = 56;
	test_sms_header.time.second = 5;

	test_sms_header.concatenated.present = true;
	test_sms_header.concatenated.total_msgs = 5;
	test_sms_header.concatenated.ref_number = 128;

	/* Part 1 */
	test_sms_data.payload_len = 153;
	strcpy(test_sms_data.payload,
		"abcdefghijklmnopqrstuvwxyz "
		"abcdefghijklmnopqrstuvwxyz "
		"abcdefghijklmnopqrstuvwxyz "
		"abcdefghijklmnopqrstuvwxyz "
		"abcdefghijklmnopqrstuvwxyz "
		"abcdefghijklmnopqr");
	test_sms_header.concatenated.seq_number = 1;

	__wrap_at_cmd_write_ExpectAndReturn("AT+CNMA=1", NULL, 0, NULL, 0);
	sms_callback_called_expected = true;
	sms_callback_called_occurred = false;
	sms_at_handler(NULL, "+CMT: \"1234567890\",159\r\n"
		"0791534874894310440A912143658709000012202280655080A0050003800501C2E231B96C3EA3D3EA35BBED7EC3E3F239BD6EBFE3F37A50583C2697CD67745ABD66B7DD6F785C3EA7D7ED777C5E0F0A8BC7E4B2F98C4EABD7ECB6FB0D8FCBE7F4BAFD8ECFEB4161F1985C369FD169F59ADD76BFE171F99C5EB7DFF1793D282C1E93CBE6333AAD5EB3DBEE373C2E9FD3EBF63B3EAF0785C56372D97C46A7D56B76DBFD86C7E5\r\n");
	TEST_ASSERT_EQUAL(sms_callback_called_expected, sms_callback_called_occurred);

	/* Part 4 */
	test_sms_header.time.second = 6;
	test_sms_data.payload_len = 153;
	strcpy(test_sms_data.payload,
		"abcdefghijklmnopqrstuvwxyz "
		"abcdefghijklmnopqrstuvwxyz "
		"abcdefghijklmnopqrstuvwxyz "
		"abcdefghijklmnopqrstuvwxyz "
		"abcdefghijklmnopqrstuvwxyz "
		"abcdefghijklmnopqr");
	test_sms_header.concatenated.seq_number = 4;

	__wrap_at_cmd_write_ExpectAndReturn("AT+CNMA=1", NULL, 0, NULL, 0);
	sms_callback_called_occurred = false;
	sms_at_handler(NULL, "+CMT: \"1234567890\",159\r\n"
		"0791534874894370440A912143658709000012202280656080A0050003800504C2E231B96C3EA3D3EA35BBED7EC3E3F239BD6EBFE3F37A50583C2697CD67745ABD66B7DD6F785C3EA7D7ED777C5E0F0A8BC7E4B2F98C4EABD7ECB6FB0D8FCBE7F4BAFD8ECFEB4161F1985C369FD169F59ADD76BFE171F99C5EB7DFF1793D282C1E93CBE6333AAD5EB3DBEE373C2E9FD3EBF63B3EAF0785C56372D97C46A7D56B76DBFD86C7E5\r\n");
	TEST_ASSERT_EQUAL(sms_callback_called_expected, sms_callback_called_occurred);

	/* Part 2 */
	test_sms_data.payload_len = 153;
	strcpy(test_sms_data.payload,
		"stuvwxyz abcdefghijklmnopqrstuvwxyz "
		"abcdefghijklmnopqrstuvwxyz "
		"abcdefghijklmnopqrstuvwxyz "
		"abcdefghijklmnopqrstuvwxyz "
		"abcdefghijklmnopqrstuvwxyz abcdefghi");
	test_sms_header.concatenated.seq_number = 2;

	__wrap_at_cmd_write_ExpectAndReturn("AT+CNMA=1", NULL, 0, NULL, 0);
	sms_callback_called_occurred = false;
	sms_at_handler(NULL, "+CMT: \"1234567890\",159\r\n"
		"0791534874894370440A912143658709000012202280656080A0050003800502E6F4BAFD8ECFEB4161F1985C369FD169F59ADD76BFE171F99C5EB7DFF1793D282C1E93CBE6333AAD5EB3DBEE373C2E9FD3EBF63B3EAF0785C56372D97C46A7D56B76DBFD86C7E5737ADD7EC7E7F5A0B0784C2E9BCFE8B47ACD6EBBDFF0B87C4EAFDBEFF8BC1E14168FC965F3199D56AFD96DF71B1E97CFE975FB1D9FD783C2E231B96C3EA3D3\r\n");
	TEST_ASSERT_EQUAL(sms_callback_called_expected, sms_callback_called_occurred);

	/* Part 3 */
	test_sms_data.payload_len = 153;
	strcpy(test_sms_data.payload,
		"jklmnopqrstuvwxyz abcdefghijklmnopqrstuvwxyz "
		"abcdefghijklmnopqrstuvwxyz "
		"abcdefghijklmnopqrstuvwxyz "
		"abcdefghijklmnopqrstuvwxyz "
		"abcdefghijklmnopqrstuvwxyz ");
	test_sms_header.concatenated.seq_number = 3;

	__wrap_at_cmd_write_ExpectAndReturn("AT+CNMA=1", NULL, 0, NULL, 0);
	sms_callback_called_occurred = false;
	sms_at_handler(NULL, "+CMT: \"1234567890\",159\r\n"
		"0791534874894310440A912143658709000012202280656080A0050003800503D46B76DBFD86C7E5737ADD7EC7E7F5A0B0784C2E9BCFE8B47ACD6EBBDFF0B87C4EAFDBEFF8BC1E14168FC965F3199D56AFD96DF71B1E97CFE975FB1D9FD783C2E231B96C3EA3D3EA35BBED7EC3E3F239BD6EBFE3F37A50583C2697CD67745ABD66B7DD6F785C3EA7D7ED777C5E0F0A8BC7E4B2F98C4EABD7ECB6FB0D8FCBE7F4BAFD8ECFEB41\r\n");
	TEST_ASSERT_EQUAL(sms_callback_called_expected, sms_callback_called_occurred);

	/* Part 5 */
	test_sms_data.payload_len = 143;
	strcpy(test_sms_data.payload,
		"stuvwxyz abcdefghijklmnopqrstuvwxyz "
		"abcdefghijklmnopqrstuvwxyz "
		"abcdefghijklmnopqrstuvwxyz "
		"abcdefghijklmnopqrstuvwxyz "
		"abcdefghijklmnopqrstuvwxyz");
	test_sms_header.concatenated.seq_number = 5;

	__wrap_at_cmd_write_ExpectAndReturn("AT+CNMA=1", NULL, 0, NULL, 0);
	sms_callback_called_occurred = false;
	sms_at_handler(NULL, "+CMT: \"1234567890\"151\r\n"
		"0791534874894310440A91214365870900001220228065608096050003800505E6F4BAFD8ECFEB4161F1985C369FD169F59ADD76BFE171F99C5EB7DFF1793D282C1E93CBE6333AAD5EB3DBEE373C2E9FD3EBF63B3EAF0785C56372D97C46A7D56B76DBFD86C7E5737ADD7EC7E7F5A0B0784C2E9BCFE8B47ACD6EBBDFF0B87C4EAFDBEFF8BC1E14168FC965F3199D56AFD96DF71B1E97CFE975FB1D9FD703\r\n");
	TEST_ASSERT_EQUAL(sms_callback_called_expected, sms_callback_called_occurred);

	sms_unreg_helper();
}

/** Test sending of special characters. */
void test_recv_special_characters(void)
{
	sms_reg_helper();

	strcpy(test_sms_header.originating_address.address_str, "123456789");
	test_sms_header.originating_address.length = 9;
	test_sms_header.originating_address.type = 0x91;
	test_sms_data.payload_len = 38;
	/* ISO-8859-15 indicates that euro sign is 0xA4 */
	strcpy(test_sms_data.payload, "Special characters: ^ { } [ ] \\ ~ | \xA4.");
	test_sms_header.time.year = 21;
	test_sms_header.time.month = 2;
	test_sms_header.time.day = 9;
	test_sms_header.time.hour = 20;
	test_sms_header.time.minute = 58;
	test_sms_header.time.second = 34;

	__wrap_at_cmd_write_ExpectAndReturn("AT+CNMA=1", NULL, 0, NULL, 0);
	sms_callback_called_expected = true;
	sms_at_handler(NULL, "+CMT: \"+123456789\",20\r\n"
		"079153487489432004099121436587F90000122090028543802F5378799C0EB3416374581E1ED3CBF2B90EB4A1803628D02605DAF0401B1F68F3026D7AA00D10B429BB00\r\n");

	sms_unreg_helper();
}

/**
 * Test receiving of special character so that escape char is the last
 * character of the first concatenated message. This is an erroneous message
 * but let's prepare for network issues.
 */
void test_recv_concat_escape_character_last(void)
{
	sms_reg_helper();

	strcpy(test_sms_header.originating_address.address_str, "1234567890");
	test_sms_header.originating_address.length = 10;
	test_sms_header.originating_address.type = 0x91;
	test_sms_header.time.year = 21;
	test_sms_header.time.month = 2;
	test_sms_header.time.day = 22;
	test_sms_header.time.hour = 8;
	test_sms_header.time.minute = 56;
	test_sms_header.time.second = 5;

	test_sms_header.concatenated.present = true;
	test_sms_header.concatenated.total_msgs = 2;
	test_sms_header.concatenated.ref_number = 81;

	/* Part 1 */
	test_sms_data.payload_len = 152;
	strcpy(test_sms_data.payload,
		"12345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012");
	test_sms_header.concatenated.seq_number = 1;

	__wrap_at_cmd_write_ExpectAndReturn("AT+CNMA=1", NULL, 0, NULL, 0);
	sms_callback_called_expected = true;
	sms_callback_called_occurred = false;
	sms_at_handler(NULL, "+CMT: \"1234567890\",159\r\n"
		"0791534874894370440A912143658709000012202280655080A005000351020162B219AD66BBE172B0986C46ABD96EB81C2C269BD16AB61B2E078BC966B49AED86CBC162B219AD66BBE172B0986C46ABD96EB81C2C269BD16AB61B2E078BC966B49AED86CBC162B219AD66BBE172B0986C46ABD96EB81C2C269BD16AB61B2E078BC966B49AED86CBC162B219AD66BBE172B0986C46ABD96EB81C2C269BD16AB61B2E078BC936\r\n");
	TEST_ASSERT_EQUAL(sms_callback_called_expected, sms_callback_called_occurred);

	/* Part 2 */
	test_sms_data.payload_len = 11;
	sprintf(test_sms_data.payload, "%c1234567890", 0xA4);
	test_sms_header.concatenated.seq_number = 2;

	__wrap_at_cmd_write_ExpectAndReturn("AT+CNMA=1", NULL, 0, NULL, 0);
	sms_callback_called_occurred = false;
	sms_at_handler(NULL, "+CMT: \"1234567890\",159\r\n"
		"0791534874894370440A9121436587090000122022806550801305000351020236E5986C46ABD96EB81C0C\r\n");
	TEST_ASSERT_EQUAL(sms_callback_called_expected, sms_callback_called_occurred);

	sms_unreg_helper();
}

/**
 * Tests:
 * - Data Coding Scheme with Coding Group Bits 7..4 set to 1111 while normally
 *   we have 7..6 set to 00.
 * - Use GSM 7bit encoding.
 */
void test_recv_dcs1111_gsm7bit(void)
{
	sms_reg_helper();

	strcpy(test_sms_header.originating_address.address_str, "12345");
	test_sms_header.originating_address.length = 5;
	test_sms_header.originating_address.type = 0x91;
	test_sms_data.payload_len = 8;
	strcpy(test_sms_data.payload, "12345678");
	test_sms_header.time.year = 21;
	test_sms_header.time.month = 2;
	test_sms_header.time.day = 9;
	test_sms_header.time.hour = 20;
	test_sms_header.time.minute = 58;
	test_sms_header.time.second = 34;

	__wrap_at_cmd_write_ExpectAndReturn("AT+CNMA=1", NULL, 0, NULL, 0);
	sms_callback_called_expected = true;
	sms_at_handler(NULL, "+CMT: \"+12345\",30\r\n"
		"07915348748943200405912143F500F0122090028543800831D98C56B3DD70\r\n");

	sms_unreg_helper();
}

/**
 * Data Coding Scheme with Coding Group Bits 7..4 set to 1111 while normally
 * we have 7..6 set to 00.
 * Use 8bit encoding.
 */
void test_recv_dcs1111_8bit(void)
{
	sms_reg_helper();

	strcpy(test_sms_header.originating_address.address_str, "12345");
	test_sms_header.originating_address.length = 5;
	test_sms_header.originating_address.type = 0x91;
	test_sms_data.payload_len = 15;
	strcpy(test_sms_data.payload,
		"\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F");
	test_sms_header.time.year = 21;
	test_sms_header.time.month = 2;
	test_sms_header.time.day = 9;
	test_sms_header.time.hour = 20;
	test_sms_header.time.minute = 58;
	test_sms_header.time.second = 34;

	__wrap_at_cmd_write_ExpectAndReturn("AT+CNMA=1", NULL, 0, NULL, 0);
	sms_callback_called_expected = true;
	sms_at_handler(NULL, "+CMT: \"+12345\",30\r\n"
		"07915348748943200405912143F500F4122090028543800F0102030405060708090A0B0C0D0E0F\r\n");

	sms_unreg_helper();
}

/** Test port addressing */
void test_recv_port_addr(void)
{
	sms_reg_helper();

	strcpy(test_sms_header.originating_address.address_str, "12345678");
	test_sms_header.originating_address.length = 8;
	test_sms_header.originating_address.type = 0x81;
	test_sms_data.payload_len = 15;
	strcpy(test_sms_data.payload,
		"\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F");
	test_sms_header.time.year = 21;
	test_sms_header.time.month = 1;
	test_sms_header.time.day = 30;
	test_sms_header.time.hour = 12;
	test_sms_header.time.minute = 34;
	test_sms_header.time.second = 56;

	test_sms_header.app_port.present = true;
	test_sms_header.app_port.dest_port = 2948;

	test_sms_header.concatenated.present = true;
	test_sms_header.concatenated.total_msgs = 1;
	test_sms_header.concatenated.ref_number = 124;
	test_sms_header.concatenated.seq_number = 1;

	__wrap_at_cmd_write_ExpectAndReturn("AT+CNMA=1", NULL, 0, NULL, 0);
	sms_callback_called_expected = true;
	sms_at_handler(NULL, "+CMT: \"12345678\",22\r\n"
		"004408812143658700041210032143652b1b0b05040b84000000037c01010102030405060708090A0B0C0D0E0F\r\n");

	sms_unreg_helper();
}

/********* SMS RECV FAIL TESTS ******************/

/** Test AT command unknown for SMS library. */
void test_recv_unknown(void)
{
	sms_reg_helper();
	sms_at_handler(NULL, "%CESQ: 54,2,16,2\r\n");
	sms_unreg_helper();
}

/** Test empty AT command string. */
void test_recv_empty_at_cmd(void)
{
	sms_reg_helper();
	sms_at_handler(NULL, "");
	sms_unreg_helper();
}

/** Test NULL AT command string. */
void test_recv_null_at_cmd(void)
{
	sms_reg_helper();
	sms_at_handler(NULL, NULL);
	sms_unreg_helper();
}

/** Test SMS receive with empty text (zero length). */
void test_recv_empty_sms_text(void)
{
	sms_reg_helper();

	strcpy(test_sms_header.originating_address.address_str, "1234567890");
	test_sms_header.originating_address.length = 10;
	test_sms_header.originating_address.type = 0x91;
	test_sms_data.payload_len = 0;
	strcpy(test_sms_data.payload, "");
	test_sms_header.time.year = 21;
	test_sms_header.time.month = 2;
	test_sms_header.time.day = 9;
	test_sms_header.time.hour = 20;
	test_sms_header.time.minute = 58;
	test_sms_header.time.second = 34;

	__wrap_at_cmd_write_ExpectAndReturn("AT+CNMA=1", NULL, 0, NULL, 0);
	sms_callback_called_expected = true;
	sms_at_handler(NULL, "+CMT: \"+1234567890\",18\r\n"
		"00040A91214365870900001220900285438000\r\n");

	sms_unreg_helper();
}

/** Test SMS receiving for PDU with non-hexadecimal digits. */
void test_recv_invalid_hex_characters(void)
{
	sms_reg_helper();

	sms_callback_called_expected = false;
	sms_at_handler(NULL, "+CMT: \"+1234567890\",18\r\n"
		"00040A91214365870900001220A00285438009123456KLAB\r\n");

	sms_unreg_helper();
}

/**
 * Test SMS receiving for PDU which is one character or
 * one semi-octet too short.
 */
void test_recv_invalid_header_too_short(void)
{
	sms_reg_helper();

	sms_callback_called_expected = false;
	sms_at_handler(NULL, "+CMT: \"+1234567890\",18\r\n"
		"00040A91214365870900001220A0028543800\r\n");

	sms_unreg_helper();
}

/** Tests too long number as maximum allowed by 3GPP TS 23.040 is 20. */
void test_recv_fail_number21(void)
{
	sms_reg_helper();

	sms_callback_called_expected = false;
	sms_at_handler(NULL, "+CMT: \"+123456789012345678901\",32\r\n"
		"0004169121436587092143658709F10000122090028543800831D98C56B3DD70\r\n");

	sms_unreg_helper();
}

/** Test erroneous CMT response which is missing some parameter. */
void test_recv_cmt_erroneous(void)
{
	sms_reg_helper();

	sms_callback_called_expected = false;
	/* Size missing (22\r\n) */
	sms_at_handler(NULL, "+CMT: \"+1234567890\","
		"0791534874894320040A91214365870900001220900285438003CD771A\r\n");

	sms_unreg_helper();
}

/** DCS not supported (UCS2): 0x08 */
void test_recv_fail_invalid_dcs_ucs2(void)
{
	sms_reg_helper();

	sms_callback_called_expected = false;
	sms_at_handler(NULL, "+CMT: \"+1234567890\",22\r\n"
		"004408812143658700081210032143652b1c0b05040b84000000037c0101010203040506070809\r\n");

	sms_unreg_helper();
}

/** DCS not supported: 0x80 */
void test_recv_fail_invalid_dcs_unsupported_format(void)
{
	sms_reg_helper();

	sms_callback_called_expected = false;
	sms_at_handler(NULL, "+CMT: \"+1234567890\",22\r\n"
		"004408812143658700801210032143652b1c0b05040b84000000037c0101010203040506070809\r\n");

	sms_unreg_helper();
}

/** Receive too long message with 161 bytes in actual text. */
void test_recv_fail_len161(void)
{
	sms_reg_helper();

	sms_callback_called_expected = false;
	sms_at_handler(NULL, "+CMT: \"+1234567890\",22\r\n"
		"00040A912143658709000012201232054480A131D98C56B3DD7039584C36A3D56C375C0E1693CD6835DB0D9783C564335ACD76C3E56031D98C56B3DD7039584C36A3D56C375C0E1693CD6835DB0D9783C564335ACD76C3E56031D98C56B3DD7039584C36A3D56C375C0E1693CD6835DB0D9783C564335ACD76C3E56031D98C56B3DD7039584C36A3D56C375C0E1693CD6835DB0D9783C564335ACD76C3E56031\r\n");

	sms_unreg_helper();
}

/**
 * Test large enough data not to fit into the internal PDU buffer.
 * Message size is still said to be maximum of 160 GSM 7bit decoded octets.
 * But the actual SMS-DELIVER message is 264 bytes, or 528 characters.
 */
void test_recv_fail_internal_pdu_buffer_too_short(void)
{
	sms_reg_helper();

	sms_callback_called_expected = false;
	sms_at_handler(NULL, "+CMT: \"+1234567890\",22\r\n"
		"00040A912143658709000012201232054480A031D98C56B3DD7039584C36A3D56C375C0E1693CD6835DB0D9783C564335ACD76C3E56031D98C56B3DD7039584C36A3D56C375C0E1693CD6835DB0D9783C564335ACD76C3E56031D98C56B3DD7039584C36A3D56C375C0E1693CD6835DB0D9783C564335ACD76C3E56031D98C56B3DD7039584C36A3D56C375C0E1693CD6835DB0D9783C564335ACD76C3E56031D98C56B3DD7039584C36A3D56C375C0E1693CD6835DB0D9783C564335ACD76C3E56031D98C56B3DD7039584C36A3D56C375C0E1693CD6835DB0D9783C564335ACD76C3E56031D98C56B3DD7039584C36A3D56C375C0E1693CD6835DB0D9783C564335ACD76C3E560\r\n");

	sms_unreg_helper();
}

/**
 * Test large enough data not to fit into the internal payload buffer.
 * Message size is still said to be maximum of 160 GSM 7bit decoded octets.
 * But the actual SMS-DELIVER message is 181 bytes, or 362 characters.
 */
void test_recv_fail_internal_payload_buffer_too_short(void)
{
	sms_reg_helper();

	sms_callback_called_expected = false;
	sms_at_handler(NULL, "+CMT: \"+1234567890\",22\r\n"
		"0B912143658709214365870904149121436587092143658709000012201232054480A031D98C56B3DD7039584C36A3D56C375C0E1693CD6835DB0D9783C564335ACD76C3E56031D98C56B3DD7039584C36A3D56C375C0E1693CD6835DB0D9783C564335ACD76C3E56031D98C56B3DD7039584C36A3D56C375C0E1693CD6835DB0D9783C564335ACD76C3E56031D98C56B3DD7039584C36A3D56C375C0E1693CD6835DB0D9783C564335ACD76C3E560313233343536\r\n");

	sms_unreg_helper();
}

/** Test User-Data-Length shorter than the actual data that is given */
void test_recv_invalid_udl_shorter_than_ud_7bit(void)
{
	sms_reg_helper();

	strcpy(test_sms_header.originating_address.address_str, "1234567890");
	test_sms_header.originating_address.length = 10;
	test_sms_header.originating_address.type = 0x91;
	test_sms_data.payload_len = 32;
	strcpy(test_sms_data.payload, "12345678901234567890123456789012");
	test_sms_header.time.year = 21;
	test_sms_header.time.month = 2;
	test_sms_header.time.day = 21;
	test_sms_header.time.hour = 23;
	test_sms_header.time.minute = 50;
	test_sms_header.time.second = 44;

	__wrap_at_cmd_write_ExpectAndReturn("AT+CNMA=1", NULL, 0, NULL, 0);
	sms_callback_called_expected = true;
	sms_at_handler(NULL, "+CMT: \"+1234567890\",22\r\n"
		"00040A9121436587090000122012320544802031D98C56B3DD7039584C36A3D56C375C0E1693CD6835DB0D9783C564335ACD76C3E56031\r\n");

	sms_unreg_helper();
}

/**
 * Test User-Data-Length longer than the actual data that is given.
 * Length here is NOT divisible by 7/8 resulting in float and not integer.
 * Using length 41 which results into 41*7/8=35.875.
 */
void test_recv_invalid_udl_longer_than_ud_7bit_len41(void)
{
	sms_reg_helper();

	strcpy(test_sms_header.originating_address.address_str, "1234567890");
	test_sms_header.originating_address.length = 10;
	test_sms_header.originating_address.type = 0x91;
	test_sms_data.payload_len = 41;
	strcpy(test_sms_data.payload, "12345678901234567890123456789012345678901");
	test_sms_header.time.year = 21;
	test_sms_header.time.month = 2;
	test_sms_header.time.day = 21;
	test_sms_header.time.hour = 23;
	test_sms_header.time.minute = 50;
	test_sms_header.time.second = 44;

	__wrap_at_cmd_write_ExpectAndReturn("AT+CNMA=1", NULL, 0, NULL, 0);
	sms_callback_called_expected = true;
	sms_at_handler(NULL, "+CMT: \"+1234567890\",22\r\n"
		"00040A9121436587090000122012320544802A31D98C56B3DD7039584C36A3D56C375C0E1693CD6835DB0D9783C564335ACD76C3E56031\r\n");

	sms_unreg_helper();
}

/**
 * Test User-Data-Length longer than the actual data that is given.
 * Length here is divisible by 7/8 resulting in integer and not float.
 * Using length 40 which results into 40*7/8=35.
 */
void test_recv_invalid_udl_longer_than_ud_7bit_len40(void)
{
	sms_reg_helper();

	strcpy(test_sms_header.originating_address.address_str, "1234567890");
	test_sms_header.originating_address.length = 10;
	test_sms_header.originating_address.type = 0x91;
	test_sms_data.payload_len = 40;
	strcpy(test_sms_data.payload, "1234567890123456789012345678901234567890");
	test_sms_header.time.year = 21;
	test_sms_header.time.month = 2;
	test_sms_header.time.day = 21;
	test_sms_header.time.hour = 23;
	test_sms_header.time.minute = 50;
	test_sms_header.time.second = 44;

	__wrap_at_cmd_write_ExpectAndReturn("AT+CNMA=1", NULL, 0, NULL, 0);
	sms_callback_called_expected = true;
	sms_at_handler(NULL, "+CMT: \"+1234567890\",22\r\n"
		"00040A9121436587090000122012320544802A31D98C56B3DD7039584C36A3D56C375C0E1693CD6835DB0D9783C564335ACD76C3E560\r\n");

	sms_unreg_helper();
}

/** Test User-Data-Length longer than the actual data that is given */
void test_recv_invalid_udl_longer_than_ud_8bit(void)
{
	sms_reg_helper();

	strcpy(test_sms_header.originating_address.address_str, "1234567890");
	test_sms_header.originating_address.length = 10;
	test_sms_header.originating_address.type = 0x91;
	test_sms_data.payload_len = 9;
	strcpy(test_sms_data.payload, "\x01\x02\x03\x04\x05\x06\x07\x08\x09");
	test_sms_header.time.year = 21;
	test_sms_header.time.month = 2;
	test_sms_header.time.day = 21;
	test_sms_header.time.hour = 23;
	test_sms_header.time.minute = 50;
	test_sms_header.time.second = 44;

	__wrap_at_cmd_write_ExpectAndReturn("AT+CNMA=1", NULL, 0, NULL, 0);
	sms_callback_called_expected = true;
	sms_at_handler(NULL, "+CMT: \"+1234567890\",22\r\n"
		"00040A9121436587090004122012320544800A010203040506070809\r\n");

	sms_unreg_helper();
}

/** Test User-Data-Length shorter than the actual data that is given */
void test_recv_invalid_udl_shorter_than_ud_8bit(void)
{
	sms_reg_helper();

	strcpy(test_sms_header.originating_address.address_str, "1234567890");
	test_sms_header.originating_address.length = 10;
	test_sms_header.originating_address.type = 0x91;
	test_sms_data.payload_len = 10;
	strcpy(test_sms_data.payload, "\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A");
	test_sms_header.time.year = 21;
	test_sms_header.time.month = 2;
	test_sms_header.time.day = 21;
	test_sms_header.time.hour = 23;
	test_sms_header.time.minute = 50;
	test_sms_header.time.second = 44;

	__wrap_at_cmd_write_ExpectAndReturn("AT+CNMA=1", NULL, 0, NULL, 0);
	sms_callback_called_expected = true;
	sms_at_handler(NULL, "+CMT: \"+1234567890\",22\r\n"
		"00040A9121436587090004122012320544800A0102030405060708090A0B0C0D0E0F\r\n");

	sms_unreg_helper();
}

/** Tests User-Data-Header Length that is longer than User-Data-Length. */
void test_recv_fail_udhl_longer_than_udh(void)
{
	sms_reg_helper();

	sms_callback_called_expected = false;
	sms_at_handler(NULL, "+CMT: \"+1234567890\",22\r\n"
		"00440A912143658709000012201232054480050500037E0201AAAA\r\n");

	sms_unreg_helper();
}

/** Tests User-Data-Header Length that is longer than the actual user data. */
void test_recv_fail_udhl_longer_than_ud(void)
{
	sms_reg_helper();

	sms_callback_called_expected = false;
	sms_at_handler(NULL, "+CMT: \"+1234567890\",22\r\n"
		"00440A91214365870900001220123205448004030201\r\n");

	sms_unreg_helper();
}

/** Tests User-Data-Header with actual data of length 0. */
void test_recv_udh_with_datalen0(void)
{
	sms_reg_helper();

	strcpy(test_sms_header.originating_address.address_str, "1234567890");
	test_sms_header.originating_address.length = 10;
	test_sms_header.originating_address.type = 0x91;
	test_sms_data.payload_len = 0;
	strcpy(test_sms_data.payload, "");
	test_sms_header.time.year = 21;
	test_sms_header.time.month = 2;
	test_sms_header.time.day = 21;
	test_sms_header.time.hour = 23;
	test_sms_header.time.minute = 50;
	test_sms_header.time.second = 44;

	test_sms_header.concatenated.present = true;
	test_sms_header.concatenated.total_msgs = 1;
	test_sms_header.concatenated.ref_number = 171;
	test_sms_header.concatenated.seq_number = 1;

	__wrap_at_cmd_write_ExpectAndReturn("AT+CNMA=1", NULL, 0, NULL, 0);
	sms_callback_called_expected = true;
	sms_at_handler(NULL, "+CMT: \"+1234567890\",22\r\n"
		"00440A91214365870900001220123205448006050003AB0101\r\n");

	sms_unreg_helper();
}

/**
 * Tests User-Data-Header with actual data of length 0
 * and extra byte for fill bits.
 */
void test_recv_udh_with_datalen0_fill_byte(void)
{
	sms_reg_helper();

	strcpy(test_sms_header.originating_address.address_str, "1234567890");
	test_sms_header.originating_address.length = 10;
	test_sms_header.originating_address.type = 0x91;
	test_sms_data.payload_len = 0;
	strcpy(test_sms_data.payload, "");
	test_sms_header.time.year = 21;
	test_sms_header.time.month = 2;
	test_sms_header.time.day = 21;
	test_sms_header.time.hour = 23;
	test_sms_header.time.minute = 50;
	test_sms_header.time.second = 44;

	test_sms_header.concatenated.present = true;
	test_sms_header.concatenated.total_msgs = 1;
	test_sms_header.concatenated.ref_number = 171;
	test_sms_header.concatenated.seq_number = 1;

	__wrap_at_cmd_write_ExpectAndReturn("AT+CNMA=1", NULL, 0, NULL, 0);
	sms_callback_called_expected = true;
	sms_at_handler(NULL, "+CMT: \"+1234567890\",22\r\n"
		"00440A91214365870900001220123205448007050003AB010100\r\n");

	sms_unreg_helper();
}

/** Tests User-Data-Header with actual data of length 1. */
void test_recv_udh_with_datalen1(void)
{
	sms_reg_helper();

	strcpy(test_sms_header.originating_address.address_str, "1234567890");
	test_sms_header.originating_address.length = 10;
	test_sms_header.originating_address.type = 0x91;
	test_sms_data.payload_len = 1;
	strcpy(test_sms_data.payload, "1");
	test_sms_header.time.year = 21;
	test_sms_header.time.month = 2;
	test_sms_header.time.day = 21;
	test_sms_header.time.hour = 23;
	test_sms_header.time.minute = 50;
	test_sms_header.time.second = 44;

	test_sms_header.concatenated.present = true;
	test_sms_header.concatenated.total_msgs = 1;
	test_sms_header.concatenated.ref_number = 171;
	test_sms_header.concatenated.seq_number = 1;

	__wrap_at_cmd_write_ExpectAndReturn("AT+CNMA=1", NULL, 0, NULL, 0);
	sms_callback_called_expected = true;
	sms_at_handler(NULL, "+CMT: \"+1234567890\",22\r\n"
		"00440A91214365870900001220123205448008050003AB010162\r\n");

	sms_unreg_helper();
}

/**
 * UDH IE is too long to fit to UDH based on UDH length:
 *   1B 0A 05040B841111 00037C01
 * Correct header for reference:
 *   1C 0B 05040B841111 00037C0101
 */
void test_recv_invalid_udh_too_long_ie(void)
{
	sms_reg_helper();

	strcpy(test_sms_header.originating_address.address_str, "12345678");
	test_sms_header.originating_address.length = 8;
	test_sms_header.originating_address.type = 0x81;
	test_sms_data.payload_len = 15;
	strcpy(test_sms_data.payload,
		"\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F");
	test_sms_header.time.year = 21;
	test_sms_header.time.month = 1;
	test_sms_header.time.day = 30;
	test_sms_header.time.hour = 12;
	test_sms_header.time.minute = 34;
	test_sms_header.time.second = 56;

	test_sms_header.app_port.present = false;
	test_sms_header.concatenated.present = false;

	__wrap_at_cmd_write_ExpectAndReturn("AT+CNMA=1", NULL, 0, NULL, 0);
	sms_callback_called_expected = true;
	sms_at_handler(NULL, "+CMT: \"12345678\",22\r\n"
		"004408812143658700041210032143652B1B0A05040B84111100037C010102030405060708090A0B0C0D0E0F\r\n");

	sms_unreg_helper();
}

/**
 * Tests the following:
 * - Concatenation IE (8-bit) is too short with 2 vs. 3 bytes (IE ignored)
 * - Concatenation IE (8-bit) total messages is 0 (IE ignored)
 * - Concatenation IE (8-bit) sequence number is 0 (IE ignored)
 * - Concatenation IE (8-bit) sequence number is bigger than total number of messages (IE ignored)
 * - Valid Port Addressing IE (8-bit)
 * - Concatenation IE (16-bit) is too long with 5 vs. 4 bytes (IE ignored)
 *
 * User Data Header for this test:
 *   2F 1E 00022A01 00032A0002 00032A0200 00032A0203 04021100
 *   08051111222222
 */
void test_recv_invalid_udh_concat_ignored_portaddr_valid(void)
{
	sms_reg_helper();

	strcpy(test_sms_header.originating_address.address_str, "12345678");
	test_sms_header.originating_address.length = 8;
	test_sms_header.originating_address.type = 0x81;
	test_sms_data.payload_len = 15;
	strcpy(test_sms_data.payload,
		"\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F");
	test_sms_header.time.year = 21;
	test_sms_header.time.month = 1;
	test_sms_header.time.day = 30;
	test_sms_header.time.hour = 12;
	test_sms_header.time.minute = 34;
	test_sms_header.time.second = 56;

	test_sms_header.app_port.present = true;
	test_sms_header.app_port.dest_port = 17;

	test_sms_header.concatenated.present = false;

	__wrap_at_cmd_write_ExpectAndReturn("AT+CNMA=1", NULL, 0, NULL, 0);
	sms_callback_called_expected = true;
	sms_at_handler(NULL, "+CMT: \"12345678\",22\r\n"
		"004408812143658700041210032143652B2F1E00022A0100032A000200032A020000032A020304021100080511112222220102030405060708090A0B0C0D0E0F\r\n");

	sms_unreg_helper();
}

/**
 * Tests the following:
 * - Unsupported UDH IE with zero length (IE ignored)
 * - Valid Concatenation IE (16-bit)
 * - Port Addressing IE (8-bit) is too short with 0 vs. 2 bytes (IE ignored)
 * - Port Addressing IE (16-bit) is too long with 7 vs. 4 bytes (IE ignored)
 * - Unsupported UDH IE (IE ignored)
 *
 * User Data Header for this test:
 *   2C 1B 0100 080411110101 0400 050712345678901234 A106123456789012
 */
void test_recv_invalid_udh_portaddr_ignored_concat_valid(void)
{
	sms_reg_helper();

	strcpy(test_sms_header.originating_address.address_str, "12345678");
	test_sms_header.originating_address.length = 8;
	test_sms_header.originating_address.type = 0x81;
	test_sms_data.payload_len = 15;
	strcpy(test_sms_data.payload,
		"\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F");
	test_sms_header.time.year = 21;
	test_sms_header.time.month = 1;
	test_sms_header.time.day = 30;
	test_sms_header.time.hour = 12;
	test_sms_header.time.minute = 34;
	test_sms_header.time.second = 56;

	test_sms_header.app_port.present = false;

	test_sms_header.concatenated.present = true;
	test_sms_header.concatenated.total_msgs = 1;
	test_sms_header.concatenated.ref_number = 4369;
	test_sms_header.concatenated.seq_number = 1;

	__wrap_at_cmd_write_ExpectAndReturn("AT+CNMA=1", NULL, 0, NULL, 0);
	sms_callback_called_expected = true;
	sms_at_handler(NULL, "+CMT: \"12345678\",22\r\n"
		"004408812143658700041210032143652B2C1B01000804111101010400050712345678901234A1061234567890120102030405060708090A0B0C0D0E0F\r\n");

	sms_unreg_helper();
}

/********* SMS MULTIPLE SEND & RECV TESTS ***********************/

/**
 * Helper function to provide basic SMS sending.
 * Useful in tests where multiple send operations are done.
 */
void send_basic(void)
{
	helper_sms_data_clear();

	enum at_cmd_state state = 0;

	__wrap_at_cmd_write_ExpectAndReturn(
		"AT+CMGS=15\r0021000A912143658709000003CD771A\x1A",
		NULL, 0, &state, 0);
	__wrap_at_cmd_write_IgnoreArg_buf();
	__wrap_at_cmd_write_IgnoreArg_buf_len();
	int ret = sms_send_text("+1234567890", "Moi");

	TEST_ASSERT_EQUAL(0, ret);
	TEST_ASSERT_EQUAL(AT_CMD_OK, state);

	/* Receive SMS-STATUS-REPORT */
	test_sms_data.type = SMS_TYPE_STATUS_REPORT;

	__wrap_at_cmd_write_ExpectAndReturn("AT+CNMA=1", NULL, 0, NULL, 0);
	sms_callback_called_expected = true;
	test_sms_header_exists = false;
	sms_at_handler(NULL, "+CDS: 24\r\n06550A912143658709122022118314801220221183148000\r\n");
}

/**
 * Helper function to provide basic SMS receiving.
 * Useful in tests where multiple send operations are done.
 */
void recv_basic(void)
{
	helper_sms_data_clear();

	test_sms_data.type = SMS_TYPE_DELIVER;
	strcpy(test_sms_header.originating_address.address_str, "1234567890");
	test_sms_header.originating_address.length = 10;
	test_sms_header.originating_address.type = 0x91;
	test_sms_data.payload_len = 3;
	strcpy(test_sms_data.payload, "Moi");
	test_sms_header.time.year = 21;
	test_sms_header.time.month = 2;
	test_sms_header.time.day = 9;
	test_sms_header.time.hour = 20;
	test_sms_header.time.minute = 58;
	test_sms_header.time.second = 34;

	__wrap_at_cmd_write_ExpectAndReturn("AT+CNMA=1", NULL, 0, NULL, 0);
	sms_callback_called_expected = true;
	sms_at_handler(NULL, "+CMT: \"+1234567890\",22\r\n"
		"0791534874894320040A91214365870900001220900285438003CD771A\r\n");
}

/**
 * Tests sending and receiving multiple times in mixed order.
 */
void test_send_recv_both(void)
{
	sms_reg_helper();

	send_basic();
	recv_basic();

	send_basic();
	send_basic();
	recv_basic();
	recv_basic();
	recv_basic();
	recv_basic();
	recv_basic();
	send_basic();

	sms_unreg_helper();
}

/**
 * Tests sending in a loop.
 */
void test_send_loop(void)
{
	sms_reg_helper();

	for (int i = 0; i < 20; i++) {
		helper_sms_data_clear();
		send_basic();
	}
	sms_unreg_helper();
}

/**
 * Tests receiving in a loop.
 */
void test_recv_loop(void)
{
	sms_reg_helper();

	for (int i = 0; i < 20; i++) {
		helper_sms_data_clear();
		recv_basic();
	}
	sms_unreg_helper();
}

/* It is required to be added to each test. That is because unity is using
 * different main signature (returns int) and zephyr expects main which does
 * not return value.
 */
extern int unity_main(void);

void main(void)
{
	(void)unity_main();
}
