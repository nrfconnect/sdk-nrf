/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stddef.h>
#include <zephyr/ztest.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include <modem/at_cmd_parser.h>
#include <modem/at_params.h>

#define TEST_PARAMS  4
#define TEST_PARAMS2 10

#define SINGLELINE_PARAM_COUNT      5
#define PDULINE_PARAM_COUNT         4
#define SINGLEPARAMLINE_PARAM_COUNT 1
#define EMPTYPARAMLINE_PARAM_COUNT  6
#define CERTIFICATE_PARAM_COUNT     5

static const char * const singleline[] = {
	"+CEREG: 2,\"76C1\",\"0102DA04\", 7\r\n+CME ERROR: 10\r\n",
	"+CEREG: 2,\"76C1\",\"0102DA04\", 7\r\nOK\r\n",
	"+CEREG: 2,\"76C1\",\"0102DA04\", 7\r\n"
};
static const char * const multiline[] = {
	"+CGEQOSRDP: 0,0,,\r\n"
	"+CGEQOSRDP: 1,2,,\r\n"
	"+CGEQOSRDP: 2,4,,,1,65280000\r\n",
	"+CGEQOSRDP: 0,0,,\r\n"
	"+CGEQOSRDP: 1,2,,\r\n"
	"+CGEQOSRDP: 2,4,,,1,65280000\r\nOK\r\n"
	"+CGEQOSRDP: 0,0,,\r\n"
	"+CGEQOSRDP: 1,2,,\r\n"
	"+CGEQOSRDP: 2,4,,,1,65280000\r\nERROR\r\n"
};
static const char * const pduline[] = {
	"+CMT: \"12345678\", 24\r\n"
	"06917429000171040A91747966543100009160402143708006C8329BFD0601\r\n+CME ERROR: 123\r\n",
	"+CMT: \"12345678\", 24\r\n"
	"06917429000171040A91747966543100009160402143708006C8329BFD0601\r\nOK\r\n",
	"\r\n+CMT: \"12345678\", 24\r\n"
	"06917429000171040A91747966543100009160402143708006C8329BFD0601\r\n\r\nOK\r\n",
	"+CMT: \"12345678\", 24\r\n"
	"06917429000171040A91747966543100009160402143708006C8329BFD0601\r\n",
	"\r\n+CMT: \"12345678\", 24\r\n"
	"06917429000171040A91747966543100009160402143708006C8329BFD0601\r\n"
};
static const char * const singleparamline[] = {
	"mfw_nrf9160_0.7.0-23.prealpha\r\n+CMS ERROR: 123\r\n",
	"mfw_nrf9160_0.7.0-23.prealpha\r\nOK\r\n",
	"mfw_nrf9160_0.7.0-23.prealpha\r\n"
};
static const char * const emptyparamline[] = {
	"+CPSMS: 1,,,\"10101111\",\"01101100\"\r\n",
	"+CPSMS: 1,,,\"10101111\",\"01101100\"\r\nOK\r\n",
	"+CPSMS: 1,,,\"10101111\",\"01101100\"\r\n+CME ERROR: 123\r\n"
};
static const char * const certificate =
	"%CMNG: 12345678, 0, \"978C...02C4\","
	"\"-----BEGIN CERTIFICATE-----"
	"MIIBc464..."
	"...bW9aAa4"
	"-----END CERTIFICATE-----\"\r\nERROR\r\n";

static struct at_param_list test_list;
static struct at_param_list test_list2;

static void test_params_before(void *fixture)
{
	ARG_UNUSED(fixture);

	at_params_list_init(&test_list, TEST_PARAMS);
	at_params_list_init(&test_list2, TEST_PARAMS2);
}

static void test_params_after(void *fixture)
{
	ARG_UNUSED(fixture);

	at_params_list_free(&test_list2);
	at_params_list_free(&test_list);
}

ZTEST(at_cmd_parser, test_params_fail_on_invalid_input)
{
	int ret;
	static struct at_param_list uninitialized;

	ret = at_parser_max_params_from_str(NULL, NULL, &test_list, TEST_PARAMS);
	zassert_true(ret == -EINVAL, "at_parser_max_params_from_str should return -EINVAL");
	ret = at_parser_params_from_str(NULL, NULL, &test_list);
	zassert_true(ret == -EINVAL, "at_parser_params_from_str should return -EINVAL");

	for (size_t i = 0; i < ARRAY_SIZE(singleline); i++) {
		ret = at_parser_max_params_from_str(singleline[i], NULL, NULL, TEST_PARAMS);
		zassert_true(ret == -EINVAL, "at_parser_max_params_from_str should return -EINVAL");

		ret = at_parser_max_params_from_str(singleline[i], NULL, &uninitialized,
						    TEST_PARAMS);
		zassert_true(ret == -EINVAL, "at_parser_max_params_from_str should return -EINVAL");

		ret = at_parser_params_from_str(singleline[i], NULL, &uninitialized);
		zassert_true(ret == -EINVAL, "at_parser_params_from_str should return -EINVAL");

		ret = at_parser_params_from_str(singleline[i], NULL, &test_list);
		zassert_true(ret == -E2BIG, "at_parser_params_from_str should return -E2BIG");
		zassert_equal(TEST_PARAMS, at_params_valid_count_get(&test_list),
			      "There should be TEST_PARAMS elements in the list");

		ret = at_parser_max_params_from_str(singleline[i], NULL, &test_list, TEST_PARAMS);
		zassert_true(ret == -E2BIG, "at_parser_params_from_str should return -E2BIG");
		zassert_equal(TEST_PARAMS, at_params_valid_count_get(&test_list),
			      "There should be TEST_PARAMS elements in the list");

		ret = at_parser_max_params_from_str(singleline[i], NULL, &test_list2, TEST_PARAMS);
		zassert_true(ret == -E2BIG, "at_parser_params_from_str should return -E2BIG");
		zassert_equal(TEST_PARAMS, at_params_valid_count_get(&test_list),
			      "There should be TEST_PARAMS elements in the list");
	}

	for (size_t i = 0; i < ARRAY_SIZE(multiline); i++) {
		ret = at_parser_params_from_str(multiline[i], NULL, &test_list);
		zassert_true(ret == -E2BIG, "at_parser_params_from_str should return -E2BIG");
		zassert_equal(TEST_PARAMS, at_params_valid_count_get(&test_list),
			      "There should be TEST_PARAMS elements in the list");

		ret = at_parser_max_params_from_str(multiline[i], NULL, &test_list, TEST_PARAMS);
		zassert_true(ret == -E2BIG, "at_parser_params_from_str should return -E2BIG");
		zassert_equal(TEST_PARAMS, at_params_valid_count_get(&test_list),
			      "There should be TEST_PARAMS elements in the list");

		ret = at_parser_max_params_from_str(multiline[i], NULL, &test_list2, TEST_PARAMS);
		zassert_true(ret == -E2BIG, "at_parser_params_from_str should return -E2BIG");
		zassert_equal(TEST_PARAMS, at_params_valid_count_get(&test_list),
			      "There should be TEST_PARAMS elements in the list");
	}
}

ZTEST(at_cmd_parser, test_params_string_parsing)
{
	int ret;
	char *remainder = NULL;
	char tmpbuf[32];
	uint32_t tmpbuf_len;
	int32_t tmpint;

	const char *str1 = "+TEST:1,\"Hello World!\"\r\n";
	const char *str2 = "%TEST: 1, \"Hello World!\"\r\n";
	const char *str3 = "%TEST:1,\"Hello World!\"\r\n"
			   "+TEST: 2, \"FOOBAR\"\r\n";

	/* String without spaces between parameters (str1)*/
	zassert_equal(0, at_parser_params_from_str(str1,
						   (char **)&remainder,
						   &test_list2),
		      "Parsing from string should return 0");

	zassert_equal('\0', *remainder,
		      "Remainder should only contain 0 termination character");

	zassert_equal(3, at_params_valid_count_get(&test_list2),
		      "There should be 3 valid params in the string");

	tmpbuf_len = sizeof(tmpbuf);
	zassert_equal(0, at_params_string_get(&test_list2, 0,
					      tmpbuf, &tmpbuf_len),
		      "Get string should not fail");
	zassert_equal(strlen("+TEST"), tmpbuf_len, "String length mismatch");
	zassert_equal(0, memcmp("+TEST", tmpbuf, tmpbuf_len),
		      "The string in tmpbuf should equal to +TEST");

	zassert_equal(0, at_params_int_get(&test_list2, 1, &tmpint),
		      "Get int should not fail");
	zassert_equal(1, tmpint, "Integer should be 1");

	tmpbuf_len = sizeof(tmpbuf);
	zassert_equal(0, at_params_string_get(&test_list2, 2,
					      tmpbuf, &tmpbuf_len),
		      "Get string should not fail");
	zassert_equal(strlen("Hello World!"), tmpbuf_len, "String length mismatch");
	zassert_equal(0, memcmp("Hello World!", tmpbuf, tmpbuf_len),
		      "The string in tmpbuf should equal to Hello World!");

	/* String with spaces between parameters (str2) */
	remainder = NULL;
	zassert_equal(0, at_parser_params_from_str(str2,
						   (char **)&remainder,
						   &test_list2),
		      "Parsing from string should return 0");

	zassert_equal('\0', *remainder,
		      "Remainder should only contain 0 termination character");

	zassert_equal(3, at_params_valid_count_get(&test_list2),
		      "There should be 3 valid params in the string");

	tmpbuf_len = sizeof(tmpbuf);
	zassert_equal(0, at_params_string_get(&test_list2, 0,
					      tmpbuf, &tmpbuf_len),
		      "Get string should not fail");
	zassert_equal(strlen("%TEST"), tmpbuf_len, "String length mismatch");
	zassert_equal(0, memcmp("%TEST", tmpbuf, tmpbuf_len),
		      "The string in tmpbuf should equal to %TEST");

	zassert_equal(0, at_params_int_get(&test_list2, 1, &tmpint),
		      "Get int should not fail");
	zassert_equal(1, tmpint, "Integer should be 1");

	tmpbuf_len = sizeof(tmpbuf);
	zassert_equal(0, at_params_string_get(&test_list2, 2,
					      tmpbuf, &tmpbuf_len),
		      "Get string should not fail");
	zassert_equal(strlen("Hello World!"), tmpbuf_len, "String length mismatch");
	zassert_equal(0, memcmp("Hello World!", tmpbuf, tmpbuf_len),
		      "The string in tmpbuf should equal to Hello World!");

	/* String with multiple notifications (str3) */
	remainder = (char *)str3;
	ret = at_parser_params_from_str(remainder, (char **)&remainder,
					&test_list2);

	zassert_true(ret == -EAGAIN, "Parser did not return -EAGAIN");

	tmpbuf_len = sizeof(tmpbuf);
	zassert_equal(0, at_params_string_get(&test_list2, 0,
					      tmpbuf, &tmpbuf_len),
		      "Get string should not fail");
	zassert_equal(strlen("%TEST"), tmpbuf_len, "String length mismatch");
	zassert_equal(0, memcmp("%TEST", tmpbuf, tmpbuf_len),
		      "The string in tmpbuf should equal to %TEST");


	zassert_equal(0, at_params_int_get(&test_list2, 1, &tmpint),
		      "Get int should not fail");
	zassert_equal(1, tmpint, "Integer should be 1");

	tmpbuf_len = sizeof(tmpbuf);
	zassert_equal(0, at_params_string_get(&test_list2, 2,
					      tmpbuf, &tmpbuf_len),
		      "Get string should not fail");
	zassert_equal(strlen("Hello World!"), tmpbuf_len, "String length mismatch");
	zassert_equal(0, memcmp("Hello World!", tmpbuf, tmpbuf_len),
		      "The string in tmpbuf should "
		      "equal to Hello World!");

	ret = at_parser_params_from_str(remainder, (char **)&remainder,
					&test_list2);
	zassert_equal('\0', *remainder,
		      "Remainder should only contain 0 termination character");

	zassert_true(ret == 0, "Parser did not return 0");

	tmpbuf_len = sizeof(tmpbuf);
	zassert_equal(0, at_params_string_get(&test_list2, 0,
					      tmpbuf, &tmpbuf_len),
		      "Get string should not fail");
	zassert_equal(strlen("+TEST"), tmpbuf_len, "String length mismatch");
	zassert_equal(0, memcmp("+TEST", tmpbuf, tmpbuf_len),
		      "The string in tmpbuf should equal to +TEST");


	zassert_equal(0, at_params_int_get(&test_list2, 1, &tmpint),
		      "Get int should not fail");
	zassert_true(tmpint == 2, "Integer should be 2");

	tmpbuf_len = sizeof(tmpbuf);
	zassert_equal(0, at_params_string_get(&test_list2, 2,
					      tmpbuf, &tmpbuf_len),
		      "Get string should not fail");
	zassert_equal(strlen("FOOBAR"), tmpbuf_len, "String length mismatch");
	zassert_equal(0, memcmp("FOOBAR", tmpbuf, tmpbuf_len),
		      "The string in tmpbuf should "
		      "equal to FOOBAR");
}

ZTEST(at_cmd_parser, test_params_empty_params)
{
	const char *str1 = "+TEST: 1,\r\n";
	const char *str2 = "+TEST: ,1\r\n";
	const char *str3 = "+TEST: 1,,\"Hello World!\"";
	const char *str4 = "+TEST: 1,,,\r\n";
	const char *str5 = "+TEST: ,,,1\r\n";

	/* Empty parameter at the end of the parameter list */
	zassert_equal(0, at_parser_params_from_str(str1, NULL, &test_list2),
		      "Parsing from string should return 0");

	zassert_equal(3, at_params_valid_count_get(&test_list2),
		      "There should be 3 valid params in the string");

	zassert_equal(AT_PARAM_TYPE_EMPTY, at_params_type_get(&test_list2,
							      2),
		      "Index 2 should be of empty type");

	/* Empty parameter at the beginning of the parameter list */
	zassert_equal(0, at_parser_params_from_str(str2, NULL, &test_list2),
		      "Parsing from string should return 0");

	zassert_equal(3, at_params_valid_count_get(&test_list2),
		      "There should be 3 valid params in the string");

	zassert_equal(AT_PARAM_TYPE_EMPTY, at_params_type_get(&test_list2,
							      1),
		      "Index 1 should be of empty type");

	/* Empty parameter between two other parameter types */
	zassert_equal(0, at_parser_params_from_str(str3, NULL, &test_list2),
		      "Parsing from string should return 0");

	zassert_equal(4, at_params_valid_count_get(&test_list2),
		      "There should be 4 valid params in the string");

	zassert_equal(AT_PARAM_TYPE_EMPTY, at_params_type_get(&test_list2,
							      2),
		      "Index 1 should be of empty type");

	/* 3 empty parameter at the end of the parameter list */
	zassert_equal(0, at_parser_params_from_str(str4, NULL, &test_list2),
		      "Parsing from string should return 0");

	zassert_equal(5, at_params_valid_count_get(&test_list2),
		      "There should be 5 valid params in the string");

	for (int i = 2; i < 4; ++i) {
		zassert_equal(AT_PARAM_TYPE_EMPTY,
			      at_params_type_get(&test_list2, i),
			      "Should be of empty type");
	}

	/* 3 empty parameter at the beginning of the parameter list */
	zassert_equal(0, at_parser_params_from_str(str5, NULL, &test_list2),
		      "Parsing from string should return 0");

	zassert_equal(5, at_params_valid_count_get(&test_list2),
		      "There should be 5 valid params in the string");

	for (int i = 1; i < 3; ++i) {
		zassert_equal(AT_PARAM_TYPE_EMPTY,
			      at_params_type_get(&test_list2, i),
			      "Should be of empty type");
	}
}

ZTEST(at_cmd_parser, test_testcases)
{
	int ret;
	char *remainding;

	/* Try to parse the singleline string */
	for (size_t i = 0; i < ARRAY_SIZE(singleline); i++) {
		ret = at_parser_params_from_str(singleline[i], NULL, &test_list2);
		zassert_true(ret == 0, "at_parser_params_from_str should return 0");
	}

	ret = at_params_valid_count_get(&test_list2);
	zassert_true(ret == SINGLELINE_PARAM_COUNT,
		      "at_params_valid_count_get returns wrong valid count");

	zassert_true(at_params_type_get(&test_list2, 0) == AT_PARAM_TYPE_STRING,
		     "Param type at index 0 should be a string");
	zassert_true(at_params_type_get(&test_list2, 1) ==
							AT_PARAM_TYPE_NUM_INT,
		     "Param type at index 1 should be a short");
	zassert_true(at_params_type_get(&test_list2, 2) == AT_PARAM_TYPE_STRING,
		     "Param type at index 2 should be a string");
	zassert_true(at_params_type_get(&test_list2, 3) == AT_PARAM_TYPE_STRING,
		     "Param type at index 3 should be a string");
	zassert_true(at_params_type_get(&test_list2, 4) ==
							AT_PARAM_TYPE_NUM_INT,
		     "Param type at index 4 should be a short");

	/* Try to parse the pduline string */
	for (size_t i = 0; i < ARRAY_SIZE(pduline); i++) {
		ret = at_parser_params_from_str(pduline[i], NULL, &test_list2);
		zassert_true(ret == 0, "at_parser_params_from_str should return 0");
	}

	ret = at_params_valid_count_get(&test_list2);
	zassert_true(ret == PDULINE_PARAM_COUNT,
		      "at_params_valid_count_get returns wrong valid count");

	zassert_true(at_params_type_get(&test_list2, 0) == AT_PARAM_TYPE_STRING,
		     "Param type at index 0 should be a string");
	zassert_true(at_params_type_get(&test_list2, 1) == AT_PARAM_TYPE_STRING,
		     "Param type at index 1 should be a string");
	zassert_true(at_params_type_get(&test_list2, 2) ==
							AT_PARAM_TYPE_NUM_INT,
		     "Param type at index 2 should be a short");
	zassert_true(at_params_type_get(&test_list2, 3) == AT_PARAM_TYPE_STRING,
		     "Param type at index 3 should be a string");

	for (size_t i = 0; i < ARRAY_SIZE(singleparamline); i++) {
		/* Try to parse the singleparamline string */
		ret = at_parser_params_from_str(singleparamline[i], NULL, &test_list2);
		zassert_true(ret == 0, "at_parser_params_from_str should return 0");
	}

	ret = at_params_valid_count_get(&test_list2);
	zassert_true(ret == SINGLEPARAMLINE_PARAM_COUNT,
		      "at_params_valid_count_get returns wrong valid count");

	zassert_true(at_params_type_get(&test_list2, 0) == AT_PARAM_TYPE_STRING,
		     "Param type at index 0 should be a string");

	for (size_t i = 0; i < ARRAY_SIZE(emptyparamline); i++) {
		/* Try to parse the string containing empty/optional parameters  */
		ret = at_parser_params_from_str(emptyparamline[i], NULL, &test_list2);
		zassert_true(ret == 0, "at_parser_params_from_str should return 0");
	}

	ret = at_params_valid_count_get(&test_list2);
	zassert_true(ret == EMPTYPARAMLINE_PARAM_COUNT,
		      "at_params_valid_count_get returns wrong valid count");

	zassert_true(at_params_type_get(&test_list2, 0) == AT_PARAM_TYPE_STRING,
		     "Param type at index 0 should be a string");
	zassert_true(at_params_type_get(&test_list2, 1) ==
							AT_PARAM_TYPE_NUM_INT,
		     "Param type at index 1 should be a short");
	zassert_true(at_params_type_get(&test_list2, 2) == AT_PARAM_TYPE_EMPTY,
		     "Param type at index 2 should be empty");
	zassert_true(at_params_type_get(&test_list2, 3) == AT_PARAM_TYPE_EMPTY,
		     "Param type at index 3 should be empty");
	zassert_true(at_params_type_get(&test_list2, 4) == AT_PARAM_TYPE_STRING,
		     "Param type at index 4 should be a string");
	zassert_true(at_params_type_get(&test_list2, 5) == AT_PARAM_TYPE_STRING,
		     "Param type at index 5 should be a string");

	for (size_t i = 0; i < ARRAY_SIZE(multiline); i++) {
		/* Try to parse the string containing multiple notifications  */
		remainding = (char *)multiline[i];
		ret = at_parser_params_from_str(remainding, (char **)&remainding, &test_list2);
		zassert_true(ret == -EAGAIN, "at_parser_params_from_str should return 0");

		ret = at_params_valid_count_get(&test_list2);
		zassert_true(ret == 5, "at_params_valid_count_get returns wrong valid count");

		zassert_true(at_params_type_get(&test_list2, 0) == AT_PARAM_TYPE_STRING,
			     "Param type at index 0 should be a string");
		zassert_true(at_params_type_get(&test_list2, 1) == AT_PARAM_TYPE_NUM_INT,
			     "Param type at index 1 should be a short");
		zassert_true(at_params_type_get(&test_list2, 2) == AT_PARAM_TYPE_NUM_INT,
			     "Param type at index 2 should be a short");
		zassert_true(at_params_type_get(&test_list2, 3) == AT_PARAM_TYPE_EMPTY,
			     "Param type at index 3 should be empty");
		zassert_true(at_params_type_get(&test_list2, 4) == AT_PARAM_TYPE_EMPTY,
			     "Param type at index 4 should be empty");

		/* 2nd iteration */
		ret = at_parser_params_from_str(remainding, (char **)&remainding, &test_list2);
		zassert_true(ret == -EAGAIN, "at_parser_params_from_str should return 0");

		ret = at_params_valid_count_get(&test_list2);
		zassert_true(ret == 5, "at_params_valid_count_get returns wrong valid count");

		zassert_true(at_params_type_get(&test_list2, 0) == AT_PARAM_TYPE_STRING,
			     "Param type at index 0 should be a string");
		zassert_true(at_params_type_get(&test_list2, 1) == AT_PARAM_TYPE_NUM_INT,
			     "Param type at index 1 should be a short");
		zassert_true(at_params_type_get(&test_list2, 2) == AT_PARAM_TYPE_NUM_INT,
			     "Param type at index 2 should be a short");
		zassert_true(at_params_type_get(&test_list2, 3) == AT_PARAM_TYPE_EMPTY,
			     "Param type at index 3 should be empty");
		zassert_true(at_params_type_get(&test_list2, 4) == AT_PARAM_TYPE_EMPTY,
			     "Param type at index 4 should be empty");

		/* 3rd iteration */
		ret = at_parser_params_from_str(remainding, (char **)&remainding, &test_list2);
		zassert_true(ret == 0, "at_parser_params_from_str should return 0");

		ret = at_params_valid_count_get(&test_list2);
		zassert_true(ret == 7, "at_params_valid_count_get returns wrong valid count");

		zassert_true(at_params_type_get(&test_list2, 0) == AT_PARAM_TYPE_STRING,
			     "Param type at index 0 should be a string");
		zassert_true(at_params_type_get(&test_list2, 1) == AT_PARAM_TYPE_NUM_INT,
			     "Param type at index 1 should be a short");
		zassert_true(at_params_type_get(&test_list2, 2) == AT_PARAM_TYPE_NUM_INT,
			     "Param type at index 2 should be a short");
		zassert_true(at_params_type_get(&test_list2, 3) == AT_PARAM_TYPE_EMPTY,
			     "Param type at index 3 should be empty");
		zassert_true(at_params_type_get(&test_list2, 4) == AT_PARAM_TYPE_EMPTY,
			     "Param type at index 4 should be empty");
		zassert_true(at_params_type_get(&test_list2, 5) == AT_PARAM_TYPE_NUM_INT,
			     "Param type at index 5 should be a short");
		zassert_true(at_params_type_get(&test_list2, 6) == AT_PARAM_TYPE_NUM_INT,
			     "Param type at index 6 should be a integer");
	}

	/* Try to parse the string containing certificate data  */
	ret = at_parser_params_from_str(certificate, NULL, &test_list2);
	zassert_true(ret == 0, "at_parser_params_from_str should return 0");

	ret = at_params_valid_count_get(&test_list2);
	zassert_true(ret == CERTIFICATE_PARAM_COUNT,
		      "at_params_valid_count_get returns wrong valid count");

	zassert_true(at_params_type_get(&test_list2, 0) == AT_PARAM_TYPE_STRING,
		     "Param type at index 0 should be a string");
	zassert_true(at_params_type_get(&test_list2, 1) ==
							AT_PARAM_TYPE_NUM_INT,
		     "Param type at index 1 should be a integer");
	zassert_true(at_params_type_get(&test_list2, 2) ==
							AT_PARAM_TYPE_NUM_INT,
		     "Param type at index 2 should be a short");
	zassert_true(at_params_type_get(&test_list2, 3) == AT_PARAM_TYPE_STRING,
		     "Param type at index 3 should be a string");
	zassert_true(at_params_type_get(&test_list2, 4) == AT_PARAM_TYPE_STRING,
		     "Param type at index 4 should be a string");
}

ZTEST(at_cmd_parser, test_at_cmd_set)
{
	int ret;
	char tmpbuf[64];
	uint32_t tmpbuf_len;
	int16_t tmpshrt;
	uint16_t tmpushrt;

	static const char at_cmd_cgmi[] = "AT+CGMI";

	ret = at_parser_params_from_str(at_cmd_cgmi, NULL, &test_list2);
	zassert_true(ret == 0, "at_parser_params_from_str should return 0");
	zassert_equal(at_parser_cmd_type_get(at_cmd_cgmi),
		      AT_CMD_TYPE_SET_COMMAND, "Invalid AT command type");

	ret = at_params_valid_count_get(&test_list2);
	zassert_true(ret == 1,
		     "at_params_valid_count_get returns wrong valid count");

	zassert_true(at_params_type_get(&test_list2, 0) == AT_PARAM_TYPE_STRING,
		     "Param type at index 0 should be a string");

	tmpbuf_len = sizeof(tmpbuf);
	zassert_equal(0, at_params_string_get(&test_list2, 0,
					      tmpbuf, &tmpbuf_len),
		      "Get string should not fail");
	zassert_equal(0, memcmp("AT+CGMI", tmpbuf, tmpbuf_len),
		      "The string in tmpbuf should equal to AT+CGMI");

	static const char at_cmd_cclk[] = "AT+CCLK=\"18/12/06,22:10:00+08\"";

	ret = at_parser_params_from_str(at_cmd_cclk, NULL, &test_list2);
	zassert_true(ret == 0, "at_parser_params_from_str should return 0");
	zassert_equal(at_parser_cmd_type_get(at_cmd_cclk),
		      AT_CMD_TYPE_SET_COMMAND, "Invalid AT command type");

	ret = at_params_valid_count_get(&test_list2);
	zassert_true(ret == 2,
		     "at_params_valid_count_get returns wrong valid count");

	zassert_true(at_params_type_get(&test_list2, 0) == AT_PARAM_TYPE_STRING,
		     "Param type at index 0 should be a string");
	zassert_true(at_params_type_get(&test_list2, 1) == AT_PARAM_TYPE_STRING,
		     "Param type at index 1 should be a string");

	tmpbuf_len = sizeof(tmpbuf);
	zassert_equal(0, at_params_string_get(&test_list2, 0,
					      tmpbuf, &tmpbuf_len),
		      "Get string should not fail");
	zassert_equal(0, memcmp("AT+CCLK", tmpbuf, tmpbuf_len),
		      "The string in tmpbuf should equal to AT+CCLK");

	tmpbuf_len = sizeof(tmpbuf);
	zassert_equal(0, at_params_string_get(&test_list2, 1,
					      tmpbuf, &tmpbuf_len),
		      "Get string should not fail");
	zassert_equal(0, memcmp("18/12/06,22:10:00+08", tmpbuf, tmpbuf_len),
		      "The string in tmpbuf should equal to "
		      "18/12/06,22:10:00+08");

	static const char at_cmd_xsystemmode[] = "AT%XSYSTEMMODE=1,2,3,4";

	ret = at_parser_params_from_str(at_cmd_xsystemmode, NULL, &test_list2);
	zassert_true(ret == 0, "at_parser_params_from_str should return 0");
	zassert_equal(at_parser_cmd_type_get(at_cmd_xsystemmode),
		      AT_CMD_TYPE_SET_COMMAND, "Invalid AT command type");

	ret = at_params_valid_count_get(&test_list2);
	zassert_true(ret == 5,
		     "at_params_valid_count_get returns wrong valid count");

	zassert_true(at_params_type_get(&test_list2, 0) == AT_PARAM_TYPE_STRING,
		     "Param type at index 0 should be a string");
	zassert_true(at_params_type_get(&test_list2, 1) ==
							AT_PARAM_TYPE_NUM_INT,
		     "Param type at index 1 should be a string");
	zassert_true(at_params_type_get(&test_list2, 2) ==
							AT_PARAM_TYPE_NUM_INT,
		     "Param type at index 2 should be a string");
	zassert_true(at_params_type_get(&test_list2, 3) ==
							AT_PARAM_TYPE_NUM_INT,
		     "Param type at index 3 should be a string");
	zassert_true(at_params_type_get(&test_list2, 4) ==
							AT_PARAM_TYPE_NUM_INT,
		     "Param type at index 4 should be a string");

	zassert_equal(0, at_params_short_get(&test_list2, 1, &tmpshrt),
		      "Get short should not fail");
	zassert_equal(1, tmpshrt, "Short should be 1");
	zassert_equal(0, at_params_short_get(&test_list2, 2, &tmpshrt),
		      "Get short should not fail");
	zassert_equal(2, tmpshrt, "Short should be 2");
	zassert_equal(0, at_params_short_get(&test_list2, 3, &tmpshrt),
		      "Get short should not fail");
	zassert_equal(3, tmpshrt, "Short should be 3");
	zassert_equal(0, at_params_short_get(&test_list2, 4, &tmpshrt),
		      "Get short should not fail");
	zassert_equal(4, tmpshrt, "Short should be 4");

	zassert_equal(0, at_params_unsigned_short_get(&test_list2, 1, &tmpushrt),
		      "Get unsigned short should not fail");
	zassert_equal(1, tmpushrt, "Short should be 1");
	zassert_equal(0, at_params_unsigned_short_get(&test_list2, 2, &tmpushrt),
		      "Get unsigned short should not fail");
	zassert_equal(2, tmpushrt, "Short should be 2");
	zassert_equal(0, at_params_unsigned_short_get(&test_list2, 3, &tmpushrt),
		      "Get unsigned short should not fail");
	zassert_equal(3, tmpushrt, "Short should be 3");
	zassert_equal(0, at_params_unsigned_short_get(&test_list2, 4, &tmpushrt),
		      "Get unsigned short should not fail");
	zassert_equal(4, tmpushrt, "Short should be 4");

	static const char lone_at_cmd[] = "AT";

	ret = at_parser_params_from_str(lone_at_cmd, NULL, &test_list2);
	zassert_true(ret == 0, "at_parser_params_from_str should return 0");
	zassert_equal(at_parser_cmd_type_get(lone_at_cmd),
		      AT_CMD_TYPE_SET_COMMAND, "Invalid AT command type");

	ret = at_params_valid_count_get(&test_list2);
	zassert_true(ret == 1,
		     "at_params_valid_count_get returns wrong valid count");

	zassert_true(at_params_type_get(&test_list2, 0) == AT_PARAM_TYPE_STRING,
		     "Param type at index 0 should be a string");

	tmpbuf_len = sizeof(tmpbuf);
	zassert_equal(0, at_params_string_get(&test_list2, 0,
					      tmpbuf, &tmpbuf_len),
		      "Get string should not fail");
	zassert_equal(0, memcmp("AT", tmpbuf, tmpbuf_len),
		      "The string in tmpbuf should equal to AT");

	static const char at_cmd_clac[] = "AT+CLAC\r\n";

	ret = at_parser_params_from_str(at_cmd_clac, NULL, &test_list2);
	zassert_true(ret == 0, "at_parser_params_from_str should return 0");
	ret = at_params_valid_count_get(&test_list2);
	zassert_true(ret == 1,
		      "at_params_valid_count_get returns wrong valid count");
	zassert_true(at_params_type_get(&test_list2, 0) == AT_PARAM_TYPE_STRING,
		     "Param type at index 0 should be a string");
	tmpbuf_len = sizeof(tmpbuf);
	zassert_equal(0, at_params_string_get(&test_list2, 0, tmpbuf, &tmpbuf_len),
		      "Get string should not fail");
	zassert_equal(0, memcmp(at_cmd_clac, tmpbuf, tmpbuf_len),
		      "The string in tmpbuf should equal to cmd_str");

	static const char at_clac_rsp[] = "AT+CLAC\r\nAT+COPS\r\nAT%COPS\r\n";

	ret = at_parser_params_from_str(at_clac_rsp, NULL, &test_list2);
	zassert_true(ret == 0, "at_parser_params_from_str should return 0");

	ret = at_params_valid_count_get(&test_list2);
	zassert_true(ret == 1,
		      "at_params_valid_count_get returns wrong valid count");
	tmpbuf_len = sizeof(tmpbuf);
	zassert_equal(0, at_params_string_get(&test_list2, 0, tmpbuf, &tmpbuf_len),
		      "Get string should not fail");
	zassert_equal(0, memcmp(at_clac_rsp, tmpbuf, tmpbuf_len),
		      "The string in tmpbuf should equal to clac_str");
}

ZTEST(at_cmd_parser, test_at_cmd_read)
{
	int ret;
	char tmpbuf[32];
	uint32_t tmpbuf_len;

	static const char at_cmd_cfun_read[] = "AT+CFUN?";

	ret = at_parser_params_from_str(at_cmd_cfun_read, NULL, &test_list2);
	zassert_true(ret == 0, "at_parser_params_from_str should return 0");
	zassert_equal(at_parser_cmd_type_get(at_cmd_cfun_read),
		      AT_CMD_TYPE_READ_COMMAND, "Invalid AT command type");

	ret = at_params_valid_count_get(&test_list2);
	zassert_true(ret == 1,
		     "at_params_valid_count_get returns wrong valid count");

	zassert_true(at_params_type_get(&test_list2, 0) == AT_PARAM_TYPE_STRING,
		     "Param type at index 0 should be a string");

	tmpbuf_len = sizeof(tmpbuf);
	zassert_equal(0, at_params_string_get(&test_list2, 0,
					      tmpbuf, &tmpbuf_len),
		      "Get string should not fail");
	zassert_equal(0, memcmp("AT+CFUN", tmpbuf, tmpbuf_len),
		      "The string in tmpbuf should equal to AT+CFUN");
}

ZTEST(at_cmd_parser, test_at_cmd_test)
{
	int ret;
	char tmpbuf[32];
	uint32_t tmpbuf_len;

	static const char at_cmd_cfun_read[] = "AT+CFUN=?";

	ret = at_parser_params_from_str(at_cmd_cfun_read, NULL, &test_list2);
	zassert_true(ret == 0, "at_parser_params_from_str should return 0");
	zassert_equal(at_parser_cmd_type_get(at_cmd_cfun_read),
		      AT_CMD_TYPE_TEST_COMMAND, "Invalid AT command type");

	ret = at_params_valid_count_get(&test_list2);
	zassert_true(ret == 1,
		     "at_params_valid_count_get returns wrong valid count");

	zassert_true(at_params_type_get(&test_list2, 0) == AT_PARAM_TYPE_STRING,
		     "Param type at index 0 should be a string");

	tmpbuf_len = sizeof(tmpbuf);
	zassert_equal(0, at_params_string_get(&test_list2, 0,
					      tmpbuf, &tmpbuf_len),
		      "Get string should not fail");
	zassert_equal(0, memcmp("AT+CFUN", tmpbuf, tmpbuf_len),
		      "The string in tmpbuf should equal to AT+CFUN");
}

ZTEST_SUITE(at_cmd_parser, NULL, NULL, test_params_before, test_params_after, NULL);
