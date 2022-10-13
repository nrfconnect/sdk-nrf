/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <net/nrf_cloud.h>
#include "fakes.h"

/* Flag to indicate if the cloud has been connected */
static atomic_t cloud_connected_t;

/* Simple event handler needed for the init function to
 * have a successful initialization in the test below
 */
void event_handler(const struct nrf_cloud_evt *const evt)
{
	switch (evt->type) {
	case NRF_CLOUD_EVT_READY:
		printk("Device is connected and ready\n");
		break;
	case NRF_CLOUD_EVT_TRANSPORT_CONNECTED:
		atomic_set(&cloud_connected_t, 1);
		printk("Device is connected\n");
		break;
	case NRF_CLOUD_EVT_TRANSPORT_DISCONNECTED:
		atomic_set(&cloud_connected_t, 0);
		printk("Device is disconnected\n");
		break;
	case NRF_CLOUD_EVT_ERROR:
		printk("Cloud error: %d\n", evt->status);
		break;
	default:
		break;
	}
}

/* This function runs before each test */
static void run_before(void *fixture)
{
	RESET_FAKE(nct_init);
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
	(void)nrf_cloud_uninit();
}

ZTEST_SUITE(nrf_cloud_init_uninit_test, NULL, NULL, run_before, run_after, NULL);

/* TEST NRF_CLOUD_INIT */

/* The library can only be initialized when the internal state is
 * idle, so the test verifies that nrf_cloud_init function will
 * return with permission denied error (-EACCES) when not idle
 */
ZTEST(nrf_cloud_init_uninit_test, test_init_not_idle)
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
	nrf_cloud_codec_init_fake.custom_fake = fake_nrf_cloud_codec_init__succeeds;
	(void)nrf_cloud_init(&params);

	int ret = nrf_cloud_init(&params);

	zassert_not_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should not be in the idle state after initialization");

	zassert_equal(-EACCES, ret, "return should be -EACCES when not idle");
}

/* Verify that nrf_cloud_init function will throw an error
 * of invalid argument (-EINVAL) when given null params
 */
ZTEST(nrf_cloud_init_uninit_test, test_init_null_params)
{
	zassert_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state (uninitialized) at the start of test");

	int ret = nrf_cloud_init(NULL);

	zassert_equal(-EINVAL, ret, "return should be -EINVAL with NULL params");
	zassert_not_equal(STATE_INITIALIZED, nfsm_get_current_state(),
		"nrf_cloud lib should not be in the initialized state after init failed");
}

/* Verify that nrf_cloud_init function will throw an
 * error of invalid argument (-EINVAL) when a null
 * event handler is passed in as part of its parameters
 */
ZTEST(nrf_cloud_init_uninit_test, test_init_null_handler)
{
	struct nrf_cloud_init_param params = {
		.event_handler = NULL,
		.client_id = NULL,
	};

	zassert_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state (uninitialized) at the start of test");

	int ret = nrf_cloud_init(&params);

	zassert_equal(-EINVAL, ret, "return should be -EINVAL with NULL event handler");
	zassert_not_equal(STATE_INITIALIZED, nfsm_get_current_state(),
		"nrf_cloud lib should not be in the initialized state after init failed");
}

/* Verify that nrf_cloud_init function succeeds given
 * valid parameters. FMFU will not be considered here
 * since the fmfu device info has not been set.
 */
ZTEST(nrf_cloud_init_uninit_test, test_init_success)
{
	struct nrf_cloud_init_param params = {
		.event_handler = event_handler,
		.client_id = NULL,
	};

	zassert_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state (uninitialized) at the start of test");

	/* Expect all fakes to run successfully */
	nct_init_fake.custom_fake = fake_nct_init__succeeds;
	nrf_cloud_codec_init_fake.custom_fake = fake_nrf_cloud_codec_init__succeeds;

	int ret = nrf_cloud_init(&params);

	zassert_ok(ret, "return should be 0 when init success");
	zassert_equal(STATE_INITIALIZED, nfsm_get_current_state(),
		"nrf_cloud lib should be in the initialized state after initialization");
}

/* Verify that nrf_cloud_init function will fail
 * when the transport routine fails to initialize
 */
ZTEST(nrf_cloud_init_uninit_test, test_init_cloud_transport_fail)
{
	struct nrf_cloud_init_param params = {
		.event_handler = event_handler,
		.client_id = NULL,
	};

	/* Expect an error return value for nct_init */
	nct_init_fake.custom_fake = fake_nct_init__fails;

	/* Expect other fake to run successfully */
	nrf_cloud_codec_init_fake.custom_fake = fake_nrf_cloud_codec_init__succeeds;

	zassert_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state (uninitialized) at the start of test");

	int ret = nrf_cloud_init(&params);

	zassert_not_equal(0, ret, "return should be non-zero when transport routine init failed");
	zassert_not_equal(STATE_INITIALIZED, nfsm_get_current_state(),
		"nrf_cloud lib should not be in the initialized state after init failed");
}

/* Verify that nrf_cloud_init function will fail
 * when full modem FOTA update fails to initialize.
 * Ignored when full modem FOTA update is disabled.
 */
ZTEST(nrf_cloud_init_uninit_test, test_init_fota_failed)
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
	nrf_cloud_codec_init_fake.custom_fake = fake_nrf_cloud_codec_init__succeeds;

	zassert_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state (uninitialized) at the start of test");

	int ret = nrf_cloud_init(&params);

	zassert_not_equal(0, ret, "return should be non-zero when FOTA init failed");
	zassert_not_equal(STATE_INITIALIZED, nfsm_get_current_state(),
		"nrf_cloud lib should not be in the initialized state after init failed");
}

/* TEST NFSM_SET_CURRENT_STATE_AND_NOTIFY */

/* Verify that nfsm_set_current_state_and notify
 * will change to the right state that is provided.
 */
ZTEST(nrf_cloud_init_uninit_test, test_set_current_state)
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
ZTEST(nrf_cloud_init_uninit_test, test_uninit_fota_active)
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
	nrf_cloud_codec_init_fake.custom_fake = fake_nrf_cloud_codec_init__succeeds;
	(void)nrf_cloud_init(&params);

	zassert_equal(STATE_INITIALIZED, nfsm_get_current_state(),
		"nrf_cloud lib should be in the initialized state after initialization");

	/* Expect nrf_cloud_fota_uninit fake returns busy */
	nrf_cloud_fota_uninit_fake.custom_fake = fake_nrf_cloud_fota_uninit__busy;
	nct_disconnect_fake.custom_fake = fake_nct_disconnect__succeeds;

	int ret = nrf_cloud_uninit();

	zassert_equal(-EBUSY, ret, "return should be -EBUSY when a FOTA job is active");
	zassert_not_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should not be in idle state when uninit interrupted with active FOTA");
}

/* Verify that nrf_cloud_uninit function succeeds
 * with inactive FOTA. Ignored when FOTA is disabled.
 * Other tests will not need FOTA since it is tested.
 */
ZTEST(nrf_cloud_init_uninit_test, test_uninit_success_fota_unactive)
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
	nrf_cloud_codec_init_fake.custom_fake = fake_nrf_cloud_codec_init__succeeds;
	(void)nrf_cloud_init(&params);

	zassert_equal(STATE_INITIALIZED, nfsm_get_current_state(),
		"nrf_cloud lib should be in the initialized state after initialization");

	/* Expect all fakes return success */
	nrf_cloud_fota_uninit_fake.custom_fake = fake_nrf_cloud_fota_uninit__succeeds;
	nct_disconnect_fake.custom_fake = fake_nct_disconnect__succeeds;

	int ret = nrf_cloud_uninit();

	zassert_equal(0, ret, "return should be 0 when uninit success");
	zassert_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state after uninit");
}

/* Verify that nrf_cloud_uninit function fails
 * when nrf_cloud is not disconnected. Ignored
 * when the CONNECTION_POLL_THREAD is active.
 */
ZTEST(nrf_cloud_init_uninit_test, test_uninit_no_disconnect)
{
	/* This test is ignored when the connection poll thread is active
	 * since it will trigger the transport disconnect event and give
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
	nrf_cloud_codec_init_fake.custom_fake = fake_nrf_cloud_codec_init__succeeds;
	(void)nrf_cloud_init(&params);

	zassert_equal(STATE_INITIALIZED, nfsm_get_current_state(),
		"nrf_cloud lib should be in the initialized state after initialization");

	/* Connect to the cloud */
	nct_connect_fake.custom_fake = fake_nct_connect__succeeds;
	(void)nrf_cloud_connect();

	/* Wait for the connected event */
	while (!atomic_get(&cloud_connected_t)) {
		k_sleep(K_MSEC(200));
	}

	zassert_equal(STATE_CONNECTED, nfsm_get_current_state(),
		"nrf_cloud lib should be in the connected state after successfully connected");

	/* Uninit without actually disconnect */
	nct_disconnect_fake.custom_fake = fake_nct_disconnect__not_actually_disconnect;

	int ret = nrf_cloud_uninit();

	zassert_not_equal(STATE_INITIALIZED, nfsm_get_current_state(),
		"nrf_cloud lib should not be in the initialized state when not disconnected");
	zassert_equal(-EISCONN, ret, "return should be -EISCONN when cloud is not disconnected");
	zassert_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state after uninit");
}

/* Verify that nrf_cloud_uninit function
 * succeeds after nrf_cloud is connected.
 */
ZTEST(nrf_cloud_init_uninit_test, test_uninit_success_after_connected)
{
	zassert_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state (uninitialized) at the start of test");

	struct nrf_cloud_init_param params = {
		.event_handler = event_handler,
		.client_id = NULL,
	};

	/* Init the cloud successfully */
	nct_init_fake.custom_fake = fake_nct_init__succeeds;
	nrf_cloud_codec_init_fake.custom_fake = fake_nrf_cloud_codec_init__succeeds;
	(void)nrf_cloud_init(&params);

	zassert_equal(STATE_INITIALIZED, nfsm_get_current_state(),
		"nrf_cloud lib should be in the initialized state after initialization");

	/* Connect to the cloud successfully */
	nct_connect_fake.custom_fake = fake_nct_connect__succeeds;
	nct_disconnect_fake.custom_fake = fake_nct_disconnect__succeeds;
	/* These fakes are for the poll thread */
	nct_socket_get_fake.custom_fake = fake_nct_socket_get_data;
	nct_keepalive_time_left_fake.custom_fake = fake_nct_keepalive_time_left_get;
	nct_process_fake.custom_fake = fake_nct_process__succeeds;
	(void)nrf_cloud_connect();

	/* Wait for the connected event */
	while (!atomic_get(&cloud_connected_t)) {
		k_sleep(K_MSEC(200));
	}

	zassert_equal(STATE_CONNECTED, nfsm_get_current_state(),
		"nrf_cloud lib should be in the connected state after successfully connected");

	int ret = nrf_cloud_uninit();

	/* Wait for the disconnected event */
	while (atomic_get(&cloud_connected_t)) {
		k_sleep(K_MSEC(200));
	}

	zassert_ok(ret, "return should be 0 when uninit success");
	zassert_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state after uninit");
}

/* Verify that nrf_cloud_uninit function succeeds
 * after initialized without cloud connection.
 */
ZTEST(nrf_cloud_init_uninit_test, test_uninit_success_after_init_no_connection)
{
	zassert_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state (uninitialized) at the start of test");

	struct nrf_cloud_init_param params = {
		.event_handler = event_handler,
		.client_id = NULL,
	};

	/* Init the cloud successfully */
	nct_init_fake.custom_fake = fake_nct_init__succeeds;
	nrf_cloud_codec_init_fake.custom_fake = fake_nrf_cloud_codec_init__succeeds;
	(void)nrf_cloud_init(&params);

	zassert_equal(STATE_INITIALIZED, nfsm_get_current_state(),
		"nrf_cloud lib should be in the initialized state after initialization");

	/* Uninit successfully */
	int ret = nrf_cloud_uninit();

	zassert_ok(ret, "return should be 0 when uninit success");
	zassert_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state after uninit");
}

/* Verify that nrf_cloud_uninit function will fail when the
 * poll thread does not become inactive after 10 seconds.
 * Ignored if CONNECTION_POLL_THREAD is disabled.
 */
ZTEST(nrf_cloud_init_uninit_test, test_uninit_poll_thread_inactive_timer_expired)
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
	nrf_cloud_codec_init_fake.custom_fake = fake_nrf_cloud_codec_init__succeeds;
	(void)nrf_cloud_init(&params);

	zassert_equal(STATE_INITIALIZED, nfsm_get_current_state(),
		"nrf_cloud lib should be in the initialized state after initialization");

	/* Connect to the cloud successfully */
	nct_connect_fake.custom_fake = fake_nct_connect__succeeds;
	nct_disconnect_fake.custom_fake = fake_nct_disconnect__succeeds;
	nct_socket_get_fake.custom_fake = fake_nct_socket_get_data;
	nct_keepalive_time_left_fake.custom_fake = fake_nct_keepalive_time_left_long_timeout;
	nct_process_fake.custom_fake = fake_nct_process__succeeds;
	(void)nrf_cloud_connect();

	/* Wait for the connected event */
	while (!atomic_get(&cloud_connected_t)) {
		k_sleep(K_MSEC(200));
	}

	zassert_equal(STATE_CONNECTED, nfsm_get_current_state(),
		"nrf_cloud lib should be in the connected state after successfully connected");

	int ret = nrf_cloud_uninit();

	/* Wait for the disconnected event */
	while (atomic_get(&cloud_connected_t)) {
		k_sleep(K_MSEC(200));
	}

	zassert_equal(-ETIME, ret,
		"return should be -ETIME when poll thread active after timer expired");
	zassert_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state after uninit");
}
