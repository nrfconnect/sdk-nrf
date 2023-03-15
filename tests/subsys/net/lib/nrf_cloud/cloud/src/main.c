/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <net/nrf_cloud.h>
#include "fakes.h"
#if defined(CONFIG_NRF_CLOUD_CONNECTION_POLL_THREAD)
#include "nrf_cloud.c"
#endif

/* Flags to indicate if the cloud state */
K_SEM_DEFINE(cloud_connected, 0, 1);
K_SEM_DEFINE(cloud_disconnected, 0, 1);
K_SEM_DEFINE(dc_connected, 0, 1);
K_SEM_DEFINE(cc_connected, 0, 1);
static atomic_t transport_connecting;
uint32_t status;

/* Simple event handler needed for the init function to
 * have a successful initialization in the test below
 */
void event_handler(const struct nrf_cloud_evt *const evt)
{
	switch (evt->type) {
	case NRF_CLOUD_EVT_READY:
		k_sem_give(&dc_connected);
		break;
	case NRF_CLOUD_EVT_TRANSPORT_CONNECTED:
		k_sem_reset(&cloud_disconnected);
		k_sem_give(&cloud_connected);
		break;
	case NRF_CLOUD_EVT_RX_DATA_GENERAL:
		k_sem_give(&cc_connected);
		break;
	case NRF_CLOUD_EVT_TRANSPORT_DISCONNECTED:
		if (evt->status != 0) {
			atomic_set(&transport_connecting, 0);
			status = evt->status;
		} else {
			k_sem_reset(&dc_connected);
			k_sem_reset(&cc_connected);
			k_sem_reset(&cloud_connected);
			k_sem_give(&cloud_disconnected);
		}
		break;
	case NRF_CLOUD_EVT_TRANSPORT_CONNECTING:
		atomic_set(&transport_connecting, 1);
		break;
	case NRF_CLOUD_EVT_TRANSPORT_CONNECT_ERROR:
		status = evt->status;
		break;
	case NRF_CLOUD_EVT_ERROR:
		printk("Cloud error: %d\n", evt->status);
		break;
	default:
		break;
	}
}

#if defined(CONFIG_NRF_CLOUD_CONNECTION_POLL_THREAD)
/* Wait for the connecting event for 2s, timeout after that.
 * Then wait for disconnected event for 2s, timeout after that.
 * Use with testing nrf_cloud_run for POLL THREAD only
 */
void wait_for_connecting_and_disconnected_poll_thread(void)
{
	uint32_t wait_counter = 10;

	while (--wait_counter && !atomic_get(&transport_connecting)) {
		k_sleep(K_MSEC(200));
	}
	zassert_not_equal(0, wait_counter, "cloud connecting timed out");

	/* Wait for the disconnected event for 2s, timeout after that */
	wait_counter = 10;
	while (--wait_counter && atomic_get(&transport_connecting)) {
		k_sleep(K_MSEC(200));
	}
	zassert_not_equal(0, wait_counter, "cloud disconnect timed out");
}
#endif

/* Global param to use in nrf_cloud_init */
const struct nrf_cloud_init_param init_param = {
	.event_handler = event_handler,
	.client_id = NULL,
};

/* Global sensor data for sending to cloud */
const struct nrf_cloud_data cloud_data = {
	.len = 0U,
	.ptr = NULL,
};

/* Global sensor param to use in nrf_cloud_sensor_data_stream and nrf_cloud_sensor_data_send */
const struct nrf_cloud_sensor_data sensor_param = {
	.type = 0U,
	.data = cloud_data,
	.tag = 20000U,
};

/* Helper function to successfully init the cloud */
void init_cloud_success(void)
{
	/* Init the cloud with all fakes */
	nct_init_fake.custom_fake = fake_nct_init__succeeds;
	nfsm_init_fake.custom_fake = fake_nfsm_init__succeeds;
	nrf_cloud_codec_init_fake.custom_fake = fake_nrf_cloud_codec_init__succeeds;
	(void)nrf_cloud_init(&init_param);

	zassert_equal(STATE_INITIALIZED, nfsm_get_current_state(),
		"nrf_cloud lib should be in the initialized state after initialization");
}

/* Helper function to set succeed fakes for connecting to the cloud */
void set_succeed_fakes_for_connect(void)
{
	nct_connect_fake.custom_fake = fake_nct_connect__succeeds;
	nct_disconnect_fake.custom_fake = fake_nct_disconnect__succeeds;
	/* These fakes are for the poll thread */
	nct_socket_get_fake.custom_fake = fake_nct_socket_get_data;
	nct_keepalive_time_left_fake.custom_fake = fake_nct_keepalive_time_left_get;
	nct_process_fake.custom_fake = fake_nct_process__succeeds;
}

/* Helper function to connect to the cloud successfully */
void connect_cloud_success(void)
{
	set_succeed_fakes_for_connect();
	(void)nrf_cloud_connect();

	/* Wait for the connected event for 2s, timeout after that */
	int err = k_sem_take(&cloud_connected, K_SECONDS(2));

	zassert_ok(err, "cloud connect timed out");

	zassert_equal(STATE_CONNECTED, nfsm_get_current_state(),
		"nrf_cloud lib should be in the connected state after successfully connected");
}

/* Helper function to connect to the cloud successfully with cc_connect */
void connect_cloud_cc_success(void)
{
	nct_connect_fake.custom_fake = fake_nct_connect_with_cc_connect__succeeds;
	nct_disconnect_fake.custom_fake = fake_nct_disconnect__succeeds;
	/* These fakes are for the poll thread */
	nct_socket_get_fake.custom_fake = fake_nct_socket_get_data;
	nct_keepalive_time_left_fake.custom_fake = fake_nct_keepalive_time_left_get;
	nct_process_fake.custom_fake = fake_nct_process__succeeds;
	(void)nrf_cloud_connect();

	/* Wait for the connected event for 2s, timeout after that */
	int err = k_sem_take(&cloud_connected, K_SECONDS(2));

	zassert_ok(err, "cloud connect timed out");

	/* Wait for the cc_connected event for 2s, timeout after that */
	err = k_sem_take(&cc_connected, K_SECONDS(2));
	zassert_ok(err, "cc connect timed out");

	zassert_equal(STATE_CC_CONNECTED, nfsm_get_current_state(),
		"nrf_cloud lib should be in the cc_connected state after successfully connected");
}

/* Helper function to connect to the cloud successfully with dc_connect */
void connect_cloud_dc_success(void)
{
	nct_connect_fake.custom_fake = fake_nct_connect_with_dc_connect__succeeds;
	nct_disconnect_fake.custom_fake = fake_nct_disconnect__succeeds;
	/* These fakes are for the poll thread */
	nct_socket_get_fake.custom_fake = fake_nct_socket_get_data;
	nct_keepalive_time_left_fake.custom_fake = fake_nct_keepalive_time_left_get;
	nct_process_fake.custom_fake = fake_nct_process__succeeds;
	(void)nrf_cloud_connect();

	/* Wait for the connected event for 2s, timeout after that */
	int err = k_sem_take(&cloud_connected, K_SECONDS(2));

	zassert_ok(err, "cloud connect timed out");

	/* Wait for the dc_connected event for 2s, timeout after that */
	err = k_sem_take(&dc_connected, K_SECONDS(2));
	zassert_ok(err, "dc connect timed out");

	zassert_equal(STATE_DC_CONNECTED, nfsm_get_current_state(),
		"nrf_cloud lib should be in the dc_connected state after successfully connected");
}

/* This function runs before each test */
static void run_before(void *fixture)
{
	ARG_UNUSED(fixture);
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
	RESET_FAKE(nct_cc_send);
	RESET_FAKE(nct_dc_stream);
	RESET_FAKE(nct_dc_send);
	RESET_FAKE(nct_dc_bulk_send);
	RESET_FAKE(nrf_cloud_shadow_dev_status_encode);
	RESET_FAKE(nrf_cloud_device_status_free);
	RESET_FAKE(nrf_cloud_sensor_data_encode);
	RESET_FAKE(nrf_cloud_os_mem_hooks_init);
	RESET_FAKE(nrf_cloud_free);
	RESET_FAKE(poll);

	/* Set the default fake for poll */
	poll_fake.custom_fake = fake_poll__pollnval;

	atomic_set(&transport_connecting, 0);
}

/* This function runs after each completed test */
static void run_after(void *fixture)
{
	ARG_UNUSED(fixture);
#if defined(CONFIG_NRF_CLOUD_FOTA)
	nrf_cloud_fota_uninit_fake.custom_fake = fake_nrf_cloud_fota_uninit__succeeds;
#endif
	nct_disconnect_fake.custom_fake = fake_nct_disconnect__succeeds;

	/* Uninit to reset the internal state to idle */
	(void)nrf_cloud_uninit();
}

ZTEST_SUITE(nrf_cloud_test, NULL, NULL, run_before, run_after, NULL);

/* TEST NRF_CLOUD_INIT */

/* The library can only be initialized when the internal state is
 * idle, so the test verifies that nrf_cloud_init function will
 * return with permission denied error (-EACCES) when not idle
 */
ZTEST(nrf_cloud_test, test_init_not_idle)
{
	zassert_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state (uninitialized) at the start of test");

	/* Call nrf_cloud_init twice with the first one
	 * being successful and check if the second one
	 * will fail since the state will not be idle
	 */
	nct_init_fake.custom_fake = fake_nct_init__succeeds;
	nfsm_init_fake.custom_fake = fake_nfsm_init__succeeds;
	nrf_cloud_codec_init_fake.custom_fake = fake_nrf_cloud_codec_init__succeeds;
	(void)nrf_cloud_init(&init_param);

	int ret = nrf_cloud_init(&init_param);

	zassert_not_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should not be in the idle state after initialization");

	zassert_equal(-EACCES, ret, "return should be -EACCES when not idle");
}

/* Verify that nrf_cloud_init function will throw an error
 * of invalid argument (-EINVAL) when given null init param
 */
ZTEST(nrf_cloud_test, test_init_null_init_param)
{
	zassert_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state (uninitialized) at the start of test");

	int ret = nrf_cloud_init(NULL);

	zassert_equal(-EINVAL, ret, "return should be -EINVAL with NULL init param");
	zassert_not_equal(STATE_INITIALIZED, nfsm_get_current_state(),
		"nrf_cloud lib should not be in the initialized state after init failed");
}

/* Verify that nrf_cloud_init function will throw an
 * error of invalid argument (-EINVAL) when a null
 * event handler is passed in as part of its parameters
 */
ZTEST(nrf_cloud_test, test_init_null_handler)
{
	struct nrf_cloud_init_param init_param_null_handler = {
		.event_handler = NULL,
		.client_id = NULL,
	};

	zassert_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state (uninitialized) at the start of test");

	int ret = nrf_cloud_init(&init_param_null_handler);

	zassert_equal(-EINVAL, ret, "return should be -EINVAL with NULL event handler");
	zassert_not_equal(STATE_INITIALIZED, nfsm_get_current_state(),
		"nrf_cloud lib should not be in the initialized state after init failed");
}

/* Verify that nrf_cloud_init function succeeds given
 * valid parameters. FMFU will not be considered here
 * since the fmfu device info has not been set.
 */
ZTEST(nrf_cloud_test, test_init_success)
{
	zassert_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state (uninitialized) at the start of test");

	/* Fake internal initialization success */
	nct_init_fake.custom_fake = fake_nct_init__succeeds;
	nfsm_init_fake.custom_fake = fake_nfsm_init__succeeds;
	nrf_cloud_codec_init_fake.custom_fake = fake_nrf_cloud_codec_init__succeeds;

	int ret = nrf_cloud_init(&init_param);

	zassert_ok(ret, "return should be 0 when init success");
	zassert_equal(STATE_INITIALIZED, nfsm_get_current_state(),
		"nrf_cloud lib should be in the initialized state after initialization");
}

/* Verify that nrf_cloud_init function will fail
 * when the transport routine fails to initialize
 */
ZTEST(nrf_cloud_test, test_init_cloud_transport_fail)
{
	/* Fake transport initialization failure */
	nct_init_fake.custom_fake = fake_nct_init__fails;

	/* Fake success for the rest of initialization */
	nfsm_init_fake.custom_fake = fake_nfsm_init__succeeds;
	nrf_cloud_codec_init_fake.custom_fake = fake_nrf_cloud_codec_init__succeeds;

	zassert_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state (uninitialized) at the start of test");

	int ret = nrf_cloud_init(&init_param);

	zassert_not_equal(0, ret, "return should be non-zero when transport routine init failed");
	zassert_not_equal(STATE_INITIALIZED, nfsm_get_current_state(),
		"nrf_cloud lib should not be in the initialized state after init failed");
}

/* Verify that nrf_cloud_init function will fail
 * when full modem FOTA update fails to initialize.
 * Ignored when full modem FOTA update is disabled.
 */
ZTEST(nrf_cloud_test, test_init_fmfu_init_failed)
{
	Z_TEST_SKIP_IFNDEF(CONFIG_NRF_CLOUD_FOTA_FULL_MODEM_UPDATE);
	struct dfu_target_fmfu_fdev fmfu_dev_info = {
		.dev = NULL,
		.offset = 0L,
		.size = 0U,
	};

	struct nrf_cloud_init_param init_param_fmfu = {
		.event_handler = event_handler,
		.client_id = NULL,
		.fmfu_dev_inf = &fmfu_dev_info,
	};

	/* Cause an error return value for nrf_cloud_fota_fmfu_dev_set */
	nrf_cloud_fota_fmfu_dev_set_fake.custom_fake = fake_nrf_cloud_fota_fmfu_dev_set__fails;

	/* Fake success for the rest of initialization */
	nct_init_fake.custom_fake = fake_nct_init__succeeds;
	nfsm_init_fake.custom_fake = fake_nfsm_init__succeeds;
	nrf_cloud_codec_init_fake.custom_fake = fake_nrf_cloud_codec_init__succeeds;

	zassert_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state (uninitialized) at the start of test");

	int ret = nrf_cloud_init(&init_param_fmfu);

	zassert_not_equal(0, ret, "return should be non-zero when FOTA init failed");
	zassert_not_equal(STATE_INITIALIZED, nfsm_get_current_state(),
		"nrf_cloud lib should not be in the initialized state after init failed");
}

/* TEST NFSM_SET_CURRENT_STATE_AND_NOTIFY */

/* Verify that nfsm_set_current_state_and notify
 * will change to the right state that is provided.
 */
ZTEST(nrf_cloud_test, test_set_current_state)
{
	zassert_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state (uninitialized) at the start of test");

	nfsm_set_current_state_and_notify(STATE_CONNECTED, NULL);

	zassert_equal(STATE_CONNECTED, nfsm_get_current_state(),
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

	/* Init the cloud successfully */
	init_cloud_success();

	/* Fake a busy return value for nrf_cloud_fota_uninit */
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
ZTEST(nrf_cloud_test, test_uninit_success_fota_unactive)
{
	Z_TEST_SKIP_IFNDEF(CONFIG_NRF_CLOUD_FOTA);
	zassert_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state (uninitialized) at the start of test");

	/* Init the cloud successfully */
	init_cloud_success();

	/* Fake uninit/disconnect success */
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
ZTEST(nrf_cloud_test, test_uninit_no_disconnect)
{
	/* This test is ignored when the connection poll thread is active
	 * since it will trigger the transport disconnect event and give
	 * the uninit_disconnect semaphore which fails the test eventually
	 */
	Z_TEST_SKIP_IFDEF(CONFIG_NRF_CLOUD_CONNECTION_POLL_THREAD);
	zassert_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state (uninitialized) at the start of test");

	/* Init the cloud successfully */
	init_cloud_success();

	/* Connect to the cloud */
	nct_connect_fake.custom_fake = fake_nct_connect__succeeds;
	(void)nrf_cloud_connect();

	/* Wait for the connected event for 2s, timeout after that */
	int err = k_sem_take(&cloud_connected, K_SECONDS(2));

	zassert_ok(err, "cloud connect timed out");

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

/* Verify that nrf_cloud_uninit function succeeds after nrf_cloud is connected */
ZTEST(nrf_cloud_test, test_uninit_success_after_connected)
{
	zassert_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state (uninitialized) at the start of test");

	/* Init the cloud successfully */
	init_cloud_success();

	/* Connect to the cloud successfully */
	connect_cloud_success();

	int ret = nrf_cloud_uninit();

	/* Wait for the disconnected event for 2s, timeout after that */
	int err = k_sem_take(&cloud_disconnected, K_SECONDS(2));

	zassert_ok(err, "cloud disconnect timed out");

	zassert_ok(ret, "return should be 0 when uninit success");
	zassert_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state after uninit");
}

/* Verify that nrf_cloud_uninit function succeeds after initialized without cloud connection */
ZTEST(nrf_cloud_test, test_uninit_success_after_init_no_connection)
{
	zassert_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state (uninitialized) at the start of test");

	/* Init the cloud successfully */
	init_cloud_success();

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
ZTEST(nrf_cloud_test, test_uninit_poll_thread_inactive_timer_expired)
{
	Z_TEST_SKIP_IFNDEF(CONFIG_NRF_CLOUD_CONNECTION_POLL_THREAD);
	zassert_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state (uninitialized) at the start of test");

	/* Init the cloud successfully */
	init_cloud_success();

	/* Connect to the cloud successfully */
	nct_connect_fake.custom_fake = fake_nct_connect__succeeds;
	nct_disconnect_fake.custom_fake = fake_nct_disconnect__succeeds;
	/* These fakes are for the poll thread */
	nct_process_fake.custom_fake = fake_nct_process__succeeds;
	nct_socket_get_fake.custom_fake = fake_nct_socket_get_data;

	/* Timeout needs to be longer than the timer */
	nct_keepalive_time_left_fake.custom_fake = fake_nct_keepalive_time_left_long_timeout;

	(void)nrf_cloud_connect();
	/* Wait for the connected event for 2s, timeout after that */
	int err = k_sem_take(&cloud_connected, K_SECONDS(2));

	zassert_ok(err, "cloud connect timed out");

	zassert_equal(STATE_CONNECTED, nfsm_get_current_state(),
		"nrf_cloud lib should be in the connected state after successfully connected");

	int ret = nrf_cloud_uninit();

	/* Wait for the disconnected event for 2s, timeout after that */
	err = k_sem_take(&cloud_disconnected, K_SECONDS(2));
	zassert_ok(err, "cloud disconnect timed out");

	zassert_equal(-ETIME, ret,
		"return should be -ETIME when poll thread active after timer expired");
	zassert_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state after uninit");
}

/* TEST NRF_CLOUD_CONNECT */

/* Verify that nrf_cloud_connect function will fail when the cloud is in the idle state */
ZTEST(nrf_cloud_test, test_connect_idle)
{
	zassert_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state (uninitialized) at the start of test");

	int ret = nrf_cloud_connect();

	zassert_equal(NRF_CLOUD_CONNECT_RES_ERR_NOT_INITD, ret,
		"return should be RES_ERR_NOT_INITD after trying to connect when idle");
}

/* Verify that nrf_cloud_connect function will fail when the cloud is already connected */
ZTEST(nrf_cloud_test, test_connect_already_connected)
{
	zassert_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state (uninitialized) at the start of test");

	/* Init the cloud successfully */
	init_cloud_success();

	/* Connect to the cloud successfully */
	connect_cloud_success();

	int ret = nrf_cloud_connect();

	zassert_equal(NRF_CLOUD_CONNECT_RES_ERR_ALREADY_CONNECTED, ret,
		"return should be RES_ERR_ALREADY_CONNECTED when trying to connnect after connected");
}

/* Verify that nrf_cloud_connect function will succeed
 * given the transport layer connected without errors.
 */
ZTEST(nrf_cloud_test, test_connect_success)
{
	zassert_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state (uninitialized) at the start of test");

	/* Init the cloud successfully */
	init_cloud_success();

	/* Connect to the cloud successfully */
	set_succeed_fakes_for_connect();

	int ret = nrf_cloud_connect();

	/* Wait for the connected event for 2s, timeout after that */
	int err = k_sem_take(&cloud_connected, K_SECONDS(2));

	zassert_ok(err, "cloud connect timed out");

	zassert_equal(NRF_CLOUD_CONNECT_RES_SUCCESS, ret,
		"return should be RES_SUCCESS when successfully connected");
	zassert_equal(STATE_CONNECTED, nfsm_get_current_state(),
		"nrf_cloud lib should be in the connected state after successfully connected");
}

/* Verify that nrf_cloud_connect function will
 * fail when the connection poll is in progress.
 * Ignored if the POLL_THREAD is disabled.
 */
ZTEST(nrf_cloud_test, test_connect_connection_poll_in_progress)
{
	Z_TEST_SKIP_IFNDEF(CONFIG_NRF_CLOUD_CONNECTION_POLL_THREAD);
	zassert_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state (uninitialized) at the start of test");

	/* Init the cloud successfully */
	init_cloud_success();

	/* Connect to the cloud successfully */
	set_succeed_fakes_for_connect();
	(void)nrf_cloud_connect();

	/* No need to wait for the connected event */
	int ret = nrf_cloud_connect();

	zassert_equal(NRF_CLOUD_CONNECT_RES_ERR_ALREADY_CONNECTED, ret,
		"return should be RES_ERR_ALREADY_CONNECTED when connection poll is in progress");
}

/* TEST NRF_CLOUD_DISCONNECT */

/* Verify that nrf_cloud_disconnect function
 * will fail when the cloud is not initialized
 * or already initialized but not connected.
 */
ZTEST(nrf_cloud_test, test_disconnect_not_connected)
{
	/* Cloud is in idle state */
	zassert_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state (uninitialized) at the start of test");

	int ret = nrf_cloud_disconnect();

	zassert_equal(-EACCES, ret, "return should be -EACCES when disconnecting in idle state");

	/* Cloud is initialized but not connected */
	init_cloud_success();

	ret = nrf_cloud_disconnect();
	zassert_equal(-EACCES, ret,
		"return should be -EACCES when disconnecting after initialized but not connected");
}

/* Verify that nrf_cloud_disconnect function will
 * fail when the cloud transport fails to disconnect.
 */
ZTEST(nrf_cloud_test, test_disconnect_transport_failed)
{
	zassert_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state (uninitialized) at the start of test");

	/* Init the cloud successfully */
	init_cloud_success();

	/* Connect to the cloud */
	nct_connect_fake.custom_fake = fake_nct_connect__succeeds;
	/* Transport failed when disconnecting */
	nct_disconnect_fake.custom_fake = fake_nct_disconnect__fails;
	/* These fakes are for the poll thread */
	nct_socket_get_fake.custom_fake = fake_nct_socket_get_data;
	nct_keepalive_time_left_fake.custom_fake = fake_nct_keepalive_time_left_get;
	nct_process_fake.custom_fake = fake_nct_process__succeeds;
	(void)nrf_cloud_connect();

	/* Wait for the connected event for 2s, timeout after that */
	int err = k_sem_take(&cloud_connected, K_SECONDS(2));

	zassert_ok(err, "cloud connect timed out");

	zassert_equal(STATE_CONNECTED, nfsm_get_current_state(),
		"nrf_cloud lib should be in the connected state after successfully connected");

	int ret = nrf_cloud_disconnect();

	zassert_not_equal(0, ret,
		"return should not be 0 when disconnecting with the transport failed");
}

/* Verify that nrf_cloud_disconnect function will succeed
 * given the cloud transport disconnected without errors.
 */
ZTEST(nrf_cloud_test, test_disconnect_success)
{
	zassert_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state (uninitialized) at the start of test");

	/* Init the cloud successfully */
	init_cloud_success();

	/* Connect to the cloud successfully */
	connect_cloud_success();

	int ret = nrf_cloud_disconnect();

	/* Wait for the disconnected event for 2s, timeout after that */
	int err = k_sem_take(&cloud_disconnected, K_SECONDS(2));

	zassert_ok(err, "cloud disconnect timed out");

	zassert_ok(ret,	"return should be 0 when success");
	zassert_equal(STATE_INITIALIZED, nfsm_get_current_state(),
		"nrf_cloud lib should be in the initialized state after disconnected successfully");
}

/* TEST NRF_CLOUD_SHADOW_DEVICE_STATUS_UPDATE */

/* Verify that nrf_cloud_shadow_device_status_update will
 * fail if the internal state of the lib is not dc_connected
 */
ZTEST(nrf_cloud_test, test_shadow_device_status_update_not_dc_connected)
{
	/* Data for shadow device status update */
	struct nrf_cloud_device_status dev_status = {
		.modem = NULL,
		.svc = NULL
	};

	/* Custom fakes for nrf_cloud_send */
	nct_cc_send_fake.custom_fake = fake_nct_cc_send__succeeds;
	nct_dc_stream_fake.custom_fake = fake_nct_dc_stream__succeeds;
	nct_dc_send_fake.custom_fake = fake_nct_dc_send__succeeds;
	nct_dc_bulk_send_fake.custom_fake = fake_nct_dc_bulk_send__succeeds;
	/* Custom fakes for shadow device status update */
	nrf_cloud_shadow_dev_status_encode_fake.custom_fake =
		fake_device_status_shadow_encode__succeeds;

	/* Cloud is in idle state */
	zassert_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state (uninitialized) at the start of test");

	int ret = nrf_cloud_shadow_device_status_update(&dev_status);

	zassert_equal(-EACCES, ret, "return should be -EACCES when not in dc_connected state");

	/* Cloud is initialized */
	init_cloud_success();

	ret = nrf_cloud_shadow_device_status_update(&dev_status);
	zassert_equal(-EACCES, ret, "return should be -EACCES when not in dc_connected state");

	/* Cloud is connected */
	connect_cloud_success();
	ret = nrf_cloud_shadow_device_status_update(&dev_status);
	zassert_equal(-EACCES, ret, "return should be -EACCES when not in dc_connected state");
}

/* Verify that nrf_cloud_shadow_device_status_update will
 * fail if the cloud device status encode function fails
 */
ZTEST(nrf_cloud_test, test_shadow_device_status_update_status_encode_failed)
{
	/* Data for shadow device status update */
	struct nrf_cloud_device_status dev_status = {
		.modem = NULL,
		.svc = NULL
	};

	/* Custom fakes for nrf_cloud_send */
	nct_cc_send_fake.custom_fake = fake_nct_cc_send__succeeds;
	nct_dc_stream_fake.custom_fake = fake_nct_dc_stream__succeeds;
	nct_dc_send_fake.custom_fake = fake_nct_dc_send__succeeds;
	nct_dc_bulk_send_fake.custom_fake = fake_nct_dc_bulk_send__succeeds;
	/* Custom fakes for shadow device status update */
	nrf_cloud_shadow_dev_status_encode_fake.custom_fake =
		fake_device_status_shadow_encode__fails;

	zassert_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state (uninitialized) at the start of test");

	/* Init the cloud successfully */
	init_cloud_success();

	/* Connect to the cloud with dc_connect */
	connect_cloud_dc_success();

	int ret = nrf_cloud_shadow_device_status_update(&dev_status);

	zassert_not_equal(0, ret, "return should not be 0 when status encoding fails");
}

/* Verify that nrf_cloud_shadow_device_status_update
 * will fail if the cloud send function fails
 */
ZTEST(nrf_cloud_test, test_shadow_device_status_update_cloud_send_failed)
{
	/* Data for shadow device status update */
	struct nrf_cloud_device_status dev_status = {
		.modem = NULL,
		.svc = NULL
	};

	/* Custom fakes for nrf_cloud_send */
	nct_cc_send_fake.custom_fake = fake_nct_cc_send__fails;
	nct_dc_stream_fake.custom_fake = fake_nct_dc_stream__succeeds;
	nct_dc_send_fake.custom_fake = fake_nct_dc_send__succeeds;
	nct_dc_bulk_send_fake.custom_fake = fake_nct_dc_bulk_send__succeeds;
	/* Custom fakes for shadow device status update */
	nrf_cloud_shadow_dev_status_encode_fake.custom_fake =
		fake_device_status_shadow_encode__succeeds;

	zassert_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state (uninitialized) at the start of test");

	/* Init the cloud successfully */
	init_cloud_success();

	/* Connect to the cloud with dc_connect */
	connect_cloud_dc_success();

	int ret = nrf_cloud_shadow_device_status_update(&dev_status);

	zassert_not_equal(0, ret, "return should not be 0 when cloud send fails");
}

/* Verify that nrf_cloud_shadow_device_status_update will
 * succeed given a suitable state, encoding and sending succeed
 */
ZTEST(nrf_cloud_test, test_shadow_device_status_update_success)
{
	/* Data for shadow device status update */
	struct nrf_cloud_device_status dev_status = {
		.modem = NULL,
		.svc = NULL
	};

	/* Custom fakes for nrf_cloud_send */
	nct_cc_send_fake.custom_fake = fake_nct_cc_send__succeeds;
	nct_dc_stream_fake.custom_fake = fake_nct_dc_stream__succeeds;
	nct_dc_send_fake.custom_fake = fake_nct_dc_send__succeeds;
	nct_dc_bulk_send_fake.custom_fake = fake_nct_dc_bulk_send__succeeds;
	/* Custom fakes for shadow device status update */
	nrf_cloud_shadow_dev_status_encode_fake.custom_fake =
		fake_device_status_shadow_encode__succeeds;

	zassert_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state (uninitialized) at the start of test");

	/* Init the cloud successfully */
	init_cloud_success();

	/* Connect to the cloud with dc_connect */
	connect_cloud_dc_success();

	int ret = nrf_cloud_shadow_device_status_update(&dev_status);

	zassert_ok(ret, "return should be 0 when success");
}

/* TEST NRF_CLOUD_SENSOR_DATA_SEND */

/* Verify that nrf_cloud_sensor_data_send will fail if
 * the internal state of the lib is not dc_connected
 */
ZTEST(nrf_cloud_test, test_cloud_sensor_data_send_not_dc_connected)
{
	/* Cloud is in idle state */
	zassert_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state (uninitialized) at the start of test");

	int ret = nrf_cloud_sensor_data_send(&sensor_param);

	zassert_equal(-EACCES, ret, "return should be -EACCES when not in dc_connected state");

	/* Cloud is initialized */
	init_cloud_success();

	ret = nrf_cloud_sensor_data_send(&sensor_param);
	zassert_equal(-EACCES, ret, "return should be -EACCES when not in dc_connected state");

	/* Cloud is connected */
	connect_cloud_success();

	ret = nrf_cloud_sensor_data_send(&sensor_param);
	zassert_equal(-EACCES, ret, "return should be -EACCES when not in dc_connected state");
}

/* Verify that nrf_cloud_sensor_data_send will fail if a NULL param is provided */
ZTEST(nrf_cloud_test, test_cloud_sensor_data_send_null_param)
{
	zassert_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state (uninitialized) at the start of test");

	/* Init the cloud successfully */
	init_cloud_success();

	/* Connect to the cloud successfully with dc_connect */
	connect_cloud_dc_success();

	int ret = nrf_cloud_sensor_data_send(NULL);

	zassert_equal(-EINVAL, ret, "return should be -EINVAL with NULL param");
}

/* Verify that nrf_cloud_sensor_data_send will fail
 * if the cloud sensor data encoding function fails
 */
ZTEST(nrf_cloud_test, test_cloud_sensor_data_send_sensor_encode_failed)
{
	zassert_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state (uninitialized) at the start of test");

	/* Init the cloud successfully */
	init_cloud_success();

	/* Connect to the cloud successfully with dc_connect */
	connect_cloud_dc_success();

	nrf_cloud_sensor_data_encode_fake.custom_fake = fake_nrf_cloud_sensor_data_encode__fails;

	int ret = nrf_cloud_sensor_data_send(&sensor_param);

	zassert_not_equal(0, ret, "return should not be 0 when sensor data encoding fails");
}

/* Verify that nrf_cloud_sensor_data_send will fail if the dc_send function fails */
ZTEST(nrf_cloud_test, test_cloud_sensor_data_send_dc_send_failed)
{
	zassert_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state (uninitialized) at the start of test");

	/* Init the cloud successfully */
	init_cloud_success();

	/* Connect to the cloud successfully with dc_connect */
	connect_cloud_dc_success();

	nrf_cloud_sensor_data_encode_fake.custom_fake = fake_nrf_cloud_sensor_data_encode__succeeds;
	nct_dc_send_fake.custom_fake = fake_nct_dc_send__fails;

	int ret = nrf_cloud_sensor_data_send(&sensor_param);

	zassert_not_equal(0, ret, "return should not be 0 when sensor data sending fails");
}

/* Verify that nrf_cloud_sensor_data_send will succeed given
 * the suitable state, data encoding and sending succeed
 */
ZTEST(nrf_cloud_test, test_cloud_sensor_data_send_success)
{
	zassert_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state (uninitialized) at the start of test");

	/* Init the cloud successfully */
	init_cloud_success();

	/* Connect to the cloud successfully with dc_connect */
	connect_cloud_dc_success();

	nrf_cloud_sensor_data_encode_fake.custom_fake = fake_nrf_cloud_sensor_data_encode__succeeds;
	nct_dc_send_fake.custom_fake = fake_nct_dc_send__succeeds;

	int ret = nrf_cloud_sensor_data_send(&sensor_param);

	zassert_ok(ret, "return should be 0 when success");
}

/* TEST NRF_CLOUD_SENSOR_DATA_STREAM */

/* Verify that nrf_cloud_sensor_data_stream will fail
 * if the internal state of the lib is not dc_connected
 */
ZTEST(nrf_cloud_test, test_cloud_sensor_data_stream_not_dc_connected)
{
	/* Cloud is in idle state */
	zassert_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state (uninitialized) at the start of test");

	int ret = nrf_cloud_sensor_data_stream(&sensor_param);

	zassert_equal(-EACCES, ret, "return should be -EACCES when not in dc_connected state");

	/* Cloud is initialized */
	init_cloud_success();

	ret = nrf_cloud_sensor_data_stream(&sensor_param);
	zassert_equal(-EACCES, ret, "return should be -EACCES when not in dc_connected state");

	/* Cloud is connected */
	connect_cloud_success();

	ret = nrf_cloud_sensor_data_stream(&sensor_param);
	zassert_equal(-EACCES, ret, "return should be -EACCES when not in dc_connected state");
}

/* Verify that nrf_cloud_sensor_data_stream will fail if a NULL param is provided */
ZTEST(nrf_cloud_test, test_cloud_sensor_data_stream_null_param)
{
	zassert_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state (uninitialized) at the start of test");

	/* Init the cloud successfully */
	init_cloud_success();

	/* Connect to the cloud successfully with dc_connect */
	connect_cloud_dc_success();

	int ret = nrf_cloud_sensor_data_stream(NULL);

	zassert_equal(-EINVAL, ret, "return should be -EINVAL with NULL param");
}

/* Verify that nrf_cloud_sensor_data_stream will fail
 * if the cloud sensor data encoding function fails
 */
ZTEST(nrf_cloud_test, test_cloud_sensor_data_stream_sensor_encode_failed)
{
	zassert_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state (uninitialized) at the start of test");

	/* Init the cloud successfully */
	init_cloud_success();

	/* Connect to the cloud successfully with dc_connect */
	connect_cloud_dc_success();

	nrf_cloud_sensor_data_encode_fake.custom_fake = fake_nrf_cloud_sensor_data_encode__fails;

	int ret = nrf_cloud_sensor_data_stream(&sensor_param);

	zassert_not_equal(0, ret, "return should not be 0 when sensor data encoding fails");
}

/* Verify that nrf_cloud_sensor_data_stream will fail if the dc_stream function fails */
ZTEST(nrf_cloud_test, test_cloud_sensor_data_stream_dc_stream_failed)
{
	zassert_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state (uninitialized) at the start of test");

	/* Init the cloud successfully */
	init_cloud_success();

	/* Connect to the cloud successfully with dc_connect */
	connect_cloud_dc_success();

	nrf_cloud_sensor_data_encode_fake.custom_fake = fake_nrf_cloud_sensor_data_encode__succeeds;
	nct_dc_stream_fake.custom_fake = fake_nct_dc_stream__fails;

	int ret = nrf_cloud_sensor_data_stream(&sensor_param);

	zassert_not_equal(0, ret, "return should not be 0 when sensor data streaming fails");
}

/* Verify that nrf_cloud_sensor_data_stream will succeed given
 * the suitable state, data encoding and streaming succeed
 */
ZTEST(nrf_cloud_test, test_cloud_sensor_data_stream_success)
{
	zassert_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state (uninitialized) at the start of test");

	/* Init the cloud successfully */
	init_cloud_success();

	/* Connect to the cloud successfully with dc_connect */
	connect_cloud_dc_success();

	nrf_cloud_sensor_data_encode_fake.custom_fake = fake_nrf_cloud_sensor_data_encode__succeeds;
	nct_dc_stream_fake.custom_fake = fake_nct_dc_stream__succeeds;

	int ret = nrf_cloud_sensor_data_stream(&sensor_param);

	zassert_ok(ret, "return should be 0 when success");
}

/* TEST NRF_CLOUD_SEND */

/* Verify that nrf_cloud_send will fail if the message to be sent is NULL */
ZTEST(nrf_cloud_test, test_cloud_send_null_message)
{
	int ret = nrf_cloud_send(NULL);

	zassert_equal(-EINVAL, ret, "return should be -EINVAL with NULL message");
}

/* Verify that nrf_cloud_send will fail if the cloud is not in cc_connected state */
ZTEST(nrf_cloud_test, test_cloud_send_not_cc_connected)
{
	/* Message data */
	struct nrf_cloud_tx_data tx_data = {
		.topic_type = NRF_CLOUD_TOPIC_STATE,
		.qos = MQTT_QOS_1_AT_LEAST_ONCE
	};

	while (tx_data.topic_type <= NRF_CLOUD_TOPIC_BULK) {
		/* Cloud is in idle state */
		zassert_equal(STATE_IDLE, nfsm_get_current_state(),
			"nrf_cloud lib should be in the idle state (uninitialized) at the start of test");

		int ret = nrf_cloud_send(&tx_data);

		zassert_equal(-EACCES, ret,
			"return should be -EACCES when not in cc_connected state");

		/* Cloud is initialized */
		init_cloud_success();

		ret = nrf_cloud_send(&tx_data);
		zassert_equal(-EACCES, ret,
			"return should be -EACCES when not in cc_connected state");

		/* Cloud is connected */
		connect_cloud_success();

		ret = nrf_cloud_send(&tx_data);
		zassert_equal(-EACCES, ret,
			"return should be -EACCES when not in cc_connected state");

		/* Uninit */
		(void)nrf_cloud_uninit();
		/* Test with the next topic type */
		tx_data.topic_type = tx_data.topic_type + 1;
	}
}

/* Verify that nrf_cloud_send will fail if
 * the cloud is not in dc_connected state
 * with the topic_type not equal topic_state
 */
ZTEST(nrf_cloud_test, test_cloud_send_not_dc_connected)
{
	/* Message data */
	struct nrf_cloud_tx_data tx_data = {
		.topic_type = NRF_CLOUD_TOPIC_MESSAGE,
		.qos = MQTT_QOS_1_AT_LEAST_ONCE
	};

	while (tx_data.topic_type <= NRF_CLOUD_TOPIC_BULK) {
		/* Cloud is in idle state */
		zassert_equal(STATE_IDLE, nfsm_get_current_state(),
			"nrf_cloud lib should be in the idle state (uninitialized) at the start of test");

		int ret = nrf_cloud_send(&tx_data);

		zassert_equal(-EACCES, ret,
			"return should be -EACCES when not in dc_connected state");

		/* Cloud is initialized */
		init_cloud_success();

		ret = nrf_cloud_send(&tx_data);
		zassert_equal(-EACCES, ret,
			"return should be -EACCES when not in dc_connected state");

		/* Cloud is connected with cc_connect */
		connect_cloud_cc_success();

		ret = nrf_cloud_send(&tx_data);
		zassert_equal(-EACCES, ret,
			"return should be -EACCES when not in dc_connected state");

		/* Uninit */
		(void)nrf_cloud_uninit();
		/* Test with the next topic type */
		tx_data.topic_type = tx_data.topic_type + 1;
	}
}

/* Verify that nrf_cloud_send will fail if the cloud
 * is in the cc_connected state with the topic_type
 * is equal topic_state and nct_cc_send failed
 */
ZTEST(nrf_cloud_test, test_cloud_topic_state_cc_send_failed)
{
	/* Message data */
	struct nrf_cloud_tx_data tx_state_qos1 = {
		.topic_type = NRF_CLOUD_TOPIC_STATE,
		.qos = MQTT_QOS_1_AT_LEAST_ONCE
	};

	zassert_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state (uninitialized) at the start of test");

	/* Init the cloud successfully */
	init_cloud_success();

	/* Connect to cloud successfully with cc_connect */
	connect_cloud_cc_success();

	nct_cc_send_fake.custom_fake = fake_nct_cc_send__fails;

	int ret = nrf_cloud_send(&tx_state_qos1);

	zassert_not_equal(0, ret, "return should not be 0 when nct_cc_send fails");
}

/* Verify that nrf_cloud_send will succeed if the cloud
 * is in the cc_connected state with the topic_type
 * is equal topic_state and nct_cc_send succeeds
 */
ZTEST(nrf_cloud_test, test_cloud_send_topic_state_success)
{
	/* Message data */
	struct nrf_cloud_tx_data tx_state_qos1 = {
		.topic_type = NRF_CLOUD_TOPIC_STATE,
		.qos = MQTT_QOS_1_AT_LEAST_ONCE
	};

	zassert_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state (uninitialized) at the start of test");

	/* Init the cloud successfully */
	init_cloud_success();

	/* Connect to cloud successfully with cc_connect */
	connect_cloud_cc_success();

	nct_cc_send_fake.custom_fake = fake_nct_cc_send__succeeds;

	int ret = nrf_cloud_send(&tx_state_qos1);

	zassert_ok(ret, "return should be 0 when success");
}

/* Verify that nrf_cloud_send will fail if the cloud is in
 * dc_connected state with the topic_type is equal
 * topic_message and nct_dc stream or nct_dc_send failed
 */
ZTEST(nrf_cloud_test, test_cloud_send_topic_message_dc_stream_send_failed)
{
	zassert_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state (uninitialized) at the start of test");

	/* Init the cloud successfully */
	init_cloud_success();

	/* Connect to the cloud successfully with dc_connect */
	connect_cloud_dc_success();

	nct_dc_stream_fake.custom_fake = fake_nct_dc_stream__fails;
	nct_dc_send_fake.custom_fake = fake_nct_dc_send__fails;

	/* Message data */
	struct nrf_cloud_tx_data tx_msg = {
		.topic_type = NRF_CLOUD_TOPIC_MESSAGE,
		.qos = MQTT_QOS_1_AT_LEAST_ONCE
	};

	int ret = nrf_cloud_send(&tx_msg);

	zassert_not_equal(0, ret, "return should not be 0 when dc_send fails");

	tx_msg.qos = MQTT_QOS_0_AT_MOST_ONCE;
	ret = nrf_cloud_send(&tx_msg);

	zassert_not_equal(0, ret, "return should not be 0 when dc_stream fails");
}

/* Verify that nrf_cloud_send will fail if the cloud
 * is in dc_connected state with the topic_type is
 * equal topic_message and the qos is invalid
 */
ZTEST(nrf_cloud_test, test_cloud_send_topic_message_invalid_qos)
{
	zassert_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state (uninitialized) at the start of test");

	/* Init the cloud successfully */
	init_cloud_success();

	/* Connect to the cloud successfully with dc_connect */
	connect_cloud_dc_success();

	nct_dc_stream_fake.custom_fake = fake_nct_dc_stream__succeeds;
	nct_dc_send_fake.custom_fake = fake_nct_dc_send__succeeds;

	/* Message data */
	struct nrf_cloud_tx_data tx_msg_qos2 = {
		.topic_type = NRF_CLOUD_TOPIC_MESSAGE,
		.qos = MQTT_QOS_2_EXACTLY_ONCE
	};

	int ret = nrf_cloud_send(&tx_msg_qos2);

	zassert_equal(-EINVAL, ret, "return should not be 0 with invalid qos");
}

/* Verify that nrf_cloud_send will succeed if the cloud
 * is in dc_connected state with the topic_type is equal
 * topic_message and nct_dc_send/nct_dc_stream succeed
 */
ZTEST(nrf_cloud_test, test_cloud_send_topic_message_success)
{
	zassert_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state (uninitialized) at the start of test");

	/* Init the cloud successfully */
	init_cloud_success();

	/* Connect to the cloud successfully with dc_connect */
	connect_cloud_dc_success();

	nct_dc_stream_fake.custom_fake = fake_nct_dc_stream__succeeds;
	nct_dc_send_fake.custom_fake = fake_nct_dc_send__succeeds;

	/* Message data */
	struct nrf_cloud_tx_data tx_msg = {
		.topic_type = NRF_CLOUD_TOPIC_MESSAGE,
		.qos = MQTT_QOS_0_AT_MOST_ONCE
	};

	int ret = nrf_cloud_send(&tx_msg);

	zassert_ok(ret, "return should be 0 when success");

	tx_msg.qos = MQTT_QOS_1_AT_LEAST_ONCE;
	ret = nrf_cloud_send(&tx_msg);
	zassert_ok(ret, "return should be 0 when success");
}

/* Verify that nrf_cloud_send will fail if the cloud
 * is in dc_connected state with the topic_type is
 * equal topic_bulk and nct_dc_bulk_send fails
 */
ZTEST(nrf_cloud_test, test_cloud_send_topic_bulk_dc_bulk_send_failed)
{
	zassert_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state (uninitialized) at the start of test");

	/* Init the cloud successfully */
	init_cloud_success();

	/* Connect to the cloud successfully with dc_connect */
	connect_cloud_dc_success();

	nct_dc_bulk_send_fake.custom_fake = fake_nct_dc_bulk_send__fails;

	/* Message data */
	struct nrf_cloud_tx_data tx_bulk_qos0 = {
		.topic_type = NRF_CLOUD_TOPIC_BULK,
		.qos = MQTT_QOS_0_AT_MOST_ONCE
	};

	int ret = nrf_cloud_send(&tx_bulk_qos0);

	zassert_not_equal(0, ret, "return should not be 0 when nct_dc_bulk_send failed");
}

/* Verify that nrf_cloud_send will succeed if the cloud
 * is in dc_connected state with the topic_type is equal
 * topic_bulk and nct_dc_bulk_send succeeds
 */
ZTEST(nrf_cloud_test, test_cloud_send_topic_bulk_success)
{
	zassert_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state (uninitialized) at the start of test");

	/* Init the cloud successfully */
	init_cloud_success();

	/* Connect to the cloud successfully with dc_connect */
	connect_cloud_dc_success();

	nct_dc_bulk_send_fake.custom_fake = fake_nct_dc_bulk_send__succeeds;

	/* Message data */
	struct nrf_cloud_tx_data tx_bulk_qos0 = {
		.topic_type = NRF_CLOUD_TOPIC_BULK,
		.qos = MQTT_QOS_0_AT_MOST_ONCE
	};

	int ret = nrf_cloud_send(&tx_bulk_qos0);

	zassert_ok(ret, "return should be 0 when success");
}

/* Verify that nrf_cloud_send will fail if an invalid topic_type is given */
ZTEST(nrf_cloud_test, test_cloud_send_invalid_topic_type)
{
	zassert_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state (uninitialized) at the start of test");

	/* Message data */
	struct nrf_cloud_tx_data tx_wrong_type = {
		.topic_type = 0,
		.qos = MQTT_QOS_0_AT_MOST_ONCE
	};

	int ret = nrf_cloud_send(&tx_wrong_type);

	zassert_equal(-ENODATA, ret, "return should be -ENODATA with invalid topic type");
}

#if defined(CONFIG_NRF_CLOUD_CONNECTION_POLL_THREAD)

/* TEST START_CONNECTION_POLL */

/* Verify that start_connection_poll will fail if the cloud is in the idle state */
ZTEST(nrf_cloud_test, test_start_connection_poll_in_idle)
{
	zassert_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state (uninitialized) at the start of test");

	int ret = start_connection_poll();

	zassert_equal(-EACCES, ret, "return should be -EACCES when in idle state");
}

/* Verify that start_connection_poll will fail if the poll thread is already active */
ZTEST(nrf_cloud_test, test_start_connection_poll_already_active)
{
	zassert_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state (uninitialized) at the start of test");

	/* Init the cloud successfully */
	init_cloud_success();

	zassert_equal(STATE_INITIALIZED, nfsm_get_current_state(),
		"nrf_cloud lib should be in the initialized state after initialization");
	(void)start_connection_poll();

	int ret = start_connection_poll();

	zassert_equal(-EINPROGRESS, ret, "return should be -EINPROGRESS when already active");
}

/* Verify that start_connection_poll will succeed if the cloud is successfully initialized */
ZTEST(nrf_cloud_test, test_start_connection_poll_success)
{
	zassert_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state (uninitialized) at the start of test");

	/* Init the cloud successfully */
	init_cloud_success();

	int ret = start_connection_poll();

	zassert_ok(ret, "return should be 0 when success");
}

/* TEST NRF_CLOUD_RUN */

/* Verify that nrf_cloud_run will change the cloud disconnect
 * event to not success if the connection to the cloud failed
 */
ZTEST(nrf_cloud_test, test_cloud_run_cloud_connecting_failed)
{
	zassert_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state (uninitialized) at the start of test");

	/* Init the cloud successfully */
	init_cloud_success();

	/* Connect to the cloud failed */
	nct_connect_fake.custom_fake = fake_nct_connect__invalid;

	(void)start_connection_poll();

	/* Wait for the connecting event for 2s, timeout after that */
	uint32_t wait_counter = 10;

	while (--wait_counter && !atomic_get(&transport_connecting)) {
		k_sleep(K_MSEC(200));
	}
	zassert_not_equal(0, wait_counter, "cloud connecting timed out");

	zassert_not_equal(NRF_CLOUD_CONNECT_RES_SUCCESS, status,
		"status should not be success when transport failed");
}

/* Verify that nrf_cloud_run will change the cloud
 * disconnect event to closed by remote if timeout
 * has expired and no connection to the cloud
 */
ZTEST(nrf_cloud_test, test_cloud_run_timed_out_connection_closed_by_remote)
{
	zassert_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state (uninitialized) at the start of test");

	/* Init the cloud successfully */
	init_cloud_success();

	/* Connect to the cloud successfully */
	nct_connect_fake.custom_fake = fake_nct_connect__succeeds;
	nct_disconnect_fake.custom_fake = fake_nct_disconnect__succeeds;
	/* These fakes are for the poll thread */
	nct_socket_get_fake.custom_fake = fake_nct_socket_get_data;
	nct_keepalive_time_left_fake.custom_fake = fake_nct_keepalive_time_left_get;

	/* Transport process needs to be failed */
	nct_process_fake.custom_fake = fake_nct_process__fails;
	/* Change poll_fake to indicate timeout expired */
	poll_fake.custom_fake = fake_poll__zero;

	(void)start_connection_poll();

	/* Wait for the events */
	wait_for_connecting_and_disconnected_poll_thread();

	zassert_equal(NRF_CLOUD_DISCONNECT_CLOSED_BY_REMOTE, status,
		"status should be closed by remote");
}

/* Verify that nrf_cloud_run will change the cloud
 * disconnect event to mics if poll function failed
 */
ZTEST(nrf_cloud_test, test_cloud_run_poll_failed)
{
	zassert_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state (uninitialized) at the start of test");

	/* Init the cloud successfully */
	init_cloud_success();

	/* Connect to the cloud successfully */
	set_succeed_fakes_for_connect();
	/* Change poll_fake to indicate poll function error */
	poll_fake.custom_fake = fake_poll__negative;

	(void)start_connection_poll();

	/* Wait for the events */
	wait_for_connecting_and_disconnected_poll_thread();

	zassert_equal(NRF_CLOUD_DISCONNECT_MISC, status, "status should be misc");
}

/* Verify that nrf_cloud_run will change the cloud
 * disconnect event to closed by remote if requested
 * event is POLLIN but no connection to the cloud
 */
ZTEST(nrf_cloud_test, test_cloud_run_connection_failed_pollin)
{
	zassert_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state (uninitialized) at the start of test");

	/* Init the cloud successfully */
	init_cloud_success();

	/* Connect to the cloud successfully */
	nct_connect_fake.custom_fake = fake_nct_connect__succeeds;
	nct_disconnect_fake.custom_fake = fake_nct_disconnect__succeeds;
	/* These fakes are for the poll thread */
	nct_socket_get_fake.custom_fake = fake_nct_socket_get_data;
	nct_keepalive_time_left_fake.custom_fake = fake_nct_keepalive_time_left_get;

	/* Transport process needs to be failed */
	nct_process_fake.custom_fake = fake_nct_process__fails;
	/* Change poll_fake to indicate POLLIN */
	poll_fake.custom_fake = fake_poll__pollin;

	(void)start_connection_poll();

	/* Wait for the events */
	wait_for_connecting_and_disconnected_poll_thread();

	zassert_equal(NRF_CLOUD_DISCONNECT_CLOSED_BY_REMOTE, status,
		"status should be closed by remote");
}

/* Verify that nrf_cloud_run will change the cloud
 * disconnect event to invalid request if the requested
 * event is POLLNVAL and user has not disconnected
 */
ZTEST(nrf_cloud_test, test_cloud_run_pollnval)
{
	zassert_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state (uninitialized) at the start of test");

	/* Init the cloud successfully */
	init_cloud_success();

	/* Connect to the cloud successfully */
	set_succeed_fakes_for_connect();
	/* Change poll_fake to indicate POLLNVAL */
	poll_fake.custom_fake = fake_poll__pollnval;

	(void)start_connection_poll();

	/* Wait for the events */
	wait_for_connecting_and_disconnected_poll_thread();

	zassert_equal(NRF_CLOUD_DISCONNECT_INVALID_REQUEST, status,
		"status should be invalid request");
}

/* Verify that nrf_cloud_run will change the cloud disconnect
 * event to closed by remote if the requested event is POLLHUP
 */
ZTEST(nrf_cloud_test, test_cloud_run_pollhup)
{
	zassert_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state (uninitialized) at the start of test");

	/* Init the cloud successfully */
	init_cloud_success();

	/* Connect to the cloud successfully */
	set_succeed_fakes_for_connect();
	/* Change poll_fake to indicate POLLHUP */
	poll_fake.custom_fake = fake_poll__pollhup;

	(void)start_connection_poll();

	/* Wait for the events */
	wait_for_connecting_and_disconnected_poll_thread();

	zassert_equal(NRF_CLOUD_DISCONNECT_CLOSED_BY_REMOTE, status,
		"status should be closed by remote");
}

/* Verify that nrf_cloud_run will change the cloud disconnect
 * event to misc if the requested event is POLLERR
 */
ZTEST(nrf_cloud_test, test_cloud_run_pollerr)
{
	zassert_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state (uninitialized) at the start of test");

	/* Init the cloud successfully */
	init_cloud_success();

	/* Connect to the cloud successfully */
	nct_connect_fake.custom_fake = fake_nct_connect__succeeds;
	nct_disconnect_fake.custom_fake = fake_nct_disconnect__succeeds;
	/* These fakes are for the poll thread */
	nct_socket_get_fake.custom_fake = fake_nct_socket_get_data;
	nct_keepalive_time_left_fake.custom_fake = fake_nct_keepalive_time_left_get;

	/* Transport process needs to be failed */
	nct_process_fake.custom_fake = fake_nct_process__fails;
	/* Change poll_fake to indicate POLLERR */
	poll_fake.custom_fake = fake_poll__pollerr;

	(void)start_connection_poll();

	/* Wait for the events */
	wait_for_connecting_and_disconnected_poll_thread();

	zassert_equal(NRF_CLOUD_DISCONNECT_MISC, status, "status should be misc");
}

#else

/* Skip the above tests these two functions */
ZTEST(nrf_cloud_test, test_start_connection_poll_in_idle)
{
	ztest_test_skip();
}

ZTEST(nrf_cloud_test, test_start_connection_poll_already_active)
{
	ztest_test_skip();
}

ZTEST(nrf_cloud_test, test_start_connection_poll_success)
{
	ztest_test_skip();
}

ZTEST(nrf_cloud_test, test_cloud_run_cloud_connecting_failed)
{
	ztest_test_skip();
}

ZTEST(nrf_cloud_test, test_cloud_run_timed_out_connection_closed_by_remote)
{
	ztest_test_skip();
}

ZTEST(nrf_cloud_test, test_cloud_run_poll_failed)
{
	ztest_test_skip();
}

ZTEST(nrf_cloud_test, test_cloud_run_connection_failed_pollin)
{
	ztest_test_skip();
}

ZTEST(nrf_cloud_test, test_cloud_run_pollnval)
{
	ztest_test_skip();
}

ZTEST(nrf_cloud_test, test_cloud_run_pollhup)
{
	ztest_test_skip();
}

ZTEST(nrf_cloud_test, test_cloud_run_pollerr)
{
	ztest_test_skip();
}
#endif
