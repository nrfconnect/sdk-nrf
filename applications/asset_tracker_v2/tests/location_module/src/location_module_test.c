/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <unity.h>
#include <stdbool.h>
#include <stdlib.h>

#include "cmock_modules_common.h"
#include "cmock_app_event_manager.h"
#include "cmock_app_event_manager_priv.h"
#include "cmock_location.h"
#include "cmock_lte_lc.h"
#include "cmock_nrf_modem_gnss.h"

#include "app_module_event.h"
#include "location_module_event.h"
#include "data_module_event.h"
#include "modem_module_event.h"
#include "cloud_module_event.h"

extern struct event_listener __event_listener_location_module;

/* The addresses of the following structures will be returned when the app_event_manager_alloc()
 * function is called.
 */
static struct app_module_event app_module_event_memory;
static struct modem_module_event modem_module_event_memory;
static struct location_module_event location_module_event_memory;
static struct data_module_event data_module_event_memory;
static struct cloud_module_event cloud_module_event_memory;

#define LOCATION_MODULE_EVT_HANDLER(aeh) __event_listener_location_module.notification(aeh)

/* Macro used to submit module events of a specific type to the Location module. */
#define TEST_SEND_EVENT(_mod, _type, _event)							\
	__cmock_app_event_manager_alloc_ExpectAnyArgsAndReturn(&_mod##_module_event_memory);	\
	__cmock_app_event_manager_free_ExpectAnyArgs();						\
	_event = new_##_mod##_module_event();							\
	_event->type = _type;									\
	TEST_ASSERT_FALSE(LOCATION_MODULE_EVT_HANDLER(						\
		(struct app_event_header *)_event));					\
	app_event_manager_free(_event)

/* location_event_handler() is implemented in location module and we'll call it directly
 * to fake received location library events.
 */
extern void location_event_handler(const struct location_event_data *event_data);

#define LOCATION_MODULE_MAX_EVENTS 5

/* Counter for received location module events. */
static uint32_t location_module_event_count;
/* Number of expected location module events. */
static uint32_t expected_location_module_event_count;
/* Array for expected location module events. */
static struct location_module_event expected_location_module_events[LOCATION_MODULE_MAX_EVENTS];
/* Semaphore for waiting for events to be received. */
static K_SEM_DEFINE(location_module_event_sem, 0, 5);

/* Dummy functions and objects. */

/* The following function needs to be stubbed this way because Application Event Manager
 * uses heap to allocate memory for events.
 * The other alternative is to mock the k_malloc using CMock framework, but
 * that will pollute the test code and will be an overkill.
 */
void *k_malloc(size_t size)
{
	return malloc(size);
}

/* Dummy structs to please the linker. The APP_EVENT_SUBSCRIBE macros in location_module.c
 * depend on these to exist. But since we are unit testing, we dont need
 * these subscriptions and hence these structs can remain uninitialized.
 */
struct event_type __event_type_location_module_event;
struct event_type __event_type_app_module_event;
struct event_type __event_type_data_module_event;
struct event_type __event_type_util_module_event;
struct event_type __event_type_modem_module_event;
struct event_type __event_type_cloud_module_event;

/* Dummy functions and objects - End.  */

/* It is required to be added to each test. That is because unity's
 * main may return nonzero, while zephyr's main currently must
 * return 0 in all cases (other values are reserved).
 */
extern int unity_main(void);

void setUp(void)
{
	location_module_event_count = 0;
	expected_location_module_event_count = 0;
	memset(&expected_location_module_events, 0, sizeof(expected_location_module_events));
}

void tearDown(void)
{
	/* Wait until we've received all events. */
	for (int i = 0; i < expected_location_module_event_count; i++) {
		k_sem_take(&location_module_event_sem, K_SECONDS(1));
	}
	TEST_ASSERT_EQUAL(expected_location_module_event_count, location_module_event_count);
}

static void validate_location_module_evt(struct app_event_header *aeh, int no_of_calls)
{
	uint32_t index = location_module_event_count;
	struct location_module_event *event = cast_location_module_event(aeh);

	struct nrf_modem_gnss_agnss_data_frame *agps_req_exp =
		&expected_location_module_events[index].data.agps_request;

	/* Make sure we don't get more events than expected. */
	TEST_ASSERT_LESS_THAN(expected_location_module_event_count, location_module_event_count);

	TEST_ASSERT_EQUAL(expected_location_module_events[index].type, event->type);

	switch (event->type) {
	case LOCATION_MODULE_EVT_GNSS_DATA_READY:
		TEST_ASSERT_EQUAL(
			expected_location_module_events[index].data.location.pvt.latitude,
			event->data.location.pvt.latitude);
		TEST_ASSERT_EQUAL(
			expected_location_module_events[index].data.location.pvt.longitude,
			event->data.location.pvt.longitude);
		TEST_ASSERT_EQUAL(
			expected_location_module_events[index].data.location.pvt.altitude,
			event->data.location.pvt.altitude);
		TEST_ASSERT_EQUAL(
			expected_location_module_events[index].data.location.pvt.accuracy,
			event->data.location.pvt.accuracy);
		TEST_ASSERT_EQUAL(
			expected_location_module_events[index].data.location.pvt.speed,
			event->data.location.pvt.speed);
		TEST_ASSERT_EQUAL(
			expected_location_module_events[index].data.location.pvt.heading,
			event->data.location.pvt.heading);
		TEST_ASSERT_EQUAL(
			expected_location_module_events[index].data.location.satellites_tracked,
			event->data.location.satellites_tracked);

		/* Ignore search_time and timestamp verification as those values might change */
		break;
	case LOCATION_MODULE_EVT_CLOUD_LOCATION_DATA_READY: {
		struct location_module_cloud_location *expected_cloud_loc =
			&expected_location_module_events[index].data.cloud_location;
		struct location_module_cloud_location *received_cloud_loc =
			&event->data.cloud_location;

		TEST_ASSERT_EQUAL(
			expected_cloud_loc->neighbor_cells.cell_data.current_cell.mcc,
			received_cloud_loc->neighbor_cells.cell_data.current_cell.mcc);
		TEST_ASSERT_EQUAL(
			expected_cloud_loc->neighbor_cells.cell_data.current_cell.mnc,
			received_cloud_loc->neighbor_cells.cell_data.current_cell.mnc);
		TEST_ASSERT_EQUAL(
			expected_cloud_loc->neighbor_cells.cell_data.current_cell.id,
			received_cloud_loc->neighbor_cells.cell_data.current_cell.id);
		TEST_ASSERT_EQUAL(
			expected_cloud_loc->neighbor_cells.cell_data.current_cell.tac,
			received_cloud_loc->neighbor_cells.cell_data.current_cell.tac);
		TEST_ASSERT_EQUAL(
			expected_cloud_loc->neighbor_cells.cell_data.current_cell.earfcn,
			received_cloud_loc->neighbor_cells.cell_data.current_cell.earfcn);
		TEST_ASSERT_EQUAL(
			expected_cloud_loc->neighbor_cells.cell_data.current_cell.timing_advance,
			received_cloud_loc->neighbor_cells.cell_data.current_cell.timing_advance);
		TEST_ASSERT_EQUAL(
			expected_cloud_loc->neighbor_cells.cell_data.current_cell
				.timing_advance_meas_time,
			received_cloud_loc->neighbor_cells.cell_data.current_cell
				.timing_advance_meas_time);
		TEST_ASSERT_EQUAL(
			expected_cloud_loc->neighbor_cells.cell_data.current_cell.measurement_time,
			received_cloud_loc->neighbor_cells.cell_data.current_cell.measurement_time);
		TEST_ASSERT_EQUAL(
			expected_cloud_loc->neighbor_cells.cell_data.current_cell.phys_cell_id,
			received_cloud_loc->neighbor_cells.cell_data.current_cell.phys_cell_id);
		TEST_ASSERT_EQUAL(
			expected_cloud_loc->neighbor_cells.cell_data.current_cell.rsrp,
			received_cloud_loc->neighbor_cells.cell_data.current_cell.rsrp);
		TEST_ASSERT_EQUAL(
			expected_cloud_loc->neighbor_cells.cell_data.current_cell.rsrq,
			received_cloud_loc->neighbor_cells.cell_data.current_cell.rsrq);

		TEST_ASSERT_EQUAL(
			expected_cloud_loc->neighbor_cells.cell_data.ncells_count,
			received_cloud_loc->neighbor_cells.cell_data.ncells_count);
		for (int i = 0; i < received_cloud_loc->neighbor_cells.cell_data.ncells_count;
		     i++) {
			TEST_ASSERT_EQUAL(
				expected_cloud_loc->neighbor_cells.neighbor_cells[i].earfcn,
				received_cloud_loc->neighbor_cells.neighbor_cells[i].earfcn);
			TEST_ASSERT_EQUAL(
				expected_cloud_loc->neighbor_cells.neighbor_cells[i].time_diff,
				received_cloud_loc->neighbor_cells.neighbor_cells[i].time_diff);
			TEST_ASSERT_EQUAL(
				expected_cloud_loc->neighbor_cells.neighbor_cells[i].phys_cell_id,
				received_cloud_loc->neighbor_cells.neighbor_cells[i].phys_cell_id);
			TEST_ASSERT_EQUAL(
				expected_cloud_loc->neighbor_cells.neighbor_cells[i].rsrp,
				received_cloud_loc->neighbor_cells.neighbor_cells[i].rsrp);
			TEST_ASSERT_EQUAL(
				expected_cloud_loc->neighbor_cells.neighbor_cells[i].rsrq,
				received_cloud_loc->neighbor_cells.neighbor_cells[i].rsrq);
		}
		break;
	}
	case LOCATION_MODULE_EVT_SHUTDOWN_READY:
		TEST_FAIL();
		break;
	case LOCATION_MODULE_EVT_AGPS_NEEDED:
		TEST_ASSERT_EQUAL(
			agps_req_exp->system[0].sv_mask_ephe,
			event->data.agps_request.system[0].sv_mask_ephe);
		TEST_ASSERT_EQUAL(
			agps_req_exp->system[0].sv_mask_alm,
			event->data.agps_request.system[0].sv_mask_alm);
		TEST_ASSERT_EQUAL(
			agps_req_exp->data_flags,
			event->data.agps_request.data_flags);
		break;
	case LOCATION_MODULE_EVT_PGPS_NEEDED:
		TEST_FAIL();
		break;
	case LOCATION_MODULE_EVT_ERROR_CODE:
		TEST_ASSERT_EQUAL(
			expected_location_module_events[index].data.err,
			event->data.err);
		break;

	case LOCATION_MODULE_EVT_DATA_NOT_READY:
	case LOCATION_MODULE_EVT_TIMEOUT:
	case LOCATION_MODULE_EVT_ACTIVE:
	case LOCATION_MODULE_EVT_INACTIVE:
	default:
		break;
	}

	location_module_event_count++;

	/* Signal that an event was received. */
	k_sem_give(&location_module_event_sem);
}

/* Stub used to verify parameters passed into module_start(). */
static int module_start_stub(struct module_data *module, int num_calls)
{
	TEST_ASSERT_EQUAL_STRING("location", module->name);
	TEST_ASSERT_NULL(module->msg_q);
	TEST_ASSERT_TRUE(module->supports_shutdown);

	return 0;
}

static void setup_location_module_in_init_state(void)
{
	struct app_module_event *app_module_event;

	__cmock_module_start_Stub(&module_start_stub);
	__cmock_app_event_manager_alloc_ExpectAnyArgsAndReturn(&location_module_event_memory);
	TEST_SEND_EVENT(app, APP_EVT_START, app_module_event);
}

static void setup_location_module_in_running_state(void)
{
	bool ret;
	struct modem_module_event *modem_module_event;

	setup_location_module_in_init_state();

	/* Send MODEM_EVT_INITIALIZED. */
	__cmock_location_init_ExpectAndReturn(&location_event_handler, 0);
	TEST_SEND_EVENT(modem, MODEM_EVT_INITIALIZED, modem_module_event);

	/* Send DATA_EVT_CONFIG_INIT. */
	__cmock_app_event_manager_alloc_ExpectAnyArgsAndReturn(&data_module_event_memory);
	__cmock_app_event_manager_free_ExpectAnyArgs();
	struct data_module_event *data_module_event = new_data_module_event();

	data_module_event->type = DATA_EVT_CONFIG_INIT;
	data_module_event->data.cfg.location_timeout = 30;
	data_module_event->data.cfg.no_data.gnss = false;
	data_module_event->data.cfg.no_data.neighbor_cell = false;

	ret = LOCATION_MODULE_EVT_HANDLER((struct app_event_header *)data_module_event);
	app_event_manager_free(data_module_event);
	TEST_ASSERT_EQUAL(0, ret);
}

/* Set location module into state where location_request() has been called.
 */
static void setup_location_module_in_active_state(void)
{
	bool ret;
	struct location_module_event *location_module_event;

	/* Pre-condition. */
	setup_location_module_in_running_state();

	/* Set up expectations.  */
	/* Set callback to validate location module events. */
	__cmock__event_submit_Stub(&validate_location_module_evt);

	__cmock_location_config_defaults_set_Expect(NULL, 0, NULL);
	__cmock_location_config_defaults_set_IgnoreArg_config();

	/* Location configuration modified by location module for location request. */
	struct location_config config_location_request = {
		.methods_count = 2,
		.interval = 0,
		.timeout = 300000,
		.mode = LOCATION_REQ_MODE_FALLBACK,
		.methods = {
			{.method = LOCATION_METHOD_GNSS, .gnss.timeout = 90000},
			{.method = LOCATION_METHOD_CELLULAR, .cellular.timeout = 11000}
		}
	};
	__cmock_location_request_ExpectAndReturn(&config_location_request, 0);
	__cmock_location_request_IgnoreArg_config(); /* TODO: REMOVE */

	/* Send APP_EVT_DATA_GET with APP_DATA_LOCATION. */
	__cmock_app_event_manager_alloc_ExpectAnyArgsAndReturn(&app_module_event_memory);
	__cmock_app_event_manager_free_ExpectAnyArgs();
	struct app_module_event *app_module_event = new_app_module_event();

	app_module_event->type = APP_EVT_DATA_GET;
	app_module_event->count = 1;
	app_module_event->data_list[0] = APP_DATA_LOCATION;

	ret = LOCATION_MODULE_EVT_HANDLER((struct app_event_header *)app_module_event);
	app_event_manager_free(app_module_event);
	TEST_ASSERT_EQUAL(0, ret);

	/* Send LOCATION_MODULE_EVT_ACTIVE */
	__cmock_app_event_manager_alloc_IgnoreAndReturn(&location_module_event_memory);
	__cmock_app_event_manager_free_ExpectAnyArgs();
	location_module_event = new_location_module_event();

	location_module_event->type = LOCATION_MODULE_EVT_ACTIVE;
	ret = LOCATION_MODULE_EVT_HANDLER((struct app_event_header *)location_module_event);
	app_event_manager_free(location_module_event);
	TEST_ASSERT_EQUAL(0, ret);
}

/* Test whether the location module generates an event
 * - for A-GPS data on receiving an A-GPS data request from the location library.
 * - the location module generates an event with GNSS position on receiving a
 *   location from the location library.
 */
void test_location_gnss_with_agps(void)
{
	struct location_module_event *location_module_event;

	/* Set expected location module events. */
	expected_location_module_event_count = 4;
	expected_location_module_events[0].type = LOCATION_MODULE_EVT_ACTIVE;

	expected_location_module_events[1].type = LOCATION_MODULE_EVT_AGPS_NEEDED;
	expected_location_module_events[1].data.agps_request.system[0].sv_mask_ephe = 0xabbaabba,
	expected_location_module_events[1].data.agps_request.system[0].sv_mask_alm = 0xdeaddead,
	expected_location_module_events[1].data.agps_request.data_flags =
		NRF_MODEM_GNSS_AGNSS_GPS_UTC_REQUEST |
		NRF_MODEM_GNSS_AGNSS_NEQUICK_REQUEST |
		NRF_MODEM_GNSS_AGNSS_POSITION_REQUEST;

	expected_location_module_events[2].type = LOCATION_MODULE_EVT_GNSS_DATA_READY;
	expected_location_module_events[2].data.location.pvt.latitude = 34.087;
	expected_location_module_events[2].data.location.pvt.longitude = 10.45;
	expected_location_module_events[2].data.location.pvt.altitude = 35;
	expected_location_module_events[2].data.location.pvt.accuracy = 7;
	expected_location_module_events[2].data.location.pvt.speed = 37;
	expected_location_module_events[2].data.location.pvt.heading = 189;
	expected_location_module_events[2].data.location.satellites_tracked = 8;

	expected_location_module_events[3].type = LOCATION_MODULE_EVT_INACTIVE;

	/* Set location module into state where location_request() has been called. */
	setup_location_module_in_active_state();

	/* Location request is responded with location library events. */
	struct location_event_data event_data_agps = {
		.id = LOCATION_EVT_GNSS_ASSISTANCE_REQUEST,
		.agps_request.system[0].sv_mask_ephe = 0xabbaabba,
		.agps_request.system[0].sv_mask_alm = 0xdeaddead,
		.agps_request.data_flags = NRF_MODEM_GNSS_AGNSS_GPS_UTC_REQUEST |
					   NRF_MODEM_GNSS_AGNSS_NEQUICK_REQUEST |
					   NRF_MODEM_GNSS_AGNSS_POSITION_REQUEST
	};
	location_event_handler(&event_data_agps);

	struct location_event_data event_data_gnss_location = {
		.id = LOCATION_EVT_LOCATION,
		.method = LOCATION_METHOD_GNSS,

		.location.latitude = 34.087,
		.location.details.gnss.pvt_data.latitude = 34.087,
		.location.longitude = 10.45,
		.location.details.gnss.pvt_data.longitude = 10.45,
		.location.accuracy = 7,
		.location.details.gnss.pvt_data.accuracy = 7,

		.location.details.gnss.pvt_data.altitude = 35,
		.location.details.gnss.pvt_data.speed = 37,
		.location.details.gnss.pvt_data.heading = 189,
		.location.details.gnss.satellites_tracked = 8
	};
	location_event_handler(&event_data_gnss_location);

	TEST_SEND_EVENT(location, LOCATION_MODULE_EVT_INACTIVE, location_module_event);
}

/* Test whether the location module generates an event for neighbor cell measurements
 * on receiving cellular request from location library.
 */
void test_location_cellular(void)
{
	struct cloud_module_event *cloud_module_event;
	struct location_module_event *location_module_event;

	struct lte_lc_ncell neighbor_cells[2] = {
		{2300, 0, 8, 60, 29},
		{2400, 184, 11, 55, 26}
	};

	struct lte_lc_cells_info lte_cells = {
		.current_cell = {
			262, 95, 0x00011B07, 0x00B7, 2300, 10512, 9034,
			150344527, 7, 63, 31 },
		.ncells_count = 2,
		.neighbor_cells = neighbor_cells
	};

	/* Set expected location module events. */
	expected_location_module_event_count = 3;
	expected_location_module_events[0].type = LOCATION_MODULE_EVT_ACTIVE;

	expected_location_module_events[1].type = LOCATION_MODULE_EVT_CLOUD_LOCATION_DATA_READY;
	expected_location_module_events[1].data.cloud_location.neighbor_cells.cell_data = lte_cells;
	expected_location_module_events[1].data.cloud_location.neighbor_cells.neighbor_cells[0] =
		neighbor_cells[0];
	expected_location_module_events[1].data.cloud_location.neighbor_cells.neighbor_cells[1] =
		neighbor_cells[1];
	expected_location_module_events[1].data.cloud_location.neighbor_cells.timestamp = 123456789;

	expected_location_module_events[2].type = LOCATION_MODULE_EVT_INACTIVE;

	/* Set location module into state where location_request() has been called. */
	setup_location_module_in_active_state();

	/* Location request is responded with location library events. */
	struct location_event_data event_data_cellular = {
		.id = LOCATION_EVT_CLOUD_LOCATION_EXT_REQUEST,
		.cloud_location_request.cell_data = &lte_cells
	};
	location_event_handler(&event_data_cellular);

	/* Location module indicates that it has handled neighbor cells
	 * but the location is undefined.
	 */
	__cmock_location_cloud_location_ext_result_set_Expect(LOCATION_EXT_RESULT_UNKNOWN, NULL);

	__cmock_app_event_manager_alloc_ExpectAnyArgsAndReturn(&location_module_event_memory);
	TEST_SEND_EVENT(cloud, CLOUD_EVT_CLOUD_LOCATION_UNKNOWN, cloud_module_event);

	/* Location library responds with and undefined status. */
	struct location_event_data event_data_undefined = {
		.id = LOCATION_EVT_RESULT_UNKNOWN,
	};
	location_event_handler(&event_data_undefined);

	TEST_SEND_EVENT(location, LOCATION_MODULE_EVT_INACTIVE, location_module_event);
}

/* Test timeout for location request.
 */
void test_location_fail_timeout(void)
{
	struct location_module_event *location_module_event;

	/* Set expected location module events. */
	expected_location_module_event_count = 3;
	expected_location_module_events[0].type = LOCATION_MODULE_EVT_ACTIVE;
	expected_location_module_events[1].type = LOCATION_MODULE_EVT_TIMEOUT;
	expected_location_module_events[2].type = LOCATION_MODULE_EVT_INACTIVE;

	/* Set location module into state where location_request() has been called. */
	setup_location_module_in_active_state();

	/* Location request is responded with location library events. */
	struct location_event_data event_data = {
		.id = LOCATION_EVT_TIMEOUT,
	};
	location_event_handler(&event_data);

	TEST_SEND_EVENT(location, LOCATION_MODULE_EVT_INACTIVE, location_module_event);
}

/* Test error while processing for location request.
 */
void test_location_fail_error(void)
{
	struct location_module_event *location_module_event;

	/* Set expected location module events. */
	expected_location_module_event_count = 3;
	expected_location_module_events[0].type = LOCATION_MODULE_EVT_ACTIVE;
	expected_location_module_events[1].type = LOCATION_MODULE_EVT_DATA_NOT_READY;
	expected_location_module_events[2].type = LOCATION_MODULE_EVT_INACTIVE;

	/* Set location module into state where location_request() has been called. */
	setup_location_module_in_active_state();

	/* Location request is responded with location library events. */
	struct location_event_data event_data = {
		.id = LOCATION_EVT_ERROR,
	};
	location_event_handler(&event_data);

	TEST_SEND_EVENT(location, LOCATION_MODULE_EVT_INACTIVE, location_module_event);
}

/* Test that an error event is sent when location library initialization fails.
 */
void test_location_fail_init(void)
{
	struct modem_module_event *modem_module_event;

	setup_location_module_in_init_state();

	__cmock__event_submit_Stub(&validate_location_module_evt);
	__cmock_location_init_ExpectAndReturn(&location_event_handler, -EINVAL);

	/* Set expected GNSS module events. */
	expected_location_module_event_count = 1;
	expected_location_module_events[0].type = LOCATION_MODULE_EVT_ERROR_CODE;
	expected_location_module_events[0].data.err = -1;

	TEST_SEND_EVENT(modem, MODEM_EVT_INITIALIZED, modem_module_event);
}

int main(void)
{
	(void)unity_main();
	return 0;
}
