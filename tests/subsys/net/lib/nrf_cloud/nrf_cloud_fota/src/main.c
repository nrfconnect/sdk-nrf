/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <net/nrf_cloud.h>
#include <net/fota_download.h>
#include "nrf_cloud_fota.h"
#include "fakes.h"

/* reset all static state of nrf_cloud_fota library */
void reset_all_static_vars(void);

/*
The following functions need to be tested:
00000000 T nrf_cloud_fota_init
00000000 T nrf_cloud_fota_ble_job_update
00000000 T nrf_cloud_fota_ble_set_handler
00000000 T nrf_cloud_fota_ble_update_check
00000000 T nrf_cloud_fota_endpoint_clear
00000000 T nrf_cloud_fota_endpoint_set
00000000 T nrf_cloud_fota_endpoint_set_and_report
00000000 T nrf_cloud_fota_is_active
00000000 T nrf_cloud_fota_is_available
00000000 T nrf_cloud_fota_job_start
00000000 T nrf_cloud_fota_mqtt_evt_handler
00000000 T nrf_cloud_fota_pending_job_type_get
00000000 T nrf_cloud_fota_pending_job_validate
00000000 T nrf_cloud_fota_subscribe
00000000 T nrf_cloud_fota_uninit
00000000 T nrf_cloud_fota_unsubscribe
00000000 T nrf_cloud_fota_update_check
00000000 T nrf_cloud_modem_fota_completed
*/

/* This function runs before each test */
static void run_before(void *fixture)
{
	ARG_UNUSED(fixture);
	RESET_FAKE(nrf_cloud_fota_job_free);
	RESET_FAKE(mqtt_publish);
	RESET_FAKE(nrf_cloud_obj_cloud_encode);
	RESET_FAKE(nrf_cloud_obj_free);
	RESET_FAKE(nrf_cloud_obj_cloud_encoded_free);
	RESET_FAKE(nrf_cloud_obj_fota_job_update_create);
	RESET_FAKE(nrf_cloud_bootloader_fota_slot_set);
	RESET_FAKE(nrf_cloud_fota_settings_save);
	RESET_FAKE(nrf_cloud_pending_fota_job_process);
	RESET_FAKE(nrf_cloud_fota_is_type_modem);
	RESET_FAKE(sys_reboot);
	RESET_FAKE(nrf_cloud_download_end);
	RESET_FAKE(nrf_cloud_fota_settings_load);
	RESET_FAKE(nrf_cloud_codec_init);
	RESET_FAKE(fota_download_init);
	RESET_FAKE(nrf_cloud_free);
	RESET_FAKE(nrf_cloud_fota_cb_handler);
	reset_all_static_vars();
}

/* This function runs after each completed test */
static void run_after(void *fixture)
{
	ARG_UNUSED(fixture);
}

ZTEST_SUITE(nrf_cloud_fota_test, NULL, NULL, run_before, run_after, NULL);


ZTEST(nrf_cloud_fota_test, test_nrf_cloud_fota_init_null_param)
{
	zassert_equal(-EINVAL, nrf_cloud_fota_init(NULL),
		"nrf_cloud_fota_init should check parameter validity");
}



ZTEST(nrf_cloud_fota_test, test_nrf_cloud_fota_init_valid_param)
{
	struct nrf_cloud_fota_init_param fota_init = {
		.evt_cb = nrf_cloud_fota_cb_handler,
		.smp_reset_cb = NULL
	};

	int ret = nrf_cloud_fota_init(&fota_init);
	printf("ret: %d\n", ret);

	zassert_true((ret == 0) || (ret == 1),
		"nrf_cloud_fota_init should succeed with valid parameters");
}
