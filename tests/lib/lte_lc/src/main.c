/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <unity.h>
#include <stdio.h>
#include <string.h>

#include "lte_lc_helpers.h"
#include "lte_lc.h"

/* The unity_main is not declared in any header file. It is only defined in the generated test
 * runner because of ncs' unity configuration. It is therefore declared here to avoid a compiler
 * warning.
 */
extern int unity_main(void);

void test_parse_edrx(void)
{
	int err;
	struct lte_lc_edrx_cfg cfg;
	char edrx_str[5];
	char ptw_str[5];
	char *at_response_none = "+CEDRXP: 0";
	char *at_response_fail = "+CEDRXP: 1,\"1000\",\"0101\",\"1011\"";
	char *at_response_ltem = "+CEDRXP: 4,\"1000\",\"0101\",\"1011\"";
	char *at_response_ltem_2 = "+CEDRXP: 4,\"1000\",\"0010\",\"1110\"";
	char *at_response_nbiot = "+CEDRXP: 5,\"1000\",\"1101\",\"0111\"";
	char *at_response_nbiot_2 = "+CEDRXP: 5,\"1000\",\"1101\",\"0101\"";

	err = parse_edrx(at_response_none, &cfg, edrx_str, ptw_str);
	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_EQUAL(0, cfg.edrx);
	TEST_ASSERT_EQUAL(0, cfg.ptw);
	TEST_ASSERT_EQUAL(LTE_LC_LTE_MODE_NONE, cfg.mode);

	err = parse_edrx(at_response_fail, &cfg, edrx_str, ptw_str);
	TEST_ASSERT_NOT_EQUAL_MESSAGE(0, err, "parse_edrx should have failed");

	err = parse_edrx(at_response_ltem, &cfg, edrx_str, ptw_str);
	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_FLOAT_WITHIN(0.1, 81.92, cfg.edrx);
	TEST_ASSERT_FLOAT_WITHIN(0.1, 15.36, cfg.ptw);
	TEST_ASSERT_EQUAL(LTE_LC_LTE_MODE_LTEM, cfg.mode);
	TEST_ASSERT_EQUAL_STRING("0101", edrx_str);
	TEST_ASSERT_EQUAL_STRING("1011", ptw_str);

	err = parse_edrx(at_response_ltem_2, &cfg, edrx_str, ptw_str);
	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_FLOAT_WITHIN(0.1, 20.48, cfg.edrx);
	TEST_ASSERT_FLOAT_WITHIN(0.1, 19.2, cfg.ptw);
	TEST_ASSERT_EQUAL(LTE_LC_LTE_MODE_LTEM, cfg.mode);
	TEST_ASSERT_EQUAL_STRING("0010", edrx_str);
	TEST_ASSERT_EQUAL_STRING("1110", ptw_str);

	err = parse_edrx(at_response_nbiot, &cfg, edrx_str, ptw_str);
	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_FLOAT_WITHIN(0.1, 2621.44, cfg.edrx);
	TEST_ASSERT_FLOAT_WITHIN(0.1, 20.48, cfg.ptw);
	TEST_ASSERT_EQUAL(LTE_LC_LTE_MODE_NBIOT, cfg.mode);
	TEST_ASSERT_EQUAL_STRING("1101", edrx_str);
	TEST_ASSERT_EQUAL_STRING("0111", ptw_str);

	err = parse_edrx(at_response_nbiot_2, &cfg, edrx_str, ptw_str);
	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_FLOAT_WITHIN(0.1, 2621.44, cfg.edrx);
	TEST_ASSERT_FLOAT_WITHIN(0.1, 15.36, cfg.ptw);
	TEST_ASSERT_EQUAL(LTE_LC_LTE_MODE_NBIOT, cfg.mode);
	TEST_ASSERT_EQUAL_STRING("1101", edrx_str);
	TEST_ASSERT_EQUAL_STRING("0101", ptw_str);
}

void test_parse_cereg(void)
{
	int err;
	enum lte_lc_nw_reg_status status;
	struct lte_lc_cell cell;
	enum lte_lc_lte_mode mode;
	struct lte_lc_psm_cfg psm_cfg;
	char *at_cereg_0 = "+CEREG: 0";
	char *at_cereg_1 = "+CEREG: 1,\"0A0B\",\"01020304\",9,,,\"00100110\",\"01011111\"";
	char *at_cereg_1_no_tau_ext = "+CEREG: 1,\"0A0B\",\"01020304\",9,,,\"01001010\"";
	char *at_cereg_1_no_tau_ext_no_active_time = "+CEREG: 1,\"0A0B\",\"01020304\",9";
	char *at_cereg_2 = "+CEREG: 2,\"ABBA\",\"DEADBEEF\",7";
	char *at_cereg_2_reject_cause = "+CEREG: 2,\"ABBA\",\"DEADBEEF\",7,0,13";
	char *at_cereg_2_unknown_cause_type = "+CEREG: 2,\"ABBA\",\"DEADBEEF\",7,1,1";
	char *at_cereg_4 = "+CEREG: 4";
	char *at_cereg_5 = "+CEREG: 5,\"0A0B\",\"01020304\",9,,,\"11100000\",\"00011111\"";
	char *at_cereg_90 = "+CEREG: 90";
	char *at_cereg_invalid = "+CEREG: 10,,,5,,,,";

	err = parse_cereg(at_cereg_0, &status, &cell, &mode, &psm_cfg);
	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_EQUAL(LTE_LC_NW_REG_NOT_REGISTERED, status);
	TEST_ASSERT_EQUAL(LTE_LC_CELL_EUTRAN_ID_INVALID, cell.id);
	TEST_ASSERT_EQUAL(LTE_LC_CELL_TAC_INVALID, cell.tac);
	TEST_ASSERT_EQUAL(LTE_LC_LTE_MODE_NONE, mode);
	TEST_ASSERT_EQUAL(-1, psm_cfg.active_time);
	TEST_ASSERT_EQUAL(-1, psm_cfg.tau);

	err = parse_cereg(at_cereg_1, &status, &cell, &mode, &psm_cfg);
	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_EQUAL(LTE_LC_NW_REG_REGISTERED_HOME, status);
	TEST_ASSERT_EQUAL(0x01020304, cell.id);
	TEST_ASSERT_EQUAL(0x0A0B, cell.tac);
	TEST_ASSERT_EQUAL(LTE_LC_LTE_MODE_NBIOT, mode);
	TEST_ASSERT_EQUAL(360, psm_cfg.active_time);
	TEST_ASSERT_EQUAL(1116000, psm_cfg.tau);

	err = parse_cereg(at_cereg_1_no_tau_ext, &status, &cell, &mode, &psm_cfg);
	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_EQUAL(LTE_LC_NW_REG_REGISTERED_HOME, status);
	TEST_ASSERT_EQUAL(0x01020304, cell.id);
	TEST_ASSERT_EQUAL(0x0A0B, cell.tac);
	TEST_ASSERT_EQUAL(LTE_LC_LTE_MODE_NBIOT, mode);
	TEST_ASSERT_EQUAL(-1, psm_cfg.active_time);
	TEST_ASSERT_EQUAL(-1, psm_cfg.tau);

	err = parse_cereg(at_cereg_1_no_tau_ext_no_active_time, &status, &cell, &mode, &psm_cfg);
	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_EQUAL(LTE_LC_NW_REG_REGISTERED_HOME, status);
	TEST_ASSERT_EQUAL(0x01020304, cell.id);
	TEST_ASSERT_EQUAL(0x0A0B, cell.tac);
	TEST_ASSERT_EQUAL(LTE_LC_LTE_MODE_NBIOT, mode);
	TEST_ASSERT_EQUAL(-1, psm_cfg.active_time);
	TEST_ASSERT_EQUAL(-1, psm_cfg.tau);

	err = parse_cereg(at_cereg_2, &status, &cell, &mode, &psm_cfg);
	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_EQUAL(LTE_LC_NW_REG_SEARCHING, status);
	TEST_ASSERT_EQUAL(0xDEADBEEF, cell.id);
	TEST_ASSERT_EQUAL(0xABBA, cell.tac);
	TEST_ASSERT_EQUAL(LTE_LC_LTE_MODE_LTEM, mode);
	TEST_ASSERT_EQUAL(-1, psm_cfg.active_time);
	TEST_ASSERT_EQUAL(-1, psm_cfg.tau);

	err = parse_cereg(at_cereg_2_reject_cause, &status, &cell, &mode, &psm_cfg);
	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_EQUAL(LTE_LC_NW_REG_SEARCHING, status);
	TEST_ASSERT_EQUAL(0xDEADBEEF, cell.id);
	TEST_ASSERT_EQUAL(0xABBA, cell.tac);
	TEST_ASSERT_EQUAL(LTE_LC_LTE_MODE_LTEM, mode);
	TEST_ASSERT_EQUAL(-1, psm_cfg.active_time);
	TEST_ASSERT_EQUAL(-1, psm_cfg.tau);

	err = parse_cereg(at_cereg_2_unknown_cause_type, &status, &cell, &mode, &psm_cfg);
	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_EQUAL(LTE_LC_NW_REG_SEARCHING, status);
	TEST_ASSERT_EQUAL(0xDEADBEEF, cell.id);
	TEST_ASSERT_EQUAL(0xABBA, cell.tac);
	TEST_ASSERT_EQUAL(LTE_LC_LTE_MODE_LTEM, mode);
	TEST_ASSERT_EQUAL(-1, psm_cfg.active_time);
	TEST_ASSERT_EQUAL(-1, psm_cfg.tau);

	err = parse_cereg(at_cereg_4, &status, &cell, &mode, &psm_cfg);
	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_EQUAL(LTE_LC_NW_REG_UNKNOWN, status);
	TEST_ASSERT_EQUAL(LTE_LC_CELL_EUTRAN_ID_INVALID, cell.id);
	TEST_ASSERT_EQUAL(LTE_LC_CELL_TAC_INVALID, cell.tac);
	TEST_ASSERT_EQUAL(LTE_LC_LTE_MODE_NONE, mode);
	TEST_ASSERT_EQUAL(-1, psm_cfg.active_time);
	TEST_ASSERT_EQUAL(-1, psm_cfg.tau);

	err = parse_cereg(at_cereg_5, &status, &cell, &mode, &psm_cfg);
	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_EQUAL(LTE_LC_NW_REG_REGISTERED_ROAMING, status);
	TEST_ASSERT_EQUAL(0x01020304, cell.id);
	TEST_ASSERT_EQUAL(0x0A0B, cell.tac);
	TEST_ASSERT_EQUAL(LTE_LC_LTE_MODE_NBIOT, mode);
	TEST_ASSERT_EQUAL(-1, psm_cfg.active_time);
	TEST_ASSERT_EQUAL(18600, psm_cfg.tau);

	err = parse_cereg(at_cereg_90, &status, &cell, &mode, &psm_cfg);
	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_EQUAL(LTE_LC_NW_REG_UICC_FAIL, status);
	TEST_ASSERT_EQUAL(LTE_LC_CELL_EUTRAN_ID_INVALID, cell.id);
	TEST_ASSERT_EQUAL(LTE_LC_CELL_TAC_INVALID, cell.tac);
	TEST_ASSERT_EQUAL(LTE_LC_LTE_MODE_NONE, mode);
	TEST_ASSERT_EQUAL(-1, psm_cfg.active_time);
	TEST_ASSERT_EQUAL(-1, psm_cfg.tau);

	/* The parser does not check registration status or LTE mode values for validity,
	 * also values not used by the modem are parsed.
	 */
	err = parse_cereg(at_cereg_invalid, &status, &cell, &mode, &psm_cfg);
	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_EQUAL(10, status);
	TEST_ASSERT_EQUAL(LTE_LC_CELL_EUTRAN_ID_INVALID, cell.id);
	TEST_ASSERT_EQUAL(LTE_LC_CELL_TAC_INVALID, cell.tac);
	TEST_ASSERT_EQUAL(5, mode);
	TEST_ASSERT_EQUAL(-1, psm_cfg.active_time);
	TEST_ASSERT_EQUAL(-1, psm_cfg.tau);
}

void test_parse_xt3412(void)
{
	int err;
	uint64_t time;
	char *at_response_short = "%XT3412: 360";
	char *at_response_long = "%XT3412: 2147483647";
	char *at_response_t3412_max = "%XT3412: 35712000000";
	char *at_response_t3412_overflow = "%XT3412: 35712000001";
	char *at_response_negative = "%XT3412: -100";

	err = parse_xt3412(at_response_short, &time);
	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_EQUAL(360, time);

	err = parse_xt3412(at_response_long, &time);
	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_EQUAL(2147483647, time);

	err = parse_xt3412(at_response_t3412_max, &time);
	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_EQUAL(35712000000, time);

	err = parse_xt3412(at_response_t3412_overflow, &time);
	TEST_ASSERT_EQUAL(err, -EINVAL);

	err = parse_xt3412(at_response_negative, &time);
	TEST_ASSERT_EQUAL(err, -EINVAL);
}

void test_parse_xmodemsleep(void)
{
	int err;
	struct lte_lc_modem_sleep modem_sleep;
	char *at_response_0 = "%XMODEMSLEEP: 1,36000";
	char *at_response_1 = "%XMODEMSLEEP: 1,35712000000";
	char *at_response_2 = "%XMODEMSLEEP: 2,100400";
	char *at_response_3 = "%XMODEMSLEEP: 3";
	char *at_response_4 = "%XMODEMSLEEP: 4";
	char *at_response_5 = "%XMODEMSLEEP: 4,0";
	char *at_response_6 = "%XMODEMSLEEP: 7";

	err = parse_xmodemsleep(at_response_0, &modem_sleep);
	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_EQUAL(LTE_LC_MODEM_SLEEP_PSM, modem_sleep.type);
	TEST_ASSERT_EQUAL(36000, modem_sleep.time);

	err = parse_xmodemsleep(at_response_1, &modem_sleep);
	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_EQUAL(LTE_LC_MODEM_SLEEP_PSM, modem_sleep.type);
	TEST_ASSERT_EQUAL(35712000000, modem_sleep.time);

	err = parse_xmodemsleep(at_response_2, &modem_sleep);
	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_EQUAL(LTE_LC_MODEM_SLEEP_RF_INACTIVITY, modem_sleep.type);
	TEST_ASSERT_EQUAL(100400, modem_sleep.time);

	err = parse_xmodemsleep(at_response_3, &modem_sleep);
	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_EQUAL(LTE_LC_MODEM_SLEEP_LIMITED_SERVICE, modem_sleep.type);
	TEST_ASSERT_EQUAL(-1, modem_sleep.time);

	err = parse_xmodemsleep(at_response_4, &modem_sleep);
	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_EQUAL(LTE_LC_MODEM_SLEEP_FLIGHT_MODE, modem_sleep.type);
	TEST_ASSERT_EQUAL(-1, modem_sleep.time);

	err = parse_xmodemsleep(at_response_5, &modem_sleep);
	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_EQUAL(LTE_LC_MODEM_SLEEP_FLIGHT_MODE, modem_sleep.type);
	TEST_ASSERT_EQUAL(0, modem_sleep.time);

	err = parse_xmodemsleep(at_response_6, &modem_sleep);
	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_EQUAL(LTE_LC_MODEM_SLEEP_PROPRIETARY_PSM, modem_sleep.type);
	TEST_ASSERT_EQUAL(-1, modem_sleep.time);

}

void test_parse_rrc_mode(void)
{
	int err;
	enum lte_lc_rrc_mode mode;
	char *at_response_0 = "+CSCON: 0";
	char *at_response_1 = "+CSCON: 1";

	err = parse_rrc_mode(at_response_0, &mode, AT_CSCON_RRC_MODE_INDEX);
	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_EQUAL(LTE_LC_RRC_MODE_IDLE, mode);

	err = parse_rrc_mode(at_response_1, &mode, AT_CSCON_RRC_MODE_INDEX);
	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_EQUAL(LTE_LC_RRC_MODE_CONNECTED, mode);

}

void test_parse_psm(void)
{
	int err;
	struct lte_lc_psm_cfg psm_cfg;
	struct psm_strings {
		char *active_time;
		char *tau_ext;
		char *tau_legacy;
	};
	struct psm_strings psm_disabled_tau_ext = {
		.active_time = "11100000",
		.tau_ext = "00001000",
		.tau_legacy = "11100000",
	};
	struct psm_strings psm_disabled_tau_legacy = {
		.active_time = "11100000",
		.tau_ext = "11100000",
		.tau_legacy = "01001010",
	};
	struct psm_strings tau_legacy = {
		.active_time = "00001000",
		.tau_ext = "11100000",
		.tau_legacy = "00101010",
	};
	struct psm_strings invalid_no_tau = {
		.active_time = "00001000",
		.tau_ext = "11100000",
		.tau_legacy = "11100000",
	};
	struct psm_strings invalid_values = {
		.active_time = "0001111111",
		.tau_ext = "00001",
		.tau_legacy = "111",
	};
	struct psm_strings valid_values_0 = {
		.active_time = "01000001",
		.tau_ext = "01000010",
		.tau_legacy = "11100000",
	};
	struct psm_strings valid_values_1 = {
		.active_time = "00100100",
		.tau_ext = "00001000",
		.tau_legacy = "11100000",
	};
	struct psm_strings valid_values_2 = {
		.active_time = "00010000",
		.tau_ext = "10111011",
		.tau_legacy = "11100000",
	};

	err = parse_psm(psm_disabled_tau_ext.active_time,
			psm_disabled_tau_ext.tau_ext,
			psm_disabled_tau_ext.tau_legacy,
			&psm_cfg);
	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_EQUAL(4800, psm_cfg.tau);
	TEST_ASSERT_EQUAL(-1, psm_cfg.active_time);

	memset(&psm_cfg, 0, sizeof(psm_cfg));

	err = parse_psm(psm_disabled_tau_legacy.active_time,
			psm_disabled_tau_legacy.tau_ext,
			psm_disabled_tau_legacy.tau_legacy,
			&psm_cfg);
	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_EQUAL(3600, psm_cfg.tau);
	TEST_ASSERT_EQUAL(-1, psm_cfg.active_time);

	memset(&psm_cfg, 0, sizeof(psm_cfg));

	err = parse_psm(tau_legacy.active_time,
			tau_legacy.tau_ext,
			tau_legacy.tau_legacy,
			&psm_cfg);
	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_EQUAL(600, psm_cfg.tau);
	TEST_ASSERT_EQUAL(16, psm_cfg.active_time);

	memset(&psm_cfg, 0, sizeof(psm_cfg));

	err = parse_psm(invalid_no_tau.active_time,
			invalid_no_tau.tau_ext,
			invalid_no_tau.tau_legacy,
			&psm_cfg);
	TEST_ASSERT_EQUAL(-EINVAL, err);

	memset(&psm_cfg, 0, sizeof(psm_cfg));

	err = parse_psm(invalid_values.active_time,
			invalid_values.tau_ext,
			invalid_values.tau_legacy,
			&psm_cfg);
	TEST_ASSERT_EQUAL(-EINVAL, err);

	memset(&psm_cfg, 0, sizeof(psm_cfg));

	err = parse_psm(valid_values_0.active_time,
			valid_values_0.tau_ext,
			valid_values_0.tau_legacy,
			&psm_cfg);
	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_EQUAL(72000, psm_cfg.tau);
	TEST_ASSERT_EQUAL(360, psm_cfg.active_time);

	memset(&psm_cfg, 0, sizeof(psm_cfg));

	err = parse_psm(valid_values_1.active_time,
			valid_values_1.tau_ext,
			valid_values_1.tau_legacy,
			&psm_cfg);
	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_EQUAL(4800, psm_cfg.tau);
	TEST_ASSERT_EQUAL(240, psm_cfg.active_time);

	memset(&psm_cfg, 0, sizeof(psm_cfg));

	err = parse_psm(valid_values_2.active_time,
			valid_values_2.tau_ext,
			valid_values_2.tau_legacy,
			&psm_cfg);
	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_EQUAL(1620, psm_cfg.tau);
	TEST_ASSERT_EQUAL(32, psm_cfg.active_time);
}

void test_encode_psm(void)
{
	int err;
	char tau_ext_str[9] = "";
	char active_time_str[9] = "";

	err = encode_psm(tau_ext_str, active_time_str, 11400, 60);
	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_EQUAL_STRING("00010011", tau_ext_str);
	TEST_ASSERT_EQUAL_STRING("00011110", active_time_str);

	memset(tau_ext_str, 0, sizeof(tau_ext_str));
	memset(active_time_str, 0, sizeof(active_time_str));

	/* Test value -1 */
	err = encode_psm(tau_ext_str, active_time_str, -1, -1);
	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_EQUAL_STRING("", tau_ext_str);
	TEST_ASSERT_EQUAL_STRING("", active_time_str);

	memset(tau_ext_str, 0, sizeof(tau_ext_str));
	memset(active_time_str, 0, sizeof(active_time_str));

	/* Test too big TAU */
	err = encode_psm(tau_ext_str, active_time_str, 35712001, 61);
	TEST_ASSERT_EQUAL(-EINVAL, err);
	TEST_ASSERT_EQUAL_STRING("", tau_ext_str);
	TEST_ASSERT_EQUAL_STRING("00011111", active_time_str);

	memset(tau_ext_str, 0, sizeof(tau_ext_str));
	memset(active_time_str, 0, sizeof(active_time_str));

	/* Test too big active time */
	err = encode_psm(tau_ext_str, active_time_str, 61, 11161);
	TEST_ASSERT_EQUAL(-EINVAL, err);
	TEST_ASSERT_EQUAL_STRING("01111111", tau_ext_str);
	TEST_ASSERT_EQUAL_STRING("", active_time_str);

	memset(tau_ext_str, 0, sizeof(tau_ext_str));
	memset(active_time_str, 0, sizeof(active_time_str));

	/* Test value 1 rounding to 2*/
	err = encode_psm(tau_ext_str, active_time_str, 1, 1);
	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_EQUAL_STRING("01100001", tau_ext_str);
	TEST_ASSERT_EQUAL_STRING("00000001", active_time_str);

	memset(tau_ext_str, 0, sizeof(tau_ext_str));
	memset(active_time_str, 0, sizeof(active_time_str));

	/* Test maximum values */
	err = encode_psm(tau_ext_str, active_time_str, 35712000, 11160);
	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_EQUAL_STRING("11011111", tau_ext_str);
	TEST_ASSERT_EQUAL_STRING("01011111", active_time_str);

	memset(tau_ext_str, 0, sizeof(tau_ext_str));
	memset(active_time_str, 0, sizeof(active_time_str));

	/****************************************/
	err = encode_psm(tau_ext_str, active_time_str, 123456, 89);
	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_EQUAL_STRING("01000100", tau_ext_str);
	TEST_ASSERT_EQUAL_STRING("00100010", active_time_str);

	memset(tau_ext_str, 0, sizeof(tau_ext_str));
	memset(active_time_str, 0, sizeof(active_time_str));

	/****************************************/
	err = encode_psm(tau_ext_str, active_time_str, 62, 63);
	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_EQUAL_STRING("01111111", tau_ext_str);
	TEST_ASSERT_EQUAL_STRING("00100010", active_time_str);

	memset(tau_ext_str, 0, sizeof(tau_ext_str));
	memset(active_time_str, 0, sizeof(active_time_str));
}

void test_parse_mdmev(void)
{
	int err;
	enum lte_lc_modem_evt modem_evt;
	const char *light_search = "%MDMEV: SEARCH STATUS 1\r\n";
	const char *search = "%MDMEV: SEARCH STATUS 2\r\n";
	const char *reset = "%MDMEV: RESET LOOP\r\n";
	const char *battery_low = "%MDMEV: ME BATTERY LOW\r\n";
	const char *overheated = "%MDMEV: ME OVERHEATED\r\n";
	const char *status_3 = "%MDMEV: SEARCH STATUS 3\r\n";
	const char *light_search_long = "%MDMEV: SEARCH STATUS 1 and then some\r\n";
	const char *no_imei = "%MDMEV: NO IMEI\r\n";
	const char *ce_level_0 = "%MDMEV: PRACH CE-LEVEL 0\r\n";
	const char *ce_level_1 = "%MDMEV: PRACH CE-LEVEL 1\r\n";
	const char *ce_level_2 = "%MDMEV: PRACH CE-LEVEL 2\r\n";
	const char *ce_level_3 = "%MDMEV: PRACH CE-LEVEL 3\r\n";
	const char *ce_level_invalid = "%MDMEV: PRACH CE-LEVEL 4\r\n";
	const char *ce_level_long = "%MDMEV: PRACH CE-LEVEL 0 and then some\r\n";

	err = parse_mdmev(light_search, &modem_evt);
	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_EQUAL(LTE_LC_MODEM_EVT_LIGHT_SEARCH_DONE, modem_evt);

	err = parse_mdmev(search, &modem_evt);
	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_EQUAL(LTE_LC_MODEM_EVT_SEARCH_DONE, modem_evt);

	err = parse_mdmev(reset, &modem_evt);
	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_EQUAL(LTE_LC_MODEM_EVT_RESET_LOOP, modem_evt);

	err = parse_mdmev(battery_low, &modem_evt);
	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_EQUAL(LTE_LC_MODEM_EVT_BATTERY_LOW, modem_evt);

	err = parse_mdmev(overheated, &modem_evt);
	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_EQUAL(LTE_LC_MODEM_EVT_OVERHEATED, modem_evt);

	err = parse_mdmev(status_3, &modem_evt);
	TEST_ASSERT_EQUAL(-ENODATA, err);

	err = parse_mdmev(light_search_long, &modem_evt);
	TEST_ASSERT_EQUAL(-ENODATA, err);

	err = parse_mdmev("%MDMEV: ", &modem_evt);
	TEST_ASSERT_EQUAL(-ENODATA, err);

	err = parse_mdmev(no_imei, &modem_evt);
	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_EQUAL(LTE_LC_MODEM_EVT_NO_IMEI, modem_evt);

	err = parse_mdmev(ce_level_0, &modem_evt);
	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_EQUAL(LTE_LC_MODEM_EVT_CE_LEVEL_0, modem_evt);

	err = parse_mdmev(ce_level_1, &modem_evt);
	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_EQUAL(LTE_LC_MODEM_EVT_CE_LEVEL_1, modem_evt);

	err = parse_mdmev(ce_level_2, &modem_evt);
	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_EQUAL(LTE_LC_MODEM_EVT_CE_LEVEL_2, modem_evt);

	err = parse_mdmev(ce_level_3, &modem_evt);
	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_EQUAL(LTE_LC_MODEM_EVT_CE_LEVEL_3, modem_evt);

	err = parse_mdmev(ce_level_invalid, &modem_evt);
	TEST_ASSERT_EQUAL(-ENODATA, err);

	err = parse_mdmev(ce_level_long, &modem_evt);
	TEST_ASSERT_EQUAL(-ENODATA, err);
}

void test_periodic_search_pattern_get(void)
{
	char buf[40] = {0};
	struct lte_lc_periodic_search_pattern pattern_range_1 = {
		.type = LTE_LC_PERIODIC_SEARCH_PATTERN_RANGE,
		.range = {
			.initial_sleep = 60,
			.final_sleep = 3600,
			.time_to_final_sleep = 300,
			.pattern_end_point = 600
		},
	};
	struct lte_lc_periodic_search_pattern pattern_range_2 = {
		.type = LTE_LC_PERIODIC_SEARCH_PATTERN_RANGE,
		.range = {
			.initial_sleep = 60,
			.final_sleep = 3600,
			.time_to_final_sleep = -1,
			.pattern_end_point = 600
		},
	};
	struct lte_lc_periodic_search_pattern pattern_table_1 = {
		.type = LTE_LC_PERIODIC_SEARCH_PATTERN_TABLE,
		.table = {
			.val_1 = 60,
			.val_2 = -1,
			.val_3 = -1,
			.val_4 = -1,
			.val_5 = -1,
		},
	};
	struct lte_lc_periodic_search_pattern pattern_table_2 = {
		.type = LTE_LC_PERIODIC_SEARCH_PATTERN_TABLE,
		.table = {
			.val_1 = 20,
			.val_2 = 80,
			.val_3 = -1,
			.val_4 = -1,
			.val_5 = -1,
		},
	};
	struct lte_lc_periodic_search_pattern pattern_table_3 = {
		.type = LTE_LC_PERIODIC_SEARCH_PATTERN_TABLE,
		.table = {
			.val_1 = 10,
			.val_2 = 70,
			.val_3 = 300,
			.val_4 = -1,
			.val_5 = -1,
		},
	};
	struct lte_lc_periodic_search_pattern pattern_table_4 = {
		.type = LTE_LC_PERIODIC_SEARCH_PATTERN_TABLE,
		.table = {
			.val_1 = 1,
			.val_2 = 60,
			.val_3 = 120,
			.val_4 = 3600,
			.val_5 = -1,
		},
	};
	struct lte_lc_periodic_search_pattern pattern_table_5 = {
		.type = LTE_LC_PERIODIC_SEARCH_PATTERN_TABLE,
		.table = {
			.val_1 = 2,
			.val_2 = 30,
			.val_3 = 40,
			.val_4 = 900,
			.val_5 = 3000,
		},
	};

	TEST_ASSERT_EQUAL_PTR(buf, periodic_search_pattern_get(buf, sizeof(buf), &pattern_range_1));
	TEST_ASSERT_EQUAL_STRING("\"0,60,3600,300,600\"", buf);

	memset(buf, 0, sizeof(buf));

	TEST_ASSERT_EQUAL_PTR(buf, periodic_search_pattern_get(buf, sizeof(buf), &pattern_range_2));
	TEST_ASSERT_EQUAL_STRING("\"0,60,3600,,600\"", buf);

	memset(buf, 0, sizeof(buf));

	TEST_ASSERT_EQUAL_PTR(buf, periodic_search_pattern_get(buf, sizeof(buf), &pattern_table_1));
	TEST_ASSERT_EQUAL_STRING("\"1,60\"", buf);

	memset(buf, 0, sizeof(buf));

	TEST_ASSERT_EQUAL_PTR(buf, periodic_search_pattern_get(buf, sizeof(buf), &pattern_table_2));
	TEST_ASSERT_EQUAL_STRING("\"1,20,80\"", buf);

	memset(buf, 0, sizeof(buf));

	TEST_ASSERT_EQUAL_PTR(buf, periodic_search_pattern_get(buf, sizeof(buf), &pattern_table_3));
	TEST_ASSERT_EQUAL_STRING("\"1,10,70,300\"", buf);

	memset(buf, 0, sizeof(buf));

	TEST_ASSERT_EQUAL_PTR(buf, periodic_search_pattern_get(buf, sizeof(buf), &pattern_table_4));
	TEST_ASSERT_EQUAL_STRING("\"1,1,60,120,3600\"", buf);

	memset(buf, 0, sizeof(buf));

	TEST_ASSERT_EQUAL_PTR(buf, periodic_search_pattern_get(buf, sizeof(buf), &pattern_table_5));
	TEST_ASSERT_EQUAL_STRING("\"1,2,30,40,900,3000\"", buf);
}

void test_parse_periodic_search_pattern(void)
{
	int err;
	struct lte_lc_periodic_search_pattern pattern;
	const char *pattern_range_1 = "0,60,3600,300,600";
	const char *pattern_range_2 = "0,60,3600,,600";
	const char *pattern_range_empty = "0";
	const char *pattern_table_1 = "1,10";
	const char *pattern_table_2 = "1,20,30";
	const char *pattern_table_3 = "1,30,40,50";
	const char *pattern_table_4 = "1,40,50,60,70";
	const char *pattern_table_5 = "1,50,60,70,80,90";
	const char *pattern_table_empty = "1";

	err = parse_periodic_search_pattern(pattern_range_1, &pattern);
	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_EQUAL(LTE_LC_PERIODIC_SEARCH_PATTERN_RANGE, pattern.type);
	TEST_ASSERT_EQUAL(60, pattern.range.initial_sleep);
	TEST_ASSERT_EQUAL(3600, pattern.range.final_sleep);
	TEST_ASSERT_EQUAL(300, pattern.range.time_to_final_sleep);
	TEST_ASSERT_EQUAL(600, pattern.range.pattern_end_point);

	memset(&pattern, 0, sizeof(pattern));

	err = parse_periodic_search_pattern(pattern_range_2, &pattern);
	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_EQUAL(LTE_LC_PERIODIC_SEARCH_PATTERN_RANGE, pattern.type);
	TEST_ASSERT_EQUAL(60, pattern.range.initial_sleep);
	TEST_ASSERT_EQUAL(3600, pattern.range.final_sleep);
	TEST_ASSERT_EQUAL(-1, pattern.range.time_to_final_sleep);
	TEST_ASSERT_EQUAL(600, pattern.range.pattern_end_point);

	memset(&pattern, 0, sizeof(pattern));

	err = parse_periodic_search_pattern(pattern_range_empty, &pattern);
	TEST_ASSERT_EQUAL(-EBADMSG, err);

	memset(&pattern, 0, sizeof(pattern));

	err = parse_periodic_search_pattern(pattern_table_1, &pattern);
	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_EQUAL(LTE_LC_PERIODIC_SEARCH_PATTERN_TABLE, pattern.type);
	TEST_ASSERT_EQUAL(10, pattern.table.val_1);
	TEST_ASSERT_EQUAL(-1, pattern.table.val_2);
	TEST_ASSERT_EQUAL(-1, pattern.table.val_3);
	TEST_ASSERT_EQUAL(-1, pattern.table.val_4);
	TEST_ASSERT_EQUAL(-1, pattern.table.val_5);

	memset(&pattern, 0, sizeof(pattern));

	err = parse_periodic_search_pattern(pattern_table_2, &pattern);
	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_EQUAL(LTE_LC_PERIODIC_SEARCH_PATTERN_TABLE, pattern.type);
	TEST_ASSERT_EQUAL(20, pattern.table.val_1);
	TEST_ASSERT_EQUAL(30, pattern.table.val_2);
	TEST_ASSERT_EQUAL(-1, pattern.table.val_3);
	TEST_ASSERT_EQUAL(-1, pattern.table.val_4);
	TEST_ASSERT_EQUAL(-1, pattern.table.val_5);

	memset(&pattern, 0, sizeof(pattern));

	err = parse_periodic_search_pattern(pattern_table_3, &pattern);
	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_EQUAL(LTE_LC_PERIODIC_SEARCH_PATTERN_TABLE, pattern.type);
	TEST_ASSERT_EQUAL(30, pattern.table.val_1);
	TEST_ASSERT_EQUAL(40, pattern.table.val_2);
	TEST_ASSERT_EQUAL(50, pattern.table.val_3);
	TEST_ASSERT_EQUAL(-1, pattern.table.val_4);
	TEST_ASSERT_EQUAL(-1, pattern.table.val_5);

	memset(&pattern, 0, sizeof(pattern));

	err = parse_periodic_search_pattern(pattern_table_4, &pattern);
	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_EQUAL(LTE_LC_PERIODIC_SEARCH_PATTERN_TABLE, pattern.type);
	TEST_ASSERT_EQUAL(40, pattern.table.val_1);
	TEST_ASSERT_EQUAL(50, pattern.table.val_2);
	TEST_ASSERT_EQUAL(60, pattern.table.val_3);
	TEST_ASSERT_EQUAL(70, pattern.table.val_4);
	TEST_ASSERT_EQUAL(-1, pattern.table.val_5);

	memset(&pattern, 0, sizeof(pattern));

	err = parse_periodic_search_pattern(pattern_table_5, &pattern);
	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_EQUAL(LTE_LC_PERIODIC_SEARCH_PATTERN_TABLE, pattern.type);
	TEST_ASSERT_EQUAL(50, pattern.table.val_1);
	TEST_ASSERT_EQUAL(60, pattern.table.val_2);
	TEST_ASSERT_EQUAL(70, pattern.table.val_3);
	TEST_ASSERT_EQUAL(80, pattern.table.val_4);
	TEST_ASSERT_EQUAL(90, pattern.table.val_5);

	memset(&pattern, 0, sizeof(pattern));

	err = parse_periodic_search_pattern(pattern_table_empty, &pattern);
	TEST_ASSERT_EQUAL(-EBADMSG, err);
}

int main(void)
{
	(void)unity_main();
	return 0;
}
