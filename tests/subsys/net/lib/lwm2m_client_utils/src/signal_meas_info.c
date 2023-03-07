/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/ztest.h>
#include <zephyr/fff.h>

#include <net/lwm2m_client_utils_location.h>
#include "stubs.h"

static struct lte_lc_ncell cells[] = {
	[0].earfcn = 12345,
	[0].time_diff = 123,
	[0].phys_cell_id = 12345,
	[0].rsrp = -54,
	[0].rsrq = -32,
	[1].earfcn = 23456,
	[1].time_diff = 233,
	[1].phys_cell_id = 23456,
	[1].rsrp = -43,
	[1].rsrq = -21,
	[2].earfcn = 34567,
	[2].time_diff = 345,
	[2].phys_cell_id = 34567,
	[2].rsrp = -32,
	[2].rsrq = -10,
};

static struct lte_lc_cells_info cell_info = {
	.ncells_count = 3,
	.neighbor_cells = cells,
};

static void setup(void)
{
	/* Register resets */
	DO_FOREACH_FAKE(RESET_FAKE);

	/* reset common FFF internal structures */
	FFF_RESET_HISTORY();
}


ZTEST_SUITE(lwm2m_client_utils_signal_meas_info, NULL, NULL, NULL, NULL, NULL);

ZTEST(lwm2m_client_utils_signal_meas_info, test_update_ncells)
{
	int rc;

	setup();

	rc = lwm2m_update_signal_meas_objects(&cell_info);
	zassert_equal(rc, 0, "Error, %d", rc);
	zassert_equal(lwm2m_set_s32_fake.call_count, 15,
		      "Cell info not updated, was %d", lwm2m_set_s32_fake.call_count);

	cell_info.ncells_count = 2;

	rc = lwm2m_update_signal_meas_objects(&cell_info);
	zassert_equal(rc, 0, "Error, %d", rc);
	zassert_equal(lwm2m_set_s32_fake.call_count, 30,
		      "Cell info not updated, was %d", lwm2m_set_s32_fake.call_count);
}
