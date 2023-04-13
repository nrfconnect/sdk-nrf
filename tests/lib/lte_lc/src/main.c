/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <stdio.h>
#include <string.h>

#include "lte_lc_helpers.h"
#include "lte_lc.h"

static void test_parse_edrx(void)
{
	int err;
	struct lte_lc_edrx_cfg cfg;
	char *at_response_none = "+CEDRXP: 0";
	char *at_response_fail = "+CEDRXP: 1,\"1000\",\"0101\",\"1011\"";
	char *at_response_ltem = "+CEDRXP: 4,\"1000\",\"0101\",\"1011\"";
	char *at_response_ltem_2 = "+CEDRXP: 4,\"1000\",\"0010\",\"1110\"";
	char *at_response_nbiot = "+CEDRXP: 5,\"1000\",\"1101\",\"0111\"";
	char *at_response_nbiot_2 = "+CEDRXP: 5,\"1000\",\"1101\",\"0101\"";

	err = parse_edrx(at_response_none, &cfg);
	zassert_equal(0, err, "parse_edrx failed, error: %d", err);
	zassert_equal(cfg.edrx, 0, "Wrong eDRX value");
	zassert_equal(cfg.ptw, 0, "Wrong PTW value");
	zassert_equal(cfg.mode, LTE_LC_LTE_MODE_NONE, "Wrong LTE mode");

	err = parse_edrx(at_response_fail, &cfg);
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
	err = parse_cereg(at_response_0, false, &status, NULL, NULL);
	zassert_equal(0, err, "parse_cereg failed, error: %d", err);
	zassert_equal(status, LTE_LC_NW_REG_NOT_REGISTERED,
		      "Wrong network registation status");

	err = parse_cereg(at_response_1, false, &status, &cell, &mode);
	zassert_equal(0, err, "parse_cereg failed, error: %d", err);
	zassert_equal(status, LTE_LC_NW_REG_REGISTERED_HOME,
		      "Wrong network registation status: %d", status);
	zassert_equal(cell.id, 0x01020304, "Wrong cell ID (0x%08x)", cell.id);
	zassert_equal(cell.tac, 0x0A0B, "Wrong area code");

	err = parse_cereg(at_response_2, false, &status, NULL, NULL);
	zassert_equal(0, err, "parse_cereg failed, error: %d", err);
	zassert_equal(status, LTE_LC_NW_REG_SEARCHING,
		      "Wrong network registation status");

	err = parse_cereg(at_response_3, false, &status, NULL, NULL);
	zassert_equal(0, err, "parse_cereg failed, error: %d", err);
	zassert_equal(status, LTE_LC_NW_REG_REGISTRATION_DENIED,
		      "Wrong network registation status");

	err = parse_cereg(at_response_4, false, &status, NULL, NULL);
	zassert_equal(0, err, "parse_cereg failed, error: %d", err);
	zassert_equal(status, LTE_LC_NW_REG_UNKNOWN,
		      "Wrong network registation status");

	err = parse_cereg(at_response_5, false, &status, NULL, NULL);
	zassert_equal(0, err, "parse_cereg failed, error: %d", err);
	zassert_equal(status, LTE_LC_NW_REG_REGISTERED_ROAMING,
		      "Wrong network registation status");

	err = parse_cereg(at_response_8, false, &status, NULL, NULL);
	zassert_equal(0, err, "parse_cereg failed, error: %d", err);
	zassert_equal(status, LTE_LC_NW_REG_REGISTERED_EMERGENCY,
		      "Wrong network registation status");

	err = parse_cereg(at_response_90, false, &status, NULL, NULL);
	zassert_equal(0, err, "parse_cereg failed, error: %d", err);
	zassert_equal(status, LTE_LC_NW_REG_UICC_FAIL,
		      "Wrong network registation status");

	err = parse_cereg(at_response_wrong, false, &status, NULL, NULL);
	zassert_not_equal(0, err, "parse_cereg should have failed");

	/* For CEREG notifications, we test the parser function, which
	 * implicitly also tests parse_nw_reg_status() for notifications.
	 */
	err = parse_cereg(at_notif_0, true, &status, &cell, &mode);
	zassert_equal(0, err, "parse_cereg failed, true, error: %d", err);
	zassert_equal(status, LTE_LC_NW_REG_NOT_REGISTERED,
		      "Wrong network registation status");

	err = parse_cereg(at_notif_1, true, &status, &cell, &mode);
	zassert_equal(0, err, "parse_cereg failed, true, error: %d", err);
	zassert_equal(status, LTE_LC_NW_REG_REGISTERED_HOME,
		      "Wrong network registation status");
	zassert_equal(cell.id, 0x01020304, "Wrong cell ID (0x%08x)", cell.id);
	zassert_equal(cell.tac, 0x0A0B, "Wrong area code");
	zassert_equal(mode, 9, "Wrong LTE mode");

	err = parse_cereg(at_notif_2, true, &status, &cell, &mode);
	zassert_equal(0, err, "parse_cereg failed, true, error: %d", err);
	zassert_equal(status, LTE_LC_NW_REG_SEARCHING,
		      "Wrong network registation status");
	zassert_equal(cell.id, 0x01020304, "Wrong cell ID");
	zassert_equal(cell.tac, 0x0A0B, "Wrong area code");
	zassert_equal(mode, 9, "Wrong LTE mode");

	err = parse_cereg(at_notif_3, true, &status, &cell, &mode);
	zassert_equal(0, err, "parse_cereg failed, true, error: %d", err);
	zassert_equal(status, LTE_LC_NW_REG_REGISTRATION_DENIED,
		      "Wrong network registation status");
	zassert_equal(cell.id, 0x01020304, "Wrong cell ID");
	zassert_equal(cell.tac, 0x0A0B, "Wrong area code");
	zassert_equal(mode, 9, "Wrong LTE mode");

	err = parse_cereg(at_notif_4, true, &status, &cell, &mode);
	zassert_equal(0, err, "parse_cereg failed, true, error: %d", err);
	zassert_equal(status, LTE_LC_NW_REG_UNKNOWN,
		      "Wrong network registation status");
	zassert_equal(cell.id, 0xFFFFFFFF, "Wrong cell ID");
	zassert_equal(cell.tac, 0xFFFF, "Wrong area code");
	zassert_equal(mode, 9, "Wrong LTE mode");

	err = parse_cereg(at_notif_5, true, &status, &cell, &mode);
	zassert_equal(0, err, "parse_cereg failed, true, error: %d", err);
	zassert_equal(status, LTE_LC_NW_REG_REGISTERED_ROAMING,
		      "Wrong network registation status");
	zassert_equal(cell.id, 0x01020304, "Wrong cell ID");
	zassert_equal(cell.tac, 0x0A0B, "Wrong area code");
	zassert_equal(mode, 9, "Wrong LTE mode");

	err = parse_cereg(at_notif_8, true, &status, &cell, &mode);
	zassert_equal(0, err, "parse_cereg failed, true, error: %d", err);
	zassert_equal(status, LTE_LC_NW_REG_REGISTERED_EMERGENCY,
		      "Wrong network registation status");
	zassert_equal(cell.id, 0x01020304, "Wrong cell ID");
	zassert_equal(cell.tac, 0x0A0B, "Wrong area code");
	zassert_equal(mode, 9, "Wrong LTE mode");

	err = parse_cereg(at_notif_90, true, &status, &cell, &mode);
	zassert_equal(0, err, "parse_cereg failed, true, error: %d", err);
	zassert_equal(status, LTE_LC_NW_REG_UICC_FAIL,
		      "Wrong network registation status");

	err = parse_cereg(at_notif_wrong, true, &status, &cell, &mode);
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

	zassert_false(response_is_valid(cscon, strlen(cscon), "+%XSYSTEMMODE"),
		     "response_is_valid should have failed");
}

static void test_parse_ncellmeas(void)
{
	int err;
	char *resp1 = "%NCELLMEAS: 0,\"021D140C\",\"24201\",\"0821\",65535,5300,"
			"449,50,15,10891,5300,194,46,8,0,1650,292,60,27,24,8061152878017748";
	char *resp2 = "%NCELLMEAS: 1,\"021D140C\",\"24201\",\"0821\",65535,5300,50000";
	char *resp3 =
		"%NCELLMEAS: 0,\"071D340C\",\"24201\",\"0821\",65535,5300,449,50,15,10891,655350";
	char *resp4 = "%NCELLMEAS: 0,\"071D340C\",\"24202\",\"0762\",65535,5300,449,50,15,10871";
	struct lte_lc_ncell ncells[17];
	struct lte_lc_cells_info cells = {
		.neighbor_cells = ncells,
	};

	/* Valid response with two neighbors and timing advance measurement time. */
	err = parse_ncellmeas(resp1, &cells);
	zassert_equal(err, 0, "parse_ncellmeas failed, error: %d", err);
	zassert_equal(cells.current_cell.mcc, 242, "Wrong MCC");
	zassert_equal(cells.current_cell.mnc, 1, "Wrong MNC");
	zassert_equal(cells.current_cell.tac, 2081, "Wrong TAC");
	zassert_equal(cells.current_cell.id, 35460108, "Wrong cell ID");
	zassert_equal(cells.current_cell.earfcn, 5300, "Wrong EARFCN");
	zassert_equal(cells.current_cell.timing_advance, 65535, "Wrong timing advance");
	zassert_equal(cells.current_cell.timing_advance_meas_time, 8061152878017748,
		      "Wrong timing advance measurement time");
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

	memset(&cells, 0, sizeof(cells));

	/* Valid response of failed measurement. */
	err = parse_ncellmeas(resp2, &cells);
	zassert_equal(err, 1, "parse_ncellmeas was expected to return 1, but returned %d", err);
	zassert_equal(cells.current_cell.id, LTE_LC_CELL_EUTRAN_ID_INVALID, "Wrong cell ID");
	zassert_equal(cells.ncells_count, 0, "Wrong neighbor cell count");

	memset(&cells, 0, sizeof(cells));

	/* Valid response with timing advance measurement time. */
	err = parse_ncellmeas(resp3, &cells);
	zassert_equal(err, 0, "parse_ncellmeas was expected to return 0, but returned %d", err);
	zassert_equal(cells.current_cell.mcc, 242, "Wrong MCC");
	zassert_equal(cells.current_cell.mnc, 1, "Wrong MNC");
	zassert_equal(cells.current_cell.tac, 2081, "Wrong TAC");
	zassert_equal(cells.current_cell.id, 119354380, "Wrong cell ID");
	zassert_equal(cells.current_cell.earfcn, 5300, "Wrong EARFCN");
	zassert_equal(cells.current_cell.timing_advance, 65535, "Wrong timing advance");
	zassert_equal(cells.current_cell.timing_advance_meas_time, 655350,
		      "Wrong timing advance measurement time");
	zassert_equal(cells.current_cell.measurement_time, 10891, "Wrong measurement time");
	zassert_equal(cells.current_cell.phys_cell_id, 449, "Wrong physical cell ID");
	zassert_equal(cells.ncells_count, 0, "Wrong neighbor cell count");

	memset(&cells, 0, sizeof(cells));

	/* Valid response without timing advance measurement time. */
	err = parse_ncellmeas(resp4, &cells);
	zassert_equal(err, 0, "parse_ncellmeas was expected to return 0, but returned %d", err);
	zassert_equal(cells.current_cell.mcc, 242, "Wrong MCC");
	zassert_equal(cells.current_cell.mnc, 2, "Wrong MNC");
	zassert_equal(cells.current_cell.tac, 1890, "Wrong TAC");
	zassert_equal(cells.current_cell.id, 119354380, "Wrong cell ID");
	zassert_equal(cells.current_cell.earfcn, 5300, "Wrong EARFCN");
	zassert_equal(cells.current_cell.timing_advance, 65535, "Wrong timing advance");
	zassert_equal(cells.current_cell.timing_advance_meas_time, 0,
		      "Wrong timing advance measurement time");
	zassert_equal(cells.current_cell.measurement_time, 10871, "Wrong measurement time");
	zassert_equal(cells.current_cell.phys_cell_id, 449, "Wrong physical cell ID");
	zassert_equal(cells.ncells_count, 0, "Wrong neighbor cell count");
}

static void test_neighborcell_count_get(void)
{
	char *resp1 = "%NCELLMEAS: 1,2,3,4,5,6,7,8,9,10,1,2,3,4,5,1,2,3,4,5,1,2,3,4,5,1,2,3,4,5,"
		      "1,2,3,4,5,1,2,3,4,5,1,2,3,4,5,1,2,3,4,5,1,2,3,4,5,1,2,3,4,5,1,2,3,4,5,"
		      "1,2,3,4,5,1,2,3,4,5,1,2,3,4,5,1,2,3,4,5,1,2,3,4,5,1,2,3,4,5,1000";
	char *resp2 = "%NCELLMEAS: 1,2,3,4,5,6,7,8,9,10,1,2,3,4,5,1,2,3,4,5,1,2,3,4,5,1,2,3,1000";
	char *resp3 = "%NCELLMEAS: 1,2,3,4,5,6,7,8,9,10,1000";
	char *resp4 = "%NCELLMEAS: ";
	char *resp5 = " ";

	zassert_equal(17, neighborcell_count_get(resp1), "Wrong neighbor cell count");
	zassert_equal(3, neighborcell_count_get(resp2), "Wrong neighbor cell count");
	zassert_equal(0, neighborcell_count_get(resp3), "Wrong neighbor cell count");
	zassert_equal(0, neighborcell_count_get(resp4), "Wrong neighbor cell count");
	zassert_equal(0, neighborcell_count_get(resp5), "Wrong neighbor cell count");
}

static void test_parse_psm(void)
{
	int err;
	struct lte_lc_psm_cfg psm_cfg;
	struct psm_strings {
		char *active_time;
		char *tau_ext;
		char *tau_legacy;
	};
	struct psm_strings disabled = {
		.active_time = "11100000",
		.tau_ext = "11100000",
		.tau_legacy = "11100000",
	};
	struct psm_strings tau_legacy = {
		.active_time = "00001000",
		.tau_ext = "11100000",
		.tau_legacy = "00101010",
	};
	struct psm_strings no_tau_legacy = {
		.active_time = "00001000",
		.tau_ext = "11100000",
	};
	struct psm_strings invalid_values = {
		.active_time = "0001111111",
		.tau_ext = "00001",
	};
	struct psm_strings valid_values_0 = {
		.active_time = "01000001",
		.tau_ext = "01000010",
	};
	struct psm_strings valid_values_1 = {
		.active_time = "00100100",
		.tau_ext = "00001000",
	};
	struct psm_strings valid_values_2 = {
		.active_time = "00010000",
		.tau_ext = "10111011",
	};

	err = parse_psm(disabled.active_time, disabled.tau_ext, disabled.tau_legacy, &psm_cfg);
	zassert_equal(err, 0, "parse_psm was expected to return 0, but returned %d", err);
	zassert_equal(psm_cfg.tau, -1, "Wrong PSM TAU (%d)", psm_cfg.tau);
	zassert_equal(psm_cfg.active_time, -1, "Wrong PSM active time (%d)", psm_cfg.active_time);

	memset(&psm_cfg, 0, sizeof(psm_cfg));

	err = parse_psm(tau_legacy.active_time, tau_legacy.tau_ext, tau_legacy.tau_legacy,
			&psm_cfg);
	zassert_equal(err, 0, "parse_psm was expected to return 0, but returned %d", err);
	zassert_equal(psm_cfg.tau, 600, "Wrong PSM TAU (%d)", psm_cfg.tau);
	zassert_equal(psm_cfg.active_time, 16, "Wrong PSM active time (%d)", psm_cfg.active_time);

	memset(&psm_cfg, 0, sizeof(psm_cfg));

	err = parse_psm(no_tau_legacy.active_time, no_tau_legacy.tau_ext, NULL, &psm_cfg);
	zassert_equal(err, 0, "parse_psm was expected to return 0, but returned %d", err);
	zassert_equal(psm_cfg.tau, -1, "Wrong PSM TAU (%d)", psm_cfg.tau);
	zassert_equal(psm_cfg.active_time, 16, "Wrong PSM active time (%d)", psm_cfg.active_time);

	memset(&psm_cfg, 0, sizeof(psm_cfg));

	err = parse_psm(invalid_values.active_time, invalid_values.tau_ext, NULL, &psm_cfg);
	zassert_equal(err, -EINVAL, "parse_psm was expected to return -EINVAL, but returned %d",
		      err);

	memset(&psm_cfg, 0, sizeof(psm_cfg));

	err = parse_psm(valid_values_0.active_time, valid_values_0.tau_ext, NULL, &psm_cfg);
	zassert_equal(err, 0, "parse_psm was expected to return 0, but returned %d", err);
	zassert_equal(psm_cfg.tau, 72000, "Wrong PSM TAU (%d)", psm_cfg.tau);
	zassert_equal(psm_cfg.active_time, 360, "Wrong PSM active time (%d)", psm_cfg.active_time);

	memset(&psm_cfg, 0, sizeof(psm_cfg));

	err = parse_psm(valid_values_1.active_time, valid_values_1.tau_ext, NULL, &psm_cfg);
	zassert_equal(err, 0, "parse_psm was expected to return 0, but returned %d", err);
	zassert_equal(psm_cfg.tau, 4800, "Wrong PSM TAU (%d)", psm_cfg.tau);
	zassert_equal(psm_cfg.active_time, 240, "Wrong PSM active time (%d)", psm_cfg.active_time);

	memset(&psm_cfg, 0, sizeof(psm_cfg));

	err = parse_psm(valid_values_2.active_time, valid_values_2.tau_ext, NULL, &psm_cfg);
	zassert_equal(err, 0, "parse_psm was expected to return 0, but returned %d", err);
	zassert_equal(psm_cfg.tau, 1620, "Wrong PSM TAU (%d)", psm_cfg.tau);
	zassert_equal(psm_cfg.active_time, 32, "Wrong PSM active time (%d)", psm_cfg.active_time);
}

static void test_parse_mdmev(void)
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

	err = parse_mdmev(light_search, &modem_evt);
	zassert_equal(err, 0, "parse_mdmev was expected to return 0, but returned %d", err);
	zassert_equal(modem_evt, LTE_LC_MODEM_EVT_LIGHT_SEARCH_DONE,
		"Wrong modem event type: %d, expected: %d", modem_evt,
		LTE_LC_MODEM_EVT_LIGHT_SEARCH_DONE);
	err = parse_mdmev(search, &modem_evt);
	zassert_equal(err, 0, "parse_mdmev was expected to return 0, but returned %d", err);
	zassert_equal(modem_evt, LTE_LC_MODEM_EVT_SEARCH_DONE,
		"Wrong modem event type: %d, expected: %d", modem_evt,
		LTE_LC_MODEM_EVT_SEARCH_DONE);

	err = parse_mdmev(reset, &modem_evt);
	zassert_equal(err, 0, "parse_mdmev was expected to return 0, but returned %d", err);
	zassert_equal(modem_evt, LTE_LC_MODEM_EVT_RESET_LOOP,
		"Wrong modem event type: %d, expected: %d", modem_evt,
		LTE_LC_MODEM_EVT_RESET_LOOP);

	err = parse_mdmev(battery_low, &modem_evt);
	zassert_equal(err, 0, "parse_mdmev was expected to return 0, but returned %d", err);
	zassert_equal(modem_evt, LTE_LC_MODEM_EVT_BATTERY_LOW,
		"Wrong modem event type: %d, expected: %d", modem_evt,
		LTE_LC_MODEM_EVT_BATTERY_LOW);

	err = parse_mdmev(overheated, &modem_evt);
	zassert_equal(err, 0, "parse_mdmev was expected to return 0, but returned %d", err);
	zassert_equal(modem_evt, LTE_LC_MODEM_EVT_OVERHEATED,
		"Wrong modem event type: %d, expected: %d", modem_evt,
		LTE_LC_MODEM_EVT_OVERHEATED);

	err = parse_mdmev(status_3, &modem_evt);
	zassert_equal(err, -ENODATA, "parse_mdmev was expected to return %d, but returned %d",
		     -ENODATA, err);

	err = parse_mdmev(search, NULL);
	zassert_equal(err, -EINVAL, "parse_mdmev was expected to return %d, but returned %d",
		      -EINVAL, err);

	err = parse_mdmev(light_search_long, &modem_evt);
	zassert_equal(err, -ENODATA, "parse_mdmev was expected to return %d, but returned %d",
		      -ENODATA, err);

	err = parse_mdmev("%MDMEVE", &modem_evt);
	zassert_equal(err, -EIO, "parse_mdmev was expected to return %d, but returned %d",
		      -EIO, err);

	err = parse_mdmev("%MDME", &modem_evt);
	zassert_equal(err, -EIO, "parse_mdmev was expected to return %d, but returned %d",
		      -EIO, err);

	err = parse_mdmev("%MDMEV: ", &modem_evt);
	zassert_equal(err, -ENODATA, "parse_mdmev was expected to return %d, but returned %d",
		      -ENODATA, err);
}

static void test_periodic_search_pattern_get(void)
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

	zassert_equal_ptr(buf, periodic_search_pattern_get(buf, sizeof(buf), &pattern_range_1),
			  "Unexpected pointer returned from periodic_search_pattern_get()");
	zassert_equal(strcmp(buf, "\"0,60,3600,300,600\""), 0, "Wrong range string");

	memset(buf, 0, sizeof(buf));

	zassert_equal_ptr(buf, periodic_search_pattern_get(buf, sizeof(buf), &pattern_range_2),
			  "Unexpected pointer returned from periodic_search_pattern_get()");
	zassert_equal(strcmp(buf, "\"0,60,3600,,600\""), 0, "Wrong range string");

	memset(buf, 0, sizeof(buf));

	zassert_equal_ptr(buf, periodic_search_pattern_get(buf, sizeof(buf), &pattern_table_1),
			  "Unexpected pointer returned from periodic_search_pattern_get()");
	zassert_equal(strcmp(buf, "\"1,60\""), 0, "Wrong table string (%s)", buf);

	memset(buf, 0, sizeof(buf));

	zassert_equal_ptr(buf, periodic_search_pattern_get(buf, sizeof(buf), &pattern_table_2),
			  "Unexpected pointer returned from periodic_search_pattern_get()");
	zassert_equal(strcmp(buf, "\"1,20,80\""), 0, "Wrong table string (%s)", buf);

	memset(buf, 0, sizeof(buf));

	zassert_equal_ptr(buf, periodic_search_pattern_get(buf, sizeof(buf), &pattern_table_3),
			  "Unexpected pointer returned from periodic_search_pattern_get()");
	zassert_equal(strcmp(buf, "\"1,10,70,300\""), 0, "Wrong table string (%s)", buf);

	memset(buf, 0, sizeof(buf));

	zassert_equal_ptr(buf, periodic_search_pattern_get(buf, sizeof(buf), &pattern_table_4),
			  "Unexpected pointer returned from periodic_search_pattern_get()");
	zassert_equal(strcmp(buf, "\"1,1,60,120,3600\""), 0, "Wrong table string (%s)", buf);

	memset(buf, 0, sizeof(buf));

	zassert_equal_ptr(buf, periodic_search_pattern_get(buf, sizeof(buf), &pattern_table_5),
			  "Unexpected pointer returned from periodic_search_pattern_get()");
	zassert_equal(strcmp(buf, "\"1,2,30,40,900,3000\""), 0, "Wrong table string (%s)", buf);
}

static void test_parse_periodic_search_pattern(void)
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
	zassert_equal(err, 0, "Expected 0, but %d was returned", err);
	zassert_equal(pattern.type, LTE_LC_PERIODIC_SEARCH_PATTERN_RANGE,
		      "Wrong pattern type (%d)", pattern.type);
	zassert_equal(pattern.range.initial_sleep, 60,
		      "Wrong 'initial_sleep' value (%d)", pattern.range.initial_sleep);
	zassert_equal(pattern.range.final_sleep, 3600,
		      "Wrong 'final_sleep' value (%d)", pattern.range.final_sleep);
	zassert_equal(pattern.range.time_to_final_sleep, 300,
		      "Wrong 'time_to_final_sleep' value (%d)", pattern.range.time_to_final_sleep);
	zassert_equal(pattern.range.pattern_end_point, 600,
		      "Wrong 'pattern_end_point' value (%d)", pattern.range.pattern_end_point);

	memset(&pattern, 0, sizeof(pattern));

	err = parse_periodic_search_pattern(pattern_range_2, &pattern);
	zassert_equal(err, 0, "Expected 0, but %d was returned", err);
	zassert_equal(pattern.type, LTE_LC_PERIODIC_SEARCH_PATTERN_RANGE,
		      "Wrong pattern type (%d)", pattern.type);
	zassert_equal(pattern.range.initial_sleep, 60,
		      "Wrong 'initial_sleep' value (%d)", pattern.range.initial_sleep);
	zassert_equal(pattern.range.final_sleep, 3600,
		      "Wrong 'final_sleep' value (%d)", pattern.range.final_sleep);
	zassert_equal(pattern.range.time_to_final_sleep, -1,
		      "Wrong 'time_to_final_sleep' value (%d)", pattern.range.time_to_final_sleep);
	zassert_equal(pattern.range.pattern_end_point, 600,
		      "Wrong 'pattern_end_point' value (%d)", pattern.range.pattern_end_point);

	memset(&pattern, 0, sizeof(pattern));

	err = parse_periodic_search_pattern(pattern_range_empty, &pattern);
	zassert_equal(err, -EBADMSG, "Expected %d, but %d was returned", -EBADMSG, err);

	memset(&pattern, 0, sizeof(pattern));

	err = parse_periodic_search_pattern(pattern_table_1, &pattern);
	zassert_equal(err, 0, "Expected 0, but %d was returned", err);
	zassert_equal(pattern.type, LTE_LC_PERIODIC_SEARCH_PATTERN_TABLE,
		      "Wrong pattern type (%d)", pattern.type);
	zassert_equal(pattern.table.val_1, 10,
		      "Wrong 'val_1' value (%d)", pattern.table.val_1);
	zassert_equal(pattern.table.val_2, -1,
		      "Wrong 'val_2' value (%d)", pattern.table.val_2);
	zassert_equal(pattern.table.val_3, -1,
		      "Wrong 'val_3' value (%d)", pattern.table.val_3);
	zassert_equal(pattern.table.val_4, -1,
		      "Wrong 'val_4' value (%d)", pattern.table.val_4);
	zassert_equal(pattern.table.val_5, -1,
		      "Wrong 'val_5' value (%d)", pattern.table.val_5);

	memset(&pattern, 0, sizeof(pattern));

	err = parse_periodic_search_pattern(pattern_table_2, &pattern);
	zassert_equal(err, 0, "Expected 0, but %d was returned", err);
	zassert_equal(pattern.type, LTE_LC_PERIODIC_SEARCH_PATTERN_TABLE,
		      "Wrong pattern type (%d)", pattern.type);
	zassert_equal(pattern.table.val_1, 20,
		      "Wrong 'val_1' value (%d)", pattern.table.val_1);
	zassert_equal(pattern.table.val_2, 30,
		      "Wrong 'val_2' value (%d)", pattern.table.val_2);
	zassert_equal(pattern.table.val_3, -1,
		      "Wrong 'val_3' value (%d)", pattern.table.val_3);
	zassert_equal(pattern.table.val_4, -1,
		      "Wrong 'val_4' value (%d)", pattern.table.val_4);
	zassert_equal(pattern.table.val_5, -1,
		      "Wrong 'val_5' value (%d)", pattern.table.val_5);

	memset(&pattern, 0, sizeof(pattern));

	err = parse_periodic_search_pattern(pattern_table_3, &pattern);
	zassert_equal(err, 0, "Expected 0, but %d was returned", err);
	zassert_equal(pattern.type, LTE_LC_PERIODIC_SEARCH_PATTERN_TABLE,
		      "Wrong pattern type (%d)", pattern.type);
	zassert_equal(pattern.table.val_1, 30,
		      "Wrong 'val_1' value (%d)", pattern.table.val_1);
	zassert_equal(pattern.table.val_2, 40,
		      "Wrong 'val_2' value (%d)", pattern.table.val_2);
	zassert_equal(pattern.table.val_3, 50,
		      "Wrong 'val_3' value (%d)", pattern.table.val_3);
	zassert_equal(pattern.table.val_4, -1,
		      "Wrong 'val_4' value (%d)", pattern.table.val_4);
	zassert_equal(pattern.table.val_5, -1,
		      "Wrong 'val_5' value (%d)", pattern.table.val_5);

	memset(&pattern, 0, sizeof(pattern));

	err = parse_periodic_search_pattern(pattern_table_4, &pattern);
	zassert_equal(err, 0, "Expected 0, but %d was returned", err);
	zassert_equal(pattern.type, LTE_LC_PERIODIC_SEARCH_PATTERN_TABLE,
		      "Wrong pattern type (%d)", pattern.type);
	zassert_equal(pattern.table.val_1, 40,
		      "Wrong 'val_1' value (%d)", pattern.table.val_1);
	zassert_equal(pattern.table.val_2, 50,
		      "Wrong 'val_2' value (%d)", pattern.table.val_2);
	zassert_equal(pattern.table.val_3, 60,
		      "Wrong 'val_3' value (%d)", pattern.table.val_3);
	zassert_equal(pattern.table.val_4, 70,
		      "Wrong 'val_4' value (%d)", pattern.table.val_4);
	zassert_equal(pattern.table.val_5, -1,
		      "Wrong 'val_5' value (%d)", pattern.table.val_5);

	memset(&pattern, 0, sizeof(pattern));

	err = parse_periodic_search_pattern(pattern_table_5, &pattern);
	zassert_equal(err, 0, "Expected 0, but %d was returned", err);
	zassert_equal(pattern.type, LTE_LC_PERIODIC_SEARCH_PATTERN_TABLE,
		      "Wrong pattern type (%d)", pattern.type);
	zassert_equal(pattern.table.val_1, 50,
		      "Wrong 'val_1' value (%d)", pattern.table.val_1);
	zassert_equal(pattern.table.val_2, 60,
		      "Wrong 'val_2' value (%d)", pattern.table.val_2);
	zassert_equal(pattern.table.val_3, 70,
		      "Wrong 'val_3' value (%d)", pattern.table.val_3);
	zassert_equal(pattern.table.val_4, 80,
		      "Wrong 'val_4' value (%d)", pattern.table.val_4);
	zassert_equal(pattern.table.val_5, 90,
		      "Wrong 'val_5' value (%d)", pattern.table.val_5);

	memset(&pattern, 0, sizeof(pattern));

	err = parse_periodic_search_pattern(pattern_table_empty, &pattern);
	zassert_equal(err, -EBADMSG, "Expected %d, but %d was returned", -EBADMSG, err);
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
		ztest_unit_test(test_parse_mdmev),
		ztest_unit_test(test_parse_psm),
		ztest_unit_test(test_periodic_search_pattern_get),
		ztest_unit_test(test_parse_periodic_search_pattern)
	);

	ztest_run_test_suite(test_lte_lc);
}
