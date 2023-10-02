/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <unity.h>
#include <stdbool.h>
#include <stdlib.h>
#include <memfault/metrics/cmock_metrics.h>
#include <memfault/core/cmock_data_packetizer.h>
#include <memfault/ports/cmock_watchdog.h>
#include <memfault/panics/cmock_coredump.h>

#include "cmock_modules_common.h"
#include "cmock_app_event_manager.h"
#include "cmock_app_event_manager_priv.h"
#include "cmock_watchdog_app.h"

#include "app_module_event.h"
#include "location_module_event.h"
#include "debug_module_event.h"
#include "data_module_event.h"

extern struct event_listener __event_listener_debug_module;

/* The addresses of the following structures will be returned when the app_event_manager_alloc()
 * function is called.
 */
static struct app_module_event app_module_event_memory;
static struct data_module_event data_module_event_memory;
static struct location_module_event location_module_event_memory;
static struct debug_module_event debug_module_event_memory;

#define DEBUG_MODULE_EVT_HANDLER(aeh) __event_listener_debug_module.notification(aeh)

/* Dummy structs to please linker. The APP_EVENT_SUBSCRIBE macros in debug_module.c
 * depend on these to exist. But since we are unit testing, we dont need
 * these subscriptions and hence these structs can remain uninitialized.
 */
struct event_type __event_type_location_module_event;
struct event_type __event_type_debug_module_event;
struct event_type __event_type_app_module_event;
struct event_type __event_type_data_module_event;
struct event_type __event_type_cloud_module_event;
struct event_type __event_type_modem_module_event;
struct event_type __event_type_sensor_module_event;
struct event_type __event_type_ui_module_event;
struct event_type __event_type_util_module_event;

/* It is required to be added to each test. That is because unity's
 * main may return nonzero, while zephyr's main currently must
 * return 0 in all cases (other values are reserved).
 */
extern int unity_main(void);

/* Local reference of the internal application watchdog callback in debug module. Used to
 * infuse callback events directly from the test runner.
 */
watchdog_evt_handler_t debug_module_watchdog_callback;

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
	__cmock_app_event_manager_alloc_ExpectAnyArgsAndReturn(&app_module_event_memory);
	__cmock_app_event_manager_free_ExpectAnyArgs();
	struct app_module_event *app_module_event = new_app_module_event();

	app_module_event->type = APP_EVT_START;

	__cmock_watchdog_register_handler_ExpectAnyArgs();
	__cmock_watchdog_register_handler_AddCallback(&latch_watchdog_callback);
	__cmock_module_start_Stub(&module_start_stub);

#if defined(CONFIG_BOARD_NATIVE_POSIX)
	__cmock_app_event_manager_alloc_ExpectAnyArgsAndReturn(&debug_module_event_memory);
	__cmock__event_submit_ExpectAnyArgs();
	__cmock_app_event_manager_alloc_ExpectAnyArgsAndReturn(&debug_module_event_memory);
	__cmock__event_submit_ExpectAnyArgs();
#endif /* defined(CONFIG_BOARD_NATIVE_POSIX) */

	TEST_ASSERT_EQUAL(0, DEBUG_MODULE_EVT_HANDLER(
		(struct app_event_header *)app_module_event));
	app_event_manager_free(app_module_event);
}

/* Test whether the correct Memfault metrics are set upon a GNSS fix. */
void test_memfault_trigger_metric_sampling_on_gnss_fix(void)
{
	setup_debug_module_in_init_state();

	__cmock_memfault_metrics_heartbeat_set_unsigned_ExpectAndReturn(
						MEMFAULT_METRICS_KEY(GnssTimeToFix),
						60000,
						0);
	__cmock_memfault_metrics_heartbeat_set_unsigned_ExpectAndReturn(
						MEMFAULT_METRICS_KEY(GnssSatellitesTracked),
						4,
						0);
	__cmock_memfault_metrics_heartbeat_debug_trigger_Expect();

	__cmock_app_event_manager_alloc_ExpectAnyArgsAndReturn(&location_module_event_memory);
	__cmock_app_event_manager_free_ExpectAnyArgs();
	struct location_module_event *location_module_event = new_location_module_event();

	location_module_event->type = LOCATION_MODULE_EVT_GNSS_DATA_READY;
	location_module_event->data.location.satellites_tracked = 4;
	location_module_event->data.location.search_time = 60000;

	TEST_ASSERT_EQUAL(0, DEBUG_MODULE_EVT_HANDLER(
		(struct app_event_header *)location_module_event));
	app_event_manager_free(location_module_event);
}

/* Test whether the correct Memfault metrics are set upon a GNSS timeout. */
void test_memfault_trigger_metric_sampling_on_location_timeout(void)
{
	resetTest();
	setup_debug_module_in_init_state();

	/* Update this function to expect the search time and number of satellites. */
	__cmock_memfault_metrics_heartbeat_set_unsigned_ExpectAndReturn(
						MEMFAULT_METRICS_KEY(LocationTimeoutSearchTime),
						30000,
						0);
	__cmock_memfault_metrics_heartbeat_set_unsigned_ExpectAndReturn(
						MEMFAULT_METRICS_KEY(GnssSatellitesTracked),
						2,
						0);
	__cmock_memfault_metrics_heartbeat_debug_trigger_Ignore();

	__cmock_app_event_manager_alloc_ExpectAnyArgsAndReturn(&location_module_event_memory);
	__cmock_app_event_manager_free_ExpectAnyArgs();
	struct location_module_event *location_module_event = new_location_module_event();

	location_module_event->type = LOCATION_MODULE_EVT_TIMEOUT;
	location_module_event->data.location.satellites_tracked = 2;
	location_module_event->data.location.search_time = 30000;

	TEST_ASSERT_EQUAL(0, DEBUG_MODULE_EVT_HANDLER(
		(struct app_event_header *)location_module_event));
	app_event_manager_free(location_module_event);
}

/* Test that the debug module is able to submit Memfault data externally through events
 * of type DEBUG_EVT_MEMFAULT_DATA_READY carrying chunks of data.
 */
void test_memfault_trigger_data_send(void)
{
	resetTest();
	setup_debug_module_in_init_state();

	__cmock__event_submit_ExpectAnyArgs();

	__cmock_memfault_packetizer_data_available_ExpectAndReturn(1);
	__cmock_memfault_packetizer_get_chunk_ExpectAnyArgsAndReturn(1);
	__cmock_memfault_packetizer_get_chunk_ExpectAnyArgsAndReturn(0);

	/* Expect the debug module to generate an event with accompanied Memfault metric data. */
	__cmock__event_submit_Stub(&validate_debug_data_ready_evt);

	__cmock_app_event_manager_alloc_ExpectAnyArgsAndReturn(&data_module_event_memory);
	__cmock_app_event_manager_free_ExpectAnyArgs();
	struct data_module_event *data_module_event = new_data_module_event();

	data_module_event->type = DATA_EVT_DATA_SEND;

	__cmock_app_event_manager_alloc_IgnoreAndReturn(&debug_module_event_memory);
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

	/* Expect no memfault APIs to be called on LOCATION_MODULE_EVT_ACTIVE */

	__cmock_app_event_manager_alloc_ExpectAnyArgsAndReturn(&location_module_event_memory);
	__cmock_app_event_manager_free_ExpectAnyArgs();
	struct location_module_event *location_module_event = new_location_module_event();

	location_module_event->type = LOCATION_MODULE_EVT_ACTIVE;
	TEST_ASSERT_EQUAL(0, DEBUG_MODULE_EVT_HANDLER(
		(struct app_event_header *)location_module_event));

	location_module_event->type = LOCATION_MODULE_EVT_INACTIVE;
	TEST_ASSERT_EQUAL(0, DEBUG_MODULE_EVT_HANDLER(
		(struct app_event_header *)location_module_event));

	location_module_event->type = LOCATION_MODULE_EVT_SHUTDOWN_READY;
	TEST_ASSERT_EQUAL(0, DEBUG_MODULE_EVT_HANDLER(
		(struct app_event_header *)location_module_event));

	location_module_event->type = LOCATION_MODULE_EVT_AGNSS_NEEDED;
	TEST_ASSERT_EQUAL(0, DEBUG_MODULE_EVT_HANDLER(
		(struct app_event_header *)location_module_event));

	location_module_event->type = LOCATION_MODULE_EVT_ERROR_CODE;
	TEST_ASSERT_EQUAL(0, DEBUG_MODULE_EVT_HANDLER(
		(struct app_event_header *)location_module_event));
	app_event_manager_free(location_module_event);
}

/* Test whether the correct Memfault software watchdog APIs are called on callbacks from the
 * application watchdog library.
 */
void test_memfault_software_watchdog_trigger_on_callback(void)
{
	resetTest();
	setup_debug_module_in_init_state();

	__cmock_memfault_software_watchdog_enable_ExpectAndReturn(0);

	/* Expect a software watchdog timer 5 seconds lower than the passed in value to be used. */
	__cmock_memfault_software_watchdog_update_timeout_ExpectAndReturn(55000, 0);
	__cmock_memfault_software_watchdog_feed_ExpectAndReturn(0);

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

int main(void)
{
	(void)unity_main();
	return 0;
}
