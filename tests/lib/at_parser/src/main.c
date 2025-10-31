/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include <modem/at_parser.h>

static const char * const singleline[] = {
	"+CEREG: 2,\"76C1\",\"0102DA04\", 7\r\n+CME ERROR: 10\r\n",
	"+CEREG: 2,\"76C1\",\"0102DA04\", 7\r\n+CMS ERROR: 11\r\n",
	"+CEREG: 2,\"76C1\",\"0102DA04\", 7\r\nOK\r\n",
	"+CEREG: 2,\"76C1\",\"0102DA04\", 7\rOK\r\n",
	"+CEREG: 2,\"76C1\",\"0102DA04\", 7\r\n",
	"+CEREG: 2,\"76C1\",\"0102DA04\", 7\r",
	"+CEREG: 2,\"76C1\",\"0102DA04\", 7\n",
	"+CEREG: 2,\"76C1\",\"0102DA04\", 7"
};

static const char * const multiline[] = {
	"+CGEQOSRDP: 0,0,,\r\n"
	"+CGEQOSRDP: 1,2,,\r\n"
	"+CGEQOSRDP: 2,4,,,1,65280000\r\n",
	"+CGEQOSRDP: 0,0,,\r\n"
	"+CGEQOSRDP: 1,2,,\r\n"
	"+CGEQOSRDP: 2,4,,,1,65280000\r\nOK\r\n",
	"+CGEQOSRDP: 0,0,,\r\n"
	"+CGEQOSRDP: 1,2,,\r\n"
	"+CGEQOSRDP: 2,4,,,1,65280000\r\nERROR\r\n",
	"+CGEQOSRDP: 0,0,,\r"
	"+CGEQOSRDP: 1,2,,\r"
	"+CGEQOSRDP: 2,4,,,1,65280000\rOK\r\n",
	"+CGEQOSRDP: 0,0,,\n"
	"+CGEQOSRDP: 1,2,,\n"
	"+CGEQOSRDP: 2,4,,,1,65280000\nOK\r\n",
	"\r\n+CGEQOSRDP: 0,0,,\r\n"
	"\r\n+CGEQOSRDP: 1,2,,\r\n"
	"\r\n+CGEQOSRDP: 2,4,,,1,65280000\r\n\r\nOK\r\n",
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
	"mfw_nrf9160_0.7.0-23.prealpha\r\n",
	"mfw_nrf9160_0.7.0-23.prealpha\r"
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

ZTEST(at_parser, test_at_parser_quoted_string)
{
	int ret;
	struct at_parser parser;
	char buffer[32];
	uint32_t buffer_len;
	int32_t num;

	const char *str1 = "+TEST:1,\"Hello World!\"\r\n";
	const char *str2 = "%TEST: 1, \"Hello World!\"\r\n";
	const char *str3 = "#TEST:1,\"Hello World!\"\r\n"
			   "+TEST: 2, \"FOOBAR\"\r\n";

	/* String without spaces between parameters (str1) */
	ret = at_parser_init(&parser, str1);
	zassert_ok(ret);

	buffer_len = sizeof(buffer);
	ret = at_parser_string_get(&parser, 0, buffer, &buffer_len);
	zassert_ok(ret);
	zassert_equal(strlen("+TEST"), buffer_len);
	zassert_mem_equal("+TEST", buffer, buffer_len);

	ret = at_parser_num_get(&parser, 1, &num);
	zassert_ok(ret);
	zassert_equal(num, 1);

	buffer_len = sizeof(buffer);
	ret = at_parser_string_get(&parser, 2, buffer, &buffer_len);
	zassert_ok(ret);
	zassert_equal(strlen("Hello World!"), buffer_len);
	zassert_mem_equal("Hello World!", buffer, buffer_len);

	/* String with spaces between parameters (str2) */
	ret = at_parser_init(&parser, str2);
	zassert_ok(ret);

	buffer_len = sizeof(buffer);
	ret = at_parser_string_get(&parser, 0, buffer, &buffer_len);
	zassert_ok(ret);
	zassert_equal(strlen("%TEST"), buffer_len);
	zassert_mem_equal("%TEST", buffer, buffer_len);

	ret = at_parser_num_get(&parser, 1, &num);
	zassert_ok(ret);
	zassert_equal(num, 1);

	buffer_len = sizeof(buffer);
	ret = at_parser_string_get(&parser, 2, buffer, &buffer_len);
	zassert_ok(ret);
	zassert_equal(strlen("Hello World!"), buffer_len);
	zassert_mem_equal("Hello World!", buffer, buffer_len);

	/* String with multiple notifications (str3) */
	ret = at_parser_init(&parser, str3);
	zassert_ok(ret);

	buffer_len = sizeof(buffer);
	ret = at_parser_string_get(&parser, 0, buffer, &buffer_len);
	zassert_ok(ret);
	zassert_equal(strlen("#TEST"), buffer_len);
	zassert_mem_equal("#TEST", buffer, buffer_len);

	ret = at_parser_num_get(&parser, 1, &num);
	zassert_ok(ret);
	zassert_equal(num, 1);

	buffer_len = sizeof(buffer);
	ret = at_parser_string_get(&parser, 2, buffer, &buffer_len);
	zassert_ok(ret);
	zassert_equal(strlen("Hello World!"), buffer_len);
	zassert_mem_equal("Hello World!", buffer, buffer_len);

	/* Second line. */
	ret = at_parser_cmd_next(&parser);
	zassert_ok(ret);

	buffer_len = sizeof(buffer);
	ret = at_parser_string_get(&parser, 0, buffer, &buffer_len);
	zassert_ok(ret);
	zassert_equal(strlen("+TEST"), buffer_len);
	zassert_mem_equal("+TEST", buffer, buffer_len);

	ret = at_parser_num_get(&parser, 1, &num);
	zassert_ok(ret);
	zassert_equal(num, 2);

	buffer_len = sizeof(buffer);
	ret = at_parser_string_get(&parser, 2, buffer, &buffer_len);
	zassert_ok(ret);
	zassert_equal(strlen("FOOBAR"), buffer_len);
	zassert_mem_equal("FOOBAR", buffer, buffer_len);
}

ZTEST(at_parser, test_unquoted_string)
{
	int ret;
	struct at_parser parser;
	char buffer[32];
	uint32_t buffer_len;
	int32_t num;

	const char *str1 = "+TEST:1,AB\r\n";
	const char *str2 = "%TEST:1,C\r\n";
	const char *str3 = "+TEST:1,DE,2\r\n";
	const char *str4 = "#TEST:1,F,2\r\n";

	/* Unquoted string at the end of the parameters (str1) */
	ret = at_parser_init(&parser, str1);
	zassert_ok(ret);

	buffer_len = sizeof(buffer);
	ret = at_parser_string_get(&parser, 0, buffer, &buffer_len);
	zassert_ok(ret);
	zassert_equal(strlen("+TEST"), buffer_len);
	zassert_mem_equal("+TEST", buffer, buffer_len);

	ret = at_parser_num_get(&parser, 1, &num);
	zassert_ok(ret);
	zassert_equal(num, 1);

	buffer_len = sizeof(buffer);
	ret = at_parser_string_get(&parser, 2, buffer, &buffer_len);
	zassert_ok(ret);
	zassert_equal(strlen("AB"), buffer_len);
	zassert_mem_equal("AB", buffer, buffer_len);

	/* Unquoted single character string at the end of the parameters (str2) */
	ret = at_parser_init(&parser, str2);
	zassert_ok(ret);

	buffer_len = sizeof(buffer);
	ret = at_parser_string_get(&parser, 0, buffer, &buffer_len);
	zassert_ok(ret);
	zassert_equal(strlen("%TEST"), buffer_len);
	zassert_mem_equal("%TEST", buffer, buffer_len);

	ret = at_parser_num_get(&parser, 1, &num);
	zassert_ok(ret);
	zassert_equal(num, 1);

	buffer_len = sizeof(buffer);
	ret = at_parser_string_get(&parser, 2, buffer, &buffer_len);
	zassert_ok(ret);
	zassert_equal(strlen("C"), buffer_len);
	zassert_mem_equal("C", buffer, buffer_len);

	/* Unquoted string between parameters (str3) */
	ret = at_parser_init(&parser, str3);
	zassert_ok(ret);

	buffer_len = sizeof(buffer);
	ret = at_parser_string_get(&parser, 0, buffer, &buffer_len);
	zassert_ok(ret);
	zassert_equal(strlen("+TEST"), buffer_len);
	zassert_mem_equal("+TEST", buffer, buffer_len);

	ret = at_parser_num_get(&parser, 1, &num);
	zassert_ok(ret);
	zassert_equal(num, 1);

	buffer_len = sizeof(buffer);
	ret = at_parser_string_get(&parser, 2, buffer, &buffer_len);
	zassert_ok(ret);
	zassert_equal(strlen("DE"), buffer_len);
	zassert_mem_equal("DE", buffer, buffer_len);

	ret = at_parser_num_get(&parser, 3, &num);
	zassert_ok(ret);
	zassert_equal(num, 2);

	/* Unquoted single character string between parameters (str4) */
	ret = at_parser_init(&parser, str4);
	zassert_ok(ret);

	buffer_len = sizeof(buffer);
	ret = at_parser_string_get(&parser, 0, buffer, &buffer_len);
	zassert_ok(ret);
	zassert_equal(strlen("#TEST"), buffer_len);
	zassert_mem_equal("#TEST", buffer, buffer_len);

	ret = at_parser_num_get(&parser, 1, &num);
	zassert_ok(ret);
	zassert_equal(num, 1);

	buffer_len = sizeof(buffer);
	ret = at_parser_string_get(&parser, 2, buffer, &buffer_len);
	zassert_ok(ret);
	zassert_equal(strlen("F"), buffer_len);
	zassert_mem_equal("F", buffer, buffer_len);

	ret = at_parser_num_get(&parser, 3, &num);
	zassert_ok(ret);
	zassert_equal(num, 2);
}

ZTEST(at_parser, test_at_parser_empty)
{
	int ret;
	struct at_parser parser;
	char buffer[32];
	uint32_t buffer_len;
	int32_t num;
	size_t count;

	const char *str1 = "+TEST: 1,\r\n";
	const char *str2 = "+TEST: ,1\r\n";
	const char *str3 = "+TEST: 1,,\"Hello World!\"";
	const char *str4 = "+TEST: ,,,1\r\n";

	/* Empty parameter at the end of the parameter list */
	ret = at_parser_init(&parser, str1);
	zassert_ok(ret);

	ret = at_parser_cmd_count_get(&parser, &count);
	zassert_ok(ret);
	zassert_equal(count, 3);

	buffer_len = sizeof(buffer);
	ret = at_parser_string_get(&parser, 0, buffer, &buffer_len);
	zassert_ok(ret);
	zassert_equal(strlen("+TEST"), buffer_len);
	zassert_mem_equal("+TEST", buffer, buffer_len);

	ret = at_parser_num_get(&parser, 1, &num);
	zassert_ok(ret);
	zassert_equal(num, 1);

	/* Empty parameter at the beginning of the parameter list */
	ret = at_parser_init(&parser, str2);
	zassert_ok(ret);

	ret = at_parser_cmd_count_get(&parser, &count);
	zassert_ok(ret);
	zassert_equal(count, 3);

	buffer_len = sizeof(buffer);
	ret = at_parser_string_get(&parser, 0, buffer, &buffer_len);
	zassert_ok(ret);
	zassert_equal(strlen("+TEST"), buffer_len);
	zassert_mem_equal("+TEST", buffer, buffer_len);

	ret = at_parser_num_get(&parser, 2, &num);
	zassert_ok(ret);
	zassert_equal(num, 1);

	/* Empty parameter between two other parameter types */
	ret = at_parser_init(&parser, str3);
	zassert_ok(ret);

	ret = at_parser_cmd_count_get(&parser, &count);
	zassert_ok(ret);
	zassert_equal(count, 4);

	buffer_len = sizeof(buffer);
	ret = at_parser_string_get(&parser, 0, buffer, &buffer_len);
	zassert_ok(ret);
	zassert_equal(strlen("+TEST"), buffer_len);
	zassert_mem_equal("+TEST", buffer, buffer_len);

	ret = at_parser_num_get(&parser, 1, &num);
	zassert_ok(ret);
	zassert_equal(num, 1);

	buffer_len = sizeof(buffer);
	ret = at_parser_string_get(&parser, 3, buffer, &buffer_len);
	zassert_ok(ret);
	zassert_equal(strlen("Hello World!"), buffer_len);
	zassert_mem_equal("Hello World!", buffer, buffer_len);

	/* 3 empty parameters at the beginning of the parameter list */
	ret = at_parser_init(&parser, str4);
	zassert_ok(ret);

	ret = at_parser_cmd_count_get(&parser, &count);
	zassert_ok(ret);
	zassert_equal(count, 5);

	buffer_len = sizeof(buffer);
	ret = at_parser_string_get(&parser, 0, buffer, &buffer_len);
	zassert_ok(ret);
	zassert_equal(strlen("+TEST"), buffer_len);
	zassert_mem_equal("+TEST", buffer, buffer_len);

	ret = at_parser_num_get(&parser, 4, &num);
	zassert_ok(ret);
	zassert_equal(num, 1);
}

ZTEST(at_parser, test_at_parser_testcases)
{
	int ret;
	struct at_parser parser;
	char buffer[128] = { 0 };
	int len = sizeof(buffer);
	int32_t num = 0;
	size_t valid_count = 0;

	/* Try to parse the singleline string */
	for (size_t i = 0; i < ARRAY_SIZE(singleline); i++) {
		ret = at_parser_init(&parser, singleline[i]);
		zassert_ok(ret);

		ret = at_parser_cmd_count_get(&parser, &valid_count);
		zassert_ok(ret);
		zassert_equal(valid_count, 5);

		len = sizeof(buffer);
		ret = at_parser_string_get(&parser, 0, buffer, &len);
		zassert_ok(ret);
		zassert_mem_equal("+CEREG", buffer, len);

		ret = at_parser_num_get(&parser, 1, &num);
		zassert_ok(ret);
		zassert_equal(num, 2);

		len = sizeof(buffer);
		ret = at_parser_string_get(&parser, 2, buffer, &len);
		zassert_ok(ret);
		zassert_mem_equal("76C1", buffer, len);

		len = sizeof(buffer);
		ret = at_parser_string_get(&parser, 3, buffer, &len);
		zassert_ok(ret);
		zassert_mem_equal("0102DA04", buffer, len);

		ret = at_parser_num_get(&parser, 4, &num);
		zassert_ok(ret);
		zassert_equal(num, 7);

		/* Try to parse beyond the last subparameter. */
		ret = at_parser_num_get(&parser, 5, &num);
		zassert_equal(ret, -EIO);
	}

	/* Try to parse the multiline string */
	for (size_t i = 0; i < ARRAY_SIZE(multiline); i++) {
		ret = at_parser_init(&parser, multiline[i]);
		zassert_ok(ret);

		ret = at_parser_cmd_count_get(&parser, &valid_count);
		zassert_ok(ret);
		zassert_equal(valid_count, 5);

		len = sizeof(buffer);
		ret = at_parser_string_get(&parser, 0, buffer, &len);
		zassert_ok(ret);
		zassert_mem_equal("+CGEQOSRDP", buffer, len);

		ret = at_parser_num_get(&parser, 1, &num);
		zassert_ok(ret);
		zassert_equal(num, 0);

		ret = at_parser_num_get(&parser, 2, &num);
		zassert_ok(ret);
		zassert_equal(num, 0);

		/* Try to parse beyond the last subparameter. */
		ret = at_parser_num_get(&parser, 5, &num);
		zassert_equal(ret, -EAGAIN);

		/* Second line. */
		ret = at_parser_cmd_next(&parser);
		zassert_ok(ret);

		ret = at_parser_cmd_count_get(&parser, &valid_count);
		zassert_ok(ret);
		zassert_equal(valid_count, 5);

		len = sizeof(buffer);
		ret = at_parser_string_get(&parser, 0, buffer, &len);
		zassert_ok(ret);
		zassert_mem_equal("+CGEQOSRDP", buffer, len);

		ret = at_parser_num_get(&parser, 1, &num);
		zassert_ok(ret);
		zassert_equal(num, 1);

		ret = at_parser_num_get(&parser, 2, &num);
		zassert_ok(ret);
		zassert_equal(num, 2);

		/* Try to parse beyond the last subparameter. */
		ret = at_parser_num_get(&parser, 5, &num);
		zassert_equal(ret, -EAGAIN);

		/* Third line. */
		ret = at_parser_cmd_next(&parser);
		zassert_ok(ret);

		ret = at_parser_cmd_count_get(&parser, &valid_count);
		zassert_ok(ret);
		zassert_equal(valid_count, 7);

		len = sizeof(buffer);
		ret = at_parser_string_get(&parser, 0, buffer, &len);
		zassert_ok(ret);
		zassert_mem_equal("+CGEQOSRDP", buffer, len);

		ret = at_parser_num_get(&parser, 1, &num);
		zassert_ok(ret);
		zassert_equal(num, 2);

		ret = at_parser_num_get(&parser, 2, &num);
		zassert_ok(ret);
		zassert_equal(num, 4);

		ret = at_parser_num_get(&parser, 5, &num);
		zassert_ok(ret);
		zassert_equal(num, 1);

		ret = at_parser_num_get(&parser, 6, &num);
		zassert_ok(ret);
		zassert_equal(num, 65280000);

		/* Try to parse beyond the last subparameter. */
		ret = at_parser_num_get(&parser, 7, &num);
		zassert_equal(ret, -EIO);
	}

	/* Try to parse the pduline string */
	for (size_t i = 0; i < ARRAY_SIZE(pduline); i++) {
		ret = at_parser_init(&parser, pduline[i]);
		zassert_ok(ret);

		len = sizeof(buffer);
		ret = at_parser_string_get(&parser, 0, buffer, &len);
		zassert_ok(ret);
		zassert_mem_equal("+CMT", buffer, len);

		len = sizeof(buffer);
		ret = at_parser_string_get(&parser, 1, buffer, &len);
		zassert_ok(ret);
		zassert_mem_equal("12345678", buffer, len);

		ret = at_parser_num_get(&parser, 2, &num);
		zassert_ok(ret);
		zassert_equal(num, 24);

		/* Second line. */
		ret = at_parser_cmd_next(&parser);
		zassert_ok(ret);

		len = sizeof(buffer);
		ret = at_parser_string_get(&parser, 0, buffer, &len);
		zassert_ok(ret);
		zassert_mem_equal("06917429000171040A91747966543100009160402143708006C8329BFD0601",
				  buffer, len);

		/* Try to parse beyond the last subparameter. */
		ret = at_parser_num_get(&parser, 1, &num);
		zassert_equal(ret, -EIO);
	}

	/* Try to parse the singleparamline string */
	for (size_t i = 0; i < ARRAY_SIZE(singleparamline); i++) {
		ret = at_parser_init(&parser, singleparamline[i]);
		zassert_ok(ret);

		len = sizeof(buffer);
		ret = at_parser_string_get(&parser, 0, buffer, &len);
		zassert_ok(ret);
		zassert_mem_equal("mfw_nrf9160_0.7.0-23.prealpha", buffer, len);

		/* Try to parse beyond the last subparameter. */
		ret = at_parser_num_get(&parser, 1, &num);
		zassert_equal(ret, -EIO);
	}

	/* Try to parse the emptyparamline string */
	for (size_t i = 0; i < ARRAY_SIZE(emptyparamline); i++) {
		ret = at_parser_init(&parser, emptyparamline[i]);
		zassert_ok(ret);

		len = sizeof(buffer);
		ret = at_parser_string_get(&parser, 0, buffer, &len);
		zassert_ok(ret);
		zassert_mem_equal("+CPSMS", buffer, len);

		ret = at_parser_num_get(&parser, 1, &num);
		zassert_ok(ret);
		zassert_equal(num, 1);

		len = sizeof(buffer);
		ret = at_parser_string_get(&parser, 4, buffer, &len);
		zassert_ok(ret);
		zassert_mem_equal("10101111", buffer, len);

		len = sizeof(buffer);
		ret = at_parser_string_get(&parser, 5, buffer, &len);
		zassert_ok(ret);
		zassert_mem_equal("01101100", buffer, len);

		/* Try to parse beyond the last subparameter. */
		ret = at_parser_num_get(&parser, 6, &num);
		zassert_equal(ret, -EIO);
	}

	/* Try to parse the certificate string */
	ret = at_parser_init(&parser, certificate);
	zassert_ok(ret);

	len = sizeof(buffer);
	ret = at_parser_string_get(&parser, 0, buffer, &len);
	zassert_ok(ret);
	zassert_mem_equal("%CMNG", buffer, len);

	ret = at_parser_num_get(&parser, 1, &num);
	zassert_ok(ret);
	zassert_equal(num, 12345678);

	ret = at_parser_num_get(&parser, 2, &num);
	zassert_ok(ret);
	zassert_equal(num, 0);

	len = sizeof(buffer);
	ret = at_parser_string_get(&parser, 3, buffer, &len);
	zassert_ok(ret);
	zassert_mem_equal("978C...02C4", buffer, len);

	len = sizeof(buffer);
	ret = at_parser_string_get(&parser, 4, buffer, &len);
	zassert_ok(ret);
	zassert_mem_equal("-----BEGIN CERTIFICATE-----"
			  "MIIBc464..."
			  "...bW9aAa4"
			  "-----END CERTIFICATE-----", buffer, len, "%s", buffer);
}

ZTEST(at_parser, test_at_cmd_type_get_cmd_set)
{
	int ret;
	struct at_parser parser;
	enum at_parser_cmd_type type;
	char buffer[64];
	uint32_t buffer_len;
	int16_t tmpshrt;

	/* CGMI */
	static const char at_cmd_cgmi[] = "AT+CGMI";

	ret = at_parser_init(&parser, at_cmd_cgmi);
	zassert_ok(ret);

	ret = at_parser_cmd_type_get(&parser, &type);
	zassert_ok(ret);
	zassert_equal(type, AT_PARSER_CMD_TYPE_SET);

	buffer_len = sizeof(buffer);
	ret = at_parser_string_get(&parser, 0, buffer, &buffer_len);
	zassert_ok(ret);
	zassert_mem_equal("AT+CGMI", buffer, buffer_len);

	/* CCLK */
	static const char at_cmd_cclk[] = "AT+CCLK=\"18/12/06,22:10:00+08\"";

	ret = at_parser_init(&parser, at_cmd_cclk);
	zassert_ok(ret);

	ret = at_parser_cmd_type_get(&parser, &type);
	zassert_ok(ret);
	zassert_equal(type, AT_PARSER_CMD_TYPE_SET);

	buffer_len = sizeof(buffer);
	ret = at_parser_string_get(&parser, 0, buffer, &buffer_len);
	zassert_ok(ret);
	zassert_mem_equal("AT+CCLK", buffer, buffer_len);

	buffer_len = sizeof(buffer);
	ret = at_parser_string_get(&parser, 1, buffer, &buffer_len);
	zassert_ok(ret);
	zassert_mem_equal("18/12/06,22:10:00+08", buffer, buffer_len);

	/* XSYSTEMMODE */
	static const char at_cmd_xsystemmode[] = "AT%XSYSTEMMODE=1,2,3,4";

	ret = at_parser_init(&parser, at_cmd_xsystemmode);
	zassert_ok(ret);

	buffer_len = sizeof(buffer);
	ret = at_parser_string_get(&parser, 0, buffer, &buffer_len);
	zassert_ok(ret);
	zassert_mem_equal("AT%XSYSTEMMODE", buffer, buffer_len);

	ret = at_parser_num_get(&parser, 1, &tmpshrt);
	zassert_ok(ret);
	zassert_equal(tmpshrt, 1);

	ret = at_parser_num_get(&parser, 2, &tmpshrt);
	zassert_ok(ret);
	zassert_equal(tmpshrt, 2);

	ret = at_parser_num_get(&parser, 3, &tmpshrt);
	zassert_ok(ret);
	zassert_equal(tmpshrt, 3);

	ret = at_parser_num_get(&parser, 4, &tmpshrt);
	zassert_ok(ret);
	zassert_equal(tmpshrt, 4);

	ret = at_parser_cmd_type_get(&parser, &type);
	zassert_ok(ret);
	zassert_equal(type, AT_PARSER_CMD_TYPE_SET);

	/* AT */
	static const char lone_at_cmd[] = "AT";

	ret = at_parser_init(&parser, lone_at_cmd);
	zassert_ok(ret);

	buffer_len = sizeof(buffer);
	ret = at_parser_string_get(&parser, 0, buffer, &buffer_len);
	zassert_ok(ret);
	zassert_mem_equal("AT", buffer, buffer_len);

	ret = at_parser_cmd_type_get(&parser, &type);
	zassert_ok(ret);
	zassert_equal(type, AT_PARSER_CMD_TYPE_SET);

	/* CLAC */
	static const char at_cmd_clac[] = "AT+CLAC\r\n";

	ret = at_parser_init(&parser, at_cmd_clac);
	zassert_ok(ret);

	buffer_len = sizeof(buffer);
	ret = at_parser_string_get(&parser, 0, buffer, &buffer_len);
	zassert_ok(ret);
	zassert_mem_equal("AT+CLAC", buffer, buffer_len);

	ret = at_parser_cmd_type_get(&parser, &type);
	zassert_ok(ret);
	zassert_equal(type, AT_PARSER_CMD_TYPE_SET);

	/* CLAC RSP */
	static const char at_clac_rsp[] = "AT+CLAC\r\nAT+COPS\r\nAT%COPS\r\n";

	ret = at_parser_init(&parser, at_clac_rsp);
	zassert_ok(ret);

	buffer_len = sizeof(buffer);
	ret = at_parser_string_get(&parser, 0, buffer, &buffer_len);
	zassert_ok(ret);
	zassert_mem_equal("AT+CLAC", buffer, buffer_len);

	ret = at_parser_cmd_next(&parser);
	zassert_ok(ret);

	buffer_len = sizeof(buffer);
	ret = at_parser_string_get(&parser, 0, buffer, &buffer_len);
	zassert_ok(ret);
	zassert_mem_equal("AT+COPS", buffer, buffer_len);

	ret = at_parser_cmd_next(&parser);
	zassert_ok(ret);

	buffer_len = sizeof(buffer);
	ret = at_parser_string_get(&parser, 0, buffer, &buffer_len);
	zassert_ok(ret);
	zassert_mem_equal("AT%COPS", buffer, buffer_len);
}

ZTEST(at_parser, test_at_cmd_type_get_cmd_read)
{
	int ret;
	char buffer[64];
	uint32_t buffer_len;
	struct at_parser parser;
	enum at_parser_cmd_type type;

	static const char at_cmd_cfun_read[] = "AT+CFUN?";

	ret = at_parser_init(&parser, at_cmd_cfun_read);
	zassert_ok(ret);

	ret = at_parser_cmd_type_get(&parser, &type);
	zassert_ok(ret);
	zassert_equal(type, AT_PARSER_CMD_TYPE_READ);

	buffer_len = sizeof(buffer);
	ret = at_parser_string_get(&parser, 0, buffer, &buffer_len);
	zassert_ok(ret);
	zassert_mem_equal("AT+CFUN", buffer, buffer_len);
}

ZTEST(at_parser, test_at_cmd_type_get_cmd_test)
{
	int ret;
	char buffer[64];
	uint32_t buffer_len;
	struct at_parser parser;
	enum at_parser_cmd_type type;

	static const char at_cmd_cfun_test[] = "AT+CFUN=?";

	ret = at_parser_init(&parser, at_cmd_cfun_test);
	zassert_ok(ret);

	ret = at_parser_cmd_type_get(&parser, &type);
	zassert_ok(ret);
	zassert_equal(type, AT_PARSER_CMD_TYPE_TEST);

	buffer_len = sizeof(buffer);
	ret = at_parser_string_get(&parser, 0, buffer, &buffer_len);
	zassert_ok(ret);
	zassert_mem_equal("AT+CFUN", buffer, buffer_len);
}

ZTEST(at_parser, test_at_parser_init_einval)
{
	int ret;
	struct at_parser parser;

	ret = at_parser_init(NULL, NULL);
	zassert_equal(ret, -EINVAL);

	ret = at_parser_init(&parser, NULL);
	zassert_equal(ret, -EINVAL);

	ret = at_parser_init(NULL, "AT+CFUN?");
	zassert_equal(ret, -EINVAL);
}

ZTEST(at_parser, test_at_parser_init)
{
	int ret;
	struct at_parser parser;

	const char *str1 = "+NOTIF: 1,2,3\r\nOK\r\n";

	ret = at_parser_init(&parser, str1);
	zassert_ok(ret);
}

ZTEST(at_parser, test_at_parser_cmd_next_einval)
{
	int ret;

	ret = at_parser_cmd_next(NULL);
	zassert_equal(ret, -EINVAL);
}

ZTEST(at_parser, test_at_parser_cmd_next_eperm)
{
	int ret;
	struct at_parser parser;

	ret = at_parser_cmd_next(&parser);
	zassert_equal(ret, -EPERM);
}

ZTEST(at_parser, test_at_parser_cmd_next_eopnotsupp)
{
	int ret;
	struct at_parser parser;

	const char *str1 = "+NOTIF: 1,2,3\r\n"
			   "OK\r\n";

	ret = at_parser_init(&parser, str1);
	zassert_ok(ret);

	/* There is no next line to move to. */
	ret = at_parser_cmd_next(&parser);
	zassert_equal(ret, -EOPNOTSUPP);
}

ZTEST(at_parser, test_at_parser_uint16_get_eperm)
{
	int ret;
	uint16_t val;
	struct at_parser parser;

	ret = at_parser_uint16_get(&parser, 1, &val);
	zassert_equal(ret, -EPERM);
}

ZTEST(at_parser, test_at_parser_uint16_get_einval)
{
	int ret;
	uint16_t val;
	struct at_parser parser;

	const char *str1 = "+NOTIF: 1,-2,3\r\nOK\r\n";

	ret = at_parser_init(&parser, str1);
	zassert_ok(ret);

	ret = at_parser_uint16_get(&parser, 1, &val);
	zassert_ok(ret);
	zassert_equal(val, 1);

	ret = at_parser_uint16_get(NULL, 1, NULL);
	zassert_equal(ret, -EINVAL);

	ret = at_parser_uint16_get(&parser, 1, NULL);
	zassert_equal(ret, -EINVAL);

	ret = at_parser_uint16_get(NULL, 1, &val);
	zassert_equal(ret, -EINVAL);
}

ZTEST(at_parser, test_at_parser_uint16_get_erange)
{
	int ret;
	uint16_t val;
	struct at_parser parser;

	char str1[64] = { 0 };

	snprintf(str1, sizeof(str1), "+NOTIF: %d,-2,3\r\nOK\r\n", UINT16_MAX + 1);

	ret = at_parser_init(&parser, str1);
	zassert_ok(ret);

	/* Greater than maximum of unsigned 16-bit integer. */
	ret = at_parser_uint16_get(&parser, 1, &val);
	zassert_equal(ret, -ERANGE);

	/* Negative number. */
	ret = at_parser_uint16_get(&parser, 2, &val);
	zassert_equal(ret, -ERANGE);
}

ZTEST(at_parser, test_at_parser_uint16_get_eopnotsupp)
{
	int ret;
	uint16_t val;
	struct at_parser parser;

	const char *str1 = "+NOTIF: 1,2";

	ret = at_parser_init(&parser, str1);
	zassert_ok(ret);

	/* Trying to parse notification string as integer. */
	ret = at_parser_uint16_get(&parser, 0, &val);
	zassert_equal(ret, -EOPNOTSUPP);
}

ZTEST(at_parser, test_at_parser_uint16_get_enodata)
{
	int ret;
	uint16_t val;
	struct at_parser parser;

	const char *str1 = "+NOTIF: 1,,3";

	ret = at_parser_init(&parser, str1);
	zassert_ok(ret);

	/* Trying to parse an empty subparameter as integer. */
	ret = at_parser_uint16_get(&parser, 2, &val);
	zassert_equal(ret, -ENODATA);
}

ZTEST(at_parser, test_at_parser_uint16_get_ebadmsg)
{
	int ret;
	uint16_t val;
	struct at_parser parser;

	/* 1) Space is not a valid separator between subparameters. */
	const char *str1 = "+NOTIF: 1,2,3 4,5,6 7,8,9";

	ret = at_parser_init(&parser, str1);
	zassert_ok(ret);

	ret = at_parser_uint16_get(&parser, 4, &val);
	zassert_equal(ret, -EBADMSG);

	/* 2) String is not a valid AT command. */
	const char *str2 = "\"abba\",1,2";

	ret = at_parser_init(&parser, str2);
	zassert_ok(ret);

	ret = at_parser_uint16_get(&parser, 1, &val);
	zassert_equal(ret, -EBADMSG);
}

ZTEST(at_parser, test_at_parser_uint16_get)
{
	int ret;
	uint16_t val;
	struct at_parser parser;

	const char *str1 = "+NOTIF: 1,-2,3\r\nOK\r\n";

	ret = at_parser_init(&parser, str1);
	zassert_ok(ret);

	ret = at_parser_uint16_get(&parser, 1, &val);
	zassert_ok(ret);
	zassert_equal(val, 1);

	char str2[64] = { 0 };

	snprintf(str2, sizeof(str2), "+NOTIF: %d,-2,3\r\nOK\r\n", UINT16_MAX);

	ret = at_parser_init(&parser, str2);
	zassert_ok(ret);

	ret = at_parser_uint16_get(&parser, 1, &val);
	zassert_ok(ret);
	zassert_equal(val, UINT16_MAX, "%d", val);
}

ZTEST(at_parser, test_at_parser_int16_get_eperm)
{
	int ret;
	int16_t val;
	struct at_parser parser;

	ret = at_parser_int16_get(&parser, 1, &val);
	zassert_equal(ret, -EPERM);
}

ZTEST(at_parser, test_at_parser_int16_get_einval)
{
	int ret;
	int16_t val;
	struct at_parser parser;

	const char *str1 = "+NOTIF: 1,-2,3\r\nOK\r\n";

	ret = at_parser_init(&parser, str1);
	zassert_ok(ret);

	ret = at_parser_int16_get(&parser, 1, &val);
	zassert_ok(ret);
	zassert_equal(val, 1);

	ret = at_parser_int16_get(NULL, 1, NULL);
	zassert_equal(ret, -EINVAL);

	ret = at_parser_int16_get(&parser, 1, NULL);
	zassert_equal(ret, -EINVAL);

	ret = at_parser_int16_get(NULL, 1, &val);
	zassert_equal(ret, -EINVAL);
}

ZTEST(at_parser, test_at_parser_int16_get_erange)
{
	int ret;
	int16_t val;
	struct at_parser parser;

	/* 1) Larger than allowed by `int16_t`. */
	char str1[64] = { 0 };

	snprintf(str1, sizeof(str1), "+NOTIF: %d,-2,3\r\nOK\r\n", INT16_MAX + 1);

	ret = at_parser_init(&parser, str1);
	zassert_ok(ret);

	ret = at_parser_int16_get(&parser, 1, &val);
	zassert_equal(ret, -ERANGE);

	/* 2) Smaller than allowed by `int16_t`. */
	char str2[64] = { 0 };

	snprintf(str2, sizeof(str2), "+NOTIF: %d,-2,3\r\nOK\r\n", INT16_MIN - 1);

	ret = at_parser_init(&parser, str2);
	zassert_ok(ret);

	ret = at_parser_int16_get(&parser, 1, &val);
	zassert_equal(ret, -ERANGE);
}

ZTEST(at_parser, test_at_parser_int16_get_eopnotsupp)
{
	int ret;
	int16_t val;
	struct at_parser parser;

	const char *str1 = "+NOTIF: 1,2";

	ret = at_parser_init(&parser, str1);
	zassert_ok(ret);

	/* Trying to parse notification string as integer. */
	ret = at_parser_int16_get(&parser, 0, &val);
	zassert_equal(ret, -EOPNOTSUPP);
}

ZTEST(at_parser, test_at_parser_int16_get_enodata)
{
	int ret;
	int16_t val;
	struct at_parser parser;

	const char *str1 = "+NOTIF: 1,,3";

	ret = at_parser_init(&parser, str1);
	zassert_ok(ret);

	/* Trying to parse an empty subparameter as integer. */
	ret = at_parser_int16_get(&parser, 2, &val);
	zassert_equal(ret, -ENODATA);
}

ZTEST(at_parser, test_at_parser_int16_get_ebadmsg)
{
	int ret;
	int16_t val;
	struct at_parser parser;

	/* 1) Space is not a valid separator between subparameters. */
	const char *str1 = "+NOTIF: 1,2,3 -4,5,6 7,8,9";

	ret = at_parser_init(&parser, str1);
	zassert_ok(ret);

	ret = at_parser_int16_get(&parser, 4, &val);
	zassert_equal(ret, -EBADMSG);

	/* 2) String is not a valid AT command. */
	const char *str2 = "\"abba\",-1,2";

	ret = at_parser_init(&parser, str2);
	zassert_ok(ret);

	ret = at_parser_int16_get(&parser, 1, &val);
	zassert_equal(ret, -EBADMSG);
}

ZTEST(at_parser, test_at_parser_int16_get)
{
	int ret;
	int16_t val;
	struct at_parser parser;

	/* 1) */
	const char *str1 = "+NOTIF: 1,-2,3\r\nOK\r\n";

	ret = at_parser_init(&parser, str1);
	zassert_ok(ret);

	ret = at_parser_int16_get(&parser, 1, &val);
	zassert_ok(ret);
	zassert_equal(val, 1);

	/* 2) */
	char str2[64] = { 0 };

	snprintf(str2, sizeof(str2), "+NOTIF: %d,-2,3\r\nOK\r\n", INT16_MAX);

	ret = at_parser_init(&parser, str2);
	zassert_ok(ret);

	ret = at_parser_int16_get(&parser, 1, &val);
	zassert_ok(ret);
	zassert_equal(val, INT16_MAX);

	/* 3) */
	char str3[64] = { 0 };

	snprintf(str3, sizeof(str3), "+NOTIF: %d,-2,3\r\nOK\r\n", INT16_MIN);

	ret = at_parser_init(&parser, str3);
	zassert_ok(ret);

	ret = at_parser_int16_get(&parser, 1, &val);
	zassert_ok(ret);
	zassert_equal(val, INT16_MIN);
}

ZTEST(at_parser, test_at_parser_uint32_get_eperm)
{
	int ret;
	uint32_t val;
	struct at_parser parser;

	ret = at_parser_uint32_get(&parser, 1, &val);
	zassert_equal(ret, -EPERM);
}

ZTEST(at_parser, test_at_parser_uint32_get_einval)
{
	int ret;
	uint32_t val;
	struct at_parser parser;

	const char *str1 = "+NOTIF: 1,-2,3\r\nOK\r\n";

	ret = at_parser_init(&parser, str1);
	zassert_ok(ret);

	ret = at_parser_uint32_get(&parser, 1, &val);
	zassert_ok(ret);
	zassert_equal(val, 1);

	ret = at_parser_uint32_get(NULL, 1, NULL);
	zassert_equal(ret, -EINVAL);

	ret = at_parser_uint32_get(&parser, 1, NULL);
	zassert_equal(ret, -EINVAL);

	ret = at_parser_uint32_get(NULL, 1, &val);
	zassert_equal(ret, -EINVAL);
}

ZTEST(at_parser, test_at_parser_uint32_get_erange)
{
	int ret;
	uint32_t val;
	struct at_parser parser;

	char str1[64] = { 0 };

	snprintf(str1, sizeof(str1), "+NOTIF: %lld,-2,3\r\nOK\r\n", (int64_t)UINT32_MAX + 1);

	ret = at_parser_init(&parser, str1);
	zassert_ok(ret);

	/* Greater than maximum of unsigned 32-bit integer. */
	ret = at_parser_uint32_get(&parser, 1, &val);
	zassert_equal(ret, -ERANGE);

	/* Negative number. */
	ret = at_parser_uint32_get(&parser, 2, &val);
	zassert_equal(ret, -ERANGE);
}

ZTEST(at_parser, test_at_parser_uint32_get_eopnotsupp)
{
	int ret;
	uint32_t val;
	struct at_parser parser;

	const char *str1 = "+NOTIF: 1,2";

	ret = at_parser_init(&parser, str1);
	zassert_ok(ret);

	/* Trying to parse notification string as integer. */
	ret = at_parser_uint32_get(&parser, 0, &val);
	zassert_equal(ret, -EOPNOTSUPP);
}

ZTEST(at_parser, test_at_parser_uint32_get_enodata)
{
	int ret;
	uint32_t val;
	struct at_parser parser;

	const char *str1 = "+NOTIF: 1,,3";

	ret = at_parser_init(&parser, str1);
	zassert_ok(ret);

	/* Trying to parse an empty subparameter as integer. */
	ret = at_parser_uint32_get(&parser, 2, &val);
	zassert_equal(ret, -ENODATA);
}

ZTEST(at_parser, test_at_parser_uint32_get_ebadmsg)
{
	int ret;
	uint32_t val;
	struct at_parser parser;

	/* 1) Space is not a valid separator between subparameters. */
	const char *str1 = "+NOTIF: 1,2,3 4,5,6 7,8,9";

	ret = at_parser_init(&parser, str1);
	zassert_ok(ret);

	ret = at_parser_uint32_get(&parser, 4, &val);
	zassert_equal(ret, -EBADMSG);

	/* 2) String is not a valid AT command. */
	const char *str2 = "\"abba\",1,2";

	ret = at_parser_init(&parser, str2);
	zassert_ok(ret);

	ret = at_parser_uint32_get(&parser, 1, &val);
	zassert_equal(ret, -EBADMSG);
}

ZTEST(at_parser, test_at_parser_uint32_get)
{
	int ret;
	uint32_t val;
	struct at_parser parser;

	/* 1) */
	const char *str1 = "+NOTIF: 1,-2,3\r\nOK\r\n";

	ret = at_parser_init(&parser, str1);
	zassert_ok(ret);

	ret = at_parser_uint32_get(&parser, 1, &val);
	zassert_ok(ret);
	zassert_equal(val, 1);

	/* 2) */
	char str2[64] = { 0 };

	snprintf(str2, sizeof(str2), "+NOTIF: %lld,-2,3\r\nOK\r\n", (int64_t)UINT32_MAX);

	ret = at_parser_init(&parser, str2);
	zassert_ok(ret);

	ret = at_parser_uint32_get(&parser, 1, &val);
	zassert_ok(ret);
	zassert_equal(val, UINT32_MAX);
}

ZTEST(at_parser, test_at_parser_int32_get_eperm)
{
	int ret;
	int32_t val;
	struct at_parser parser;

	ret = at_parser_int32_get(&parser, 1, &val);
	zassert_equal(ret, -EPERM);
}

ZTEST(at_parser, test_at_parser_int32_get_einval)
{
	int ret;
	int32_t val;
	struct at_parser parser;

	const char *str1 = "+NOTIF: 1,-2,3\r\nOK\r\n";

	ret = at_parser_init(&parser, str1);
	zassert_ok(ret);

	ret = at_parser_int32_get(&parser, 1, &val);
	zassert_ok(ret);
	zassert_equal(val, 1);

	ret = at_parser_int32_get(NULL, 1, NULL);
	zassert_equal(ret, -EINVAL);

	ret = at_parser_int32_get(&parser, 1, NULL);
	zassert_equal(ret, -EINVAL);

	ret = at_parser_int32_get(NULL, 1, &val);
	zassert_equal(ret, -EINVAL);
}

ZTEST(at_parser, test_at_token_int32_get_erange)
{
	int ret;
	int32_t val;
	struct at_parser parser;

	/* 1) Larger than maximum. */
	char str1[64] = { 0 };

	snprintf(str1, sizeof(str1), "+NOTIF: %lld,-2,3\r\nOK\r\n", (int64_t)INT32_MAX + 1);

	ret = at_parser_init(&parser, str1);
	zassert_ok(ret);

	ret = at_parser_int32_get(&parser, 1, &val);
	zassert_equal(ret, -ERANGE);

	/* 2) Smaller than minimum. */
	char str2[64] = { 0 };

	snprintf(str2, sizeof(str2), "+NOTIF: %lld,-2,3\r\nOK\r\n", (int64_t)INT32_MIN - 1);

	ret = at_parser_init(&parser, str1);
	zassert_ok(ret);

	ret = at_parser_int32_get(&parser, 1, &val);
	zassert_equal(ret, -ERANGE);
}

ZTEST(at_parser, test_at_parser_int32_get_eopnotsupp)
{
	int ret;
	int32_t val;
	struct at_parser parser;

	const char *str1 = "+NOTIF: 1,2";

	ret = at_parser_init(&parser, str1);
	zassert_ok(ret);

	/* Trying to parse notification string as integer. */
	ret = at_parser_int32_get(&parser, 0, &val);
	zassert_equal(ret, -EOPNOTSUPP);
}

ZTEST(at_parser, test_at_parser_int32_get_enodata)
{
	int ret;
	int32_t val;
	struct at_parser parser;

	const char *str1 = "+NOTIF: 1,,3";

	ret = at_parser_init(&parser, str1);
	zassert_ok(ret);

	/* Trying to parse an empty subparameter as integer. */
	ret = at_parser_int32_get(&parser, 2, &val);
	zassert_equal(ret, -ENODATA);
}

ZTEST(at_parser, test_at_parser_int32_get_ebadmsg)
{
	int ret;
	int32_t val;
	struct at_parser parser;

	/* 1) Space is not a valid separator between subparameters. */
	const char *str1 = "+NOTIF: 1,2,3 -4,5,6 7,8,9";

	ret = at_parser_init(&parser, str1);
	zassert_ok(ret);

	ret = at_parser_int32_get(&parser, 4, &val);
	zassert_equal(ret, -EBADMSG);

	/* 2) String is not a valid AT command. */
	const char *str2 = "\"abba\",-1,2";

	ret = at_parser_init(&parser, str2);
	zassert_ok(ret);

	ret = at_parser_int32_get(&parser, 1, &val);
	zassert_equal(ret, -EBADMSG);
}

ZTEST(at_parser, test_at_parser_int32_get)
{
	int ret;
	int32_t val;
	struct at_parser parser;

	/* 1) */
	const char *str1 = "+NOTIF: 1,-2,3\r\nOK\r\n";

	ret = at_parser_init(&parser, str1);
	zassert_ok(ret);

	ret = at_parser_int32_get(&parser, 1, &val);
	zassert_ok(ret);
	zassert_equal(val, 1);

	/* 2) */
	char str2[64] = { 0 };

	snprintf(str2, sizeof(str2), "+NOTIF: %lld,-2,3\r\nOK\r\n", (int64_t)INT32_MAX);

	ret = at_parser_init(&parser, str2);
	zassert_ok(ret);

	ret = at_parser_int32_get(&parser, 1, &val);
	zassert_ok(ret);
	zassert_equal(val, INT32_MAX);

	/* 3) */
	char str3[64] = { 0 };

	snprintf(str3, sizeof(str3), "+NOTIF: %lld,-2,3\r\nOK\r\n", (int64_t)INT32_MIN);

	ret = at_parser_init(&parser, str3);
	zassert_ok(ret);

	ret = at_parser_int32_get(&parser, 1, &val);
	zassert_ok(ret);
	zassert_equal(val, INT32_MIN);
}

ZTEST(at_parser, test_at_parser_uint64_get_eperm)
{
	int ret;
	uint64_t val;
	struct at_parser parser;

	ret = at_parser_uint64_get(&parser, 1, &val);
	zassert_equal(ret, -EPERM);
}

ZTEST(at_parser, test_at_parser_uint64_get_einval)
{
	int ret;
	uint64_t val;
	struct at_parser parser;

	const char *str1 = "+NOTIF: 1,-2,3\r\nOK\r\n";

	ret = at_parser_init(&parser, str1);
	zassert_ok(ret);

	ret = at_parser_uint64_get(&parser, 1, &val);
	zassert_ok(ret);
	zassert_equal(val, 1);

	ret = at_parser_uint64_get(NULL, 1, NULL);
	zassert_equal(ret, -EINVAL);

	ret = at_parser_uint64_get(&parser, 1, NULL);
	zassert_equal(ret, -EINVAL);

	ret = at_parser_uint64_get(NULL, 1, &val);
	zassert_equal(ret, -EINVAL);
}

ZTEST(at_parser, test_at_parser_uint64_get_erange)
{
	int ret;
	uint64_t val;
	struct at_parser parser;

	/* 1) */
	const char *str1 = "+NOTIF: 100000000000000000000,-2,3\r\nOK\r\n";

	ret = at_parser_init(&parser, str1);
	zassert_ok(ret);

	/* Greater than maximum of unsigned 64-bit integer. */
	ret = at_parser_uint64_get(&parser, 1, &val);
	zassert_equal(ret, -ERANGE);

	/* Negative number. */
	ret = at_parser_uint64_get(&parser, 1, &val);
	zassert_equal(ret, -ERANGE);
}

ZTEST(at_parser, test_at_parser_uint64_get_eopnotsupp)
{
	int ret;
	uint64_t val;
	struct at_parser parser;

	const char *str1 = "+NOTIF: 1,2";

	ret = at_parser_init(&parser, str1);
	zassert_ok(ret);

	/* Trying to parse notification string as integer. */
	ret = at_parser_uint64_get(&parser, 0, &val);
	zassert_equal(ret, -EOPNOTSUPP);
}

ZTEST(at_parser, test_at_parser_uint64_get_enodata)
{
	int ret;
	uint64_t val;
	struct at_parser parser;

	const char *str1 = "+NOTIF: 1,,3";

	ret = at_parser_init(&parser, str1);
	zassert_ok(ret);

	/* Trying to parse an empty subparameter as integer. */
	ret = at_parser_uint64_get(&parser, 2, &val);
	zassert_equal(ret, -ENODATA);
}

ZTEST(at_parser, test_at_parser_uint64_get_ebadmsg)
{
	int ret;
	uint64_t val;
	struct at_parser parser;

	/* 1) Space is not a valid separator between subparameters. */
	const char *str1 = "+NOTIF: 1,2,3 4,5,6 7,8,9";

	ret = at_parser_init(&parser, str1);
	zassert_ok(ret);

	ret = at_parser_uint64_get(&parser, 4, &val);
	zassert_equal(ret, -EBADMSG);

	/* 2) String is not a valid AT command. */
	const char *str2 = "\"abba\",1,2";

	ret = at_parser_init(&parser, str2);
	zassert_ok(ret);

	ret = at_parser_uint64_get(&parser, 1, &val);
	zassert_equal(ret, -EBADMSG);
}

ZTEST(at_parser, test_at_token_uint64_get)
{
	int ret;
	uint64_t val;
	struct at_parser parser;

	/* 1) */
	const char *str1 = "+NOTIF: 1,-2,3\r\nOK\r\n";

	ret = at_parser_init(&parser, str1);
	zassert_ok(ret);

	ret = at_parser_uint64_get(&parser, 1, &val);
	zassert_ok(ret);
	zassert_equal(val, 1);

	/* 2) */
	char str2[64] = { 0 };

	snprintf(str2, sizeof(str2), "+NOTIF: %llu,-2,3\r\nOK\r\n", (uint64_t)UINT64_MAX);

	ret = at_parser_init(&parser, str2);
	zassert_ok(ret);

	ret = at_parser_uint64_get(&parser, 1, &val);
	zassert_ok(ret, "%d", ret);
	zassert_equal(val, UINT64_MAX);

	/* 3) */
	char str3[64] = { 0 };

	snprintf(str3, sizeof(str3), "+NOTIF: 0,-2,3\r\nOK\r\n");

	ret = at_parser_init(&parser, str3);
	zassert_ok(ret);

	ret = at_parser_uint64_get(&parser, 1, &val);
	zassert_ok(ret);
	zassert_equal(val, 0);
}

ZTEST(at_parser, test_at_parser_int64_get_eperm)
{
	int ret;
	int64_t val;
	struct at_parser parser;

	ret = at_parser_int64_get(&parser, 1, &val);
	zassert_equal(ret, -EPERM);
}

ZTEST(at_parser, test_at_parser_int64_get_einval)
{
	int ret;
	int64_t val;
	struct at_parser parser;

	const char *str1 = "+NOTIF: 1,-2,3\r\nOK\r\n";

	ret = at_parser_init(&parser, str1);
	zassert_ok(ret);

	ret = at_parser_int64_get(&parser, 1, &val);
	zassert_ok(ret);
	zassert_equal(val, 1);

	ret = at_parser_int64_get(NULL, 1, NULL);
	zassert_equal(ret, -EINVAL);

	ret = at_parser_int64_get(&parser, 1, NULL);
	zassert_equal(ret, -EINVAL);

	ret = at_parser_int64_get(NULL, 1, &val);
	zassert_equal(ret, -EINVAL);
}

ZTEST(at_parser, test_at_parser_int64_get_erange)
{
	int ret;
	int64_t val;
	struct at_parser parser;

	/* 1) */
	const char *str1 = "+NOTIF: 100000000000000000000,-2,3\r\nOK\r\n";

	ret = at_parser_init(&parser, str1);
	zassert_ok(ret);

	ret = at_parser_int64_get(&parser, 1, &val);
	zassert_equal(ret, -ERANGE);

	/* 2) */
	const char *str2 = "+NOTIF: -100000000000000000000,-2,3\r\nOK\r\n";

	ret = at_parser_init(&parser, str2);
	zassert_ok(ret);

	ret = at_parser_int64_get(&parser, 1, &val);
	zassert_equal(ret, -ERANGE);
}

ZTEST(at_parser, test_at_parser_int64_get_eopnotsupp)
{
	int ret;
	int64_t val;
	struct at_parser parser;

	const char *str1 = "+NOTIF: 1,2";

	ret = at_parser_init(&parser, str1);
	zassert_ok(ret);

	/* Trying to parse notification string as integer. */
	ret = at_parser_int64_get(&parser, 0, &val);
	zassert_equal(ret, -EOPNOTSUPP);
}

ZTEST(at_parser, test_at_parser_int64_get_enodata)
{
	int ret;
	int64_t val;
	struct at_parser parser;

	const char *str1 = "+NOTIF: 1,,3";

	ret = at_parser_init(&parser, str1);
	zassert_ok(ret);

	/* Trying to parse an empty subparameter as integer. */
	ret = at_parser_int64_get(&parser, 2, &val);
	zassert_equal(ret, -ENODATA);
}

ZTEST(at_parser, test_at_parser_int64_get_ebadmsg)
{
	int ret;
	int64_t val;
	struct at_parser parser;

	/* 1) Space is not a valid separator between subparameters. */
	const char *str1 = "+NOTIF: 1,2,3 -4,5,6 7,8,9";

	ret = at_parser_init(&parser, str1);
	zassert_ok(ret);

	ret = at_parser_int64_get(&parser, 4, &val);
	zassert_equal(ret, -EBADMSG);

	/* 2) String is not a valid AT command. */
	const char *str2 = "\"abba\",-1,2";

	ret = at_parser_init(&parser, str2);
	zassert_ok(ret);

	ret = at_parser_int64_get(&parser, 1, &val);
	zassert_equal(ret, -EBADMSG);
}

ZTEST(at_parser, test_at_token_int64_get)
{
	int ret;
	int64_t val;
	struct at_parser parser;

	/* 1) */
	const char *str1 = "+NOTIF: 1,-2,3\r\nOK\r\n";

	ret = at_parser_init(&parser, str1);
	zassert_ok(ret);

	ret = at_parser_int64_get(&parser, 1, &val);
	zassert_ok(ret);
	zassert_equal(val, 1);

	/* 2) */
	char str2[64] = { 0 };

	snprintf(str2, sizeof(str2), "+NOTIF: %lld,-2,3\r\nOK\r\n", (int64_t)INT64_MAX);

	ret = at_parser_init(&parser, str2);
	zassert_ok(ret);

	ret = at_parser_int64_get(&parser, 1, &val);
	zassert_ok(ret);
	zassert_equal(val, INT64_MAX);

	/* 3) */
	char str3[64] = { 0 };

	snprintf(str3, sizeof(str3), "+NOTIF: %lld,-2,3\r\nOK\r\n", (int64_t)INT64_MIN);

	ret = at_parser_init(&parser, str3);
	zassert_ok(ret);

	ret = at_parser_int64_get(&parser, 1, &val);
	zassert_ok(ret);
	zassert_equal(val, INT64_MIN);
}

ZTEST(at_parser, test_at_parser_string_get_enomem)
{
	int ret;
	struct at_parser parser;
	char buffer[32] = { 0 };
	size_t len = sizeof(buffer);

	const char *str1 = "+NOTIF: 1,2,3\r\nOK\r\n";

	ret = at_parser_init(&parser, str1);
	zassert_ok(ret);

	len = 1;

	ret = at_parser_string_get(&parser, 0, buffer, &len);
	zassert_equal(ret, -ENOMEM);
}

ZTEST(at_parser, test_at_parser_string_get_einval)
{
	int ret;
	struct at_parser parser;
	char buffer[32] = { 0 };
	size_t len = sizeof(buffer);

	const char *str1 = "+NOTIF: 1,2,3\r\nOK\r\n";

	ret = at_parser_init(&parser, str1);
	zassert_ok(ret);

	ret = at_parser_string_get(NULL, 0, buffer, &len);
	zassert_equal(ret, -EINVAL);

	ret = at_parser_string_get(&parser, 0, NULL, &len);
	zassert_equal(ret, -EINVAL);

	ret = at_parser_string_get(&parser, 0, buffer, NULL);
	zassert_equal(ret, -EINVAL);
}

ZTEST(at_parser, test_at_parser_string_get_eperm)
{
	int ret;
	struct at_parser parser;
	char buffer[32] = { 0 };
	size_t len = sizeof(buffer);

	ret = at_parser_string_get(&parser, 1, buffer, &len);
	zassert_equal(ret, -EPERM);
}

ZTEST(at_parser, test_at_parser_string_get_eopnotsupp)
{
	int ret;
	struct at_parser parser;
	char buffer[32] = { 0 };
	size_t len = sizeof(buffer);

	const char *str1 = "+NOTIF: 1,2,3";

	ret = at_parser_init(&parser, str1);
	zassert_ok(ret);

	ret = at_parser_string_get(&parser, 1, buffer, &len);
	zassert_equal(ret, -EOPNOTSUPP);
}

ZTEST(at_parser, test_at_parser_string_get_ebadmsg)
{
	int ret;
	struct at_parser parser;
	char buffer[32] = { 0 };
	size_t len = sizeof(buffer);

	/* Malformed string where separator is missing between index 2 and index 3. */
	const char *str1 = "+NOTIF: 1,2 3,\"test\"";

	ret = at_parser_init(&parser, str1);
	zassert_ok(ret);

	ret = at_parser_string_get(&parser, 4, buffer, &len);
	zassert_equal(ret, -EBADMSG);

	/* Malformed string where command is malformed. */
	const char *str2 = "++NOTIF: 1,2,3,\"test\"";

	ret = at_parser_init(&parser, str2);
	zassert_ok(ret);

	ret = at_parser_string_get(&parser, 4, buffer, &len);
	zassert_equal(ret, -EBADMSG);

	/* Malformed string where command is malformed. */
	const char *str3 = "+NOTIF+: 1,2,3,\"test\"";

	ret = at_parser_init(&parser, str3);
	zassert_ok(ret);

	ret = at_parser_string_get(&parser, 4, buffer, &len);
	zassert_equal(ret, -EBADMSG);

	/* Malformed string where command is malformed. */
	const char *str4 = "AT+CMD+: 1,2,3,\"test\"";

	ret = at_parser_init(&parser, str4);
	zassert_ok(ret);

	ret = at_parser_string_get(&parser, 4, buffer, &len);
	zassert_equal(ret, -EBADMSG);

	/* Malformed string where array is empty. */
	const char *str5 = "+NOTIF: (),2,3,\"test\"";

	ret = at_parser_init(&parser, str5);
	zassert_ok(ret);

	ret = at_parser_string_get(&parser, 4, buffer, &len);
	zassert_equal(ret, -EBADMSG);
}

ZTEST(at_parser, test_at_parser_string_get)
{
	int ret;
	struct at_parser parser;
	char buffer[64] = { 0 };
	size_t len = sizeof(buffer);

	const char *str1 = "+CGEV: ME PDN ACT 0";

	ret = at_parser_init(&parser, str1);
	zassert_ok(ret);

	ret = at_parser_string_get(&parser, 1, buffer, &len);
	zassert_ok(ret);
	zassert_mem_equal("ME PDN ACT 0", buffer, len);

	len = sizeof(buffer);

	const char *str2 = "+NOTIF: \"\"";

	ret = at_parser_init(&parser, str2);
	zassert_ok(ret);

	ret = at_parser_string_get(&parser, 1, buffer, &len);
	zassert_ok(ret);
	zassert_mem_equal("", buffer, len);

	len = sizeof(buffer);

	const char *str3 = "mfw-nr+_nrf91x1_0.0.0-110.nr+-test\r\nOK\r\n";

	ret = at_parser_init(&parser, str3);
	zassert_ok(ret);

	ret = at_parser_string_get(&parser, 0, buffer, &len);
	zassert_ok(ret);
	zassert_mem_equal("mfw-nr+_nrf91x1_0.0.0-110.nr+-test", buffer, len);
}

ZTEST(at_parser, test_at_parser_string_get_array)
{
	int ret;
	struct at_parser parser;
	char buffer[32] = { 0 };
	size_t len = sizeof(buffer);

	const char *str1 = "+NOTIF: 1,(1,2),3";

	ret = at_parser_init(&parser, str1);
	zassert_ok(ret);

	ret = at_parser_string_get(&parser, 2, buffer, &len);
	zassert_ok(ret);
	zassert_mem_equal("(1,2)", buffer, len);
}

ZTEST(at_parser, test_at_parser_string_ptr_get_einval)
{
	int ret;
	struct at_parser parser;
	const char *str;
	size_t len;

	const char *str1 = "+CGEV: ME PDN ACT 0";

	ret = at_parser_init(&parser, str1);
	zassert_ok(ret);

	ret = at_parser_string_ptr_get(NULL, 1, &str, &len);
	zassert_equal(ret, -EINVAL);

	ret = at_parser_string_ptr_get(&parser, 1, NULL, &len);
	zassert_equal(ret, -EINVAL);

	ret = at_parser_string_ptr_get(&parser, 1, &str, NULL);
	zassert_equal(ret, -EINVAL);
}

ZTEST(at_parser, test_at_parser_string_ptr_get_eperm)
{
	int ret;
	struct at_parser parser;
	const char *str;
	size_t len;

	ret = at_parser_string_ptr_get(&parser, 1, &str, &len);
	zassert_equal(ret, -EPERM);
}

ZTEST(at_parser, test_at_parser_string_ptr_get_eopnotsupp)
{
	int ret;
	struct at_parser parser;
	const char *str;
	size_t len;

	const char *str1 = "+NOTIF: 1,2,3";

	ret = at_parser_init(&parser, str1);
	zassert_ok(ret);

	ret = at_parser_string_ptr_get(&parser, 1, &str, &len);
	zassert_equal(ret, -EOPNOTSUPP);
}

ZTEST(at_parser, test_at_parser_string_ptr_get_ebadmsg)
{
	int ret;
	struct at_parser parser;
	const char *str;
	size_t len;

	/* Malformed string where separator is missing between index 2 and index 3. */
	const char *str1 = "+NOTIF: 1,2 3,\"test\"";

	ret = at_parser_init(&parser, str1);
	zassert_ok(ret);

	ret = at_parser_string_ptr_get(&parser, 4, &str, &len);
	zassert_equal(ret, -EBADMSG);
}

ZTEST(at_parser, test_at_parser_string_ptr_get)
{
	int ret;
	struct at_parser parser;
	const char *str;
	size_t len;

	const char *str1 = "+CGEV: ME PDN ACT 0";

	ret = at_parser_init(&parser, str1);
	zassert_ok(ret);

	ret = at_parser_string_ptr_get(&parser, 1, &str, &len);
	zassert_ok(ret);
	zassert_mem_equal("ME PDN ACT 0", str, len);
}

ZTEST(at_parser, test_at_parser_cmd_type_get_einval)
{
	int ret;
	struct at_parser parser;
	enum at_parser_cmd_type type;

	const char *str1 = "ABBA";

	ret = at_parser_init(&parser, str1);
	zassert_ok(ret);

	ret = at_parser_cmd_type_get(&parser, NULL);
	zassert_equal(ret, -EINVAL);

	ret = at_parser_cmd_type_get(NULL, &type);
	zassert_equal(ret, -EINVAL);
}

ZTEST(at_parser, test_at_parser_cmd_type_get_eperm)
{
	int ret;
	struct at_parser parser;
	enum at_parser_cmd_type type;

	ret = at_parser_cmd_type_get(&parser, &type);
	zassert_equal(ret, -EPERM);
}

ZTEST(at_parser, test_at_parser_cmd_type_get_eopnotsupp)
{
	int ret;
	struct at_parser parser;
	enum at_parser_cmd_type type;

	const char *str1 = "ABBA";

	ret = at_parser_init(&parser, str1);
	zassert_ok(ret);

	ret = at_parser_cmd_type_get(&parser, &type);
	zassert_equal(ret, -EOPNOTSUPP);
	zassert_equal(type, AT_PARSER_CMD_TYPE_UNKNOWN);

	/* Not a valid command (note: it's a notification). */
	const char *str2 = "+NOTIF: 1,2,3\r\nOK\r\n";

	ret = at_parser_init(&parser, str2);
	zassert_ok(ret);

	ret = at_parser_cmd_type_get(&parser, &type);
	zassert_equal(ret, -EOPNOTSUPP);
	zassert_equal(type, AT_PARSER_CMD_TYPE_UNKNOWN);
}

ZTEST(at_parser, test_at_parser_cmd_type_get)
{
	int ret;
	struct at_parser parser;
	enum at_parser_cmd_type type;

	const char *str1 = "AT+CMD=?";

	ret = at_parser_init(&parser, str1);
	zassert_ok(ret);

	ret = at_parser_cmd_type_get(&parser, &type);
	zassert_ok(ret);
	zassert_equal(type, AT_PARSER_CMD_TYPE_TEST);

	const char *str2 = "AT+CMD?";

	ret = at_parser_init(&parser, str2);
	zassert_ok(ret);

	ret = at_parser_cmd_type_get(&parser, &type);
	zassert_ok(ret);
	zassert_equal(type, AT_PARSER_CMD_TYPE_READ);

	const char *str3 = "AT+CMD=1,2,3";

	ret = at_parser_init(&parser, str3);
	zassert_ok(ret);

	ret = at_parser_cmd_type_get(&parser, &type);
	zassert_ok(ret);
	zassert_equal(type, AT_PARSER_CMD_TYPE_SET);
}

ZTEST(at_parser, test_at_parser_cmd_count_get_einval)
{
	int ret;
	struct at_parser parser;
	size_t count = 0;

	const char *str1 = "+NOTIF: 1,2,3\r\nOK\r\n";

	ret = at_parser_init(&parser, str1);
	zassert_ok(ret);

	ret = at_parser_cmd_count_get(&parser, NULL);
	zassert_equal(ret, -EINVAL);

	ret = at_parser_cmd_count_get(NULL, &count);
	zassert_equal(ret, -EINVAL);
}

ZTEST(at_parser, test_at_parser_cmd_count_get_eperm)
{
	int ret;
	struct at_parser parser;
	size_t count = 0;

	ret = at_parser_cmd_count_get(&parser, &count);
	zassert_equal(ret, -EPERM);
}

ZTEST(at_parser, test_at_parser_cmd_count_get)
{
	int ret;
	struct at_parser parser;
	size_t count = 0;

	const char *str1 = "+NOTIF: 1,2,3\r\nOK\r\n";

	ret = at_parser_init(&parser, str1);
	zassert_ok(ret);

	ret = at_parser_cmd_count_get(&parser, &count);
	zassert_ok(ret);
	zassert_equal(count, 4);
}

ZTEST(at_parser, test_at_parser_cmd_next)
{
	int ret;
	struct at_parser parser;
	int32_t num = 0;

	const char *first_line = "+NOTIF: 1,2,3,,\r\n";
	const char *second_line = "+NOTIF2: 4,5\r\n";
	const char *third_line = "+NOTIF3: 6,7,8\r\n"
				 "OK\r\n";
	char at1[128] = { 0 };
	char at2[64] = { 0 };

	snprintf(at2, sizeof(at2), "%s%s", second_line, third_line);
	snprintf(at1, sizeof(at1), "%s%s", first_line, at2);

	ret = at_parser_init(&parser, at1);
	zassert_ok(ret);

	ret = at_parser_num_get(&parser, 1, &num);
	zassert_ok(ret);
	zassert_equal(num, 1);

	ret = at_parser_cmd_next(&parser);
	zassert_ok(ret);

	ret = at_parser_num_get(&parser, 1, &num);
	zassert_ok(ret);
	zassert_equal(num, 4);

	ret = at_parser_cmd_next(&parser);
	zassert_ok(ret);

	ret = at_parser_num_get(&parser, 1, &num);
	zassert_ok(ret);
	zassert_equal(num, 6);
}

ZTEST_SUITE(at_parser, NULL, NULL, NULL, NULL, NULL);
