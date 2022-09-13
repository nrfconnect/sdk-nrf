/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <net/nrf_cloud.h>
#include "nrf_cloud_fakes.h"

/* Simple event handler needed for the init function to
 * have a success initialization in the test below
 */
void event_handler(const struct nrf_cloud_evt *const evt)
{
	switch (evt->type) {
	case NRF_CLOUD_EVT_READY:
		printk("Device is connected and ready");
		break;
	case NRF_CLOUD_EVT_TRANSPORT_CONNECTED:
		printk("Device is connected");
		break;
	case NRF_CLOUD_EVT_TRANSPORT_DISCONNECTED:
		printk("Device is disconnected");
		break;
	case NRF_CLOUD_EVT_ERROR:
		printk("Cloud error: %d", evt->status);
		break;
	default:
		break;
	}
}

int fake_nct_init__succeeds(const char *const client_id)
{
	return 0;
}

int fake_nct_init__fails(const char *const cliend_id)
{
	return -ENODEV;
}

int fake_nfsm_init__succeeds(void)
{
	return 0;
}

int fake_nrf_cloud_codec_init__succeeds(void)
{
	return 0;
}

int fake_nrf_cloud_fota_fmfu_dev_set__succeeds(const struct dfu_target_fmfu_fdev *const dev_inf)
{
	return 0;
}

int fake_nrf_cloud_fota_fmfu_dev_set__fails(const struct dfu_target_fmfu_fdev *const dev_inf)
{
	return -ENODEV;
}

int fake_nct_disconnect__succeeds(void)
{
	struct nrf_cloud_evt evt = {
		.type = NRF_CLOUD_EVT_TRANSPORT_DISCONNECTED,
	};

	nfsm_set_current_state_and_notify(STATE_INITIALIZED, &evt);
	return 0;
}

int fake_nct_disconnect__not_actually_disconnect(void)
{
	return 0;
}

int fake_nrf_cloud_fota_uninit__succeeds(void)
{
	return 0;
}

int fake_nrf_cloud_fota_uninit__busy(void)
{
	return -EBUSY;
}

int fake_nct_connect__succeeds(void)
{
	nfsm_set_current_state_and_notify(STATE_CONNECTED, NULL);
	return 0;
}

int fake_nct_connect__invalid(void)
{
	return -EINVAL;
}

int fake_nct_socket_get_data(void)
{
	return 1234;
}

int fake_nct_keepalive_time_left_get(void)
{
	return 1000;
}

int fake_nct_process__succeeds(void)
{
	return 0;
}

/* This function runs before each test */
static void run_before(void *fixture)
{
	RESET_FAKE(nct_init);
	RESET_FAKE(nfsm_init);
	RESET_FAKE(nrf_cloud_codec_init);
	RESET_FAKE(nrf_cloud_fota_fmfu_dev_set);
	RESET_FAKE(nct_disconnect);
	RESET_FAKE(nct_uninit);
	RESET_FAKE(nrf_cloud_fota_uninit);
	RESET_FAKE(nct_connect);
	RESET_FAKE(nct_socket_get);
	RESET_FAKE(nct_keepalive_time_left);
	RESET_FAKE(nct_process);
}

/* This function runs after each completed test */
static void run_after(void *fixture)
{
#if defined(CONFIG_NRF_CLOUD_FOTA)
	nrf_cloud_fota_uninit_fake.custom_fake = fake_nrf_cloud_fota_uninit__succeeds;
#endif
	nct_disconnect_fake.custom_fake = fake_nct_disconnect__succeeds;

	/* Uninit to reset the internal state to idle */
	nrf_cloud_uninit();
}

ZTEST_SUITE(nrf_cloud_test, NULL, NULL, run_before, run_after, NULL);

/* TEST NRF_CLOUD_INIT */

/* The library can only be initialized when the internal state is
 * idle, so the test verifies that nrf_cloud_init function will
 * return with permission denied error (-EACCES) when not idle
 */
ZTEST(nrf_cloud_test, test_init_not_idle)
{
	struct nrf_cloud_init_param params = {
		.event_handler = event_handler,
		.client_id = NULL,
	};

	zassert_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state (uninitialized) at the start of test");

	/* Call nrf_cloud_init twice with the first one
	 * being successful and check if the second one
	 * will fail since the state will not be idle
	 */
	nct_init_fake.custom_fake = fake_nct_init__succeeds;
	nfsm_init_fake.custom_fake = fake_nfsm_init__succeeds;
	nrf_cloud_codec_init_fake.custom_fake = fake_nrf_cloud_codec_init__succeeds;
	nrf_cloud_init(&params);

	int ret = nrf_cloud_init(&params);

	zassert_not_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should not be in the idle state after initialization");

	zassert_equal(-EACCES, ret, "return should be -EACCES when not idle");
}

/* Verify that nrf_cloud_init function will throw an error
 * of invalid argument (-EINVAL) when given null params
 */
ZTEST(nrf_cloud_test, test_init_null_params)
{
	zassert_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state (uninitialized) at the start of test");

	int ret = nrf_cloud_init(NULL);

	zassert_equal(-EINVAL, ret, "return should be -EINVAL with NULL params");
}

/* Verify that nrf_cloud_init function will throw an
 * error of invalid argument (-EINVAL) when a null
 * event handler is passed in as part of its parameters
 */
ZTEST(nrf_cloud_test, test_init_null_handler)
{
	struct nrf_cloud_init_param params = {
		.event_handler = NULL,
		.client_id = NULL,
	};

	zassert_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state (uninitialized) at the start of test");

	int ret = nrf_cloud_init(&params);

	zassert_equal(-EINVAL, ret, "return should be -EINVAL with NULL event handler");
}

/* Verify that nrf_cloud_init function succeeds given valid internal
 * state, event handler with full modem FOTA update is DISABLED.
 * Other tests will not need FMFU since it is already tested here.
 */
ZTEST(nrf_cloud_test, test_init_success_no_fota)
{
	struct nrf_cloud_init_param params = {
		.event_handler = event_handler,
		.client_id = NULL,
	};

	zassert_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state (uninitialized) at the start of test");

	/* Expect all fakes to run successfully */
	nct_init_fake.custom_fake = fake_nct_init__succeeds;
	nfsm_init_fake.custom_fake = fake_nfsm_init__succeeds;
	nrf_cloud_codec_init_fake.custom_fake = fake_nrf_cloud_codec_init__succeeds;

	int ret = nrf_cloud_init(&params);

	zassert_ok(ret, "return should be 0 when init success");
}

/* Verify that nrf_cloud_init function succeeds given valid internal
 * state, event handler with full modem FOTA update is ENABLED.
 * Other tests will not need FMFU since it is already tested here.
 */
ZTEST(nrf_cloud_test, test_init_success_with_fota)
{
	Z_TEST_SKIP_IFNDEF(CONFIG_NRF_CLOUD_FOTA_FULL_MODEM_UPDATE);
	struct nrf_cloud_init_param params = {
		.event_handler = event_handler,
		.client_id = NULL,
	};

	zassert_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state (uninitialized) at the start of test");

	/* Expect all fakes to run successfully */
	nct_init_fake.custom_fake = fake_nct_init__succeeds;
	nfsm_init_fake.custom_fake = fake_nfsm_init__succeeds;
	nrf_cloud_codec_init_fake.custom_fake = fake_nrf_cloud_codec_init__succeeds;
	nrf_cloud_fota_fmfu_dev_set_fake.custom_fake = fake_nrf_cloud_fota_fmfu_dev_set__succeeds;

	int ret = nrf_cloud_init(&params);

	zassert_ok(ret, "return should be 0 when init success");
}

/* Verify that nrf_cloud_init function will fail
 * when the transport routine fails to initialize
 */
ZTEST(nrf_cloud_test, test_init_cloud_transport_fail)
{
	struct nrf_cloud_init_param params = {
		.event_handler = event_handler,
		.client_id = NULL,
	};

	/* Expect an error return value for nct_init */
	nct_init_fake.custom_fake = fake_nct_init__fails;

	/* Expect other fakes to run successfully */
	nfsm_init_fake.custom_fake = fake_nfsm_init__succeeds;
	nrf_cloud_codec_init_fake.custom_fake = fake_nrf_cloud_codec_init__succeeds;

	zassert_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state (uninitialized) at the start of test");

	int ret = nrf_cloud_init(&params);

	zassert_not_equal(0, ret, "return should be non-zero when transport routine init failed");
}

/* Verify that nrf_cloud_init function will fail
 * when full modem FOTA update fails to initialize.
 * Ignored when full modem FOTA update is disabled
 */
ZTEST(nrf_cloud_test, test_init_fota_failed)
{
	Z_TEST_SKIP_IFNDEF(CONFIG_NRF_CLOUD_FOTA_FULL_MODEM_UPDATE);
	struct dfu_target_fmfu_fdev fmfu_dev_info = {
		.dev = NULL,
		.offset = 0L,
		.size = 0U,
	};

	struct nrf_cloud_init_param params = {
		.event_handler = event_handler,
		.client_id = NULL,
		.fmfu_dev_inf = &fmfu_dev_info,
	};

	/* Expect an error return value for nrf_cloud_fota_fmfu_dev_set */
	nrf_cloud_fota_fmfu_dev_set_fake.custom_fake = fake_nrf_cloud_fota_fmfu_dev_set__fails;

	/* Expect other fakes to run successfully */
	nct_init_fake.custom_fake = fake_nct_init__succeeds;
	nfsm_init_fake.custom_fake = fake_nfsm_init__succeeds;
	nrf_cloud_codec_init_fake.custom_fake = fake_nrf_cloud_codec_init__succeeds;

	zassert_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state (uninitialized) at the start of test");

	int ret = nrf_cloud_init(&params);

	zassert_not_equal(0, ret, "return should be non-zero when FOTA init failed");
}

/* TEST NFSM_SET_CURRENT_STATE_AND_NOTIFY */

/* Verify that nfsm_set_current_state_and notify
 * wil change to the right state that is provided.
 */
ZTEST(nrf_cloud_test, test_set_current_state)
{
	zassert_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state (uninitialized) at the start of test");

	nfsm_set_current_state_and_notify(STATE_TOTAL, NULL);

	zassert_equal(STATE_TOTAL, nfsm_get_current_state(),
		"nrf_cloud lib should be in STATE_TOTAL after changing");
}

/* TEST NRF_CLOUD_UNINIT */

/* Verify that nrf_cloud_uninit function will fail
 * with active FOTA. Ignored when FOTA is disabled.
 */
ZTEST(nrf_cloud_test, test_uninit_fota_active)
{
	Z_TEST_SKIP_IFNDEF(CONFIG_NRF_CLOUD_FOTA);
	zassert_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state (uninitialized) at the start of test");

	struct nrf_cloud_init_param params = {
		.event_handler = event_handler,
		.client_id = NULL,
	};

	/* Init the cloud with all fakes */
	nct_init_fake.custom_fake = fake_nct_init__succeeds;
	nfsm_init_fake.custom_fake = fake_nfsm_init__succeeds;
	nrf_cloud_codec_init_fake.custom_fake = fake_nrf_cloud_codec_init__succeeds;
	nrf_cloud_init(&params);

	/* Expect nrf_cloud_fota_uninit fake returns busy */
	nrf_cloud_fota_uninit_fake.custom_fake = fake_nrf_cloud_fota_uninit__busy;
	nct_disconnect_fake.custom_fake = fake_nct_disconnect__succeeds;

	int ret = nrf_cloud_uninit();

	zassert_equal(-EBUSY, ret, "return should be -EBUSY when a FOTA job is active");
}

/* Verify that nrf_cloud_uninit function succeeds
 * with unactive FOTA. Ignored when FOTA is disabled.
 * Other tests will not need FOTA since it is tested.
 */
ZTEST(nrf_cloud_test, test_uninit_success_fota_unactive)
{
	Z_TEST_SKIP_IFNDEF(CONFIG_NRF_CLOUD_FOTA);
	zassert_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state (uninitialized) at the start of test");

	struct nrf_cloud_init_param params = {
		.event_handler = event_handler,
		.client_id = NULL,
	};

	/* Init the cloud with all fakes */
	nct_init_fake.custom_fake = fake_nct_init__succeeds;
	nfsm_init_fake.custom_fake = fake_nfsm_init__succeeds;
	nrf_cloud_codec_init_fake.custom_fake = fake_nrf_cloud_codec_init__succeeds;
	nrf_cloud_init(&params);

	/* Expect all fakes return success */
	nrf_cloud_fota_uninit_fake.custom_fake = fake_nrf_cloud_fota_uninit__succeeds;
	nct_disconnect_fake.custom_fake = fake_nct_disconnect__succeeds;

	int ret = nrf_cloud_uninit();

	zassert_equal(0, ret, "return should be 0 when uninit success");
}

/* Verify that nrf_cloud_uninit function fails
 * when nrf_cloud is not disconnected. Ignored
 * when the CONNECTION_POLL_THREAD is active.
 */
ZTEST(nrf_cloud_test, test_uninit_no_disconnect)
{
	/* This test is ignored when the connection poll thread is active
	 * since it will triggered the transport disconnect event and give
	 * the uninit_disconnect semaphore which fails the test eventually
	 */
	Z_TEST_SKIP_IFDEF(CONFIG_NRF_CLOUD_CONNECTION_POLL_THREAD);
	zassert_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state (uninitialized) at the start of test");

	struct nrf_cloud_init_param params = {
		.event_handler = event_handler,
		.client_id = NULL,
	};

	/* Init the cloud successfully */
	nct_init_fake.custom_fake = fake_nct_init__succeeds;
	nfsm_init_fake.custom_fake = fake_nfsm_init__succeeds;
	nrf_cloud_codec_init_fake.custom_fake = fake_nrf_cloud_codec_init__succeeds;
	nrf_cloud_init(&params);

	/* Connect to the cloud */
	nct_connect_fake.custom_fake = fake_nct_connect__succeeds;
	nrf_cloud_connect(NULL);

	/* Uninit without actually disconnect */
	nct_disconnect_fake.custom_fake = fake_nct_disconnect__not_actually_disconnect;

	int ret = nrf_cloud_uninit();

	zassert_equal(-EISCONN, ret, "return should be -EISCONN when cloud is not disconnected");
}

/* Verify that nrf_cloud_uninit function
 * suceeds when nrf_cloud is disconnected.
 */
ZTEST(nrf_cloud_test, test_uninit_success)
{
	zassert_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state (uninitialized) at the start of test");

	struct nrf_cloud_init_param params = {
		.event_handler = event_handler,
		.client_id = NULL,
	};

	/* Init the cloud successfully */
	nct_init_fake.custom_fake = fake_nct_init__succeeds;
	nfsm_init_fake.custom_fake = fake_nfsm_init__succeeds;
	nrf_cloud_codec_init_fake.custom_fake = fake_nrf_cloud_codec_init__succeeds;
	nrf_cloud_init(&params);

	/* Connect to the cloud */
	nct_connect_fake.custom_fake = fake_nct_connect__succeeds;
	nrf_cloud_connect(NULL);

	/* Uninit with disconnect */
	nct_disconnect_fake.custom_fake = fake_nct_disconnect__succeeds;

	int ret = nrf_cloud_uninit();

	zassert_ok(ret, "return should be 0 when uninit success");
}

/* TEST NRF_CLOUD_CONNECT */

/* Verify that nrf_cloud_connect function will
 * fail when the cloud internal state is idle.
 */
ZTEST(nrf_cloud_test, test_connect_idle)
{
	zassert_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state (uninitialized) at the start of test");
	zassert_equal(NRF_CLOUD_CONNECT_RES_ERR_NOT_INITD, nrf_cloud_connect(NULL),
		"return should be RES_ERR_NOT_INITD when in the idle state");
}

/* Verify that nrf_cloud_connect function will
 * fail when connect to cloud with invalid key.
 * Ignored when CONNECTION_POLL_THREAD enabled.
 */
ZTEST(nrf_cloud_test, test_connect_invalid_private_key_no_connection_poll)
{
	Z_TEST_SKIP_IFDEF(CONFIG_NRF_CLOUD_CONNECTION_POLL_THREAD);
	zassert_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state (uninitialized) at the start of test");

	struct nrf_cloud_init_param params = {
		.event_handler = event_handler,
		.client_id = NULL,
	};

	/* Init the cloud successfully */
	nct_init_fake.custom_fake = fake_nct_init__succeeds;
	nfsm_init_fake.custom_fake = fake_nfsm_init__succeeds;
	nrf_cloud_codec_init_fake.custom_fake = fake_nrf_cloud_codec_init__succeeds;
	nrf_cloud_init(&params);

	/* Connect to cloud with invalid private key */
	nct_connect_fake.custom_fake = fake_nct_connect__invalid;

	int ret = nrf_cloud_connect(NULL);

	/* Verify failed with invalid key provided */
	zassert_equal(NRF_CLOUD_CONNECT_RES_ERR_PRV_KEY, ret,
		"return should be RES_ERR_PRV_KEY with invalid private key provided");
}

/* Verify that nrf_cloud_connect function will
 * fail when the cloud is already connected.
 * Ignored when CONNECTION_POLL_THREAD enabled.
 */
ZTEST(nrf_cloud_test, test_connect_already_connected_no_connection_poll)
{
	Z_TEST_SKIP_IFDEF(CONFIG_NRF_CLOUD_CONNECTION_POLL_THREAD);
	zassert_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state (uninitialized) at the start of test");

	struct nrf_cloud_init_param params = {
		.event_handler = event_handler,
		.client_id = NULL,
	};

	nct_init_fake.custom_fake = fake_nct_init__succeeds;
	nfsm_init_fake.custom_fake = fake_nfsm_init__succeeds;
	nrf_cloud_codec_init_fake.custom_fake = fake_nrf_cloud_codec_init__succeeds;
	nrf_cloud_init(&params);

	/* Connect to cloud */
	nct_connect_fake.custom_fake = fake_nct_connect__succeeds;
	nrf_cloud_connect(NULL);

	int ret = nrf_cloud_connect(NULL);

	/* Verify failed if already connected */
	zassert_equal(NRF_CLOUD_CONNECT_RES_ERR_ALREADY_CONNECTED, ret,
		"return should be RES_ERR_ALREADY_CONNECTED when cloud is already connected");
}

/* Verify that nrf_cloud_connect function will
 * succeed given a successful transport connection.
 * Ignored when CONNECTION_POLL_THREAD is enabled.
 */
ZTEST(nrf_cloud_test, test_connect_success_no_connection_poll)
{
	Z_TEST_SKIP_IFDEF(CONFIG_NRF_CLOUD_CONNECTION_POLL_THREAD);
	zassert_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state (uninitialized) at the start of test");

	struct nrf_cloud_init_param params = {
		.event_handler = event_handler,
		.client_id = NULL,
	};

	/* Init the cloud successfully */
	nct_init_fake.custom_fake = fake_nct_init__succeeds;
	nfsm_init_fake.custom_fake = fake_nfsm_init__succeeds;
	nrf_cloud_codec_init_fake.custom_fake = fake_nrf_cloud_codec_init__succeeds;
	nrf_cloud_init(&params);

	/* Connect to cloud */
	nct_connect_fake.custom_fake = fake_nct_connect__succeeds;

	int ret = nrf_cloud_connect(NULL);

	/* Verify successfully connected */
	zassert_ok(ret, "return should be 0 when cloud is connected successfully");
}

/* Verify that nrf_cloud_connect function will succeed given
 * a successful transport connection when CONNECTION_POLL_THREAD
 * is enabled. Ignored when CONNECTION_POLL_THREAD is disabled.
 */
ZTEST(nrf_cloud_test, test_connect_success_with_connection_poll)
{
	Z_TEST_SKIP_IFNDEF(CONFIG_NRF_CLOUD_CONNECTION_POLL_THREAD);
	zassert_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state (uninitialized) at the start of test");

	struct nrf_cloud_init_param params = {
		.event_handler = event_handler,
		.client_id = NULL,
	};

	/* Init the cloud successfully */
	nct_init_fake.custom_fake = fake_nct_init__succeeds;
	nfsm_init_fake.custom_fake = fake_nfsm_init__succeeds;
	nrf_cloud_codec_init_fake.custom_fake = fake_nrf_cloud_codec_init__succeeds;
	nrf_cloud_init(&params);

	/* Expect all fakes return success */
	nct_connect_fake.custom_fake = fake_nct_connect__succeeds;
	nct_disconnect_fake.custom_fake = fake_nct_connect__succeeds;
	nct_socket_get_fake.custom_fake = fake_nct_socket_get_data;
	nct_keepalive_time_left_fake.custom_fake = fake_nct_keepalive_time_left_get;
	nct_process_fake.custom_fake = fake_nct_process__succeeds;

	int ret = nrf_cloud_connect(NULL);

	/* Verify successfully connected */
	zassert_ok(ret, "return should be 0 when cloud is connected successfully");
}

/* Verify that nrf_cloud_connect function will fail when
 * connecting after CONNECTION_POLL_THREAD already run.
 * Ignored when CONNECTION_POLL_THREAD is disabled.
 */
ZTEST(nrf_cloud_test, test_connect_connection_poll_already_active)
{
	Z_TEST_SKIP_IFNDEF(CONFIG_NRF_CLOUD_CONNECTION_POLL_THREAD);
	zassert_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state (uninitialized) at the start of test");

	struct nrf_cloud_init_param params = {
		.event_handler = event_handler,
		.client_id = NULL,
	};

	/* Init the cloud successfully */
	nct_init_fake.custom_fake = fake_nct_init__succeeds;
	nfsm_init_fake.custom_fake = fake_nfsm_init__succeeds;
	nrf_cloud_codec_init_fake.custom_fake = fake_nrf_cloud_codec_init__succeeds;
	nrf_cloud_init(&params);

	/* Connect the cloud once */
	nct_connect_fake.custom_fake = fake_nct_connect__succeeds;
	nct_disconnect_fake.custom_fake = fake_nct_connect__succeeds;
	nct_socket_get_fake.custom_fake = fake_nct_socket_get_data;
	nct_keepalive_time_left_fake.custom_fake = fake_nct_keepalive_time_left_get;
	nct_process_fake.custom_fake = fake_nct_process__succeeds;
	nrf_cloud_connect(NULL);

	/* Try connecting again */
	int ret = nrf_cloud_connect(NULL);

	/* Verify failed if already connected */
	zassert_equal(NRF_CLOUD_CONNECT_RES_ERR_ALREADY_CONNECTED, ret,
		"return should be RES_ERR_ALREADY_CONNECTED when connection poll already active");
}

/* TEST NRF_CLOUD_DISCONNECT */

/* TEST NRF_CLOUD_SHADOW_UPDATE */

/* TEST NRF_CLOUD_SHADOW_DEVICE_STATUS_UPDATE */

/* TEST NRF_CLOUD_SENSOR_DATA_SEND */

/* TEST NRF_CLOUD_SENSOR_DATA_STREAM */

/* TEST NRF_CLOUD_SEND */

/* TEST START_CONNECTION_POLL */

/* TEST NRF_CLOUD_RUN */
