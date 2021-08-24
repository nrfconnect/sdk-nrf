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
#include <mock_watchdog_app.h>
#include <memfault/metrics/mock_metrics.h>
#include <memfault/core/mock_data_packetizer.h>
#include <memfault/ports/mock_watchdog.h>

#include "app_module_event.h"
#include "gps_module_event.h"
#include "debug_module_event.h"
#include "data_module_event.h"

extern struct event_listener __event_listener_debug_module;

#define DEBUG_MODULE_EVT_HANDLER(eh) __event_listener_debug_module.notification(eh)

/* Copy of the application specific Memfault metrics defined in
 * configuration/memfault/memfault_metrics_heartbeat_config.def
 */
const char * const g_memfault_metrics_id_GpsTimeToFix;
const char * const g_memfault_metrics_id_GpsSatellitesTracked;
const char * const g_memfault_metrics_id_GpsTimeoutSearchTime;

/* Dummy structs to please linker. The EVENT_SUBSCRIBE macros in debug_module.c
 * depend on these to exist. But since we are unit testing, we dont need
 * these subscriptions and hence these structs can remain uninitialized.
 */
const struct event_type __event_type_gps_module_event;
const struct event_type __event_type_debug_module_event;
const struct event_type __event_type_app_module_event;
const struct event_type __event_type_data_module_event;
const struct event_type __event_type_cloud_module_event;
const struct event_type __event_type_modem_module_event;
const struct event_type __event_type_sensor_module_event;
const struct event_type __event_type_ui_module_event;
const struct event_type __event_type_util_module_event;

/* The following is required because unity is using a different main signature
 * (returns int) and zephyr expects main to not return value.
 */
extern int unity_main(void);

/* Local reference of the internal application watchdog callback in debug module. Used to
 * infuse callback events directly from the test runner.
 */
watchdog_evt_handler_t debug_module_watchdog_callback;

/* Suite teardown finalizes with mandatory call to generic_suiteTearDown. */
extern int generic_suiteTearDown(int num_failures);

int test_suiteTearDown(int num_failures)
{
	return generic_suiteTearDown(num_failures);
}

void setUp(void)
{
	mock_watchdog_app_Init();
	mock_modules_common_Init();
	mock_event_manager_Init();
}

void tearDown(void)
{
	mock_watchdog_app_Verify();
	mock_modules_common_Verify();
	mock_event_manager_Verify();
}

static void latch_watchdog_callback(watchdog_evt_handler_t handler, int no_of_calls)
{
	debug_module_watchdog_callback = handler;
}

static void validate_debug_data_ready_evt(struct event_header *eh, int no_of_calls)
{
	struct debug_module_event *event = cast_debug_module_event(eh);

	TEST_ASSERT_EQUAL(DEBUG_EVT_MEMFAULT_DATA_READY, event->type);
}

void setup_debug_module_in_init_state(void)
{
	struct app_module_event *app_module_event = new_app_module_event();

	app_module_event->type = APP_EVT_START;

	static struct module_data expected_module_data = {
		.name = "debug",
		.msg_q = NULL,
		.supports_shutdown = true,
	};

	__wrap_watchdog_register_handler_ExpectAnyArgs();
	__wrap_watchdog_register_handler_AddCallback(&latch_watchdog_callback);
	__wrap_module_start_ExpectAndReturn(&expected_module_data, 0);

	TEST_ASSERT_EQUAL(0, DEBUG_MODULE_EVT_HANDLER((struct event_header *)app_module_event));

	k_free(app_module_event);
}

/* Test whether the correct Memfault metrics are set upon a GPS fix. */
void test_memfault_trigger_metric_sampling_on_gps_fix(void)
{
	setup_debug_module_in_init_state();

	__wrap_memfault_metrics_heartbeat_set_unsigned_ExpectAndReturn(
						MEMFAULT_METRICS_KEY(GpsTimeToFix),
						60000,
						0);
	__wrap_memfault_metrics_heartbeat_set_unsigned_ExpectAndReturn(
						MEMFAULT_METRICS_KEY(GpsSatellitesTracked),
						4,
						0);
	__wrap_memfault_metrics_heartbeat_debug_trigger_Expect();

	struct gps_module_event *gps_module_event = new_gps_module_event();

	gps_module_event->type = GPS_EVT_DATA_READY;
	gps_module_event->data.gps.satellites_tracked = 4;
	gps_module_event->data.gps.search_time = 60000;

	TEST_ASSERT_EQUAL(0, DEBUG_MODULE_EVT_HANDLER((struct event_header *)gps_module_event));

	k_free(gps_module_event);
}

/* Test whether the correct Memfault metrics are set upon a GPS timeout. */
void test_memfault_trigger_metric_sampling_on_gps_timeout(void)
{
	resetTest();
	setup_debug_module_in_init_state();

	/* Update this function to expect the search time and number of satellites. */
	__wrap_memfault_metrics_heartbeat_set_unsigned_ExpectAndReturn(
						MEMFAULT_METRICS_KEY(GpsTimeoutSearchTime),
						30000,
						0);
	__wrap_memfault_metrics_heartbeat_set_unsigned_ExpectAndReturn(
						MEMFAULT_METRICS_KEY(GpsSatellitesTracked),
						2,
						0);
	__wrap_memfault_metrics_heartbeat_debug_trigger_Ignore();

	struct gps_module_event *gps_module_event = new_gps_module_event();

	gps_module_event->type = GPS_EVT_TIMEOUT;
	gps_module_event->data.gps.satellites_tracked = 2;
	gps_module_event->data.gps.search_time = 30000;

	TEST_ASSERT_EQUAL(0, DEBUG_MODULE_EVT_HANDLER((struct event_header *)gps_module_event));

	k_free(gps_module_event);
}

/* Test that the debug module is able to submit Memfault data externally through events
 * of type DEBUG_EVT_MEMFAULT_DATA_READY carrying chunks of data.
 */
void test_memfault_trigger_data_send(void)
{
	resetTest();
	setup_debug_module_in_init_state();

	__wrap__event_submit_ExpectAnyArgs();

	__wrap_memfault_packetizer_data_available_ExpectAndReturn(1);
	__wrap_memfault_packetizer_get_chunk_ExpectAnyArgsAndReturn(1);
	__wrap_memfault_packetizer_get_chunk_ExpectAnyArgsAndReturn(0);

	/* Expect the debug module to generate an event with accompanied Memfault metric data. */
	__wrap__event_submit_Stub(&validate_debug_data_ready_evt);

	struct data_module_event *data_module_event = new_data_module_event();

	data_module_event->type = DATA_EVT_DATA_SEND;

	TEST_ASSERT_EQUAL(0, DEBUG_MODULE_EVT_HANDLER((struct event_header *)data_module_event));

	k_free(data_module_event);
}

/* Test that no Memfault SDK specific APIs are called on GPS module events
 * that should not be handled.
 */
void test_memfault_unhandled_event(void)
{
	resetTest();
	setup_debug_module_in_init_state();

	/* Expect no memfault APIs to be called on GPS_EVT_ACTIVE */

	struct gps_module_event *gps_module_event = new_gps_module_event();

	gps_module_event->type = GPS_EVT_ACTIVE;
	TEST_ASSERT_EQUAL(0, DEBUG_MODULE_EVT_HANDLER((struct event_header *)gps_module_event));

	gps_module_event->type = GPS_EVT_INACTIVE;
	TEST_ASSERT_EQUAL(0, DEBUG_MODULE_EVT_HANDLER((struct event_header *)gps_module_event));

	gps_module_event->type = GPS_EVT_SHUTDOWN_READY;
	TEST_ASSERT_EQUAL(0, DEBUG_MODULE_EVT_HANDLER((struct event_header *)gps_module_event));

	gps_module_event->type = GPS_EVT_AGPS_NEEDED;
	TEST_ASSERT_EQUAL(0, DEBUG_MODULE_EVT_HANDLER((struct event_header *)gps_module_event));

	gps_module_event->type = GPS_EVT_ERROR_CODE;
	TEST_ASSERT_EQUAL(0, DEBUG_MODULE_EVT_HANDLER((struct event_header *)gps_module_event));

	k_free(gps_module_event);
}

/* Test whether the correct Memfault software watchdog APIs are called on callbacks from the
 * application watchdog library.
 */
void test_memfault_software_watchdog_trigger_on_callback(void)
{
	resetTest();
	setup_debug_module_in_init_state();

	__wrap_memfault_software_watchdog_enable_ExpectAndReturn(0);

	/* Expect a software watchdog timer 5 seconds lower than the passed in value to be used. */
	__wrap_memfault_software_watchdog_update_timeout_ExpectAndReturn(55000, 0);
	__wrap_memfault_software_watchdog_feed_ExpectAndReturn(0);

	struct watchdog_evt evt = {
		.type = WATCHDOG_EVT_START
	};

	debug_module_watchdog_callback(&evt);

	memset(&evt, 0, sizeof(struct watchdog_evt));

	evt.type = WATCHDOG_EVT_TIMEOUT_INSTALLED;
	evt.timeout = 60000;

	debug_module_watchdog_callback(&evt);

	memset(&evt, 0, sizeof(struct watchdog_evt));

	evt.type = WATCHDOG_EVT_FEED;
}

void main(void)
{
	(void)unity_main();
}
