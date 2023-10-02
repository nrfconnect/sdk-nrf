/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/ztest.h>
#include <zephyr/fff.h>
#include "lwm2m_object.h"

#include <net/lwm2m_client_utils_location.h>
#include "stubs.h"

#include "gnss_assistance_obj.h"
#include "ground_fix_obj.h"

#define GNSS_ASSIST_ASSIST_DATA 6
#define GNSS_ASSIST_RESULT_CODE 7

static lwm2m_engine_obj_create_cb_t create_obj_cb;
static int32_t last_result_code;

static void setup(void)
{
	/* Register resets */
	DO_FOREACH_FAKE(RESET_FAKE);

	/* reset common FFF internal structures */
	FFF_RESET_HISTORY();
}

void result_code_cb(int32_t result_code)
{
	last_result_code = result_code;
}

void fake_lwm2m_register_obj(struct lwm2m_engine_obj *obj)
{
	if (obj->obj_id == GNSS_ASSIST_OBJECT_ID) {
		create_obj_cb = obj->create_cb;
	}
}

static struct nrf_modem_gnss_agnss_data_frame agnss_req = {
	.data_flags = 0xffffffff,
	.system_count = 1,
	.system[0].system_id = NRF_MODEM_GNSS_SYSTEM_GPS,
	.system[0].sv_mask_ephe = 0xffffffff,
	.system[0].sv_mask_alm = 0xffffffff,
};

static struct lwm2m_ctx client_ctx;

static int suite_setup(void)
{
	lwm2m_register_obj_fake.custom_fake = fake_lwm2m_register_obj;

	return 0;
}

ZTEST_SUITE(lwm2m_client_utils_location_assistance, NULL, NULL, NULL, NULL, NULL);

ZTEST(lwm2m_client_utils_location_assistance, test_agnss_send)
{
	int rc;
	uint8_t buf[8] = {1, 2, 3, 4, 5, 6, 7, 8};

	setup();

	zassert_not_null(create_obj_cb, "Callback was null");
	struct lwm2m_engine_obj_inst *gnss_obj = create_obj_cb(0);

	rc = location_assistance_agnss_set_mask(&agnss_req);
	zassert_equal(rc, 0, "Error %d", rc);

	rc = location_assistance_agnss_request_send(&client_ctx);
	zassert_equal(rc, 0, "Error %d", rc);
	zassert_equal(lwm2m_send_cb_fake.call_count, 1, "Request not sent");

	gnss_obj->resources[GNSS_ASSIST_ASSIST_DATA].post_write_cb(0, GNSS_ASSIST_ASSIST_DATA, 0,
								   buf, 8, true, 8);

	zassert_equal(nrf_cloud_agps_process_fake.call_count, 1, "Data not processed");
}

ZTEST(lwm2m_client_utils_location_assistance, test_pgps_send)
{
	int rc;
	uint8_t buf[8] = {1, 2, 3, 4, 5, 6, 7, 8};

	setup();

	zassert_not_null(create_obj_cb, "Callback was null");
	struct lwm2m_engine_obj_inst *gnss_obj = create_obj_cb(0);

	rc = location_assistance_pgps_request_send(&client_ctx);
	zassert_equal(rc, 0, "Error %d", rc);
	zassert_equal(lwm2m_send_cb_fake.call_count, 1, "Request not sent");

	gnss_obj->resources[GNSS_ASSIST_ASSIST_DATA].post_write_cb(0, GNSS_ASSIST_ASSIST_DATA, 0,
								   buf, 8, true, 8);

	zassert_equal(nrf_cloud_pgps_finish_update_fake.call_count, 1, "Data not processed");
}

ZTEST(lwm2m_client_utils_location_assistance, test_simultaneous_send)
{
	int rc;
	uint8_t buf[8] = {1, 2, 3, 4, 5, 6, 7, 8};

	setup();

	zassert_not_null(create_obj_cb, "Callback was null");
	struct lwm2m_engine_obj_inst *gnss_obj = create_obj_cb(0);

	rc = location_assistance_agnss_set_mask(&agnss_req);
	zassert_equal(rc, 0, "Error %d", rc);

	rc = location_assistance_agnss_request_send(&client_ctx);
	zassert_equal(rc, 0, "Error %d", rc);
	zassert_equal(lwm2m_send_cb_fake.call_count, 1, "Request not sent");

	rc = location_assistance_pgps_request_send(&client_ctx);
	zassert_equal(rc, -EAGAIN, "Error %d", rc);

	gnss_obj->resources[GNSS_ASSIST_ASSIST_DATA].post_write_cb(0, GNSS_ASSIST_ASSIST_DATA, 0,
								   buf, 8, true, 8);

	zassert_equal(nrf_cloud_agps_process_fake.call_count, 1, "Data not processed");

	rc = location_assistance_pgps_request_send(&client_ctx);
	zassert_equal(rc, 0, "Error %d", rc);
	zassert_equal(lwm2m_send_cb_fake.call_count, 2, "Request not sent");

	gnss_obj->resources[GNSS_ASSIST_ASSIST_DATA].post_write_cb(0, GNSS_ASSIST_ASSIST_DATA, 0,
								   buf, 8, true, 8);

	zassert_equal(nrf_cloud_pgps_finish_update_fake.call_count, 1, "Data not processed");
}

ZTEST(lwm2m_client_utils_location_assistance, test_ground_fix_send)
{
	int rc;

	setup();

	rc = location_assistance_ground_fix_request_send(&client_ctx);

	zassert_equal(rc, 0, "Error %d", rc);
	zassert_equal(lwm2m_send_cb_fake.call_count, 1, "Request not sent");
}

ZTEST(lwm2m_client_utils_location_assistance, test_temporary_failure)
{
	int rc;
	uint8_t buf[8] = {1, 2, 3, 4, 5, 6, 7, 8};
	int32_t result = 1;
	void *resbuf = &result;

	setup();

	location_assistance_init_resend_handler();
	location_assistance_set_result_code_cb(result_code_cb);
	zassert_not_null(create_obj_cb, "Callback was null");
	struct lwm2m_engine_obj_inst *gnss_obj = create_obj_cb(0);

	rc = location_assistance_agnss_set_mask(&agnss_req);
	zassert_equal(rc, 0, "Error %d", rc);

	rc = location_assistance_agnss_request_send(&client_ctx);
	zassert_equal(rc, 0, "Error %d", rc);
	zassert_equal(lwm2m_send_cb_fake.call_count, 1, "Request not sent");

	gnss_obj->resources[GNSS_ASSIST_RESULT_CODE].post_write_cb(0, GNSS_ASSIST_RESULT_CODE, 0,
								   resbuf, 4, true, 4);
	k_sleep(K_MSEC(100));

	rc = location_assistance_agnss_request_send(&client_ctx);
	zassert_equal(rc, -EALREADY, "Error %d", rc);
	zassert_equal(last_result_code, LOCATION_ASSIST_RESULT_CODE_TEMP_ERR, "Wrong result");

	result = 0;
	gnss_obj->resources[GNSS_ASSIST_ASSIST_DATA].post_write_cb(0, GNSS_ASSIST_ASSIST_DATA, 0,
								   buf, 8, true, 8);
	gnss_obj->resources[GNSS_ASSIST_RESULT_CODE].post_write_cb(0, GNSS_ASSIST_RESULT_CODE, 0,
								   resbuf, 4, true, 4);
}

ZTEST(lwm2m_client_utils_location_assistance, test_zzzpermanent_failure)
{
	int rc;
	int32_t result = -1;
	void *resbuf = &result;

	setup();

	location_assistance_init_resend_handler();
	location_assistance_set_result_code_cb(result_code_cb);
	zassert_not_null(create_obj_cb, "Callback was null");
	struct lwm2m_engine_obj_inst *gnss_obj = create_obj_cb(0);

	rc = location_assistance_agnss_set_mask(&agnss_req);
	zassert_equal(rc, 0, "Error %d", rc);

	rc = location_assistance_agnss_request_send(&client_ctx);
	zassert_equal(rc, 0, "Error %d", rc);
	zassert_equal(lwm2m_send_cb_fake.call_count, 1, "Request not sent");

	gnss_obj->resources[GNSS_ASSIST_RESULT_CODE].post_write_cb(0, GNSS_ASSIST_RESULT_CODE, 0,
								   resbuf, 4, true, 4);
	k_sleep(K_MSEC(100));
	rc = location_assistance_agnss_set_mask(&agnss_req);
	zassert_equal(rc, -EPIPE, "Error %d", rc);
	zassert_equal(last_result_code, LOCATION_ASSIST_RESULT_CODE_PERMANENT_ERR, "Wrong result");
}

SYS_INIT(suite_setup, APPLICATION, 0);
