/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <zephyr/fff.h>

#include <net/lwm2m_client_utils.h>
#include <modem/lte_lc.h>
#include "stubs.h"
#include "lwm2m_object.h"
#include "lwm2m_engine.h"

static int copy_uri_write_cb(const struct lwm2m_obj_path *path, lwm2m_engine_set_data_cb_t cb);

static lwm2m_engine_set_data_cb_t set_uri_cb;
static lwm2m_engine_set_data_cb_t set_uri_cb_psm;
static lwm2m_engine_set_data_cb_t set_uri_cb_active_time;
static lwm2m_engine_set_data_cb_t set_uri_cb_edrx;
static lwm2m_engine_set_data_cb_t set_uri_cb_psm_update;
static lwm2m_engine_set_data_cb_t set_uri_cb_rai;

static void setup(void)
{
	/* Register resets */
	DO_FOREACH_FAKE(RESET_FAKE);

	/* reset common FFF internal structures */
	FFF_RESET_HISTORY();

	lwm2m_register_post_write_callback_fake.custom_fake = copy_uri_write_cb;
	call_lwm2m_init_callbacks();
}

static int copy_uri_write_cb(const struct lwm2m_obj_path *path, lwm2m_engine_set_data_cb_t cb)
{
	if (path->obj_id == 10 && path->obj_inst_id == 0 && path->res_id == 1) {
		set_uri_cb = cb;
	} else if (path->obj_id == 10 && path->obj_inst_id == 0 && path->res_id == 4) {
		set_uri_cb_psm = cb;
	} else if (path->obj_id == 10 && path->obj_inst_id == 0 && path->res_id == 5) {
		set_uri_cb_active_time = cb;
	} else if (path->obj_id == 10 && path->obj_inst_id == 0 && path->res_id == 8) {
		set_uri_cb_edrx = cb;
	} else if (path->obj_id == 10 && path->obj_inst_id == 0 && path->res_id == 13) {
		set_uri_cb_psm_update = cb;
	} else if (path->obj_id == 10 && path->obj_inst_id == 0 && path->res_id == 14) {
		set_uri_cb_rai = cb;
	}

	return 0;
}

ZTEST_SUITE(lwm2m_client_utils_cellconn, NULL, NULL, NULL, NULL, NULL);

ZTEST(lwm2m_client_utils_cellconn, test_disable_radio_period_cb)
{
	int rc;
	uint16_t period = 1;

	setup();

	rc = set_uri_cb(10, 0, 1, (uint8_t *)&period, sizeof(period), true, sizeof(period), 0);
	zassert_equal(rc, 0, "Wrong return value");
	k_sleep(K_SECONDS(70)); /* Wait for timer */
}

ZTEST(lwm2m_client_utils_cellconn, test_psm_time_cb)
{
	int rc;
	int32_t time = 1115999;

	setup();

	rc = set_uri_cb_psm(10, 0, 4, (uint8_t *)&time, sizeof(time), true, sizeof(time), 0);
	zassert_equal(rc, 0, "Wrong return value");
}

ZTEST(lwm2m_client_utils_cellconn, test_active_time_cb)
{
	int rc;
	int32_t time = 30;

	setup();

	rc = set_uri_cb_active_time(10, 0, 5, (uint8_t *)&time, sizeof(time),
				    true, sizeof(time), 0);
	zassert_equal(rc, 0, "Wrong return value");
}

ZTEST(lwm2m_client_utils_cellconn, test_edrx_update_cb)
{
	int rc;
	uint8_t data = 0x32;

	setup();

	rc = set_uri_cb_edrx(10, 0, 8, &data, sizeof(data), true, sizeof(data), 0);
	zassert_equal(rc, 0, "Wrong return value");
}

ZTEST(lwm2m_client_utils_cellconn, test_active_psm_update_cb)
{
	int rc;
	uint8_t psm = 3;

	setup();

	rc = set_uri_cb_psm_update(10, 0, 13, &psm, sizeof(psm), true, sizeof(psm), 0);
	zassert_equal(rc, 0, "Wrong return value");
}

ZTEST(lwm2m_client_utils_cellconn, test_rai_update_cb)
{
	int rc;
	uint8_t rai = 2;

	setup();

	rc = set_uri_cb_rai(10, 0, 14, &rai, sizeof(rai), true, sizeof(rai), 0);
	zassert_equal(rc, 0, "Wrong return value");
	rai = 6;
	rc = set_uri_cb_rai(10, 0, 14, &rai, sizeof(rai), true, sizeof(rai), 0);
	zassert_equal(rc, -ENOTSUP, "Wrong return value");
	rai = 7;
	rc = set_uri_cb_rai(10, 0, 14, &rai, sizeof(rai), true, sizeof(rai), 0);
	zassert_equal(rc, -EINVAL, "Wrong return value");
}

ZTEST(lwm2m_client_utils_cellconn, test_psm_update)
{
	struct lte_lc_evt evt = {0};

	setup();

	evt.type = LTE_LC_EVT_PSM_UPDATE;
	evt.psm_cfg.tau = 600;
	evt.psm_cfg.active_time = 30;
	call_lte_handlers(&evt);
	zassert_equal(lwm2m_notify_observer_fake.call_count, 3,
		      "Incorrect number of lwm2m_notify_observer() calls");
}

ZTEST(lwm2m_client_utils_cellconn, test_edrx_update)
{
	struct lte_lc_evt evt = {0};

	setup();

	evt.type = LTE_LC_EVT_LTE_MODE_UPDATE;
	evt.lte_mode = LTE_LC_LTE_MODE_LTEM;
	call_lte_handlers(&evt);

	evt.type = LTE_LC_EVT_EDRX_UPDATE;
	evt.edrx_cfg.edrx = 20;
	evt.edrx_cfg.mode = LTE_LC_LTE_MODE_LTEM;
	call_lte_handlers(&evt);
	evt.edrx_cfg.mode = LTE_LC_LTE_MODE_NBIOT;
	call_lte_handlers(&evt);
	zassert_equal(lwm2m_notify_observer_fake.call_count, 3,
		      "Incorrect number of lwm2m_notify_observer() calls");
}
