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
#include "location_assistance.h"

#define GNSS_ASSIST_ASSIST_DATA 6
#define GNSS_ASSIST_RESULT_CODE 7

#define GROUND_FIX_RESULT_CODE 1

static struct lwm2m_engine_obj_inst *gnss_obj;
static struct lwm2m_engine_obj_inst *ground_obj;
static int32_t last_result_code;

#define SERVER_RETRY_TIMEOT ((LOCATION_ASSISTANT_RESULT_TIMEOUT * 2) + 2)

static void gnss_result_set(int32_t result);
static void ground_result_set(int32_t result);


void result_code_cb(uint16_t object_id, int32_t result_code)
{
	printf("Result code %d from object %d\r\n", result_code, object_id);
	last_result_code = result_code;
}



int lwm2m_set_s32_custom_fake(const struct lwm2m_obj_path *path, int32_t value)
{
	printf("SET custom s32 %d, /%d/0/%d\r\n", value, path->obj_id, path->res_id);
	if (path->obj_id == GNSS_ASSIST_OBJECT_ID && path->res_id == GNSS_ASSIST_RESULT_CODE) {
		gnss_result_set(value);
	} else if (path->obj_id == GROUND_FIX_OBJECT_ID && path->res_id == GROUND_FIX_RESULT_CODE) {
		ground_result_set(value);
	}

	return 0;
}

static void setup(void)
{
	/* Register resets */
	DO_FOREACH_FAKE(RESET_FAKE);

	/* reset common FFF internal structures */
	FFF_RESET_HISTORY();

	location_assistance_retry_init(false);
	location_assistance_set_result_code_cb(result_code_cb);

	lwm2m_set_s32_fake.custom_fake = lwm2m_set_s32_custom_fake;
}

void fake_lwm2m_register_obj(struct lwm2m_engine_obj *obj)
{
	lwm2m_engine_obj_create_cb_t create_obj_cb;

	create_obj_cb = obj->create_cb;

	if (obj->obj_id == GNSS_ASSIST_OBJECT_ID) {
		if (!gnss_obj) {
			gnss_obj = create_obj_cb(0);
		}
	} else if (obj->obj_id == GROUND_FIX_OBJECT_ID) {
		if (!ground_obj) {
			ground_obj = create_obj_cb(0);
		}
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

static void gnss_result_set(int32_t result)
{
	void *ptr = &result;

	gnss_obj->resources[GNSS_ASSIST_RESULT_CODE].post_write_cb(0, GNSS_ASSIST_RESULT_CODE, 0,
								   ptr, 4, true, 4);
}

static void ground_result_set(int32_t result)
{
	void *ptr = &result;

	ground_obj->resources[GROUND_FIX_RESULT_CODE].post_write_cb(0, GROUND_FIX_RESULT_CODE, 0,
								   ptr, 4, true, 4);

}

static void gnss_assist_data_write(uint8_t *buf, int length)
{
	gnss_obj->resources[GNSS_ASSIST_ASSIST_DATA].post_write_cb(0, GNSS_ASSIST_ASSIST_DATA, 0,
								   buf, length, true, length);
}

static void *loaction_assist_suite_setup(void)
{
	/* Validate that Object are initialized for testing */
	zassert_not_null(gnss_obj, "GNSS object was null");
	zassert_not_null(ground_obj, "Ground object was null");

	return NULL;
}

ZTEST_SUITE(lwm2m_client_utils_location_assistance, NULL, loaction_assist_suite_setup, NULL, NULL,
	    NULL);

ZTEST(lwm2m_client_utils_location_assistance, test_agnss_send)
{
	int rc;
	uint8_t buf[8] = {1, 2, 3, 4, 5, 6, 7, 8};

	setup();

	rc = location_assistance_agnss_set_mask(&agnss_req);
	zassert_equal(rc, 0, "Error %d", rc);

	rc = location_assistance_agnss_request_send(&client_ctx);
	zassert_equal(rc, 0, "Error %d", rc);
	zassert_equal(lwm2m_send_cb_fake.call_count, 1, "Request not sent");

	gnss_result_set(LOCATION_ASSIST_RESULT_CODE_OK);

	gnss_assist_data_write(buf, 8);

	zassert_equal(last_result_code, LOCATION_ASSIST_RESULT_CODE_OK, "Wrong result");
	zassert_equal(nrf_cloud_agnss_process_fake.call_count, 1, "Data not processed %d",
		      nrf_cloud_agnss_process_fake.call_count);

	/* test timeout + 1 retry*/
	rc = location_assistance_agnss_set_mask(&agnss_req);
	zassert_equal(rc, 0, "Error %d", rc);

	rc = location_assistance_agnss_request_send(&client_ctx);
	zassert_equal(rc, 0, "Error %d", rc);
	zassert_equal(lwm2m_send_cb_fake.call_count, 2, "Request not sent");
	k_sleep(K_SECONDS(SERVER_RETRY_TIMEOT));
	zassert_equal(last_result_code, LOCATION_ASSIST_RESULT_CODE_NO_RESP_ERR, "Wrong result %d",
		      last_result_code);
}

ZTEST(lwm2m_client_utils_location_assistance, test_pgps_send)
{
	int rc;
	uint8_t buf[8] = {1, 2, 3, 4, 5, 6, 7, 8};

	setup();

	rc = location_assistance_pgps_request_send(&client_ctx);
	zassert_equal(rc, 0, "Error %d", rc);
	zassert_equal(lwm2m_send_cb_fake.call_count, 1, "Request not sent");

	gnss_result_set(LOCATION_ASSIST_RESULT_CODE_OK);
	gnss_assist_data_write(buf, 8);
	zassert_equal(last_result_code, LOCATION_ASSIST_RESULT_CODE_OK, "Wrong result");
	zassert_equal(nrf_cloud_pgps_finish_update_fake.call_count, 1, "Data not processed");
	/* Test Timeout + 1 retry*/
	rc = location_assistance_pgps_request_send(&client_ctx);
	zassert_equal(rc, 0, "Error %d", rc);
	zassert_equal(lwm2m_send_cb_fake.call_count, 2, "Request not sent");
	k_sleep(K_SECONDS(SERVER_RETRY_TIMEOT));
	zassert_equal(last_result_code, LOCATION_ASSIST_RESULT_CODE_NO_RESP_ERR, "Wrong result %d",
		      last_result_code);
	zassert_equal(lwm2m_send_cb_fake.call_count, 3, "Request not sent");
	rc = location_assistance_pgps_request_send(&client_ctx);
	zassert_equal(rc, 0, "Error %d", rc);
	zassert_equal(lwm2m_send_cb_fake.call_count, 4, "Request not sent");

	gnss_assist_data_write(buf, 8);
	gnss_result_set(LOCATION_ASSIST_RESULT_CODE_OK);
	zassert_equal(last_result_code, LOCATION_ASSIST_RESULT_CODE_OK, "Wrong result");
	zassert_equal(nrf_cloud_pgps_finish_update_fake.call_count, 2, "Data not processed");
}

ZTEST(lwm2m_client_utils_location_assistance, test_simultaneous_send)
{
	int rc;
	uint8_t buf[8] = {1, 2, 3, 4, 5, 6, 7, 8};

	setup();

	rc = location_assistance_agnss_set_mask(&agnss_req);
	zassert_equal(rc, 0, "Error %d", rc);

	rc = location_assistance_agnss_request_send(&client_ctx);
	zassert_equal(rc, 0, "Error %d", rc);
	zassert_equal(lwm2m_send_cb_fake.call_count, 1, "Request not sent");

	rc = location_assistance_pgps_request_send(&client_ctx);
	zassert_equal(rc, -EAGAIN, "Error %d", rc);

	gnss_assist_data_write(buf, 8);
	gnss_result_set(LOCATION_ASSIST_RESULT_CODE_OK);
	zassert_equal(last_result_code, LOCATION_ASSIST_RESULT_CODE_OK, "Wrong result");
	zassert_equal(nrf_cloud_agnss_process_fake.call_count, 1, "Data not processed");

	rc = location_assistance_pgps_request_send(&client_ctx);
	zassert_equal(rc, 0, "Error %d", rc);
	zassert_equal(lwm2m_send_cb_fake.call_count, 2, "Request not sent");
	gnss_assist_data_write(buf, 8);
	gnss_result_set(LOCATION_ASSIST_RESULT_CODE_OK);
	zassert_equal(last_result_code, LOCATION_ASSIST_RESULT_CODE_OK, "Wrong result");
	zassert_equal(nrf_cloud_pgps_finish_update_fake.call_count, 1, "Data not processed");
}

ZTEST(lwm2m_client_utils_location_assistance, test_ground_fix_send)
{
	int rc;

	setup();

	location_assistance_retry_init(true);

	/* Test Timeout */
	rc = location_assistance_ground_fix_request_send(&client_ctx);
	last_result_code = 0;
	zassert_equal(rc, 0, "Error %d", rc);
	zassert_equal(lwm2m_send_cb_fake.call_count, 1, "Request not sent");
	k_sleep(K_SECONDS(SERVER_RETRY_TIMEOT));
	zassert_equal(lwm2m_send_cb_fake.call_count, 2, "Request not sent");
	zassert_equal(last_result_code, LOCATION_ASSIST_RESULT_CODE_NO_RESP_ERR, "Wrong result %d",
		      last_result_code);
	/*Test temporary error*/
	rc = location_assistance_ground_fix_request_send(&client_ctx);
	last_result_code = 0;
	zassert_equal(rc, 0, "Error %d", rc);
	zassert_equal(lwm2m_send_cb_fake.call_count, 3, "Request not sent");
	k_sleep(K_SECONDS(1));
	ground_result_set(LOCATION_ASSIST_RESULT_CODE_TEMP_ERR);


	zassert_equal(last_result_code, LOCATION_ASSIST_RESULT_CODE_TEMP_ERR, "Wrong result %d",
		      last_result_code);
	/* Test that new request is blocked */
	rc = location_assistance_ground_fix_request_send(&client_ctx);
	zassert_equal(rc, -EALREADY, "Error %d", rc);
	k_sleep(K_SECONDS(LOCATION_ASSISTANT_INITIAL_RETRY_INTERVAL + 5));
	zassert_equal(lwm2m_send_cb_fake.call_count, 4, "Request not sent");
	/* Simulate response */
	ground_result_set(LOCATION_ASSIST_RESULT_CODE_OK);
	zassert_equal(last_result_code, LOCATION_ASSIST_RESULT_CODE_OK, "Wrong result %d",
		      last_result_code);
	/*Test pernament error*/
	rc = location_assistance_ground_fix_request_send(&client_ctx);
	last_result_code = 0;
	zassert_equal(rc, 0, "Error %d", rc);
	zassert_equal(lwm2m_send_cb_fake.call_count, 5, "Request not sent");
	k_sleep(K_SECONDS(1));
	ground_result_set(LOCATION_ASSIST_RESULT_CODE_PERMANENT_ERR);
	zassert_equal(last_result_code, LOCATION_ASSIST_RESULT_CODE_PERMANENT_ERR,
		      "Wrong result %d", last_result_code);
	rc = location_assistance_ground_fix_request_send(&client_ctx);
	last_result_code = 0;
	zassert_equal(rc, -EPIPE, "Error %d", rc);
}

ZTEST(lwm2m_client_utils_location_assistance, test_temporary_failure)
{
	int rc;
	uint8_t buf[8] = {1, 2, 3, 4, 5, 6, 7, 8};

	setup();

	location_assistance_retry_init(true);

	rc = location_assistance_agnss_set_mask(&agnss_req);
	zassert_equal(rc, 0, "Error %d", rc);

	rc = location_assistance_agnss_request_send(&client_ctx);
	zassert_equal(rc, 0, "Error %d", rc);
	zassert_equal(lwm2m_send_cb_fake.call_count, 1, "Request not sent");
	gnss_result_set(LOCATION_ASSIST_RESULT_CODE_TEMP_ERR);
	k_sleep(K_MSEC(100));

	rc = location_assistance_agnss_request_send(&client_ctx);
	zassert_equal(rc, -EALREADY, "Error %d", rc);
	zassert_equal(last_result_code, LOCATION_ASSIST_RESULT_CODE_TEMP_ERR, "Wrong result");

	k_sleep(K_SECONDS(LOCATION_ASSISTANT_INITIAL_RETRY_INTERVAL + 5));
	zassert_equal(lwm2m_send_cb_fake.call_count, 2, "Request not sent");

	/* Simulate response */
	gnss_assist_data_write(buf, 8);
	gnss_result_set(LOCATION_ASSIST_RESULT_CODE_OK);
	zassert_equal(last_result_code, LOCATION_ASSIST_RESULT_CODE_OK, "Wrong result");
	zassert_equal(nrf_cloud_agnss_process_fake.call_count, 1, "Data not processed %d",
		      nrf_cloud_pgps_finish_update_fake.call_count);
	/* Test Timeout with 1 retry */
	rc = location_assistance_agnss_set_mask(&agnss_req);
	zassert_equal(rc, 0, "Error %d", rc);
	rc = location_assistance_agnss_request_send(&client_ctx);
	zassert_equal(rc, 0, "Error %d", rc);
	zassert_equal(lwm2m_send_cb_fake.call_count, 3, "Request not sent");
	k_sleep(K_SECONDS(SERVER_RETRY_TIMEOT));
	zassert_equal(lwm2m_send_cb_fake.call_count, 4, "Request not sent");
	zassert_equal(last_result_code, LOCATION_ASSIST_RESULT_CODE_NO_RESP_ERR, "Wrong result");
}

ZTEST(lwm2m_client_utils_location_assistance, test_zzzpermanent_failure)
{
	int rc;

	setup();

	location_assistance_retry_init(true);

	rc = location_assistance_agnss_set_mask(&agnss_req);
	zassert_equal(rc, 0, "Error %d", rc);

	rc = location_assistance_agnss_request_send(&client_ctx);
	zassert_equal(rc, 0, "Error %d", rc);
	zassert_equal(lwm2m_send_cb_fake.call_count, 1, "Request not sent");
	gnss_result_set(LOCATION_ASSIST_RESULT_CODE_PERMANENT_ERR);

	k_sleep(K_MSEC(100));
	rc = location_assistance_agnss_set_mask(&agnss_req);
	zassert_equal(rc, -EPIPE, "Error %d", rc);
	zassert_equal(last_result_code, LOCATION_ASSIST_RESULT_CODE_PERMANENT_ERR, "Wrong result");
}

SYS_INIT(suite_setup, APPLICATION, 0);
