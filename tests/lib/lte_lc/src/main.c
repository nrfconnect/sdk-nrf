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
	zassert_equal(cfg.mode, LTE_LC_LTE_MODE_LTEM, "Wrong LTE mode");

	err = parse_edrx(at_response_ltem_2, &cfg);
	zassert_equal(0, err, "parse_edrx failed, error: %d", err);
	zassert_within(cfg.edrx, 20.48, 0.1, "Wrong eDRX value");
	zassert_within(cfg.ptw, 19.2, 0.1, "Wrong PTW value");
	zassert_equal(cfg.mode, LTE_LC_LTE_MODE_LTEM, "Wrong LTE mode");

	err = parse_edrx(at_response_nbiot, &cfg);
	zassert_equal(0, err, "parse_edrx failed, error: %d", err);
	zassert_within(cfg.edrx, 2621.44, 0.1, "Wrong eDRX value");
	zassert_within(cfg.ptw, 20.48, 0.1, "Wrong PTW value");
	zassert_equal(cfg.mode, LTE_LC_LTE_MODE_NBIOT, "Wrong LTE mode");

	err = parse_edrx(at_response_nbiot_2, &cfg);
	zassert_equal(0, err, "parse_edrx failed, error: %d", err);
	zassert_within(cfg.edrx, 2621.44, 0.1, "Wrong eDRX value");
	zassert_within(cfg.ptw, 15.36, 0.1, "Wrong PTW value");
	zassert_equal(cfg.mode, LTE_LC_LTE_MODE_NBIOT, "Wrong LTE mode");
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
	zassert_equal(0, err, "parse_xt3412 failed, error: %d", err);
	zassert_equal(time, 360, "Wrong time parameter");

	err = parse_xt3412(at_response_long, &time);
	zassert_equal(0, err, "parse_xt3412 failed, error: %d", err);
	zassert_equal(time, 2147483647, "Wrong time parameter");

	err = parse_xt3412(at_response_t3412_max, &time);
	zassert_equal(0, err, "parse_xt3412 failed, error: %d", err);
	zassert_equal(time, 35712000000, "Wrong time parameter");

	err = parse_xt3412(at_response_t3412_overflow, &time);
	zassert_equal(-EINVAL, err, "parse_xt3412 failed, error: %d", err);

	err = parse_xt3412(at_response_negative, &time);
	zassert_equal(-EINVAL, err, "parse_xt3412 failed, error: %d", err);

	err = parse_xt3412(NULL, &time);
	zassert_equal(-EINVAL, err, "parse_xt3412 failed, error: %d", err);

	err = parse_xt3412(at_response_negative, NULL);
	zassert_equal(-EINVAL, err, "parse_xt3412 failed, error: %d", err);
}

void test_parse_xmodemsleep(void)
{
	int err;
	struct lte_lc_modem_sleep modem_sleep;
	char *at_response_0 = "%XMODEMSLEEP: 1,36000";
	char *at_response_1 = "%XMODEMSLEEP: 1,35712000000";
	char *at_response_2 = "%XMODEMSLEEP: 2,100400";
	char *at_response_3 = "%XMODEMSLEEP: 4";
	char *at_response_4 = "%XMODEMSLEEP: 4,0";

	err = parse_xmodemsleep(at_response_0, &modem_sleep);
	zassert_equal(0, err, "parse_xmodemsleep failed, error: %d", err);
	zassert_equal(modem_sleep.type, 1, "Wrong modem sleep type parameter");
	zassert_equal(modem_sleep.time, 36000, "Wrong modem sleep time parameter");

	err = parse_xmodemsleep(at_response_1, &modem_sleep);
	zassert_equal(0, err, "parse_xmodemsleep failed, error: %d", err);
	zassert_equal(modem_sleep.type, 1, "Wrong modem sleep type parameter");
	zassert_equal(modem_sleep.time, 35712000000, "Wrong modem sleep time parameter");

	err = parse_xmodemsleep(at_response_2, &modem_sleep);
	zassert_equal(0, err, "parse_xmodemsleep failed, error: %d", err);
	zassert_equal(modem_sleep.type, 2, "Wrong modem sleep type parameter");
	zassert_equal(modem_sleep.time, 100400, "Wrong modem sleep time parameter");

	err = parse_xmodemsleep(at_response_3, &modem_sleep);
	zassert_equal(0, err, "parse_xmodemsleep failed, error: %d", err);
	zassert_equal(modem_sleep.type, 4, "Wrong modem sleep type parameter");
	zassert_equal(modem_sleep.time, -1, "Wrong modem sleep time parameter");

	err = parse_xmodemsleep(at_response_4, &modem_sleep);
	zassert_equal(0, err, "parse_xmodemsleep failed, error: %d", err);
	zassert_equal(modem_sleep.type, 4, "Wrong modem sleep type parameter");
	zassert_equal(modem_sleep.time, 0, "Wrong modem sleep time parameter");

	err = parse_xmodemsleep(NULL, &modem_sleep);
	zassert_equal(-EINVAL, err, "parse_xmodemsleep failed, error: %d", err);

	err = parse_xmodemsleep(at_response_0, NULL);
	zassert_equal(-EINVAL, err, "parse_xmodemsleep failed, error: %d", err);
}

void test_parse_coneval(void)
{
	int err;
	struct lte_lc_conn_eval_params params = {0};

	const char *at_response = "%CONEVAL: 0,1,8,41,19,31,\"02026616\",\"24202\","
				  "397,6300,20,0,0,21,1,1,117";
	const char *at_response_no_cell = "%CONEVAL: 1";
	const char *at_response_uicc_unavail = "%CONEVAL: 2";
	const char *at_response_barred_cells = "%CONEVAL: 3";
	const char *at_response_radio_busy = "%CONEVAL: 4";
	const char *at_response_higher_prio = "%CONEVAL: 5";
	const char *at_response_unregistered = "%CONEVAL: 6";
	const char *at_response_unspecified = "%CONEVAL: 7";
	const char *at_response_empty = "";

	err = parse_coneval(at_response, &params);
	zassert_equal(0, err, "parse_coneval failed, error: %d", err);
	zassert_equal(params.rrc_state, LTE_LC_RRC_MODE_CONNECTED, "Wrong RRC state");
	zassert_equal(params.energy_estimate, LTE_LC_ENERGY_CONSUMPTION_REDUCED,
		      "Wrong energy estimate parameter");
	zassert_equal(params.rsrp, 41, "Wrong RSRP");
	zassert_equal(params.rsrq, 19, "Wrong RSRQ");
	zassert_equal(params.snr, 31, "Wrong SNR");
	zassert_equal(params.cell_id, 33711638, "Wrong cell ID");
	zassert_equal(params.mnc, 2, "Wrong MNC");
	zassert_equal(params.mcc, 242, "Wrong MNC");
	zassert_equal(params.phy_cid, 397, "Wrong physical cell ID");
	zassert_equal(params.earfcn, 6300, "Wrong EARFCN");
	zassert_equal(params.band, 20, "Wrong Band");
	zassert_equal(params.tau_trig, 0, "Wrong TAU triggered parameter");
	zassert_equal(params.ce_level, 0, "wrong CE level");
	zassert_equal(params.tx_power, 21, "Wrong TX power");
	zassert_equal(params.tx_rep, 1, "Wrong TX repetition");
	zassert_equal(params.rx_rep, 1, "Wrong RX repetition");
	zassert_equal(params.dl_pathloss, 117, "Wrong download pathloss parameter");

	err = parse_coneval(at_response_no_cell, &params);
	zassert_equal(1, err, "Wrong parse_coneval return value, returns: %d", err);

	err = parse_coneval(at_response_uicc_unavail, &params);
	zassert_equal(2, err, "Wrong parse_coneval return value, returns: %d", err);

	err = parse_coneval(at_response_barred_cells, &params);
	zassert_equal(3, err, "Wrong parse_coneval return value, returns: %d", err);

	err = parse_coneval(at_response_radio_busy, &params);
	zassert_equal(4, err, "Wrong parse_coneval return value, returns: %d", err);

	err = parse_coneval(at_response_higher_prio, &params);
	zassert_equal(5, err, "Wrong parse_coneval return value, returns: %d", err);

	err = parse_coneval(at_response_unregistered, &params);
	zassert_equal(6, err, "Wrong parse_coneval return value, returns: %d", err);

	err = parse_coneval(at_response_unspecified, &params);
	zassert_equal(7, err, "Wrong parse_coneval return value, returns: %d", err);

	err = parse_coneval(at_response_empty, NULL);
	zassert_equal(-EINVAL, err, "Wrong parse_coneval return value, returns: %d", err);

	err = parse_coneval(NULL, &params);
	zassert_equal(-EINVAL, err, "Wrong parse_coneval return value, returns: %d", err);
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

static void test_parse_ncellmeas(void)
{
	int err;
	char *resp1 = "%NCELLMEAS: 0,\"021D140C\",\"24201\",\"0821\",65535,5300,"
			"449,50,15,10891,5300,194,46,8,0,1650,292,60,27,24";
	char *resp2 = "%NCELLMEAS: 1,\"021D140C\",\"24201\",\"0821\",65535,5300";
	char *resp3 = "%NCELLMEAS: 0,\"021D140C\",\"24201\",\"0821\",65535,5300,449,50,15,10891";
	struct lte_lc_ncell ncells[17];
	struct lte_lc_cells_info cells = {
		.neighbor_cells = ncells,
	};

	err = parse_ncellmeas(resp1, &cells);
	zassert_equal(err, 0, "parse_ncellmeas failed, error: %d", err);
	zassert_equal(cells.current_cell.mcc, 242, "Wrong MCC");
	zassert_equal(cells.current_cell.mnc, 1, "Wrong MNC");
	zassert_equal(cells.current_cell.tac, 2081, "Wrong TAC");
	zassert_equal(cells.current_cell.id, 35460108, "Wrong cell ID");
	zassert_equal(cells.current_cell.earfcn, 5300, "Wrong EARFCN");
	zassert_equal(cells.current_cell.timing_advance, 65535, "Wrong timing advance");
	zassert_equal(cells.current_cell.measurement_time, 10891, "Wrong measurement time");
	zassert_equal(cells.current_cell.phys_cell_id, 449, "Wrong physical cell ID");
	zassert_equal(cells.ncells_count, 2, "Wrong neighbor cell count");
	zassert_equal(cells.neighbor_cells[0].earfcn, 5300, "Wrong EARFCN");
	zassert_equal(cells.neighbor_cells[0].phys_cell_id, 194, "Wrong physical cell ID");
	zassert_equal(cells.neighbor_cells[0].rsrp, 46, "Wrong RSRP");
	zassert_equal(cells.neighbor_cells[0].rsrq, 8, "Wrong RSRQ");
	zassert_equal(cells.neighbor_cells[0].time_diff, 0, "Wrong time difference");
	zassert_equal(cells.neighbor_cells[1].earfcn, 1650, "Wrong EARFCN");
	zassert_equal(cells.neighbor_cells[1].phys_cell_id, 292, "Wrong physical cell ID");
	zassert_equal(cells.neighbor_cells[1].rsrp, 60, "Wrong RSRP");
	zassert_equal(cells.neighbor_cells[1].rsrq, 27, "Wrong RSRQ");
	zassert_equal(cells.neighbor_cells[1].time_diff, 24, "Wrong time difference");

	err = parse_ncellmeas(resp2, &cells);
	zassert_equal(err, 1, "parse_ncellmeas was expected to return 1, but returned %d", err);

	err = parse_ncellmeas(resp3, &cells);
	zassert_equal(err, 0, "parse_ncellmeas was expected to return 0, but returned %d", err);
}

static void test_neighborcell_count_get(void)
{
	char *resp1 = "%NCELLMEAS: 1,2,3,4,5,6,7,8,9,10,1,2,3,4,5,1,2,3,4,5,1,2,3,4,5,1,2,3,4,5,"
		      "1,2,3,4,5,1,2,3,4,5,1,2,3,4,5,1,2,3,4,5,1,2,3,4,5,1,2,3,4,5,1,2,3,4,5,"
		      "1,2,3,4,5,1,2,3,4,5,1,2,3,4,5,1,2,3,4,5,1,2,3,4,5,1,2,3,4,5";
	char *resp2 = "%NCELLMEAS: 1,2,3,4,5,6,7,8,9,10,1,2,3,4,5,1,2,3,4,5,1,2,3,4,5,1,2,3,4";
	char *resp3 = "%NCELLMEAS: 1,2,3,4,5,6,7,8,9,10";
	char *resp4 = "%NCELLMEAS: ";
	char *resp5 = " ";

	zassert_equal(17, neighborcell_count_get(resp1), "Wrong neighbor cell count");
	zassert_equal(3, neighborcell_count_get(resp2), "Wrong neighbor cell count");
	zassert_equal(0, neighborcell_count_get(resp3), "Wrong neighbor cell count");
	zassert_equal(0, neighborcell_count_get(resp4), "Wrong neighbor cell count");
	zassert_equal(0, neighborcell_count_get(resp5), "Wrong neighbor cell count");
}

void test_main(void)
{
	ztest_test_suite(test_lte_lc,
		ztest_unit_test(test_parse_edrx),
		ztest_unit_test(test_parse_cereg),
		ztest_unit_test(test_parse_xt3412),
		ztest_unit_test(test_parse_xmodemsleep),
		ztest_unit_test(test_parse_rrc_mode),
		ztest_unit_test(test_response_is_valid),
		ztest_unit_test(test_parse_ncellmeas),
		ztest_unit_test(test_neighborcell_count_get),
		ztest_unit_test(test_parse_coneval)
	);

	ztest_run_test_suite(test_lte_lc);
}
