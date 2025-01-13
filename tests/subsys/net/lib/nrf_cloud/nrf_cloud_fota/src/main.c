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

struct nrf_cloud_fota_c_ctx {
	struct mqtt_client ** client_mqtt;
	nrf_cloud_fota_callback_t * event_cb;
	nrf_cloud_fota_ble_callback_t * ble_cb;
	bool * initialized;
	bool * fota_dl_initialized;
	bool * reboot_on_init;
	bool * fota_report_ack_pending;
	enum fota_download_evt_id * last_fota_dl_evt;
	struct nrf_cloud_fota_job * current_fota;
	struct nrf_cloud_settings_fota_job * saved_job;
	struct mqtt_topic * sub_topics;
	struct mqtt_topic * topic_updt;
	struct mqtt_topic * topic_req;
	size_t  sub_topics_size;
};

/* get pointers to all internal vars of the library */
void access_internal_state(struct nrf_cloud_fota_c_ctx* ctx);

/* This function runs before each test */
static void run_before(void *fixture)
{
	ARG_UNUSED(fixture);
	reset_all_static_vars();
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
	RESET_FAKE(nrf_cloud_obj_fota_ble_job_request_create);
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
 * if nrf_cloud_fota_pending_job_validate returns 0
 * also, nrf_cloud_fota_uninit should succeed if FOTA is not in progress */
ZTEST(nrf_cloud_fota_test, test_nrf_cloud_fota_init_valid_param)
{
	struct nrf_cloud_fota_init_param fota_init = {
		.evt_cb = nrf_cloud_fota_cb_handler,
		.smp_reset_cb = NULL
	};

	int ret = nrf_cloud_fota_init(&fota_init);

	zassert_equal(1, ret,
		"nrf_cloud_fota_init should succeed with valid parameters");

	zassert_equal(false, nrf_cloud_fota_is_active(),
		"nrf_cloud_fota_is_active should return false if FOTA is not in progress");

	zassert_equal(0, nrf_cloud_fota_uninit(),
		"nrf_cloud_fota_uninit should succeed if FOTA is not in progress");
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


	// initialize sub_topics
	struct nrf_cloud_fota_c_ctx ctx;
	access_internal_state(&ctx);
	ctx.sub_topics[0].topic.utf8 = "lte-receive-topic";
	ctx.sub_topics[0].topic.size = strlen(ctx.sub_topics[0].topic.utf8);
	ctx.sub_topics[1].topic.utf8 = "ble-receive-topic";
	ctx.sub_topics[1].topic.size = strlen(ctx.sub_topics[1].topic.utf8);

	char job_payload[100] = "foo";
	enum nrf_cloud_fota_type fota_type = NRF_CLOUD_FOTA_TYPE__INVALID;

	/* simulate new job */
	const struct mqtt_evt evt = {
		.type = MQTT_EVT_PUBLISH,
		.param.publish.message_id = 42,
		.param.publish.message.payload.data = job_payload,
		.param.publish.message.payload.len = strlen(job_payload),
		.param.publish.message.topic.topic.utf8 = ctx.sub_topics[0].topic.utf8,
		.param.publish.message.topic.topic.size = ctx.sub_topics[0].topic.size,
	};
	nrf_cloud_calloc_fake.return_val = job_payload;
	nrf_cloud_fota_job_decode_fake.custom_fake = fake_nrf_cloud_fota_job_decode__newjob;
	// mqtt_readall_publish_payload (skip that)
	nrf_cloud_fota_mqtt_evt_handler(&evt);
	/* simulate some progress */
	http_fota_handler_t handler = fota_download_init_fake.arg0_history[0];
	struct fota_download_evt evt2 = { .id = FOTA_DOWNLOAD_EVT_PROGRESS,
	 				       .progress = 42 };
	handler(&evt2);

	zassert_equal(true, nrf_cloud_fota_is_active(),
		"nrf_cloud_fota_is_active should return true if FOTA is in progress");

	zassert_equal(-EBUSY, nrf_cloud_fota_uninit(),
		"nrf_cloud_fota_uninit should fail if FOTA is in progress");

	/* make FOTA pending */
	evt2.id = FOTA_DOWNLOAD_EVT_FINISHED;
	handler(&evt2);
	(void) nrf_cloud_fota_pending_job_type_get(&fota_type);

	zassert_equal(NRF_CLOUD_FOTA_MODEM_DELTA, fota_type,
		"nrf_cloud_fota_is_active should return true if FOTA is in progress");
}

/* nrf_cloud_fota_ble_job_update fails on invalid params */
ZTEST(nrf_cloud_fota_test, test_nrf_cloud_fota_ble_job_update_invalid_params)
{
	zassert_equal(-EINVAL, nrf_cloud_fota_ble_job_update(NULL, NRF_CLOUD_FOTA_REJECTED),
		"nrf_cloud_fota_ble_job_update should check parameter validity");
}

/* nrf_cloud_fota_ble_job_update fails if no client_mqtt is registered */
ZTEST(nrf_cloud_fota_test, test_nrf_cloud_fota_ble_job_update_no_client)
{
	struct nrf_cloud_fota_ble_job ble_job = {
		.error = NRF_CLOUD_FOTA_ERROR_NONE,
		.dl_progress = 0,
	};

	zassert_equal(-ENXIO, nrf_cloud_fota_ble_job_update(&ble_job, NRF_CLOUD_FOTA_REJECTED),
		"nrf_cloud_fota_ble_job_update should check that client_mqtt is registered");
}

/* nrf_cloud_fota_ble_job_update succeeds after initialization
 * TODO: should we check for lib init? */
ZTEST(nrf_cloud_fota_test, test_nrf_cloud_fota_ble_job_update_success)
{
	struct nrf_cloud_fota_ble_job ble_job = {
		.error = NRF_CLOUD_FOTA_ERROR_NONE,
		.dl_progress = 0,
	};

	struct mqtt_client client;
	struct mqtt_utf8 endpoint = {
		.utf8 = "endpoint",
		.size = strlen("endpoint"),
	};
	char buf[100];

	nrf_cloud_calloc_fake.return_val = buf;

	zassert_equal(0, nrf_cloud_fota_endpoint_set_and_report(&client, "client_id", &endpoint),
		"nrf_cloud_fota_endpoint_set_and_report should check that topic is set");

	zassert_equal(0, nrf_cloud_fota_ble_job_update(&ble_job, NRF_CLOUD_FOTA_REJECTED),
		"nrf_cloud_fota_ble_job_update should succeed");
}

/* nrf_cloud_fota_ble_set_handler fails on invalid param */
ZTEST(nrf_cloud_fota_test, test_nrf_cloud_fota_ble_set_handler_invalid_param)
{
	zassert_equal(-EINVAL, nrf_cloud_fota_ble_set_handler(NULL),
		"nrf_cloud_fota_ble_set_handler should check parameter validity");
}

/* nrf_cloud_fota_ble_set_handler succeeds on valid cb */
ZTEST(nrf_cloud_fota_test, test_nrf_cloud_fota_ble_set_handler_success)
{
	zassert_equal(0, nrf_cloud_fota_ble_set_handler(fake_nrf_cloud_fota_ble_callback__dummy),
		"nrf_cloud_fota_ble_set_handler should succeed with valid parameters");
}

/* nrf_cloud_fota_ble_update_check fails on invalid params */
ZTEST(nrf_cloud_fota_test, test_nrf_cloud_fota_ble_update_check_invalid_params)
{
	zassert_equal(-EINVAL, nrf_cloud_fota_ble_update_check(NULL),
		"nrf_cloud_fota_ble_update_check should check parameter validity");
}

/* nrf_cloud_fota_ble_update_check fails if no client_mqtt is registered */
ZTEST(nrf_cloud_fota_test, test_nrf_cloud_fota_ble_update_check_no_client)
{
	const bt_addr_t ble_id = { 0 };
	zassert_equal(-ENXIO, nrf_cloud_fota_ble_update_check(&ble_id),
		"nrf_cloud_fota_ble_update_check should check that client_mqtt is registered");
}
/* nrf_cloud_fota_ble_update_check succeeds after initialization */
ZTEST(nrf_cloud_fota_test, test_nrf_cloud_fota_ble_update_check_success)
{
	struct mqtt_client client;
	struct mqtt_utf8 endpoint = {
		.utf8 = "endpoint",
		.size = strlen("endpoint"),
	};
	char buf[100];
	const bt_addr_t ble_id = { 0 };

	nrf_cloud_calloc_fake.return_val = buf;

	zassert_equal(0, nrf_cloud_fota_endpoint_set_and_report(&client, "client_id", &endpoint),
		"nrf_cloud_fota_endpoint_set_and_report should check that topic is set");

	zassert_equal(0, nrf_cloud_fota_ble_update_check(&ble_id),
		"nrf_cloud_fota_ble_update_check should succeed");
}

/* nrf_cloud_fota_endpoint_set_and_report fails on invalid params */
ZTEST(nrf_cloud_fota_test, test_nrf_cloud_fota_endpoint_set_and_report_invalid_params)
{
	struct mqtt_utf8 endpoint = {
		.utf8 = "endpoint",
		.size = strlen("endpoint"),
	};
	struct mqtt_client client;
	zassert_equal(-EINVAL, nrf_cloud_fota_endpoint_set_and_report(NULL, "client_id", &endpoint),
		"nrf_cloud_fota_endpoint_set_and_report should check that topic is set");
	zassert_equal(-EINVAL, nrf_cloud_fota_endpoint_set_and_report(&client, NULL, &endpoint),
		"nrf_cloud_fota_endpoint_set_and_report should check parameter validity");
	zassert_equal(-EINVAL, nrf_cloud_fota_endpoint_set_and_report(&client, "client_id", NULL),
		"nrf_cloud_fota_endpoint_set_and_report should check parameter validity");
	endpoint.size = 0;
	zassert_equal(-EINVAL, nrf_cloud_fota_endpoint_set_and_report(&client, "client_id", &endpoint),
		"nrf_cloud_fota_endpoint_set_and_report should check that topic is set");
	endpoint.size = strlen("endpoint");
	endpoint.utf8 = NULL;
	zassert_equal(-EINVAL, nrf_cloud_fota_endpoint_set_and_report(&client, "client_id", &endpoint),
		"nrf_cloud_fota_endpoint_set_and_report should check that topic is set");
}

/* nrf_cloud_fota_endpoint_set fails if allocation fails, cleanup procedure is followed */
ZTEST(nrf_cloud_fota_test, test_nrf_cloud_fota_endpoint_set_allocation_fails)
{
	struct mqtt_utf8 endpoint = {
		.utf8 = "endpoint",
		.size = strlen("endpoint"),
	};
	struct mqtt_client client;

	nrf_cloud_calloc_fake.return_val = NULL;

	zassert_equal(-ENOMEM, nrf_cloud_fota_endpoint_set_and_report(&client, "client_id", &endpoint),
		"nrf_cloud_fota_endpoint_set_and_report should check that topic is set");

	/* reset_topics does 6 frees and one additional free for the failed alloc (should we do that?)*/
	zassert_equal(6+6+1, nrf_cloud_free_fake.call_count,
		"nrf_cloud_fota_endpoint_set_and_report should free memory on failure");
}

/* nrf_cloud_fota_endpoint_clear clears the mqtt client and all topics */
ZTEST(nrf_cloud_fota_test, test_nrf_cloud_fota_endpoint_clear)
{
	struct mqtt_client client;
	struct mqtt_utf8 endpoint = {
		.utf8 = "endpoint",
		.size = strlen("endpoint"),
	};
	char buf[100];
	struct nrf_cloud_fota_c_ctx ctx;
	access_internal_state(&ctx);

	nrf_cloud_calloc_fake.return_val = buf;

	zassert_equal(0, nrf_cloud_fota_endpoint_set_and_report(&client, "client_id", &endpoint),
		"nrf_cloud_fota_endpoint_set_and_report should check that topic is set");

	zassert_equal(buf, ctx.sub_topics[0].topic.utf8,
		"nrf_cloud_fota_endpoint_set_and_report should set topic");
	zassert_equal(buf, ctx.sub_topics[1].topic.utf8,
		"nrf_cloud_fota_endpoint_set_and_report should set topic");
	zassert_equal(buf, ctx.topic_updt->topic.utf8,
		"nrf_cloud_fota_endpoint_set_and_report should set topic");
	zassert_equal(buf, ctx.topic_req->topic.utf8,
		"nrf_cloud_fota_endpoint_set_and_report should set topic");
	zassert_equal(&client, *ctx.client_mqtt,
		"nrf_cloud_fota_endpoint_set_and_report should set client");

	nrf_cloud_fota_endpoint_clear();

	zassert_equal(NULL, ctx.sub_topics[0].topic.utf8,
		"nrf_cloud_fota_endpoint_clear should clear topic");
	zassert_equal(NULL, ctx.sub_topics[1].topic.utf8,
		"nrf_cloud_fota_endpoint_clear should clear topic");
	zassert_equal(NULL, ctx.topic_updt->topic.utf8,
		"nrf_cloud_fota_endpoint_clear should clear topic");
	zassert_equal(NULL, ctx.topic_req->topic.utf8,
		"nrf_cloud_fota_endpoint_clear should clear topic");
	zassert_equal(NULL, *ctx.client_mqtt,
		"nrf_cloud_fota_endpoint_clear should clear client");
}

/* cannot really test nrf_cloud_fota_job_start with CONFIG_NRF_CLOUD_FOTA_AUTO_START_JOB
 * note: there is nothing using this function in the SDK. */
