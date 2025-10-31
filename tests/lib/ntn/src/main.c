/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <unity.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <modem/ntn.h>
#include <modem/lte_lc.h>
#include <nrf_errno.h>
#include <mock_nrf_modem_at.h>

#include "cmock_nrf_modem_at.h"

#define TEST_EVENT_MAX_COUNT 20
#define IGNORE NULL

static struct ntn_evt test_event_data[TEST_EVENT_MAX_COUNT];
static uint8_t ntn_callback_count_occurred;
static uint8_t ntn_callback_count_expected;
static char at_notif[2048];

K_SEM_DEFINE(event_handler_called_sem, 0, TEST_EVENT_MAX_COUNT);

/* at_monitor_dispatch() is implemented in at_monitor library and
 * we'll call it directly to fake received AT commands/notifications
 */
extern void at_monitor_dispatch(const char *at_notif);

/* ntn_on_modem_init() and ntn_on_modem_cfun() are implemented in NTN library and
 * we'll call those directly to fake nrf_modem_lib calls to these functions.
 */
extern void ntn_on_modem_init(int err, void *ctx);
extern void ntn_on_modem_cfun(int mode, void *ctx);

static void ntn_event_handler(const struct ntn_evt *const evt)
{
	uint8_t index = ntn_callback_count_occurred;

	TEST_ASSERT_MESSAGE(ntn_callback_count_occurred < ntn_callback_count_expected,
			    "NTN event callback called more times than expected");

	TEST_ASSERT_EQUAL(test_event_data[index].type, evt->type);

	switch (evt->type) {
	case NTN_EVT_LOCATION_REQUEST:
		TEST_ASSERT_EQUAL(test_event_data[index].location_request.requested,
				  evt->location_request.requested);
		TEST_ASSERT_EQUAL(test_event_data[index].location_request.accuracy,
				  evt->location_request.accuracy);
		break;

	default:
		TEST_FAIL_MESSAGE("Unhandled test event");
		break;
	}

	ntn_callback_count_occurred++;

	k_sem_give(&event_handler_called_sem);
}

static void helper_ntn_data_clear(void)
{
	memset(&test_event_data, 0, sizeof(test_event_data));
}

void setUp(void)
{
	mock_nrf_modem_at_Init();

	helper_ntn_data_clear();
	ntn_register_handler(ntn_event_handler);

	ntn_callback_count_occurred = 0;
	ntn_callback_count_expected = 0;
	k_sem_reset(&event_handler_called_sem);
}

void tearDown(void)
{
	TEST_ASSERT_EQUAL(ntn_callback_count_expected, ntn_callback_count_occurred);

	ntn_register_handler(NULL);

	mock_nrf_modem_at_Verify();
}

/* This is needed because AT Monitor library is initialized in SYS_INIT. */
static int sys_init_helper(void)
{
	__cmock_nrf_modem_at_notif_handler_set_ExpectAnyArgsAndReturn(0);

	return 0;
}

/* Tests that notification subscription is requested on modemlib init. */
void test_ntn_on_modem_init(void)
{
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%LOCATION=1", EXIT_SUCCESS);

	ntn_on_modem_init(0, NULL);
}

/* Tests that notification subscription is not requested on modemlib init fail. */
void test_ntn_on_modem_init_error(void)
{
	ntn_on_modem_init(-NRF_EACCES, NULL);
}

/* Tests error handling for notification subscription. */
void test_ntn_on_modem_init_subscribe_error(void)
{
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%LOCATION=1", -NRF_ENOMEM);

	ntn_on_modem_init(0, NULL);
}

/* Tests that notification subscription is requested with functional mode 0. */
void test_ntn_on_modem_cfun(void)
{
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%LOCATION=1", EXIT_SUCCESS);

	ntn_on_modem_cfun(LTE_LC_FUNC_MODE_POWER_OFF, NULL);
}

/* Tests that notification subscription is not requested for other modes. */
void test_ntn_on_modem_cfun_no_subscribe(void)
{
	ntn_on_modem_cfun(LTE_LC_FUNC_MODE_NORMAL, NULL);
	ntn_on_modem_cfun(LTE_LC_FUNC_MODE_RX_ONLY, NULL);
	ntn_on_modem_cfun(LTE_LC_FUNC_MODE_OFFLINE, NULL);
	ntn_on_modem_cfun(LTE_LC_FUNC_MODE_DEACTIVATE_LTE, NULL);
	ntn_on_modem_cfun(LTE_LC_FUNC_MODE_ACTIVATE_LTE, NULL);
	ntn_on_modem_cfun(LTE_LC_FUNC_MODE_DEACTIVATE_GNSS, NULL);
	ntn_on_modem_cfun(LTE_LC_FUNC_MODE_DEACTIVATE_UICC, NULL);
	ntn_on_modem_cfun(LTE_LC_FUNC_MODE_ACTIVATE_UICC, NULL);
	ntn_on_modem_cfun(LTE_LC_FUNC_MODE_OFFLINE_UICC_ON, NULL);
	ntn_on_modem_cfun(LTE_LC_FUNC_MODE_OFFLINE_KEEP_REG, NULL);
	ntn_on_modem_cfun(LTE_LC_FUNC_MODE_OFFLINE_KEEP_REG_UICC_ON, NULL);
}

/* Tests that %LOCATION notification is handled correctly when no event handler is set. */
void test_ntn_no_event_handler(void)
{
	ntn_callback_count_expected = 0;

	ntn_register_handler(NULL);

	strcpy(at_notif, "%LOCATION: 2000\r\n");
	at_monitor_dispatch(at_notif);

	strcpy(at_notif, "%LOCATION: 0\r\n");
	at_monitor_dispatch(at_notif);
}

void test_ntn_malformed_notification(void)
{
	ntn_callback_count_expected = 0;

	ntn_register_handler(NULL);

	strcpy(at_notif, "%LOCATION: \r\n");
	at_monitor_dispatch(at_notif);

	strcpy(at_notif, "%LOCATION: aa\r\n");
	at_monitor_dispatch(at_notif);

	strcpy(at_notif, "%LOCATION: 200,\r\n");
	at_monitor_dispatch(at_notif);

	strcpy(at_notif, "%LOCATION: 200,\"\"\r\n");
	at_monitor_dispatch(at_notif);

	strcpy(at_notif, "%LOCATION: 200,60,\r\n");
	at_monitor_dispatch(at_notif);

	strcpy(at_notif, "%LOCATION: 200,60,\"bb\"\r\n");
	at_monitor_dispatch(at_notif);
}

/* Tests that location updates are requested correctly. */
void test_ntn_location_request_event(void)
{
	ntn_callback_count_expected = 3;

	test_event_data[0].type = NTN_EVT_LOCATION_REQUEST;
	test_event_data[0].location_request.requested = true;
	test_event_data[0].location_request.accuracy = 200;

	test_event_data[1].type = NTN_EVT_LOCATION_REQUEST;
	test_event_data[1].location_request.requested = true;
	test_event_data[1].location_request.accuracy = 2000;

	test_event_data[2].type = NTN_EVT_LOCATION_REQUEST;
	test_event_data[2].location_request.requested = false;
	test_event_data[2].location_request.accuracy = 0;

	strcpy(at_notif, "%LOCATION: 200\r\n");
	at_monitor_dispatch(at_notif);

	/* No event expected because the accuracy does not change. */
	strcpy(at_notif, "%LOCATION: 200\r\n");
	at_monitor_dispatch(at_notif);

	strcpy(at_notif, "%LOCATION: 2000\r\n");
	at_monitor_dispatch(at_notif);

	strcpy(at_notif, "%LOCATION: 0\r\n");
	at_monitor_dispatch(at_notif);
}

/* Tests that location updates are requested correctly when next_requested_accuracy is used. */
void test_ntn_location_request_event_next(void)
{
	ntn_callback_count_expected = 2;

	test_event_data[0].type = NTN_EVT_LOCATION_REQUEST;
	test_event_data[0].location_request.requested = true;
	test_event_data[0].location_request.accuracy = 200;

	test_event_data[1].type = NTN_EVT_LOCATION_REQUEST;
	test_event_data[1].location_request.requested = false;
	test_event_data[1].location_request.accuracy = 0;

	/* Modem needs location in 60 seconds. The event is sent earlier to allow enough
	 * time for GNSS to get a fix.
	 */
	strcpy(at_notif, "%LOCATION: 0,200,60\r\n");
	at_monitor_dispatch(at_notif);

	k_sleep(K_SECONDS(5));

	/* No event expected yet */
	TEST_ASSERT_EQUAL(0, ntn_callback_count_occurred);

	/* Wait until event has been sent */
	k_sleep(K_SECONDS(60 - CONFIG_NTN_LOCATION_REQUEST_ADVANCE));

	/* Modem no longer needs location after 60 seconds, the event should be sent after
	 * 60 seconds.
	 */
	strcpy(at_notif, "%LOCATION: 200,0,60\r\n");
	at_monitor_dispatch(at_notif);

	k_sleep(K_SECONDS(55));

	/* No event expected yet */
	TEST_ASSERT_EQUAL(1, ntn_callback_count_occurred);

	/* Wait until event has been sent */
	k_sleep(K_SECONDS(5));
}

/* Tests that location updates are requested correctly when next_requested_accuracy is used,
 * but the requested accuracy does not change.
 */
void test_ntn_location_request_event_next_no_change(void)
{
	ntn_callback_count_expected = 2;

	test_event_data[0].type = NTN_EVT_LOCATION_REQUEST;
	test_event_data[0].location_request.requested = true;
	test_event_data[0].location_request.accuracy = 200;

	test_event_data[1].type = NTN_EVT_LOCATION_REQUEST;
	test_event_data[1].location_request.requested = false;
	test_event_data[1].location_request.accuracy = 0;

	/* Request same accuracy immediately and after some time. */
	strcpy(at_notif, "%LOCATION: 200,200,60\r\n");
	at_monitor_dispatch(at_notif);

	/* Wait until accuracy has been updated. No event is expected for the change. */
	k_sleep(K_SECONDS(61));

	strcpy(at_notif, "%LOCATION: 0\r\n");
	at_monitor_dispatch(at_notif);
}

/* Tests that location updates are requested correctly when next_requested_accuracy is used
 * and it's needed in less than CONFIG_NTN_LOCATION_REQUEST_ADVANCE seconds.
 */
void test_ntn_location_request_event_next_immediate(void)
{
	ntn_callback_count_expected = 2;

	test_event_data[0].type = NTN_EVT_LOCATION_REQUEST;
	test_event_data[0].location_request.requested = true;
	test_event_data[0].location_request.accuracy = 200;

	test_event_data[1].type = NTN_EVT_LOCATION_REQUEST;
	test_event_data[1].location_request.requested = false;
	test_event_data[1].location_request.accuracy = 0;

	/* Modem needs location in 10 seconds. This is less than
	 * CONFIG_NTN_LOCATION_REQUEST_ADVANCE, so the event is sent immediately.
	 */
	strcpy(at_notif, "%LOCATION: 0,200,10\r\n");
	at_monitor_dispatch(at_notif);

	/* The event is dispatched from work queue, so a short sleep is needed for the work to get
	 * scheduled.
	 */
	k_sleep(K_MSEC(1));

	strcpy(at_notif, "%LOCATION: 0\r\n");
	at_monitor_dispatch(at_notif);
}

/* Tests that location can be set correctly. */
void test_ntn_location_set(void)
{
	int ret;

	ntn_callback_count_expected = 2;

	test_event_data[0].type = NTN_EVT_LOCATION_REQUEST;
	test_event_data[0].location_request.requested = true;
	test_event_data[0].location_request.accuracy = 2000;

	test_event_data[1].type = NTN_EVT_LOCATION_REQUEST;
	test_event_data[1].location_request.requested = false;
	test_event_data[1].location_request.accuracy = 0;

	strcpy(at_notif, "%LOCATION: 2000\r\n");
	at_monitor_dispatch(at_notif);

	__mock_nrf_modem_at_printf_ExpectAndReturn(
		"AT%LOCATION=2,\"60.000000\",\"24.000000\",\"100.0\",2000,43200", EXIT_SUCCESS);

	ret = ntn_location_set(60.0, 24.0, 100.0, 43200);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);

	strcpy(at_notif, "%LOCATION: 0\r\n");
	at_monitor_dispatch(at_notif);
}

/* Tests that location set handles AT command error correctly. */
void test_ntn_location_set_error(void)
{
	int ret;

	ntn_callback_count_expected = 2;

	test_event_data[0].type = NTN_EVT_LOCATION_REQUEST;
	test_event_data[0].location_request.requested = true;
	test_event_data[0].location_request.accuracy = 500;

	test_event_data[1].type = NTN_EVT_LOCATION_REQUEST;
	test_event_data[1].location_request.requested = false;
	test_event_data[1].location_request.accuracy = 0;

	strcpy(at_notif, "%LOCATION: 500\r\n");
	at_monitor_dispatch(at_notif);

	__mock_nrf_modem_at_printf_ExpectAndReturn(
		"AT%LOCATION=2,\"60.000000\",\"24.000000\",\"100.0\",500,0", -NRF_ENOMEM);

	ret = ntn_location_set(60.0, 24.0, 100.0, 0);
	TEST_ASSERT_EQUAL(-EFAULT, ret);

	strcpy(at_notif, "%LOCATION: 0\r\n");
	at_monitor_dispatch(at_notif);
}

/* Tests that location can be invalidated correctly. */
void test_ntn_location_invalidate(void)
{
	int ret;

	__mock_nrf_modem_at_printf_ExpectAndReturn(
		"AT%LOCATION=2", EXIT_SUCCESS);

	ret = ntn_location_invalidate();
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
}

/* Tests that location invalidate handles AT command error correctly. */
void test_ntn_location_invalidate_error(void)
{
	int ret;

	__mock_nrf_modem_at_printf_ExpectAndReturn(
		"AT%LOCATION=2", -NRF_ENOMEM);

	ret = ntn_location_invalidate();
	TEST_ASSERT_EQUAL(-EFAULT, ret);
}

/* It is required to be added to each test. That is because unity's
 * main may return nonzero, while zephyr's main currently must
 * return 0 in all cases (other values are reserved).
 */
extern int unity_main(void);

int main(void)
{
	(void)unity_main();

	return 0;
}

SYS_INIT(sys_init_helper, POST_KERNEL, 0);
