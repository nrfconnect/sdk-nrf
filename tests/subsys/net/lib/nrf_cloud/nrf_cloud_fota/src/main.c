/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <net/nrf_cloud.h>
#include <net/fota_download.h>
#include <zephyr/net/mqtt.h>
#include "nrf_cloud_fota.h"
#include "fakes.h"

/* reset all static state of nrf_cloud_fota library */
void reset_all_static_vars(void);

/* This function runs before each test */
static void run_before(void *fixture)
{
	ARG_UNUSED(fixture);
	RESET_FAKE(nrf_cloud_fota_job_free);
	RESET_FAKE(mqtt_publish);
	RESET_FAKE(mqtt_unsubscribe);
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
	RESET_FAKE(nrf_cloud_fota_smp_client_init);
	RESET_FAKE(nrf_cloud_obj_fota_ble_job_update_create);
	RESET_FAKE(mqtt_readall_publish_payload);
	RESET_FAKE(nrf_cloud_fota_job_decode);
	RESET_FAKE(mqtt_publish_qos1_ack);
	RESET_FAKE(nrf_cloud_calloc);
	reset_all_static_vars();
}

/* This function runs after each completed test */
static void run_after(void *fixture)
{
	ARG_UNUSED(fixture);
}

ZTEST_SUITE(nrf_cloud_fota_test, NULL, NULL, run_before, run_after, NULL);

/* nrf_cloud_fota_init fails when cb parameter is NULL */
ZTEST(nrf_cloud_fota_test, test_nrf_cloud_fota_init_null_param)
{
	zassert_equal(-EINVAL, nrf_cloud_fota_init(NULL),
		"nrf_cloud_fota_init should check parameter validity");
}

/* nrf_cloud_fota_init fails when nrf_cloud_fota_smp_client_init fails */
ZTEST(nrf_cloud_fota_test, test_nrf_cloud_fota_init_smp_client_init_fails)
{
	struct nrf_cloud_fota_init_param fota_init = {
		.evt_cb = nrf_cloud_fota_cb_handler,
		.smp_reset_cb = NULL
	};

	nrf_cloud_fota_smp_client_init_fake.return_val = -ENOTSUP;

	int ret = nrf_cloud_fota_init(&fota_init);

	zassert_equal(-ENOTSUP, ret,
		"nrf_cloud_fota_init should succeed with valid parameters");
}

/* nrf_cloud_fota_init fails when fota_download_init fails */
ZTEST(nrf_cloud_fota_test, test_nrf_cloud_fota_init_download_init_fails)
{
	struct nrf_cloud_fota_init_param fota_init = {
		.evt_cb = nrf_cloud_fota_cb_handler,
		.smp_reset_cb = NULL
	};

	fota_download_init_fake.return_val = -EINVAL;

	int ret = nrf_cloud_fota_init(&fota_init);

	zassert_equal(-EINVAL, ret,
		"nrf_cloud_fota_init should succeed with valid parameters");
}

/* nrf_cloud_fota_init: no fota pending and no fota done yet */
ZTEST(nrf_cloud_fota_test, test_nrf_cloud_fota_init_no_job)
{
	struct nrf_cloud_fota_init_param fota_init = {
		.evt_cb = nrf_cloud_fota_cb_handler,
		.smp_reset_cb = NULL
	};

	nrf_cloud_pending_fota_job_process_fake.return_val = -ENODEV;

	/* ignore return value, doesn't return normally */
	int ret = nrf_cloud_fota_init(&fota_init);

	zassert_equal(0, ret,
		"nrf_cloud_fota_init should succeed if there was no pending job");
}

/* nrf_cloud_fota_init: failed to process fota job */
ZTEST(nrf_cloud_fota_test, test_nrf_cloud_fota_init_pending_job_validate_fails)
{
	struct nrf_cloud_fota_init_param fota_init = {
		.evt_cb = nrf_cloud_fota_cb_handler,
		.smp_reset_cb = NULL
	};

	nrf_cloud_pending_fota_job_process_fake.return_val = -EINVAL;

	/* ignore return value, doesn't return normally */
	int ret = nrf_cloud_fota_init(&fota_init);

	zassert_equal(-EINVAL, ret,
		"nrf_cloud_fota_init should succeed if there was no pending job");
}

/* nrf_cloud_fota_init: double init skips early */
ZTEST(nrf_cloud_fota_test, test_nrf_cloud_fota_init_double_init_skips)
{
	int ret;

	/* first init */
	struct nrf_cloud_fota_init_param fota_init = {
		.evt_cb = nrf_cloud_fota_cb_handler,
		.smp_reset_cb = NULL
	};

	nrf_cloud_fota_is_type_modem_fake.return_val = true;

	ret = nrf_cloud_fota_init(&fota_init);

	zassert_equal(1, ret,
		"nrf_cloud_fota_init should succeed with valid parameters");

	/* second init */
	ret = nrf_cloud_fota_init(&fota_init);

	zassert_equal(0, ret,
		"nrf_cloud_fota_init should the second time as well, having skipped early");
}

/* nrf_cloud_fota_init: reboot early if reboot_on_init is set */
ZTEST(nrf_cloud_fota_test, test_nrf_cloud_fota_init_double_init_reboot)
{
	/* first init */
	struct nrf_cloud_fota_init_param fota_init = {
		.evt_cb = nrf_cloud_fota_cb_handler,
		.smp_reset_cb = NULL
	};

	nrf_cloud_fota_is_type_modem_fake.return_val = false;
	nrf_cloud_pending_fota_job_process_fake.custom_fake =
		fake_nrf_cloud_pending_fota_job_process__set_reboot;

	/* ignore return value, doesn't return normally */
	(void) nrf_cloud_fota_init(&fota_init);

	/* ignore return value, doesn't return normally */
	(void) nrf_cloud_fota_init(&fota_init);

	printk("sys_reboot_fake.call_count: %d\n", sys_reboot_fake.call_count);

	/* TODO: this is probably broken and only works by chance.
	 * possibly need to refactor reboot_on_init */

	zassert_equal(2, sys_reboot_fake.call_count,
		"nrf_cloud_fota_init should have rebooted twice");
	zassert_equal(SYS_REBOOT_COLD, sys_reboot_fake.arg0_history[0],
		"nrf_cloud_fota_init should force a cold reboot");
	zassert_equal(SYS_REBOOT_COLD, sys_reboot_fake.arg0_history[1],
		"nrf_cloud_fota_init should force a cold reboot");
}

/* nrf_cloud_fota_init forces cold reboot after successful fota */
ZTEST(nrf_cloud_fota_test, test_nrf_cloud_fota_init_reboot_needed)
{
	struct nrf_cloud_fota_init_param fota_init = {
		.evt_cb = nrf_cloud_fota_cb_handler,
		.smp_reset_cb = NULL
	};

	nrf_cloud_pending_fota_job_process_fake.return_val = 1;

	/* ignore return value, doesn't return normally */
	(void) nrf_cloud_fota_init(&fota_init);

	zassert_equal(1, sys_reboot_fake.call_count,
		"nrf_cloud_fota_init should force a cold reboot after successful fota");
	zassert_equal(SYS_REBOOT_COLD, sys_reboot_fake.arg0_history[0],
		"nrf_cloud_fota_init should force a cold reboot after successful fota");
}

/* nrf_cloud_fota_init succeeds with return code 1
 * if nrf_cloud_fota_pending_job_validate returns 0 */
ZTEST(nrf_cloud_fota_test, test_nrf_cloud_fota_init_valid_param)
{
	struct nrf_cloud_fota_init_param fota_init = {
		.evt_cb = nrf_cloud_fota_cb_handler,
		.smp_reset_cb = NULL
	};

	int ret = nrf_cloud_fota_init(&fota_init);

	zassert_equal(1, ret,
		"nrf_cloud_fota_init should succeed with valid parameters");
}

typedef void (*http_fota_handler_t)(const struct fota_download_evt *evt);

/* nrf_cloud_fota_uninit fails if FOTA is curently in progress */
ZTEST(nrf_cloud_fota_test, test_nrf_cloud_fota_uninit_fota_in_progress)
{
	/* init fota lib */
	struct nrf_cloud_fota_init_param fota_init = {
		.evt_cb = nrf_cloud_fota_cb_handler,
		.smp_reset_cb = NULL
	};

	int ret = nrf_cloud_fota_init(&fota_init);
	zassert_equal(1, ret,
		"nrf_cloud_fota_init should succeed with valid parameters");


	/* simulate new job */
	// TODO initialize sub_topics[SUB_TOPIC_IDX_RCV].topic.utf8
	const struct mqtt_evt evt = {
		.type = MQTT_EVT_PUBLISH,
		.param.publish.message.payload.data = "some data",
	};
	nrf_cloud_fota_mqtt_evt_handler(&evt);
	/* simulate some progress */
	// http_fota_handler_t handler = fota_download_init_fake.arg0_history[0];
	// const struct fota_download_evt evt = { .id = FOTA_DOWNLOAD_EVT_PROGRESS,
	// 				       .progress = 42 };
	// handler(&evt);


	zassert_equal(-EBUSY, nrf_cloud_fota_uninit(),
		"nrf_cloud_fota_uninit should fail if FOTA is in progress");
}
