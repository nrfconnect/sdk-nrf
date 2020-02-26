#include <ztest.h>
#include <stdio.h>
#include <string.h>
#include <kernel.h>

#include <at_cmd_parser/at_cmd_parser.h>
#include <modem/at_params.h>
#include <../lib/at_cmd_parser/at_utils.h>

char *notification_str1 = "+CEREG:";
char *notification_str2 = "%CEREG:";
char *string_return     = "mfw_nrf9160_0.7.0-23.prealpha";

static void test_notification_detection(void)
{
	for (char c = 0; c < 127; ++c) {
		if ((c == '%') || (c == '+')) {
			continue;
		}

		zassert_false(is_notification(c),
			      "Notification char was detected");
	}

	zassert_true(is_notification('%'),
		     "Notification char was not detected");
	zassert_true(is_notification('+'),
		     "Notification char was not detected");
}

static void test_command_detection(void)
{
	zassert_true(is_command("AT"), "Command was not detected");
	zassert_true(is_command("AT\r"), "Command was not detected");
	zassert_true(is_command("AT\n"), "Command was not detected");
	zassert_true(is_command("AT+"), "Command was not detected");
	zassert_true(is_command("AT%"), "Command was not detected");
	zassert_true(is_command("AT#"), "Command was not detected");
	zassert_true(is_command("at+"), "Command was not detected");
	zassert_true(is_command("at%"), "Command was not detected");
	zassert_true(is_command("at#"), "Command was not detected");
	zassert_false(is_command("BT+"), "Should fail, invalid string");
	zassert_false(is_command("AB+"), "Should fail, invalid string");
	zassert_false(is_command("AT$"), "Should fail, invalid string");
}

static void test_valid_notification_char_detection(void)
{
	for (char c = 0; c < 127; ++c) {
		if ((c >= 'A') && (c <= 'Z')) {
			continue;
		}

		if ((c >= 'a') && (c <= 'z')) {
			continue;
		}

		zassert_false(is_valid_notification_char(c),
			      "Valid notificaion char was detected");
	}

	for (char c = 'A'; c <= 'Z'; ++c) {
		zassert_true(is_valid_notification_char(c),
			      "Valid notificaion char was detected");
	}

	for (char c = 'a'; c <= 'z'; ++c) {
		zassert_true(is_valid_notification_char(c),
			      "Valid notificaion char was detected");
	}
}

static void test_string_termination(void)
{
	for (char c = 1; c < 127; ++c) {
		zassert_false(is_terminated(c), "String termination detected");
	}

	zassert_true(is_terminated('\0'),
		     "String termination was not detected");
}

static void test_string_separator(void)
{
	for (char c = 0; c < 127; ++c) {
		if ((c == ':') || (c == ',') || (c == '=')) {
			continue;
		}

		zassert_false(is_separator(c), "Separator detected");
	}

	zassert_true(is_separator('='), "Separator not detected");
	zassert_true(is_separator(':'), "Separator not detected");
	zassert_true(is_separator(','), "Separator not detected");
}

static void test_lfcr(void)
{
	for (char c = 0; c < 127; ++c) {
		if ((c == '\r') || (c == '\n')) {
			continue;
		}

		zassert_false(is_lfcr(c), "LFCR detected");
	}

	zassert_true(is_lfcr('\n'), "LFCR was not detected");
	zassert_true(is_lfcr('\r'), "LFCR was not detected");
}

static void test_dblquote(void)
{
	for (char c = 0; c < 127; ++c) {
		if (c == '"') {
			continue;
		}

		zassert_false(is_dblquote(c), "Double quote char was detected");
	}

	zassert_true(is_dblquote('"'), "Double quote char was not detected");
}

static void test_array_detection(void)
{
	for (char c = 0; c < 127; ++c) {
		if ((c == '(') || (c == ')')) {
			continue;
		}

		zassert_false(is_array_start(c) || is_array_stop(c),
			      "Array was detected detected");
	}

	zassert_true(is_array_start('('), "Array start char was not detected");
	zassert_true(is_array_stop(')'), "Array stop char was not detected");
}

static void test_number_detection(void)
{
	for (char c = 0; c < 127; ++c) {
		if ((c >= '0') && (c <= '9')) {
			continue;
		}

		if ((c == '+') || (c == '-')) {
			continue;
		}

		zassert_false(is_number(c), "Number was detected detected");
	}

	for (char c = '0'; c <= '9'; ++c) {
		zassert_true(is_number(c), "Number was not detected");
	}

	zassert_true(is_number('+'), "Number was not detected");
	zassert_true(is_number('-'), "Number was not detected");
}

void test_main(void)
{
	ztest_test_suite(at_cmd_parser,
			ztest_unit_test(test_notification_detection),
			ztest_unit_test(test_command_detection),
			ztest_unit_test(test_valid_notification_char_detection),
			ztest_unit_test(test_string_termination),
			ztest_unit_test(test_string_separator),
			ztest_unit_test(test_lfcr),
			ztest_unit_test(test_dblquote),
			ztest_unit_test(test_array_detection),
			ztest_unit_test(test_number_detection)
			);

	ztest_run_test_suite(at_cmd_parser);
}
