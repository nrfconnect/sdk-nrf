/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <ztest.h>
#include <stdio.h>
#include <string.h>

#include "lte_lc_helpers.h"

static void test_parse_edrx(void)
{
	int err;
	struct lte_lc_edrx_cfg cfg;
	char *at_response_none = "+CEDRXP: 1,\"1000\",\"0101\",\"1011\"";
	char *at_response_ltem = "+CEDRXP: 4,\"1000\",\"0101\",\"1011\"";
	char *at_response_ltem_2 = "+CEDRXP: 4,\"1000\",\"0010\",\"1110\"";
	char *at_response_nbiot = "+CEDRXP: 5,\"1000\",\"1101\",\"0111\"";
	char *at_response_nbiot_2 = "+CEDRXP: 5,\"1000\",\"1101\",\"0101\"";

	err = parse_edrx(at_response_none, &cfg);
	zassert_not_equal(0, err, "parse_edrx should have failed");

	err = parse_edrx(at_response_ltem, &cfg);
	zassert_equal(0, err, "parse_edrx failed, error: %d", err);
	zassert_within(cfg.edrx, 81.92, 0.1, "Wrong eDRX value");
	zassert_within(cfg.ptw, 15.36,  0.1, "Wrong PTW value");

	err = parse_edrx(at_response_ltem_2, &cfg);
	zassert_equal(0, err, "parse_edrx failed, error: %d", err);
	zassert_within(cfg.edrx, 20.48, 0.1, "Wrong eDRX value");
	zassert_within(cfg.ptw, 19.2, 0.1, "Wrong PTW value");

	err = parse_edrx(at_response_nbiot, &cfg);
	zassert_equal(0, err, "parse_edrx failed, error: %d", err);
	zassert_within(cfg.edrx, 2621.44, 0.1, "Wrong eDRX value");
	zassert_within(cfg.ptw, 20.48, 0.1, "Wrong PTW value");

	err = parse_edrx(at_response_nbiot_2, &cfg);
	zassert_equal(0, err, "parse_edrx failed, error: %d", err);
	zassert_within(cfg.edrx, 2621.44, 0.1, "Wrong eDRX value");
	zassert_within(cfg.ptw, 15.36, 0.1, "Wrong PTW value");
}

void test_parse_cereg(void)
{
	int err;
	enum lte_lc_nw_reg_status status;
	enum lte_lc_lte_mode mode;
	struct lte_lc_cell cell;
	struct lte_lc_psm_cfg psm_cfg;
	char *at_response_0 = "+CEREG: 5,0,,,9,0,0,,";
	char *at_response_1 = "+CEREG: 5,1,\"0A0B\",\"01020304\",9,0,0,\"00100110\",\"01011111\"";
	char *at_response_2 = "+CEREG: 5,2,\"0A0B\",\"01020304\",9,0,0,\"11100000\",\"11100000\"";
	char *at_response_3 = "+CEREG: 5,3,\"0A0B\",\"01020304\",9,0,0,,";
	char *at_response_4 = "+CEREG: 5,4,\"0A0B\",\"FFFFFFFF\",9,0,0,,";
	char *at_response_5 = "+CEREG: 5,5,\"0A0B\",\"01020304\",9,0,0,\"11100000\",\"11100000\"";
	char *at_response_8 = "+CEREG: 5,8,\"0A0B\",\"01020304\",9,0,0,,";
	char *at_response_90 = "+CEREG: 5,90,,\"FFFFFFFF\",,,,,";
	char *at_response_wrong = "+CEREG: 5,10,,\"FFFFFFFF\",,,,,";
	char *at_notif_0 = "+CEREG: 0,\"FFFF\",\"FFFFFFFF\",9,0,0,,";
	char *at_notif_1 = "+CEREG: 1,\"0A0B\",\"01020304\",9,0,0,\"00100110\",\"01011111\"";
	char *at_notif_2 = "+CEREG: 2,\"0A0B\",\"01020304\",9,0,0,,";
	char *at_notif_3 = "+CEREG: 3,\"0A0B\",\"01020304\",9,0,0,,";
	char *at_notif_4 = "+CEREG: 4,\"FFFF\",\"FFFFFFFF\",9,0,0,,";
	char *at_notif_5 = "+CEREG: 5,\"0A0B\",\"01020304\",9,0,0,\"11100000\",\"00011111\"";
	char *at_notif_8 = "+CEREG: 8,\"0A0B\",\"01020304\",9,0,0,,";
	char *at_notif_90 = "+CEREG: 90,,\"FFFFFFFF\",,,,,";
	char *at_notif_wrong = "+CEREG: 10,,\"FFFFFFFF\",,,,,";

	/* For CEREG reads, we only check the network status, as that's the only
	 * functionality that is exposed.
	 */
	err = parse_cereg(at_response_0, false, &status, NULL, NULL, NULL);
	zassert_equal(0, err, "parse_cereg failed, error: %d", err);
	zassert_equal(status, LTE_LC_NW_REG_NOT_REGISTERED,
		      "Wrong network registation status");

	err = parse_cereg(at_response_1, false, &status, &cell, &mode, &psm_cfg);
	zassert_equal(0, err, "parse_cereg failed, error: %d", err);
	zassert_equal(status, LTE_LC_NW_REG_REGISTERED_HOME,
		      "Wrong network registation status: %d", status);
	zassert_equal(cell.id, 0x01020304, "Wrong cell ID (0x%08x)", cell.id);
	zassert_equal(cell.tac, 0x0A0B, "Wrong area code");
	zassert_equal(mode, 9, "Wrong LTE mode");
	zassert_equal(psm_cfg.tau, 1116000, "Wrong PSM TAU");
	zassert_equal(psm_cfg.active_time, 360, "Wrong PSM active time");

	err = parse_cereg(at_response_2, false, &status, NULL, NULL, NULL);
	zassert_equal(0, err, "parse_cereg failed, error: %d", err);
	zassert_equal(status, LTE_LC_NW_REG_SEARCHING,
		      "Wrong network registation status");

	err = parse_cereg(at_response_3, false, &status, NULL, NULL, NULL);
	zassert_equal(0, err, "parse_cereg failed, error: %d", err);
	zassert_equal(status, LTE_LC_NW_REG_REGISTRATION_DENIED,
		      "Wrong network registation status");

	err = parse_cereg(at_response_4, false, &status, NULL, NULL, NULL);
	zassert_equal(0, err, "parse_cereg failed, error: %d", err);
	zassert_equal(status, LTE_LC_NW_REG_UNKNOWN,
		      "Wrong network registation status");

	err = parse_cereg(at_response_5, false, &status, NULL, NULL, NULL);
	zassert_equal(0, err, "parse_cereg failed, error: %d", err);
	zassert_equal(status, LTE_LC_NW_REG_REGISTERED_ROAMING,
		      "Wrong network registation status");

	err = parse_cereg(at_response_8, false, &status, NULL, NULL, NULL);
	zassert_equal(0, err, "parse_cereg failed, error: %d", err);
	zassert_equal(status, LTE_LC_NW_REG_REGISTERED_EMERGENCY,
		      "Wrong network registation status");

	err = parse_cereg(at_response_90, false, &status, NULL, NULL, NULL);
	zassert_equal(0, err, "parse_cereg failed, error: %d", err);
	zassert_equal(status, LTE_LC_NW_REG_UICC_FAIL,
		      "Wrong network registation status");

	err = parse_cereg(at_response_wrong, false, &status, NULL, NULL, NULL);
	zassert_not_equal(0, err, "parse_cereg should have failed");

	/* For CEREG notifications, we test the parser function, which
	 * implicitly also tests parse_nw_reg_status() for notifications.
	 */
	err = parse_cereg(at_notif_0, true, &status, &cell, &mode, &psm_cfg);
	zassert_equal(0, err, "parse_cereg failed, true, error: %d", err);
	zassert_equal(status, LTE_LC_NW_REG_NOT_REGISTERED,
		      "Wrong network registation status");

	err = parse_cereg(at_notif_1, true, &status, &cell, &mode, &psm_cfg);
	zassert_equal(0, err, "parse_cereg failed, true, error: %d", err);
	zassert_equal(status, LTE_LC_NW_REG_REGISTERED_HOME,
		      "Wrong network registation status");
	zassert_equal(cell.id, 0x01020304, "Wrong cell ID (0x%08x)", cell.id);
	zassert_equal(cell.tac, 0x0A0B, "Wrong area code");
	zassert_equal(mode, 9, "Wrong LTE mode");
	zassert_equal(psm_cfg.tau, 1116000, "Wrong PSM TAU");
	zassert_equal(psm_cfg.active_time, 360, "Wrong PSM active time");

	err = parse_cereg(at_notif_2, true, &status, &cell, &mode, &psm_cfg);
	zassert_equal(0, err, "parse_cereg failed, true, error: %d", err);
	zassert_equal(status, LTE_LC_NW_REG_SEARCHING,
		      "Wrong network registation status");
	zassert_equal(cell.id, 0x01020304, "Wrong cell ID");
	zassert_equal(cell.tac, 0x0A0B, "Wrong area code");
	zassert_equal(mode, 9, "Wrong LTE mode");

	err = parse_cereg(at_notif_3, true, &status, &cell, &mode, &psm_cfg);
	zassert_equal(0, err, "parse_cereg failed, true, error: %d", err);
	zassert_equal(status, LTE_LC_NW_REG_REGISTRATION_DENIED,
		      "Wrong network registation status");
	zassert_equal(cell.id, 0x01020304, "Wrong cell ID");
	zassert_equal(cell.tac, 0x0A0B, "Wrong area code");
	zassert_equal(mode, 9, "Wrong LTE mode");

	err = parse_cereg(at_notif_4, true, &status, &cell, &mode, &psm_cfg);
	zassert_equal(0, err, "parse_cereg failed, true, error: %d", err);
	zassert_equal(status, LTE_LC_NW_REG_UNKNOWN,
		      "Wrong network registation status");
	zassert_equal(cell.id, 0xFFFFFFFF, "Wrong cell ID");
	zassert_equal(cell.tac, 0xFFFF, "Wrong area code");
	zassert_equal(mode, 9, "Wrong LTE mode");

	err = parse_cereg(at_notif_5, true, &status, &cell, &mode, &psm_cfg);
	zassert_equal(0, err, "parse_cereg failed, true, error: %d", err);
	zassert_equal(status, LTE_LC_NW_REG_REGISTERED_ROAMING,
		      "Wrong network registation status");
	zassert_equal(cell.id, 0x01020304, "Wrong cell ID");
	zassert_equal(cell.tac, 0x0A0B, "Wrong area code");
	zassert_equal(mode, 9, "Wrong LTE mode");
	zassert_equal(psm_cfg.tau, 18600, "Wrong PSM TAU: %d", psm_cfg.tau);
	zassert_equal(psm_cfg.active_time, -1, "Wrong PSM active time: %d",
		      psm_cfg.active_time);

	err = parse_cereg(at_notif_8, true, &status, &cell, &mode, &psm_cfg);
	zassert_equal(0, err, "parse_cereg failed, true, error: %d", err);
	zassert_equal(status, LTE_LC_NW_REG_REGISTERED_EMERGENCY,
		      "Wrong network registation status");
	zassert_equal(cell.id, 0x01020304, "Wrong cell ID");
	zassert_equal(cell.tac, 0x0A0B, "Wrong area code");
	zassert_equal(mode, 9, "Wrong LTE mode");

	err = parse_cereg(at_notif_90, true, &status, &cell, &mode, &psm_cfg);
	zassert_equal(0, err, "parse_cereg failed, true, error: %d", err);
	zassert_equal(status, LTE_LC_NW_REG_UICC_FAIL,
		      "Wrong network registation status");

	err = parse_cereg(at_notif_wrong, true, &status, &cell, &mode, &psm_cfg);
	zassert_not_equal(0, err, "parse_cereg should have true, failed");
}

static void test_parse_rrc_mode(void)
{
	int err;
	enum lte_lc_rrc_mode mode;
	char *at_response_0 = "+CSCON: 0";
	char *at_response_1 = "+CSCON: 1";

	err = parse_rrc_mode(at_response_0, &mode, AT_CSCON_RRC_MODE_INDEX);
	zassert_equal(0, err, "parse_rrc_mode failed, error: %d", err);
	zassert_equal(mode, LTE_LC_RRC_MODE_IDLE, "Wrong RRC mode");

	err = parse_rrc_mode(at_response_1, &mode, AT_CSCON_RRC_MODE_INDEX);
	zassert_equal(0, err, "parse_rrc_mode failed, error: %d", err);
	zassert_equal(mode, LTE_LC_RRC_MODE_CONNECTED, "Wrong RRC mode");

}

static void test_response_is_valid(void)
{
	char *cscon = "+CSCON";
	char *xsystemmode = "+%XSYSTEMMODE";

	zassert_true(response_is_valid(cscon, strlen(cscon), "+CSCON"),
		     "response_is_valid failed");

	zassert_true(response_is_valid(xsystemmode, strlen(xsystemmode), "+%XSYSTEMMODE"),
		     "response_is_valid failed");

	zassert_false(response_is_valid(cscon, strlen(xsystemmode), "+%XSYSTEMMODE"),
		     "response_is_valid should have failed");
}

void test_main(void)
{
	ztest_test_suite(test_lte_lc,
		ztest_unit_test(test_parse_edrx),
		ztest_unit_test(test_parse_cereg),
		ztest_unit_test(test_parse_rrc_mode),
		ztest_unit_test(test_response_is_valid)
	);

	ztest_run_test_suite(test_lte_lc);
}
