/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

<<<<<<< HEAD
#include <net/nrf_cloud.h>
=======
>>>>>>> 4a9a19152 (nrf_cloud: unittest: Add more tests to the lib)
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
}

/* This function runs after each completed test */
static void run_after(void *fixture)
{
	/* Uninit to reset the internal state to idle */
	(void)nrf_cloud_uninit();
}

ZTEST_SUITE(nrf_cloud_test, NULL, NULL, run_before, run_after, NULL);

/* TEST NRF_CLOUD_CONNECT */

/* Verify that nrf_cloud_connect will fail
 * if connecting to cloud in the idle state
 */
ZTEST(nrf_cloud_test, test_connect_idle)
{
	zassert_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the state of idle at the beginning of the test");

	int ret = nrf_cloud_connect();

	zassert_equal(NRF_CLOUD_CONNECT_RES_ERR_NOT_INITD, ret,
		"return should be CONNECT_RES_ERR_NOT_INITD when in the idle state");
	zassert_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should still be in the state of idle at the end of the test");
}

/* Verify that nrf_cloud_connect will fail
 * if connecting to cloud after connected
 */
ZTEST(nrf_cloud_test, test_connect_already_connected)
{
	zassert_equal(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the state of idle at the beginning of the test");

	struct nrf_cloud_init_param params = {
		.event_handler = event_handler,
		.client_id = NULL,
	};

	/* Init and connect to the cloud */
	(void)nrf_cloud_init(&params);
	zassert_equal(STATE_INITIALIZED, nfsm_get_current_state(),
		"nrf_cloud lib should be in the state of initialized");

	(void)nrf_cloud_connect();
	
	/* Wait for the connected event */
	while (!atomic_get(&cloud_connected_t)) {
		k_sleep(K_MSEC(200));
	}
	zassert_equal(STATE_CONNECTED, nfsm_get_current_state(),
		"nrf_cloud lib should be in the connected state after successfully connected");

	/* Try connecting again */
	int ret = nrf_cloud_connect();

	zassert_equal(NRF_CLOUD_CONNECT_RES_ERR_ALREADY_CONNECTED, ret,
		"return should be CONNECT_RES_ERR_ALREADY_CONNECTED");
	zassert_equal(STATE_CONNECTED, nfsm_get_current_state(),
		"nrf_cloud lib should still be in the connected state at the end of the test");
}

/* TEST NRF_CLOUD_DISCONNECT */
/* TEST NRF_CLOUD_SHADOW_DEVICE_STATUS_UPDATE */
/* TEST NRF_CLOUD_SENSOR_DATA_SEND */
/* TEST NRF_CLOUD_SENSOR_DATA_STREAM */
/* TEST NRF_CLOUD_SEND */
/* TEST NRF_CLOUD_START_CONNECTION_POLL */
/* TEST NRF_CLOUD_CLOUD_RUN */
