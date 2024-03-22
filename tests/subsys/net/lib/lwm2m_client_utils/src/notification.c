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

ZTEST_SUITE(lwm2m_client_utils_lte_notification, NULL, NULL, NULL, NULL, NULL);


ZTEST(lwm2m_client_utils_lte_notification, test_ncell_schedule_measurement)
{
	int rc;
	struct lte_lc_evt evt = {0};

	setup();
	lwm2m_ncell_schedule_measurement();
	zassert_equal(lte_lc_neighbor_cell_measurement_fake.call_count, 0,
		      "Cell_measurement call count should be 0");
	rc = lwm2m_ncell_handler_register();
	zassert_equal(rc, 0, "Wrong return value");
	evt.type = LTE_LC_EVT_RRC_UPDATE;
	evt.rrc_mode = LTE_LC_RRC_MODE_CONNECTED;
	call_lte_handlers(&evt);
	zassert_equal(lte_lc_neighbor_cell_measurement_fake.call_count, 0,
		      "No call to lte_lc_neighbor_cell_measurement()");
	evt.rrc_mode = LTE_LC_RRC_MODE_IDLE;
	call_lte_handlers(&evt);
	zassert_equal(lte_lc_neighbor_cell_measurement_fake.call_count, 1,
		      "No call to lte_lc_neighbor_cell_measurement()");
	lwm2m_ncell_schedule_measurement();
}

static struct lwm2m_ctx ctx;
ZTEST(lwm2m_client_utils_lte_notification, test_tau_prewarning)
{
	int rc;
	struct lte_lc_evt evt = {0};

	setup();

	rc = lwm2m_ncell_handler_register();
	zassert_equal(rc, 0, "Wrong return value");
	evt.type = LTE_LC_EVT_TAU_PRE_WARNING;
	call_lte_handlers(&evt);
	zassert_equal(lwm2m_rd_client_update_fake.call_count, 0,
		      "LwM2M RD client update call count should be 0");

	lwm2m_rd_client_ctx_fake.return_val = &ctx;
	call_lte_handlers(&evt);
	zassert_equal(lwm2m_rd_client_update_fake.call_count, 1,
		      "LwM2M RD client not updated");
}

ZTEST(lwm2m_client_utils_lte_notification, test_neighbor_cell_meas)
{
	int rc;
	struct lte_lc_evt evt = {
		.cells_info = cell_info,
	};

	setup();

	rc = lwm2m_ncell_handler_register();
	zassert_equal(rc, 0, "Wrong return value");
	evt.type = LTE_LC_EVT_NEIGHBOR_CELL_MEAS;
	call_lte_handlers(&evt);

	zassert_equal(lwm2m_set_s32_fake.call_count, 15,
		      "Cell info not updated, was %d", lwm2m_set_s32_fake.call_count);
}
