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
#include <drivers/mock_gps.h>

#include "app_module_event.h"
#include "gps_module_event.h"
#include "data_module_event.h"

extern struct event_listener __event_listener_gps_module;

#define GPS_MODULE_EVT_HANDLER(eh) __event_listener_gps_module.notification(eh)

/* NMEA sample data, including CRC.
 * Source: https://docs.novatel.com/OEM7/Content/Logs/GPGGA.htm
 */
#define GPS_NMEA_SAMPLE                                                        \
	"$GPGGA,134658.00,5106.9792,N,11402.3003,W,2,09,1.0,1048.47,M,-16.27,M,08,AAAA*60"

static char exp_nmea[NMEA_MAX_LEN];

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

/* Dummy structs to please the linker. The EVENT_SUBSCRIBE macros in gps_module.c
 * depend on these to exist. But since we are unit testing, we dont need
 * these subscriptions and hence these structs can remain uninitialized.
 */
const struct event_type __event_type_gps_module_event;
const struct event_type __event_type_app_module_event;
const struct event_type __event_type_data_module_event;
const struct event_type __event_type_util_module_event;

/* Dummy functions and objects - End.  */

/* The following is required because unity is using a different main signature
 * (returns int) and zephyr expects main to not return value.
 */
extern int unity_main(void);

/* Variable to store the event handler function that the GPS module registers
 * with the GPS driver through the gps_init() function. This can be used to
 * simulate events from the GPS driver.
 */
static gps_event_handler_t gps_module_gps_evt_handler;

/* Suite teardown finalizes with mandatory call to generic_suiteTearDown. */
extern int generic_suiteTearDown(int num_failures);

int test_suiteTearDown(int num_failures)
{
	return generic_suiteTearDown(num_failures);
}

void setUp(void)
{
	mock_gps_Init();
	mock_modules_common_Init();
	mock_event_manager_Init();
}

void tearDown(void)
{
	mock_gps_Verify();
	mock_modules_common_Verify();
	mock_event_manager_Verify();
}

static int gps_init_callback(const struct device *dev,
			     gps_event_handler_t handler, int number_of_calls)
{
	/* Latch the gps_evt_handler for future use. */
	gps_module_gps_evt_handler = handler;
	return 0;
}

static void validate_gps_active_evt(struct event_header *eh, int no_of_calls)
{
	struct gps_module_event *event = cast_gps_module_event(eh);

	TEST_ASSERT_EQUAL(GPS_EVT_ACTIVE, event->type);
}

static void validate_gps_data_ready_evt(struct event_header *eh, int no_of_calls)
{
	struct gps_module_event *event = cast_gps_module_event(eh);

	TEST_ASSERT_EQUAL(GPS_EVT_DATA_READY, event->type);
	TEST_ASSERT_EQUAL(GPS_MODULE_DATA_FORMAT_NMEA, event->data.gps.format);
	TEST_ASSERT_EQUAL_CHAR_ARRAY(exp_nmea, event->data.gps.nmea, sizeof(exp_nmea));
}

static void setup_gps_module_in_init_state(void)
{
	struct app_module_event *app_module_event = new_app_module_event();

	static struct module_data expected_module_data = {
		.name = "gps",
		.msg_q = NULL,
		.supports_shutdown = true,
	};

	__wrap_gps_init_ExpectAnyArgsAndReturn(0);
	__wrap_gps_init_AddCallback(&gps_init_callback);
	__wrap_module_start_ExpectAndReturn(&expected_module_data, 0);

	app_module_event->type = APP_EVT_START;

	bool ret = GPS_MODULE_EVT_HANDLER((struct event_header *)app_module_event);

	TEST_ASSERT_EQUAL(0, ret);

	free(app_module_event);
}

static void setup_gps_module_in_running_state(void)
{
	setup_gps_module_in_init_state();

	struct data_module_event *data_module_event = new_data_module_event();

	data_module_event->type = DATA_EVT_CONFIG_INIT;
	data_module_event->data.cfg.gps_timeout = 60;

	bool ret = GPS_MODULE_EVT_HANDLER((struct event_header *)data_module_event);

	TEST_ASSERT_EQUAL(0, ret);

	free(data_module_event);
}

static struct gps_event *generate_gps_fix_evt(void)
{
	static struct gps_event evt = {
		.type = GPS_EVT_NMEA_FIX,
	};

	evt.nmea.len = snprintf(evt.nmea.buf, GPS_NMEA_SENTENCE_MAX_LENGTH,
				"%s", GPS_NMEA_SAMPLE);

	return &evt;
}

/* Test whether sending a APP_EVT_DATA_GET event to the GPS module generates
 * the GPS_EVT_ACTIVE event, when the GPS module is in the running state.
 */
void test_gps_start(void)
{
	/* Pre-condition. */
	setup_gps_module_in_running_state();

	/* Setup expectations.  */
	__wrap_gps_start_ExpectAnyArgsAndReturn(0);

	__wrap__event_submit_Stub(&validate_gps_active_evt);

	/* Stimulus. */
	struct app_module_event *app_module_event = new_app_module_event();

	app_module_event->type = APP_EVT_DATA_GET;
	app_module_event->count = 1;
	app_module_event->data_list[0] = APP_DATA_GNSS;

	bool ret = GPS_MODULE_EVT_HANDLER((struct event_header *)app_module_event);

	TEST_ASSERT_EQUAL(0, ret);

	/* Cleanup */
	free(app_module_event);
	__wrap__event_submit_Stub(NULL);
}

/* Test whether the GPS module generates an event with GPS fix on receiving a
 * fix from the GPS driver.
 */
void test_gps_fix(void)
{
	resetTest();

	/* Pre-condition. */
	setup_gps_module_in_running_state();

	/* Make gps_module start gps driver. */
	__wrap_gps_start_ExpectAnyArgsAndReturn(0);
	__wrap__event_submit_ExpectAnyArgs();

	struct app_module_event *app_module_event = new_app_module_event();

	app_module_event->type = APP_EVT_DATA_GET;
	app_module_event->count = 1;
	app_module_event->data_list[0] = APP_DATA_GNSS;

	bool ret = GPS_MODULE_EVT_HANDLER((struct event_header *)app_module_event);

	TEST_ASSERT_EQUAL(0, ret);

	free(app_module_event);

	/* Simulate a GPS FIX and send it to the GPS module.  */
	struct gps_event *gps_fix_evt = generate_gps_fix_evt();

	/* Expect the gps module to generate an event with the fix data. */
	__wrap__event_submit_Stub(&validate_gps_data_ready_evt);

	strncpy(exp_nmea, gps_fix_evt->nmea.buf, sizeof(exp_nmea) - 1);
	exp_nmea[sizeof(exp_nmea) - 1] = '\0';

	gps_module_gps_evt_handler(NULL, gps_fix_evt);
	__wrap__event_submit_Stub(NULL);
}

void main(void)
{
	(void)unity_main();
}
