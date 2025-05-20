/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>

#include <modem/at_cmd_parser.h>
#include <modem/at_params.h>
#include <../lib/at_cmd_parser/at_utils.h>

ZTEST(at_utils, test_notification_detection)
{
	for (char c = 0; c < 127; ++c) {
		if ((c == '%') || (c == '+') || (c == '#')) {
			continue;
		}

		zassert_false(is_notification(c),
			      "Notification char was detected");
	}

	zassert_true(is_notification('%'),
		     "Notification char was not detected");
	zassert_true(is_notification('+'),
		     "Notification char was not detected");
	zassert_true(is_notification('#'),
		     "Notification char was not detected");
}

ZTEST(at_utils, test_command_detection)
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

ZTEST(at_utils, test_valid_notification_char_detection)
{
	for (char c = 0; c < 127; ++c) {
		if (((c >= 'A') && (c <= 'Z')) || (c == '_')) {
			continue;
		}

		if ((c >= 'a') && (c <= 'z')) {
			continue;
		}

		zassert_false(is_valid_notification_char(c),
			      "Valid notificaion char was detected");
	}

	zassert_true(is_valid_notification_char('_'),
		     "Underscore should be a valid notification char");

	for (char c = 'A'; c <= 'Z'; ++c) {
		zassert_true(is_valid_notification_char(c),
			      "Valid notificaion char was detected");
	}

	for (char c = 'a'; c <= 'z'; ++c) {
		zassert_true(is_valid_notification_char(c),
			      "Valid notificaion char was detected");
	}
}

ZTEST(at_utils, test_string_termination)
{
	for (char c = 1; c < 127; ++c) {
		zassert_false(is_terminated(c), "String termination detected");
	}

	zassert_true(is_terminated('\0'),
		     "String termination was not detected");
}

ZTEST(at_utils, test_string_separator)
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

ZTEST(at_utils, test_lfcr)
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

ZTEST(at_utils, test_dblquote)
{
	for (char c = 0; c < 127; ++c) {
		if (c == '"') {
			continue;
		}

		zassert_false(is_dblquote(c), "Double quote char was detected");
	}

	zassert_true(is_dblquote('"'), "Double quote char was not detected");
}

ZTEST(at_utils, test_array_detection)
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

ZTEST(at_utils, test_number_detection)
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

ZTEST(at_utils, test_is_clac)
{
	char str[5] = "AT+ ";

	zassert_true(is_clac(str), "AT+ was not detected");

	str[2] = '%';
	zassert_true(is_clac(str), "AT%  was not detected");

	str[3] = 'X';
	zassert_false(is_clac(str), "AT%X was detected");
}

ZTEST_SUITE(at_utils, NULL, NULL, NULL, NULL, NULL);
