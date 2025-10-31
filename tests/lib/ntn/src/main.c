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

/* ntn_on_modem_cfun() is implemented in NTN library and
 * we'll call it directly to fake nrf_modem_lib call to this function
 */
extern void ntn_on_modem_cfun(int mode, void *ctx);

static void ntn_event_handler(const struct ntn_evt *const evt)
{
	uint8_t index = ntn_callback_count_occurred;

	TEST_ASSERT_MESSAGE(ntn_callback_count_occurred < ntn_callback_count_expected,
			    "NTN event callback called more times than expected");

	TEST_ASSERT_EQUAL(test_event_data[index].type, evt->type);

	switch (evt->type) {
	case NTN_EVT_MODEM_LOCATION_UPDATE:
		TEST_ASSERT_EQUAL(test_event_data[index].modem_location.updates_requested,
				  evt->modem_location.updates_requested);
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

/* Tests that notification subscription is requested when expected. */
void test_ntn_on_modem_cfun(void)
{
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%LOCATION=1", EXIT_SUCCESS);

	ntn_on_modem_cfun(LTE_LC_FUNC_MODE_NORMAL, NULL);

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%LOCATION=1", EXIT_SUCCESS);

	ntn_on_modem_cfun(LTE_LC_FUNC_MODE_ACTIVATE_LTE, NULL);
}

/* Tests that notification subscription is not requested for other modes. */
void test_ntn_on_modem_cfun_no_subscribe(void)
{
	ntn_on_modem_cfun(LTE_LC_FUNC_MODE_POWER_OFF, NULL);
	ntn_on_modem_cfun(LTE_LC_FUNC_MODE_RX_ONLY, NULL);
	ntn_on_modem_cfun(LTE_LC_FUNC_MODE_OFFLINE, NULL);
	ntn_on_modem_cfun(LTE_LC_FUNC_MODE_DEACTIVATE_LTE, NULL);
	ntn_on_modem_cfun(LTE_LC_FUNC_MODE_DEACTIVATE_GNSS, NULL);
	ntn_on_modem_cfun(LTE_LC_FUNC_MODE_ACTIVATE_GNSS, NULL);
	ntn_on_modem_cfun(LTE_LC_FUNC_MODE_DEACTIVATE_UICC, NULL);
	ntn_on_modem_cfun(LTE_LC_FUNC_MODE_ACTIVATE_UICC, NULL);
	ntn_on_modem_cfun(LTE_LC_FUNC_MODE_OFFLINE_UICC_ON, NULL);
	ntn_on_modem_cfun(LTE_LC_FUNC_MODE_OFFLINE_KEEP_REG, NULL);
	ntn_on_modem_cfun(LTE_LC_FUNC_MODE_OFFLINE_KEEP_REG_UICC_ON, NULL);
}

/* Tests that location is updated to modem to keep location within the given accuracy. */
void test_ntn_modem_location_update(void)
{
	ntn_callback_count_expected = 2;

	test_event_data[0].type = NTN_EVT_MODEM_LOCATION_UPDATE;
	test_event_data[0].modem_location.updates_requested = true;

	test_event_data[1].type = NTN_EVT_MODEM_LOCATION_UPDATE;
	test_event_data[1].modem_location.updates_requested = false;

	/* This update should be ignored, because no updates have been requested */
	ntn_modem_location_update(0.0, 0.0, 0.0, 0.0);
	k_sleep(K_SECONDS(1));

	strcpy(at_notif, "%LOCATION: 200\r\n");
	at_monitor_dispatch(at_notif);

	__mock_nrf_modem_at_printf_ExpectAndReturn(
		"AT%LOCATION=2,\"60.000000\",\"24.000000\",\"100.0\",200,60", EXIT_SUCCESS);

	__mock_nrf_modem_at_printf_ExpectAndReturn(
		"AT%LOCATION=2,\"60.002000\",\"24.002000\",\"100.0\",200,6", EXIT_SUCCESS);

	__mock_nrf_modem_at_printf_ExpectAndReturn(
		"AT%LOCATION=2,\"60.004000\",\"24.004000\",\"200.0\",2000,60", EXIT_SUCCESS);

	__mock_nrf_modem_at_printf_ExpectAndReturn(
		"AT%LOCATION=2,\"60.100000\",\"24.100000\",\"200.0\",2000,60", EXIT_SUCCESS);

	ntn_modem_location_update(60.0, 24.0, 100.0, 0.0);
	k_sleep(K_SECONDS(1));

	/* This should not cause an update to modem, because the location is within accuracy */
	ntn_modem_location_update(60.001, 24.001, 100.0, 30.0);
	k_sleep(K_SECONDS(1));

	ntn_modem_location_update(60.002, 24.002, 100.0, 30.0);
	k_sleep(K_SECONDS(1));

	strcpy(at_notif, "%LOCATION: 2000\r\n");
	at_monitor_dispatch(at_notif);

	ntn_modem_location_update(60.004, 24.004, 200.0, 5.0);
	k_sleep(K_SECONDS(1));

	ntn_modem_location_update(60.100, 24.100, 200.0, 5.0);
	k_sleep(K_SECONDS(1));

	strcpy(at_notif, "%LOCATION: 0\r\n");
	at_monitor_dispatch(at_notif);

	/* This update should be ignored, because no updates have been requested */
	ntn_modem_location_update(60.200, 24.200, 200.0, 0.0);
}

/* Tests that location updates are requested correctly when modem indicates that location
 * is needed later.
 */
void test_ntn_modem_location_update_next(void)
{
	ntn_callback_count_expected = 2;

	test_event_data[0].type = NTN_EVT_MODEM_LOCATION_UPDATE;
	test_event_data[0].modem_location.updates_requested = true;

	test_event_data[1].type = NTN_EVT_MODEM_LOCATION_UPDATE;
	test_event_data[1].modem_location.updates_requested = false;

	/* Modem needs location in 60 seconds. The event is sent earlier to allow enough
	 * time for GNSS to get a fix.
	 */
	strcpy(at_notif, "%LOCATION: 0,200,60\r\n");
	at_monitor_dispatch(at_notif);

	k_sleep(K_SECONDS(5));

	/* No event expected yet */
	TEST_ASSERT_EQUAL(0, ntn_callback_count_occurred);

	/* Wait until event has been sent */
	k_sleep(K_SECONDS(60 - CONFIG_NTN_MODEM_LOCATION_REQ_MARGIN));

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

/* Tests that accuracy changes are handled correctly. */
void test_ntn_modem_location_update_accuracy_change(void)
{
	ntn_callback_count_expected = 2;

	test_event_data[0].type = NTN_EVT_MODEM_LOCATION_UPDATE;
	test_event_data[0].modem_location.updates_requested = true;

	test_event_data[1].type = NTN_EVT_MODEM_LOCATION_UPDATE;
	test_event_data[1].modem_location.updates_requested = false;

	/* Modem needs location with 200 m accuracy for 30 seconds, after that it needs location
	 * with 2000 m accuracy.
	 */
	strcpy(at_notif, "%LOCATION: 200,2000,30\r\n");
	at_monitor_dispatch(at_notif);

	__mock_nrf_modem_at_printf_ExpectAndReturn(
		"AT%LOCATION=2,\"60.000000\",\"24.000000\",\"100.0\",200,60", EXIT_SUCCESS);

	__mock_nrf_modem_at_printf_ExpectAndReturn(
		"AT%LOCATION=2,\"60.000290\",\"24.000290\",\"100.0\",2000,60", EXIT_SUCCESS);

	double latitude = 60.0;
	double longitude = 24.0;

	for (int i = 0; i < 30; i++) {
		ntn_modem_location_update(latitude, longitude, 100.0, 0.0);

		k_sleep(K_SECONDS(1));

		latitude += 0.00001;
		longitude += 0.00001;
	}

	strcpy(at_notif, "%LOCATION: 0\r\n");
	at_monitor_dispatch(at_notif);
}

/* Tests that the validity time is calculated correctly based on the speed and that location
 * is updated to the modem before the validity time expires.
 */
void test_ntn_modem_location_update_validity_time(void)
{
	ntn_callback_count_expected = 2;

	test_event_data[0].type = NTN_EVT_MODEM_LOCATION_UPDATE;
	test_event_data[0].modem_location.updates_requested = true;

	test_event_data[1].type = NTN_EVT_MODEM_LOCATION_UPDATE;
	test_event_data[1].modem_location.updates_requested = false;

	/* Modem needs location with 200 m accuracy */
	strcpy(at_notif, "%LOCATION: 200\r\n");
	at_monitor_dispatch(at_notif);

	__mock_nrf_modem_at_printf_ExpectAndReturn(
		"AT%LOCATION=2,\"60.000000\",\"24.000000\",\"100.0\",200,7", EXIT_SUCCESS);

	__mock_nrf_modem_at_printf_ExpectAndReturn(
		"AT%LOCATION=2,\"60.000500\",\"24.000500\",\"100.0\",200,6", EXIT_SUCCESS);

	__mock_nrf_modem_at_printf_ExpectAndReturn(
		"AT%LOCATION=2,\"60.001000\",\"24.001000\",\"100.0\",200,6", EXIT_SUCCESS);

	__mock_nrf_modem_at_printf_ExpectAndReturn(
		"AT%LOCATION=2,\"60.001500\",\"24.001500\",\"100.0\",200,6", EXIT_SUCCESS);

	__mock_nrf_modem_at_printf_ExpectAndReturn(
		"AT%LOCATION=2,\"60.002000\",\"24.002000\",\"100.0\",200,6", EXIT_SUCCESS);

	__mock_nrf_modem_at_printf_ExpectAndReturn(
		"AT%LOCATION=2,\"60.002500\",\"24.002500\",\"100.0\",200,6", EXIT_SUCCESS);

	double latitude = 60.0;
	double longitude = 24.0;

	for (int i = 0; i < 30; i++) {
		/* Speed is 100 km/h = 27.78 m/s */
		ntn_modem_location_update(latitude, longitude, 100.0, 27.78);

		k_sleep(K_SECONDS(1));

		latitude += 0.0001;
		longitude += 0.0001;
	}

	strcpy(at_notif, "%LOCATION: 0\r\n");
	at_monitor_dispatch(at_notif);
}

/* Tests that location can be set and invalidated correctly. */
void test_ntn_modem_location_set_and_invalidate(void)
{
	int ret;

	__mock_nrf_modem_at_printf_ExpectAndReturn(
		"AT%LOCATION=2,\"60.000000\",\"24.000000\",\"100.0\",0,0", EXIT_SUCCESS);

	ret = ntn_modem_location_set(60.0, 24.0, 100.0, 0);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);

	__mock_nrf_modem_at_printf_ExpectAndReturn(
		"AT%LOCATION=2", EXIT_SUCCESS);

	ret = ntn_modem_location_invalidate();
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
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
