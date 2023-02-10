/*
 * Copyright (c) 2022 grandcentrix GmbH
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <zephyr/fff.h>

#include "stubs.h"
#include <net/lwm2m_client_utils.h>

static const struct lwm2m_obj_path FIRMWARE_UPDATE_URI =
	LWM2M_OBJ(LWM2M_OBJECT_FIRMWARE_ID, 0, LWM2M_FOTA_PACKAGE_URI_ID);
static const struct lwm2m_obj_path FIRMWARE_UPDATE_STATE =
	LWM2M_OBJ(LWM2M_OBJECT_FIRMWARE_ID, 0, LWM2M_FOTA_STATE_ID);

void client_acknowledge(void)
{
}

bool check_set_update_result(uint8_t expected_val, uint8_t arg_index)
{
	if (lwm2m_firmware_set_update_result_inst_fake.call_count > arg_index) {
		return lwm2m_firmware_set_update_result_inst_fake.arg1_history[arg_index] ==
		       expected_val;
	}

	return false;
}

bool check_set_update_state(uint8_t expected_val, uint8_t arg_index)
{
	if (lwm2m_firmware_set_update_state_inst_fake.call_count > arg_index) {
		return lwm2m_firmware_set_update_state_inst_fake.arg1_history[arg_index] ==
		       expected_val;
	}

	return false;
}

bool is_last_update_state(uint8_t expected)
{
	if (lwm2m_firmware_set_update_state_inst_fake.call_count > 0) {
		return lwm2m_firmware_set_update_state_inst_fake
			       .arg1_history[lwm2m_firmware_set_update_state_inst_fake.call_count -
					     1] == expected;
	}

	return true;
}

static void my_suite_before(void *f)
{
	/* Register resets */
	DO_FOREACH_FAKE(RESET_FAKE);

	/* reset common FFF internal structures */
	FFF_RESET_HISTORY();

	test_stubs_teardown();
}

ZTEST_SUITE(lwm2m_firmware, NULL, NULL, my_suite_before, NULL, NULL);

ZTEST(lwm2m_firmware, test_on_init_downloading)
{
	lwm2m_firmware_get_update_state_inst_fake.return_val = STATE_DOWNLOADING;
	lwm2m_init_firmware();

	zassert_true(check_set_update_result(RESULT_DEFAULT, 0), NULL);
}

ZTEST(lwm2m_firmware, test_on_init_image_updating_confirmed)
{
	lwm2m_firmware_get_update_state_inst_fake.return_val = STATE_UPDATING;
	boot_is_img_confirmed_fake.return_val = false;
	lwm2m_init_image();

	zassert_true(check_set_update_result(RESULT_SUCCESS, 0), NULL);
}

ZTEST(lwm2m_firmware, test_on_init_image_updating_not_confirmed)
{
	lwm2m_firmware_get_update_state_inst_fake.return_val = STATE_UPDATING;
	boot_is_img_confirmed_fake.return_val = true;
	lwm2m_init_image();

	zassert_true(check_set_update_result(RESULT_UPDATE_FAILED, 0), NULL);
}

ZTEST(lwm2m_firmware, test_on_init_image_downloading)
{
	lwm2m_firmware_get_update_state_inst_fake.return_val = STATE_DOWNLOADING;
	boot_is_img_confirmed_fake.return_val = true;
	lwm2m_init_image();

	zassert_equal(lwm2m_firmware_set_update_result_inst_fake.call_count, 0, NULL);
}

ZTEST(lwm2m_firmware, test_write_coap_uri_start_download)
{
	char test_uri[] = "coap://10.1.0.10:5683/tmp/test.tmp";

	lwm2m_register_post_write_callback_fake.custom_fake =
		lwm2m_register_post_write_callback_fake1;
	lwm2m_firmware_get_update_state_inst_fake.return_val = STATE_IDLE;
	lwm2m_init_firmware();

	test_lwm2m_engine_call_post_write_cb(&FIRMWARE_UPDATE_URI, 0, 0, 0, test_uri,
					     strlen(test_uri), 1, strlen(test_uri));
	k_sleep(K_MSEC(20));
	zassert_equal(lwm2m_firmware_set_update_result_inst_fake.call_count, 0, NULL);
	zassert_true(check_set_update_state(STATE_DOWNLOADING, 0), NULL);
}

ZTEST(lwm2m_firmware, test_write_coap_sec_uri_start_download)
{
	char test_uri[] = "coaps://10.1.0.10:5684/tmp/test.tmp";

	lwm2m_register_post_write_callback_fake.custom_fake =
		lwm2m_register_post_write_callback_fake1;
	lwm2m_firmware_get_update_state_inst_fake.return_val = STATE_IDLE;
	lwm2m_init_firmware();

	test_lwm2m_engine_call_post_write_cb(&FIRMWARE_UPDATE_URI, 0, 0, 0, test_uri,
					     strlen(test_uri), 1, strlen(test_uri));
	k_sleep(K_MSEC(20));
	zassert_equal(lwm2m_firmware_set_update_result_inst_fake.call_count, 0, NULL);
	zassert_true(check_set_update_state(STATE_DOWNLOADING, 0), NULL);
}

ZTEST(lwm2m_firmware, test_write_http_uri_start_download)
{
	char test_uri[] = "http://10.1.0.10:80/tmp/test.tmp";

	lwm2m_register_post_write_callback_fake.custom_fake =
		lwm2m_register_post_write_callback_fake1;
	lwm2m_firmware_get_update_state_inst_fake.return_val = STATE_IDLE;
	lwm2m_init_firmware();

	test_lwm2m_engine_call_post_write_cb(&FIRMWARE_UPDATE_URI, 0, 0, 0, test_uri,
					     strlen(test_uri), 1, strlen(test_uri));
	k_sleep(K_MSEC(20));
	zassert_equal(lwm2m_firmware_set_update_result_inst_fake.call_count, 0, NULL);
	zassert_true(check_set_update_state(STATE_DOWNLOADING, 0), NULL);
}

ZTEST(lwm2m_firmware, test_write_http_sec_uri_start_download)
{
	char test_uri[] = "https://10.1.0.10:443/tmp/test.tmp";

	lwm2m_register_post_write_callback_fake.custom_fake =
		lwm2m_register_post_write_callback_fake1;
	lwm2m_firmware_get_update_state_inst_fake.return_val = STATE_IDLE;
	lwm2m_init_firmware();

	test_lwm2m_engine_call_post_write_cb(&FIRMWARE_UPDATE_URI, 0, 0, 0, test_uri,
					     strlen(test_uri), 1, strlen(test_uri));
	k_sleep(K_MSEC(20));
	zassert_equal(lwm2m_firmware_set_update_result_inst_fake.call_count, 0, NULL);
	zassert_true(check_set_update_state(STATE_DOWNLOADING, 0), NULL);
}

ZTEST(lwm2m_firmware, test_write_uri_twice)
{
	char test_uri[] = "coap://10.1.0.10:5683/tmp/test.tmp";

	lwm2m_register_post_write_callback_fake.custom_fake =
		lwm2m_register_post_write_callback_fake1;
	lwm2m_firmware_get_update_state_inst_fake.return_val = STATE_IDLE;
	lwm2m_init_firmware();

	lwm2m_firmware_get_update_state_inst_fake.return_val = STATE_DOWNLOADING;
	test_lwm2m_engine_call_post_write_cb(&FIRMWARE_UPDATE_URI, 0, 0, 0, test_uri,
					     strlen(test_uri), 1, strlen(test_uri));
	k_sleep(K_MSEC(20));
	zassert_equal(lwm2m_firmware_set_update_result_inst_fake.call_count, 0, NULL);
	zassert_equal(lwm2m_firmware_set_update_state_inst_fake.call_count, 0, NULL);
}

ZTEST(lwm2m_firmware, test_write_invalid_uri_no_path)
{
	char test_uri[] = "coap://10.1.0.10:5683";

	lwm2m_register_post_write_callback_fake.custom_fake =
		lwm2m_register_post_write_callback_fake1;
	lwm2m_firmware_get_update_state_inst_fake.return_val = STATE_IDLE;
	lwm2m_init_firmware();

	test_lwm2m_engine_call_post_write_cb(&FIRMWARE_UPDATE_URI, 0, 0, 0, test_uri,
					     strlen(test_uri), 1, strlen(test_uri));
	k_sleep(K_MSEC(20));
	zassert_true(check_set_update_result(RESULT_INVALID_URI, 0), NULL);
}

ZTEST(lwm2m_firmware, test_write_invalid_uri_invalid_host)
{
	char test_uri[] = "10.1.0.10:5683/tmp/test.tmp";

	lwm2m_register_post_write_callback_fake.custom_fake =
		lwm2m_register_post_write_callback_fake1;
	lwm2m_firmware_get_update_state_inst_fake.return_val = STATE_IDLE;
	lwm2m_init_firmware();

	test_lwm2m_engine_call_post_write_cb(&FIRMWARE_UPDATE_URI, 0, 0, 0, test_uri,
					     strlen(test_uri), 1, strlen(test_uri));
	k_sleep(K_MSEC(20));
	zassert_true(check_set_update_result(RESULT_INVALID_URI, 0), NULL);
}

ZTEST(lwm2m_firmware, test_write_uri_reset_download)
{
	char test_uri[] = "10.1.0.10:5683/tmp/test.tmp";

	lwm2m_register_post_write_callback_fake.custom_fake =
		lwm2m_register_post_write_callback_fake1;
	lwm2m_firmware_get_update_state_inst_fake.return_val = STATE_IDLE;
	lwm2m_init_firmware();

	lwm2m_firmware_get_update_state_inst_fake.return_val = STATE_DOWNLOADED;
	test_lwm2m_engine_call_post_write_cb(&FIRMWARE_UPDATE_URI, 0, 0, 0, test_uri, 0, 1,
					     strlen(test_uri));
	k_sleep(K_MSEC(20));
	zassert_true(check_set_update_result(RESULT_DEFAULT, 0), NULL);
}

ZTEST(lwm2m_firmware, test_download_evt_progres)
{
	char test_uri[] = "coap://10.1.0.10:5683/tmp/test.tmp";

	lwm2m_register_post_write_callback_fake.custom_fake =
		lwm2m_register_post_write_callback_fake1;
	lwm2m_firmware_get_update_state_inst_fake.return_val = STATE_IDLE;
	lwm2m_init_firmware();

	fota_download_init_fake.custom_fake = fota_download_init_fake1;
	test_lwm2m_engine_call_post_write_cb(&FIRMWARE_UPDATE_URI, 0, 0, 0, test_uri,
					     strlen(test_uri), 1, strlen(test_uri));
	k_sleep(K_MSEC(20));

	test_fota_download_throw_evt(FOTA_DOWNLOAD_EVT_PROGRESS);
	zassert_true(check_set_update_state(STATE_DOWNLOADING, 0), NULL);
}

ZTEST(lwm2m_firmware, test_download_evt_cancelled)
{
	char test_uri[] = "coap://10.1.0.10:5683/tmp/test.tmp";

	lwm2m_register_post_write_callback_fake.custom_fake =
		lwm2m_register_post_write_callback_fake1;
	lwm2m_firmware_get_update_state_inst_fake.return_val = STATE_IDLE;
	lwm2m_init_firmware();

	fota_download_init_fake.custom_fake = fota_download_init_fake1;
	test_lwm2m_engine_call_post_write_cb(&FIRMWARE_UPDATE_URI, 0, 0, 0, test_uri,
					     strlen(test_uri), 1, strlen(test_uri));
	k_sleep(K_MSEC(20));
	zassert_true(check_set_update_state(STATE_DOWNLOADING, 0), NULL);

	test_fota_download_throw_evt(FOTA_DOWNLOAD_EVT_CANCELLED);
	zassert_true(check_set_update_result(RESULT_CONNECTION_LOST, 0), NULL);
}

ZTEST(lwm2m_firmware, test_download_evt_finished)
{
	char test_uri[] = "coap://10.1.0.10:5683/tmp/test.tmp";

	lwm2m_register_post_write_callback_fake.custom_fake =
		lwm2m_register_post_write_callback_fake1;
	lwm2m_firmware_get_update_state_inst_fake.return_val = STATE_IDLE;
	lwm2m_init_firmware();

	fota_download_init_fake.custom_fake = fota_download_init_fake1;
	test_lwm2m_engine_call_post_write_cb(&FIRMWARE_UPDATE_URI, 0, 0, 0, test_uri,
					     strlen(test_uri), 1, strlen(test_uri));
	k_sleep(K_MSEC(20));
	zassert_true(check_set_update_state(STATE_DOWNLOADING, 0), NULL);

	test_fota_download_throw_evt(FOTA_DOWNLOAD_EVT_FINISHED);
	zassert_true(check_set_update_state(STATE_DOWNLOADED, 1), NULL);
}

ZTEST(lwm2m_firmware, test_download_evt_download_failed)
{
	char test_uri[] = "coap://10.1.0.10:5683/tmp/test.tmp";

	lwm2m_register_post_write_callback_fake.custom_fake =
		lwm2m_register_post_write_callback_fake1;
	lwm2m_firmware_get_update_state_inst_fake.return_val = STATE_IDLE;
	lwm2m_init_firmware();

	fota_download_init_fake.custom_fake = fota_download_init_fake1;
	test_lwm2m_engine_call_post_write_cb(&FIRMWARE_UPDATE_URI, 0, 0, 0, test_uri,
					     strlen(test_uri), 1, strlen(test_uri));
	k_sleep(K_MSEC(20));
	zassert_true(check_set_update_state(STATE_DOWNLOADING, 0), NULL);

	test_fota_download_throw_evt_error(FOTA_DOWNLOAD_ERROR_CAUSE_DOWNLOAD_FAILED);
	zassert_true(check_set_update_result(RESULT_CONNECTION_LOST, 0), NULL);
}

ZTEST(lwm2m_firmware, test_download_evt_download_invalid_update)
{
	char test_uri[] = "coap://10.1.0.10:5683/tmp/test.tmp";

	lwm2m_register_post_write_callback_fake.custom_fake =
		lwm2m_register_post_write_callback_fake1;
	lwm2m_firmware_get_update_state_inst_fake.return_val = STATE_IDLE;
	lwm2m_init_firmware();

	fota_download_init_fake.custom_fake = fota_download_init_fake1;
	test_lwm2m_engine_call_post_write_cb(&FIRMWARE_UPDATE_URI, 0, 0, 0, test_uri,
					     strlen(test_uri), 1, strlen(test_uri));
	k_sleep(K_MSEC(20));
	zassert_true(check_set_update_state(STATE_DOWNLOADING, 0), NULL);

	test_fota_download_throw_evt_error(FOTA_DOWNLOAD_ERROR_CAUSE_INVALID_UPDATE);
	zassert_true(check_set_update_result(RESULT_UNSUP_FW, 0), NULL);
}

ZTEST(lwm2m_firmware, test_download_evt_download_type_mismatch)
{
	char test_uri[] = "coap://10.1.0.10:5683/tmp/test.tmp";

	lwm2m_register_post_write_callback_fake.custom_fake =
		lwm2m_register_post_write_callback_fake1;
	lwm2m_firmware_get_update_state_inst_fake.return_val = STATE_IDLE;
	lwm2m_init_firmware();

	fota_download_init_fake.custom_fake = fota_download_init_fake1;
	test_lwm2m_engine_call_post_write_cb(&FIRMWARE_UPDATE_URI, 0, 0, 0, test_uri,
					     strlen(test_uri), 1, strlen(test_uri));
	k_sleep(K_MSEC(20));
	zassert_true(check_set_update_state(STATE_DOWNLOADING, 0), NULL);

	test_fota_download_throw_evt_error(FOTA_DOWNLOAD_ERROR_CAUSE_TYPE_MISMATCH);
	zassert_true(check_set_update_result(RESULT_UNSUP_FW, 0), NULL);
}

ZTEST(lwm2m_firmware, test_write_update_state_reset_download)
{
	uint8_t idle_state = STATE_IDLE;
	uint8_t dl_data[4];

	lwm2m_firmware_set_write_cb_fake.custom_fake = lwm2m_firmware_set_write_cb_fake1;
	lwm2m_register_post_write_callback_fake.custom_fake =
		lwm2m_register_post_write_callback_fake1;
	lwm2m_firmware_get_update_state_inst_fake.return_val = STATE_IDLE;
	lwm2m_init_firmware();

	dfu_target_img_type_fake.custom_fake = dfu_target_img_type_fake_return_mcuboot;
	test_call_lwm2m_firmware_set_write_cb(0, 0, 0, dl_data, 2, 0, sizeof(dl_data));

	test_lwm2m_engine_call_post_write_cb(&FIRMWARE_UPDATE_STATE, 0, 0, 0, &idle_state, 0, 0, 0);
	k_sleep(K_MSEC(20));
	zassert_equal(fota_download_cancel_fake.call_count, 1, NULL);
	zassert_equal(dfu_target_reset_fake.call_count, 1, NULL);
}
