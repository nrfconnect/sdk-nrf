/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <unity.h>
#include <stdbool.h>
#include <stdlib.h>
#include <mock_modules_common.h>
#include <mock_event_manager.h>
#include <mock_event_manager_priv.h>
#include <mock_date_time.h>
#include <mock_nrf_modem_at.h>
#include <mock_nrf_modem_gnss.h>

#include "app_module_event.h"
#include "gnss_module_event.h"
#include "data_module_event.h"
#include "modem_module_event.h"

extern struct event_listener __event_listener_gnss_module;

/* The addresses of the following structures will be returned when the event_manager_alloc()
 * function is called.
 */
static struct app_module_event app_module_event_memory;
static struct modem_module_event modem_module_event_memory;
static struct gnss_module_event gnss_module_event_memory;

#define GNSS_MODULE_EVT_HANDLER(eh) __event_listener_gnss_module.notification(eh)

/* PVT data with a valid fix. */
static struct nrf_modem_gnss_pvt_data_frame pvt_data = {
	.latitude = 60.0,
	.longitude = 25.0,
	.altitude = 100.0,
	.accuracy = 5.0,
	.speed = 0.1,
	.heading = 90.0,
	.datetime = {
		.year = 2021,
		.month = 8,
		.day = 25,
		.hour = 14,
		.minute = 58,
		.seconds = 42,
		.ms = 5,
	},
	.flags = NRF_MODEM_GNSS_PVT_FLAG_FIX_VALID
};

/* A-GPS request data. */
static struct nrf_modem_gnss_agps_data_frame agps_data = {
	.sv_mask_ephe = 0xabbaabba,
	.sv_mask_alm = 0xdeaddead,
	.data_flags = NRF_MODEM_GNSS_AGPS_GPS_UTC_REQUEST |
		      NRF_MODEM_GNSS_AGPS_NEQUICK_REQUEST |
		      NRF_MODEM_GNSS_AGPS_POSITION_REQUEST
};

/* NMEA sample data, including CRC.
 * Source: https://docs.novatel.com/OEM7/Content/Logs/GPGGA.htm
 */
static char exp_nmea[] =
	"$GPGGA,134658.00,5106.9792,N,11402.3003,W,2,09,1.0,1048.47,M,-16.27,M,08,AAAA*60";

#define GNSS_MODULE_MAX_EVENTS 5

/* Counter for received GNSS module events. */
static uint32_t gnss_module_event_count;
/* Number of expected GNSS module events. */
static uint32_t expected_gnss_module_event_count;
/* Array for expected GNSS module events. */
static struct gnss_module_event expected_gnss_module_events[GNSS_MODULE_MAX_EVENTS];
/* Semaphore for waiting for events to be received. */
static K_SEM_DEFINE(gnss_module_event_sem, 0, 5);

/* Dummy functions and objects. */

/* The following function needs to be stubbed this way because event manager
 * uses heap to allocate memory for events.
 * The other alternative is to mock the k_malloc using CMock framework, but
 * that will pollute the test code and will be an overkill.
 */
void *k_malloc(size_t size)
{
	return malloc(size);
}

/* Dummy structs to please the linker. The EVENT_SUBSCRIBE macros in gnss_module.c
 * depend on these to exist. But since we are unit testing, we dont need
 * these subscriptions and hence these structs can remain uninitialized.
 */
struct event_type __event_type_gnss_module_event;
struct event_type __event_type_app_module_event;
struct event_type __event_type_data_module_event;
struct event_type __event_type_util_module_event;
struct event_type __event_type_modem_module_event;

/* Dummy functions and objects - End.  */

/* The following is required because unity is using a different main signature
 * (returns int) and zephyr expects main to not return value.
 */
extern int unity_main(void);

/* Variable to store the event handler function that the GNSS module registers with the
 * GNSS API through the nrf_modem_gnss_event_handler_set() function. This can be used to
 * simulate events from the GNSS API.
 */
static nrf_modem_gnss_event_handler_type_t gnss_module_gnss_evt_handler;

/* Suite teardown finalizes with mandatory call to generic_suiteTearDown. */
extern int generic_suiteTearDown(int num_failures);

int test_suiteTearDown(int num_failures)
{
	return generic_suiteTearDown(num_failures);
}

void setUp(void)
{
	mock_modules_common_Init();
	mock_event_manager_Init();
	mock_date_time_Init();

	gnss_module_event_count = 0;
	expected_gnss_module_event_count = 0;
	memset(&expected_gnss_module_events, 0, sizeof(expected_gnss_module_events));
}

void tearDown(void)
{
	/* Wait until we've received all events. */
	for (int i = 0; i < expected_gnss_module_event_count; i++) {
		k_sem_take(&gnss_module_event_sem, K_SECONDS(1));
	}
	TEST_ASSERT_EQUAL(expected_gnss_module_event_count, gnss_module_event_count);

	mock_modules_common_Verify();
	mock_event_manager_Verify();
	mock_date_time_Verify();
	mock_nrf_modem_gnss_Verify();
}

static int nrf_modem_gnss_event_handler_set_callback(
	nrf_modem_gnss_event_handler_type_t handler,
	int no_of_calls)
{
	/* Latch the GNSS event handler for future use. */
	gnss_module_gnss_evt_handler = handler;
	return 0;
}

static void validate_gnss_evt(struct event_header *eh, int no_of_calls)
{
	uint32_t index = gnss_module_event_count;
	struct gnss_module_event *event = cast_gnss_module_event(eh);

	/* Make sure we don't get more events than expected. */
	TEST_ASSERT_LESS_THAN(expected_gnss_module_event_count, gnss_module_event_count);

	TEST_ASSERT_EQUAL(expected_gnss_module_events[index].type, event->type);

	switch (event->type) {
	case GNSS_EVT_DATA_READY:
		TEST_ASSERT_EQUAL(
			expected_gnss_module_events[index].data.gnss.format,
			event->data.gnss.format);
		TEST_ASSERT_EQUAL_CHAR_ARRAY(
			expected_gnss_module_events[index].data.gnss.nmea,
			event->data.gnss.nmea,
			NMEA_MAX_LEN);
		break;
	case GNSS_EVT_AGPS_NEEDED:
		TEST_ASSERT_EQUAL(
			expected_gnss_module_events[index].data.agps_request.sv_mask_ephe,
			event->data.agps_request.sv_mask_ephe);
		TEST_ASSERT_EQUAL(
			expected_gnss_module_events[index].data.agps_request.sv_mask_alm,
			event->data.agps_request.sv_mask_alm);
		TEST_ASSERT_EQUAL(
			expected_gnss_module_events[index].data.agps_request.data_flags,
			event->data.agps_request.data_flags);
		break;
	case GNSS_EVT_ERROR_CODE:
		TEST_ASSERT_EQUAL(
			expected_gnss_module_events[index].data.err,
			event->data.err);
		break;
	default:
		break;
	}

	gnss_module_event_count++;

	/* Signal that an event was received. */
	k_sem_give(&gnss_module_event_sem);
}

static int32_t gnss_read_callback(void *buf, int32_t len, int type, int no_of_calls)
{
	TEST_ASSERT_NOT_NULL(buf);

	switch (type) {
	case NRF_MODEM_GNSS_DATA_PVT:
		TEST_ASSERT_EQUAL(sizeof(struct nrf_modem_gnss_pvt_data_frame), len);

		memcpy(buf, &pvt_data, len);
		break;
	case NRF_MODEM_GNSS_DATA_NMEA:
		TEST_ASSERT_EQUAL(sizeof(struct nrf_modem_gnss_nmea_data_frame), len);

		strncpy(buf, exp_nmea, len);
		break;
	case NRF_MODEM_GNSS_DATA_AGPS_REQ:
		TEST_ASSERT_EQUAL(sizeof(struct nrf_modem_gnss_agps_data_frame), len);

		memcpy(buf, &agps_data, len);
		break;
	default:
		TEST_ASSERT(false);
		break;
	}

	return 0;
}

static int validate_date_time(const struct tm *new_date_time, int no_of_calls)
{
	/* tm_year is years since 1900. */
	TEST_ASSERT_EQUAL(pvt_data.datetime.year - 1900, new_date_time->tm_year);
	/* tm_mon is months since January. */
	TEST_ASSERT_EQUAL(pvt_data.datetime.month - 1, new_date_time->tm_mon);
	TEST_ASSERT_EQUAL(pvt_data.datetime.day, new_date_time->tm_mday);
	TEST_ASSERT_EQUAL(pvt_data.datetime.hour, new_date_time->tm_hour);
	TEST_ASSERT_EQUAL(pvt_data.datetime.minute, new_date_time->tm_min);
	TEST_ASSERT_EQUAL(pvt_data.datetime.seconds, new_date_time->tm_sec);

	return 0;
}

static void setup_gnss_module_in_init_state(void)
{
	__wrap_event_manager_alloc_ExpectAnyArgsAndReturn(&app_module_event_memory);
	struct app_module_event *app_module_event = new_app_module_event();

	static struct module_data expected_module_data = {
		.name = "gnss",
		.msg_q = NULL,
		.supports_shutdown = true,
	};

	/* AT%XMAGPIO and AT%XCOEX0 AT commands for LNA configuration. */
	__wrap_nrf_modem_at_printf_ExpectAnyArgsAndReturn(0);
	__wrap_nrf_modem_at_printf_ExpectAnyArgsAndReturn(0);

	__wrap_nrf_modem_gnss_event_handler_set_AddCallback(
		&nrf_modem_gnss_event_handler_set_callback);
	__wrap_nrf_modem_gnss_event_handler_set_ExpectAnyArgsAndReturn(0);
	__wrap_module_start_ExpectAndReturn(&expected_module_data, 0);

	app_module_event->type = APP_EVT_START;

	__wrap_event_manager_alloc_ExpectAnyArgsAndReturn(&gnss_module_event_memory);
	bool ret = GNSS_MODULE_EVT_HANDLER((struct event_header *)app_module_event);

	TEST_ASSERT_EQUAL(0, ret);
}

static void setup_gnss_module_in_running_state(void)
{
	setup_gnss_module_in_init_state();

	__wrap_event_manager_alloc_ExpectAnyArgsAndReturn(&modem_module_event_memory);
	struct modem_module_event *modem_module_event = new_modem_module_event();

	modem_module_event->type = MODEM_EVT_INITIALIZED;

	bool ret = GNSS_MODULE_EVT_HANDLER((struct event_header *)modem_module_event);

	TEST_ASSERT_EQUAL(0, ret);
}

/* Test whether sending a APP_EVT_DATA_GET event to the GNSS module generates
 * the GNSS_EVT_ACTIVE event, when the GNSS module is in the running state.
 */
void test_gnss_start(void)
{
	/* Pre-condition. */
	setup_gnss_module_in_running_state();

	/* Setup expectations.  */
	/* Stop is executed before each start to ensure GNSS is in idle. */
	__wrap_nrf_modem_gnss_stop_ExpectAndReturn(0);
	/* Single fix mode. */
	__wrap_nrf_modem_gnss_fix_interval_set_ExpectAndReturn(0, 0);
	/* 60 second timeout. */
	__wrap_nrf_modem_gnss_fix_retry_set_ExpectAndReturn(60, 0);
	/* NMEA mask (GPGGA only). */
	__wrap_nrf_modem_gnss_nmea_mask_set_ExpectAndReturn(NRF_MODEM_GNSS_NMEA_GGA_MASK, 0);
	__wrap_nrf_modem_gnss_start_ExpectAndReturn(0);

	/* Set callback to validate GNSS module events. */
	__wrap__event_submit_Stub(&validate_gnss_evt);

	/* Set expected GNSS module events. */
	expected_gnss_module_event_count = 1;
	expected_gnss_module_events[0].type = GNSS_EVT_ACTIVE;

	/* Stimulus. */
	__wrap_event_manager_alloc_ExpectAnyArgsAndReturn(&app_module_event_memory);
	struct app_module_event *app_module_event = new_app_module_event();

	app_module_event->type = APP_EVT_DATA_GET;
	app_module_event->count = 1;
	app_module_event->data_list[0] = APP_DATA_GNSS;

	__wrap_event_manager_alloc_IgnoreAndReturn(&gnss_module_event_memory);
	bool ret = GNSS_MODULE_EVT_HANDLER((struct event_header *)app_module_event);

	TEST_ASSERT_EQUAL(0, ret);
}

/* Test whether the GNSS module generates an event with GNSS fix on receiving a
 * fix from the GNSS API.
 */
void test_gnss_fix(void)
{
	/* Pre-condition. */
	setup_gnss_module_in_running_state();

	/* Make gnss_module start GNSS. */
	/* Stop is executed before each start to ensure GNSS is in idle. */
	__wrap_nrf_modem_gnss_stop_ExpectAndReturn(0);
	/* Single fix mode. */
	__wrap_nrf_modem_gnss_fix_interval_set_ExpectAndReturn(0, 0);
	/* 60 second timeout. */
	__wrap_nrf_modem_gnss_fix_retry_set_ExpectAndReturn(60, 0);
	/* NMEA mask (GPGGA only). */
	__wrap_nrf_modem_gnss_nmea_mask_set_ExpectAndReturn(NRF_MODEM_GNSS_NMEA_GGA_MASK, 0);
	__wrap_nrf_modem_gnss_start_ExpectAndReturn(0);

	/* Set callback to validate GNSS module events. */
	__wrap__event_submit_Stub(&validate_gnss_evt);

	/* Set expected GNSS module events. */
	expected_gnss_module_event_count = 3;
	expected_gnss_module_events[0].type = GNSS_EVT_ACTIVE;
	expected_gnss_module_events[1].type = GNSS_EVT_INACTIVE;
	expected_gnss_module_events[2].type = GNSS_EVT_DATA_READY;
	expected_gnss_module_events[2].data.gnss.format = GNSS_MODULE_DATA_FORMAT_NMEA;
	strncpy(expected_gnss_module_events[2].data.gnss.nmea, exp_nmea, NMEA_MAX_LEN);

	/* Set callback to handle all reads from GNSS API. */
	__wrap_nrf_modem_gnss_read_Stub(&gnss_read_callback);

	/* Set callback to validate setting of date and time. */
	__wrap_date_time_set_Stub(&validate_date_time);

	__wrap_event_manager_alloc_ExpectAnyArgsAndReturn(&app_module_event_memory);
	struct app_module_event *app_module_event = new_app_module_event();

	app_module_event->type = APP_EVT_DATA_GET;
	app_module_event->count = 1;
	app_module_event->data_list[0] = APP_DATA_GNSS;

	__wrap_event_manager_alloc_IgnoreAndReturn(&gnss_module_event_memory);
	bool ret = GNSS_MODULE_EVT_HANDLER((struct event_header *)app_module_event);

	TEST_ASSERT_EQUAL(0, ret);

	/* Send PVT and NMEA events from the GNSS API to trigger a fix. */
	gnss_module_gnss_evt_handler(NRF_MODEM_GNSS_EVT_PVT);

	gnss_module_gnss_evt_handler(NRF_MODEM_GNSS_EVT_NMEA);
}

/* Test whether the GNSS module generates an event for A-GPS data on receiving an
 * A-GPS data request from the GNSS API.
 */
void test_agps_request(void)
{
	/* Pre-condition. */
	setup_gnss_module_in_running_state();

	/* Make gnss_module start GNSS. */
	/* Stop is executed before each start to ensure GNSS is in idle. */
	__wrap_nrf_modem_gnss_stop_ExpectAndReturn(0);
	/* Single fix mode. */
	__wrap_nrf_modem_gnss_fix_interval_set_ExpectAndReturn(0, 0);
	/* 60 second timeout. */
	__wrap_nrf_modem_gnss_fix_retry_set_ExpectAndReturn(60, 0);
	/* NMEA mask (GPGGA only). */
	__wrap_nrf_modem_gnss_nmea_mask_set_ExpectAndReturn(NRF_MODEM_GNSS_NMEA_GGA_MASK, 0);
	__wrap_nrf_modem_gnss_start_ExpectAndReturn(0);

	/* Set callback to validate GNSS module events. */
	__wrap__event_submit_Stub(&validate_gnss_evt);

	/* Set expected GNSS module events. */
	expected_gnss_module_event_count = 2;
	expected_gnss_module_events[0].type = GNSS_EVT_ACTIVE;
	expected_gnss_module_events[1].type = GNSS_EVT_AGPS_NEEDED;
	expected_gnss_module_events[1].data.agps_request.sv_mask_ephe = agps_data.sv_mask_ephe;
	expected_gnss_module_events[1].data.agps_request.sv_mask_alm = agps_data.sv_mask_alm;
	expected_gnss_module_events[1].data.agps_request.data_flags = agps_data.data_flags;

	/* Set callback to handle all reads from GNSS API. */
	__wrap_nrf_modem_gnss_read_Stub(&gnss_read_callback);

	__wrap_event_manager_alloc_ExpectAnyArgsAndReturn(&app_module_event_memory);
	struct app_module_event *app_module_event = new_app_module_event();

	app_module_event->type = APP_EVT_DATA_GET;
	app_module_event->count = 1;
	app_module_event->data_list[0] = APP_DATA_GNSS;

	__wrap_event_manager_alloc_IgnoreAndReturn(&gnss_module_event_memory);
	bool ret = GNSS_MODULE_EVT_HANDLER((struct event_header *)app_module_event);

	TEST_ASSERT_EQUAL(0, ret);

	/* Send A-GPS req event from the GNSS API. */
	gnss_module_gnss_evt_handler(NRF_MODEM_GNSS_EVT_AGPS_REQ);
}

/* Test that an error event is sent when the message queue becomes full.
 */
void test_msgq_full(void)
{
	/* Pre-condition. */
	setup_gnss_module_in_running_state();

	/* Make gnss_module start GNSS. */
	/* Stop is executed before each start to ensure GNSS is in idle. */
	__wrap_nrf_modem_gnss_stop_ExpectAndReturn(0);
	/* Single fix mode. */
	__wrap_nrf_modem_gnss_fix_interval_set_ExpectAndReturn(0, 0);
	/* 60 second timeout. */
	__wrap_nrf_modem_gnss_fix_retry_set_ExpectAndReturn(60, 0);
	/* NMEA mask (GPGGA only). */
	__wrap_nrf_modem_gnss_nmea_mask_set_ExpectAndReturn(NRF_MODEM_GNSS_NMEA_GGA_MASK, 0);
	__wrap_nrf_modem_gnss_start_ExpectAndReturn(0);

	/* Set callback to validate GNSS module events. */
	__wrap__event_submit_Stub(&validate_gnss_evt);

	/* Set expected GNSS module events. */
	expected_gnss_module_event_count = 2;
	expected_gnss_module_events[0].type = GNSS_EVT_ACTIVE;
	expected_gnss_module_events[1].type = GNSS_EVT_ERROR_CODE;
	expected_gnss_module_events[1].data.err = -ENOMSG;

	/* Set callback to handle all reads from GNSS API. */
	__wrap_nrf_modem_gnss_read_Stub(&gnss_read_callback);

	__wrap_event_manager_alloc_ExpectAnyArgsAndReturn(&app_module_event_memory);
	struct app_module_event *app_module_event = new_app_module_event();

	app_module_event->type = APP_EVT_DATA_GET;
	app_module_event->count = 1;
	app_module_event->data_list[0] = APP_DATA_GNSS;

	__wrap_event_manager_alloc_IgnoreAndReturn(&gnss_module_event_memory);
	bool ret = GNSS_MODULE_EVT_HANDLER((struct event_header *)app_module_event);

	TEST_ASSERT_EQUAL(0, ret);

	/* Send so many events that the message queue becomes full. */
	gnss_module_gnss_evt_handler(NRF_MODEM_GNSS_EVT_BLOCKED);
	gnss_module_gnss_evt_handler(NRF_MODEM_GNSS_EVT_UNBLOCKED);
	gnss_module_gnss_evt_handler(NRF_MODEM_GNSS_EVT_BLOCKED);
	gnss_module_gnss_evt_handler(NRF_MODEM_GNSS_EVT_UNBLOCKED);
	gnss_module_gnss_evt_handler(NRF_MODEM_GNSS_EVT_BLOCKED);
	gnss_module_gnss_evt_handler(NRF_MODEM_GNSS_EVT_UNBLOCKED);
	gnss_module_gnss_evt_handler(NRF_MODEM_GNSS_EVT_BLOCKED);
	gnss_module_gnss_evt_handler(NRF_MODEM_GNSS_EVT_UNBLOCKED);
	gnss_module_gnss_evt_handler(NRF_MODEM_GNSS_EVT_BLOCKED);
	gnss_module_gnss_evt_handler(NRF_MODEM_GNSS_EVT_UNBLOCKED);
	gnss_module_gnss_evt_handler(NRF_MODEM_GNSS_EVT_BLOCKED);
}

void main(void)
{
	(void)unity_main();
}
