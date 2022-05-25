/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <unity.h>
#include <stdbool.h>
#include <stdlib.h>
#include <mock_modules_common.h>
#include <mock_app_event_manager.h>
#include <mock_app_event_manager_priv.h>
#include <mock_watchdog_app.h>
#include <memfault/metrics/mock_metrics.h>
#include <memfault/core/mock_data_packetizer.h>
#include <memfault/ports/mock_watchdog.h>
#include <memfault/panics/mock_coredump.h>

#include "app_module_event.h"
#include "gnss_module_event.h"
#include "debug_module_event.h"
#include "data_module_event.h"

extern struct event_listener __event_listener_debug_module;

/* The addresses of the following structures will be returned when the app_event_manager_alloc()
 * function is called.
 */
static struct app_module_event app_module_event_memory;
static struct data_module_event data_module_event_memory;
static struct gnss_module_event gnss_module_event_memory;
static struct debug_module_event debug_module_event_memory;

#define DEBUG_MODULE_EVT_HANDLER(aeh) __event_listener_debug_module.notification(aeh)

/* Copy of the application specific Memfault metrics defined in
 * configuration/memfault/memfault_metrics_heartbeat_config.def
 */
const char * const g_memfault_metrics_id_GnssTimeToFix;
const char * const g_memfault_metrics_id_GnssSatellitesTracked;
const char * const g_memfault_metrics_id_GnssTimeoutSearchTime;

/* Dummy structs to please linker. The APP_EVENT_SUBSCRIBE macros in debug_module.c
 * depend on these to exist. But since we are unit testing, we dont need
 * these subscriptions and hence these structs can remain uninitialized.
 */
struct event_type __event_type_gnss_module_event;
struct event_type __event_type_debug_module_event;
struct event_type __event_type_app_module_event;
struct event_type __event_type_data_module_event;
struct event_type __event_type_cloud_module_event;
struct event_type __event_type_modem_module_event;
struct event_type __event_type_sensor_module_event;
struct event_type __event_type_ui_module_event;
struct event_type __event_type_util_module_event;

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
	mock_app_event_manager_Init();
}

void tearDown(void)
{
	mock_watchdog_app_Verify();
	mock_modules_common_Verify();
	mock_app_event_manager_Verify();
}

static void latch_watchdog_callback(watchdog_evt_handler_t handler, int no_of_calls)
{
	debug_module_watchdog_callback = handler;
}

static void validate_debug_data_ready_evt(struct app_event_header *aeh, int no_of_calls)
{
	struct debug_module_event *event = cast_debug_module_event(aeh);

	TEST_ASSERT_EQUAL(DEBUG_EVT_MEMFAULT_DATA_READY, event->type);

	/* Free payload received in the DEBUG_EVT_MEMFAULT_DATA_READY event. */
	k_free(event->data.memfault.buf);
}

/* Stub used to verify parameters passed into module_start(). */
static int module_start_stub(struct module_data *module, int num_calls)
{
	TEST_ASSERT_EQUAL_STRING("debug", module->name);
	TEST_ASSERT_NULL(module->msg_q);
	TEST_ASSERT_FALSE(module->supports_shutdown);

	return 0;
}

void setup_debug_module_in_init_state(void)
{
	__wrap_app_event_manager_alloc_ExpectAnyArgsAndReturn(&app_module_event_memory);
	__wrap_app_event_manager_free_ExpectAnyArgs();
	struct app_module_event *app_module_event = new_app_module_event();

	app_module_event->type = APP_EVT_START;

	__wrap_watchdog_register_handler_ExpectAnyArgs();
	__wrap_watchdog_register_handler_AddCallback(&latch_watchdog_callback);
	__wrap_module_start_Stub(&module_start_stub);

	TEST_ASSERT_EQUAL(0, DEBUG_MODULE_EVT_HANDLER(
		(struct app_event_header *)app_module_event));
	app_event_manager_free(app_module_event);
}

/* Test whether the correct Memfault metrics are set upon a GNSS fix. */
void test_memfault_trigger_metric_sampling_on_gnss_fix(void)
{
	setup_debug_module_in_init_state();

	__wrap_memfault_metrics_heartbeat_set_unsigned_ExpectAndReturn(
						MEMFAULT_METRICS_KEY(GnssTimeToFix),
						60000,
						0);
	__wrap_memfault_metrics_heartbeat_set_unsigned_ExpectAndReturn(
						MEMFAULT_METRICS_KEY(GnssSatellitesTracked),
						4,
						0);
	__wrap_memfault_metrics_heartbeat_debug_trigger_Expect();

	__wrap_app_event_manager_alloc_ExpectAnyArgsAndReturn(&gnss_module_event_memory);
	__wrap_app_event_manager_free_ExpectAnyArgs();
	struct gnss_module_event *gnss_module_event = new_gnss_module_event();

	gnss_module_event->type = GNSS_EVT_DATA_READY;
	gnss_module_event->data.gnss.satellites_tracked = 4;
	gnss_module_event->data.gnss.search_time = 60000;

	TEST_ASSERT_EQUAL(0, DEBUG_MODULE_EVT_HANDLER(
		(struct app_event_header *)gnss_module_event));
	app_event_manager_free(gnss_module_event);
}

/* Test whether the correct Memfault metrics are set upon a GNSS timeout. */
void test_memfault_trigger_metric_sampling_on_gnss_timeout(void)
{
	resetTest();
	setup_debug_module_in_init_state();

	/* Update this function to expect the search time and number of satellites. */
	__wrap_memfault_metrics_heartbeat_set_unsigned_ExpectAndReturn(
						MEMFAULT_METRICS_KEY(GnssTimeoutSearchTime),
						30000,
						0);
	__wrap_memfault_metrics_heartbeat_set_unsigned_ExpectAndReturn(
						MEMFAULT_METRICS_KEY(GnssSatellitesTracked),
						2,
						0);
	__wrap_memfault_metrics_heartbeat_debug_trigger_Ignore();

	__wrap_app_event_manager_alloc_ExpectAnyArgsAndReturn(&gnss_module_event_memory);
	__wrap_app_event_manager_free_ExpectAnyArgs();
	struct gnss_module_event *gnss_module_event = new_gnss_module_event();

	gnss_module_event->type = GNSS_EVT_TIMEOUT;
	gnss_module_event->data.gnss.satellites_tracked = 2;
	gnss_module_event->data.gnss.search_time = 30000;

	TEST_ASSERT_EQUAL(0, DEBUG_MODULE_EVT_HANDLER(
		(struct app_event_header *)gnss_module_event));
	app_event_manager_free(gnss_module_event);
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

	__wrap_app_event_manager_alloc_ExpectAnyArgsAndReturn(&data_module_event_memory);
	__wrap_app_event_manager_free_ExpectAnyArgs();
	struct data_module_event *data_module_event = new_data_module_event();

	data_module_event->type = DATA_EVT_DATA_SEND;

	__wrap_app_event_manager_alloc_IgnoreAndReturn(&debug_module_event_memory);
	TEST_ASSERT_EQUAL(0, DEBUG_MODULE_EVT_HANDLER(
		(struct app_event_header *)data_module_event));
	app_event_manager_free(data_module_event);

	/* Add a minor sleep to allow execution of the internal memfault transport thread in the
	 * debug module to finish before the rest runner.
	 */
	k_sleep(K_SECONDS(1));
}

/* Test that no Memfault SDK specific APIs are called on GNSS module events
 * that should not be handled.
 */
void test_memfault_unhandled_event(void)
{
	resetTest();
	setup_debug_module_in_init_state();

	/* Expect no memfault APIs to be called on GNSS_EVT_ACTIVE */

	__wrap_app_event_manager_alloc_ExpectAnyArgsAndReturn(&gnss_module_event_memory);
	__wrap_app_event_manager_free_ExpectAnyArgs();
	struct gnss_module_event *gnss_module_event = new_gnss_module_event();

	gnss_module_event->type = GNSS_EVT_ACTIVE;
	TEST_ASSERT_EQUAL(0, DEBUG_MODULE_EVT_HANDLER(
		(struct app_event_header *)gnss_module_event));

	gnss_module_event->type = GNSS_EVT_INACTIVE;
	TEST_ASSERT_EQUAL(0, DEBUG_MODULE_EVT_HANDLER(
		(struct app_event_header *)gnss_module_event));

	gnss_module_event->type = GNSS_EVT_SHUTDOWN_READY;
	TEST_ASSERT_EQUAL(0, DEBUG_MODULE_EVT_HANDLER(
		(struct app_event_header *)gnss_module_event));

	gnss_module_event->type = GNSS_EVT_AGPS_NEEDED;
	TEST_ASSERT_EQUAL(0, DEBUG_MODULE_EVT_HANDLER(
		(struct app_event_header *)gnss_module_event));

	gnss_module_event->type = GNSS_EVT_ERROR_CODE;
	TEST_ASSERT_EQUAL(0, DEBUG_MODULE_EVT_HANDLER(
		(struct app_event_header *)gnss_module_event));
	app_event_manager_free(gnss_module_event);
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
