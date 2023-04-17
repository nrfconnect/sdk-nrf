/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <unity.h>
#include <stdbool.h>
#include <stdlib.h>

#include "cmock_modules_common.h"
#include "cmock_app_event_manager_priv.h"
#include "cmock_app_event_manager.h"
#include "cmock_dk_buttons_and_leds.h"

#include "events/app_module_event.h"
#include "events/data_module_event.h"
#include "events/ui_module_event.h"
#include "events/location_module_event.h"
#include "events/modem_module_event.h"
#include "events/cloud_module_event.h"
#include "events/util_module_event.h"

#include "src/vars_internal.h"

extern struct event_listener __event_listener_ui_module;

/* The addresses of the following structures will be returned when the app_event_manager_alloc()
 * function is called.
 */
static struct app_module_event app_module_event_memory;
static struct modem_module_event modem_module_event_memory;
static struct cloud_module_event cloud_module_event_memory;
static struct util_module_event util_module_event_memory;
static struct ui_module_event ui_module_event_memory;
static struct data_module_event data_module_event_memory;
static struct location_module_event location_module_event_memory;

#define UI_MODULE_EVT_HANDLER(aeh) __event_listener_ui_module.notification(aeh)

/* Macro used to submit module events of a specific type to the UI module. */
#define TEST_SEND_EVENT(_mod, _type, _event)							\
	__cmock_app_event_manager_alloc_ExpectAnyArgsAndReturn(&_mod##_module_event_memory);	\
	__cmock_app_event_manager_free_ExpectAnyArgs();						\
	_event = new_##_mod##_module_event();							\
	_event->type = _type;									\
	TEST_ASSERT_FALSE(UI_MODULE_EVT_HANDLER(						\
		(struct app_event_header *)_event));					\
	app_event_manager_free(_event)

/* Dummy structs to please linker. The APP_EVENT_SUBSCRIBE macros in ui_module.c
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

void setUp(void)
{
	/* Reset internal module states. */
	state = 0;
	sub_state = 0;
	sub_sub_state = 0;

	/* Clear internal LED pattern transition list. */
	transition_list_clear();
}

/* Stub used to verify parameters passed into module_start(). */
static int module_start_stub(struct module_data *module, int num_calls)
{
	TEST_ASSERT_EQUAL_STRING("ui", module->name);
	TEST_ASSERT_NULL(module->msg_q);
	TEST_ASSERT_TRUE(module->supports_shutdown);

	return 0;
}

/* Handler that validates events sent from the UI module. */
static void validate_ui_evt(struct app_event_header *aeh, int no_of_calls)
{
	struct ui_module_event *event = cast_ui_module_event(aeh);

	TEST_ASSERT_EQUAL(UI_EVT_SHUTDOWN_READY, event->type);
}

/* Function used to verify the internal state of the UI module. */
static void state_verify(enum state_type state_new, enum sub_state_type sub_state_new,
			 enum sub_sub_state_type sub_sub_state_new)
{
	TEST_ASSERT_EQUAL(state_new, state);
	TEST_ASSERT_EQUAL(sub_state_new, sub_state);
	TEST_ASSERT_EQUAL(sub_sub_state_new, sub_sub_state);
}

/**
 * @brief Function that validates the next node in the led pattern_transition_list.
 *	  If the passed in led_state and duration_sec variables are 0 the function returns
 *	  true if no more nodes are present in the list.
 *
 * @param led_state LED state that will be verified against.
 * @param duration_sec Duration of the LED state.
 *
 * @return true if the LED state and duration is associated with the next node.
 * @return false if the LED state and duration is not associated with the next node.
 *
 */
static bool list_node_verify_next(enum led_state led_state, int16_t duration_sec)
{
	struct led_pattern *led_pattern = NULL;
	sys_snode_t *node = sys_slist_get(&pattern_transition_list);

	if ((node == NULL) && (led_state == 0) && (duration_sec == 0)) {
		return true;
	} else if (node == NULL) {
		return false;
	}

	led_pattern = CONTAINER_OF(node, struct led_pattern, header);

	/* Verify the expected LED pattern. */
	if ((led_pattern->led_state == led_state) && (led_pattern->duration_sec == duration_sec)) {
		return true;
	}

	return false;
}

/* Function used to verify LED pattern transitions upon publication to cloud. */
static void verify_publication(bool active_mode)
{
	struct data_module_event *data_module_event;
	struct location_module_event *location_module_event;
	enum led_state led_state_mode_verify = LED_STATE_ACTIVE_MODE;
	enum sub_state_type sub_state_verify = SUB_STATE_ACTIVE;

	if (!active_mode) {
		led_state_mode_verify = LED_STATE_PASSIVE_MODE;
		sub_state_verify = SUB_STATE_PASSIVE;
	}

	/* Send cloud publication event. */
	TEST_SEND_EVENT(data, DATA_EVT_DATA_SEND, data_module_event);
	TEST_ASSERT_TRUE(list_node_verify_next(LED_STATE_CLOUD_PUBLISHING, 5));
	TEST_ASSERT_TRUE(list_node_verify_next(led_state_mode_verify, 5));
	TEST_ASSERT_TRUE(list_node_verify_next(LED_STATE_TURN_OFF, -1));
	TEST_ASSERT_TRUE(list_node_verify_next(0, 0));

	/* Set module in SUB_SUB_STATE_LOCATION_ACTIVE. */
	TEST_SEND_EVENT(location, LOCATION_MODULE_EVT_ACTIVE, location_module_event);
	state_verify(STATE_RUNNING, sub_state_verify, SUB_SUB_STATE_LOCATION_ACTIVE);

	/* Send cloud publication event. */
	TEST_SEND_EVENT(data, DATA_EVT_DATA_SEND_BATCH, data_module_event);
	TEST_ASSERT_TRUE(list_node_verify_next(LED_STATE_CLOUD_PUBLISHING, 5));
	TEST_ASSERT_TRUE(list_node_verify_next(led_state_mode_verify, 5));
	TEST_ASSERT_TRUE(list_node_verify_next(LED_STATE_LOCATION_SEARCHING, -1));
	TEST_ASSERT_TRUE(list_node_verify_next(0, 0));

	TEST_SEND_EVENT(location, LOCATION_MODULE_EVT_INACTIVE, location_module_event);
	state_verify(STATE_RUNNING, sub_state_verify, SUB_SUB_STATE_LOCATION_INACTIVE);

	/* Send cloud publication event. */
	TEST_SEND_EVENT(data, DATA_EVT_UI_DATA_SEND, data_module_event);
	TEST_ASSERT_TRUE(list_node_verify_next(LED_STATE_CLOUD_PUBLISHING, 5));
	TEST_ASSERT_TRUE(list_node_verify_next(led_state_mode_verify, 5));
	TEST_ASSERT_TRUE(list_node_verify_next(LED_STATE_TURN_OFF, -1));
	TEST_ASSERT_TRUE(list_node_verify_next(0, 0));
}

void setup_ui_module_in_init_state(void)
{
	state_verify(STATE_INIT, SUB_STATE_ACTIVE, SUB_SUB_STATE_LOCATION_INACTIVE);

	__cmock_module_start_Stub(&module_start_stub);

	struct app_module_event *app_module_event;

	TEST_SEND_EVENT(app, APP_EVT_START, app_module_event);
	state_verify(STATE_RUNNING, SUB_STATE_ACTIVE, SUB_SUB_STATE_LOCATION_INACTIVE);
}

void test_state_lte_connecting(void)
{
	resetTest();
	setup_ui_module_in_init_state();

	struct modem_module_event *modem_module_event;

	/* Verify state transition to STATE_LTE_CONNECTING. */
	TEST_SEND_EVENT(modem, MODEM_EVT_LTE_CONNECTING, modem_module_event);
	state_verify(STATE_LTE_CONNECTING, SUB_STATE_ACTIVE, SUB_SUB_STATE_LOCATION_INACTIVE);

	TEST_ASSERT_TRUE(list_node_verify_next(LED_STATE_LTE_CONNECTING, -1));
	TEST_ASSERT_TRUE(list_node_verify_next(0, 0));

	/* Verify state transition to STATE_RUNNING. */
	TEST_SEND_EVENT(modem, MODEM_EVT_LTE_CONNECTED, modem_module_event);
	state_verify(STATE_RUNNING, SUB_STATE_ACTIVE, SUB_SUB_STATE_LOCATION_INACTIVE);

	TEST_ASSERT_TRUE(list_node_verify_next(LED_STATE_TURN_OFF, -1));
	TEST_ASSERT_TRUE(list_node_verify_next(0, 0));
}

void test_state_cloud_connecting(void)
{
	resetTest();
	setup_ui_module_in_init_state();

	struct cloud_module_event *cloud_module_event;

	/* Verify state transition to STATE_CLOUD_CONNECTING. */
	TEST_SEND_EVENT(cloud, CLOUD_EVT_CONNECTING, cloud_module_event);
	state_verify(STATE_CLOUD_CONNECTING, SUB_STATE_ACTIVE, SUB_SUB_STATE_LOCATION_INACTIVE);

	TEST_ASSERT_TRUE(list_node_verify_next(LED_STATE_CLOUD_CONNECTING, -1));
	TEST_ASSERT_TRUE(list_node_verify_next(0, 0));

	/* Verify state transition to STATE_RUNNING. */
	TEST_SEND_EVENT(cloud, CLOUD_EVT_CONNECTED, cloud_module_event);
	state_verify(STATE_RUNNING, SUB_STATE_ACTIVE, SUB_SUB_STATE_LOCATION_INACTIVE);

	TEST_ASSERT_TRUE(list_node_verify_next(LED_STATE_TURN_OFF, -1));
	TEST_ASSERT_TRUE(list_node_verify_next(0, 0));
}

void test_state_cloud_associating(void)
{
	resetTest();
	setup_ui_module_in_init_state();

	struct cloud_module_event *cloud_module_event;

	/* Verify state transition to STATE_CLOUD_ASSOCIATING. */
	TEST_SEND_EVENT(cloud, CLOUD_EVT_USER_ASSOCIATION_REQUEST, cloud_module_event);
	state_verify(STATE_CLOUD_ASSOCIATING, SUB_STATE_ACTIVE, SUB_SUB_STATE_LOCATION_INACTIVE);

	TEST_ASSERT_TRUE(list_node_verify_next(LED_STATE_CLOUD_ASSOCIATING, -1));
	TEST_ASSERT_TRUE(list_node_verify_next(0, 0));

	/* Verify state transition to STATE_RUNNING. */
	TEST_SEND_EVENT(cloud, CLOUD_EVT_USER_ASSOCIATED, cloud_module_event);
	state_verify(STATE_RUNNING, SUB_STATE_ACTIVE, SUB_SUB_STATE_LOCATION_INACTIVE);

	TEST_ASSERT_TRUE(list_node_verify_next(LED_STATE_CLOUD_ASSOCIATED, -1));
	TEST_ASSERT_TRUE(list_node_verify_next(0, 0));
}

void test_state_fota_updating(void)
{
	resetTest();
	setup_ui_module_in_init_state();

	struct cloud_module_event *cloud_module_event;

	/* Verify state transition to STATE_FOTA_UPDATING. */
	TEST_SEND_EVENT(cloud, CLOUD_EVT_FOTA_START, cloud_module_event);
	state_verify(STATE_FOTA_UPDATING, SUB_STATE_ACTIVE, SUB_SUB_STATE_LOCATION_INACTIVE);

	TEST_ASSERT_TRUE(list_node_verify_next(LED_STATE_FOTA_UPDATING, -1));
	TEST_ASSERT_TRUE(list_node_verify_next(0, 0));

	/* Verify state transition to STATE_RUNNING. */
	TEST_SEND_EVENT(cloud, CLOUD_EVT_FOTA_DONE, cloud_module_event);
	state_verify(STATE_RUNNING, SUB_STATE_ACTIVE, SUB_SUB_STATE_LOCATION_INACTIVE);

	TEST_ASSERT_TRUE(list_node_verify_next(LED_STATE_TURN_OFF, -1));
	TEST_ASSERT_TRUE(list_node_verify_next(0, 0));
}

void test_state_shutdown_fota(void)
{
	resetTest();
	setup_ui_module_in_init_state();

	/* Verify state transition to STATE_SHUTDOWN. */
	__cmock_app_event_manager_alloc_ExpectAnyArgsAndReturn(&util_module_event_memory);

	/* When a shutdown request is notified by the utility module it is expected that
	 * the UI module acknowledges the shutdown request.
	 */
	__cmock_app_event_manager_alloc_ExpectAnyArgsAndReturn(&ui_module_event_memory);
	__cmock_app_event_manager_free_ExpectAnyArgs();
	__cmock__event_submit_Stub(&validate_ui_evt);

	struct util_module_event *util_module_event = new_util_module_event();

	util_module_event->type = UTIL_EVT_SHUTDOWN_REQUEST;
	util_module_event->reason = REASON_FOTA_UPDATE;

	TEST_ASSERT_FALSE(UI_MODULE_EVT_HANDLER(
		(struct app_event_header *)util_module_event));
	app_event_manager_free(util_module_event);

	state_verify(STATE_SHUTDOWN, SUB_STATE_ACTIVE, SUB_SUB_STATE_LOCATION_INACTIVE);

	TEST_ASSERT_TRUE(list_node_verify_next(LED_STATE_FOTA_UPDATE_REBOOT, -1));
	TEST_ASSERT_TRUE(list_node_verify_next(0, 0));
}

void test_state_shutdown_error(void)
{
	resetTest();
	setup_ui_module_in_init_state();

	/* Verify state transition to STATE_SHUTDOWN. */
	__cmock_app_event_manager_alloc_ExpectAnyArgsAndReturn(&util_module_event_memory);

	/* When a shutdown request is notified by the utility module it is expected that
	 * the UI module acknowledges the shutdown request.
	 */
	__cmock_app_event_manager_alloc_ExpectAnyArgsAndReturn(&ui_module_event_memory);
	__cmock_app_event_manager_free_ExpectAnyArgs();
	__cmock__event_submit_Stub(&validate_ui_evt);

	struct util_module_event *util_module_event = new_util_module_event();

	util_module_event->type = UTIL_EVT_SHUTDOWN_REQUEST;
	util_module_event->reason = REASON_GENERIC;

	TEST_ASSERT_FALSE(UI_MODULE_EVT_HANDLER(
		(struct app_event_header *)util_module_event));
	app_event_manager_free(util_module_event);

	state_verify(STATE_SHUTDOWN, SUB_STATE_ACTIVE, SUB_SUB_STATE_LOCATION_INACTIVE);

	TEST_ASSERT_TRUE(list_node_verify_next(LED_STATE_ERROR_SYSTEM_FAULT, -1));
	TEST_ASSERT_TRUE(list_node_verify_next(0, 0));
}

void test_gps_search(void)
{
	resetTest();
	setup_ui_module_in_init_state();

	struct location_module_event *location_module_event;

	/* Set module in SUB_SUB_STATE_LOCATION_ACTIVE. */
	TEST_SEND_EVENT(location, LOCATION_MODULE_EVT_ACTIVE, location_module_event);
	state_verify(STATE_RUNNING, SUB_STATE_ACTIVE, SUB_SUB_STATE_LOCATION_ACTIVE);

	TEST_ASSERT_TRUE(list_node_verify_next(LED_STATE_LOCATION_SEARCHING, -1));
	TEST_ASSERT_TRUE(list_node_verify_next(0, 0));

	/* Set module in SUB_SUB_STATE_LOCATION_INACTIVE. */
	TEST_SEND_EVENT(location, LOCATION_MODULE_EVT_INACTIVE, location_module_event);
	state_verify(STATE_RUNNING, SUB_STATE_ACTIVE, SUB_SUB_STATE_LOCATION_INACTIVE);

	TEST_ASSERT_TRUE(list_node_verify_next(LED_STATE_TURN_OFF, -1));
	TEST_ASSERT_TRUE(list_node_verify_next(0, 0));
}

void test_mode_transition(void)
{
	resetTest();
	setup_ui_module_in_init_state();

	struct data_module_event *data_module_event;

	verify_publication(true);

	__cmock_app_event_manager_alloc_ExpectAnyArgsAndReturn(&data_module_event_memory);
	__cmock_app_event_manager_alloc_ExpectAnyArgsAndReturn(&data_module_event_memory);
	__cmock_app_event_manager_free_ExpectAnyArgs();
	__cmock_app_event_manager_free_ExpectAnyArgs();

	/* Set module in SUB_STATE_PASSIVE */
	data_module_event = new_data_module_event();
	data_module_event->type = DATA_EVT_CONFIG_READY;
	data_module_event->data.cfg.active_mode = false;

	TEST_ASSERT_FALSE(UI_MODULE_EVT_HANDLER(
		(struct app_event_header *)data_module_event));
	app_event_manager_free(data_module_event);

	state_verify(STATE_RUNNING, SUB_STATE_PASSIVE, SUB_SUB_STATE_LOCATION_INACTIVE);

	verify_publication(false);

	/* Set module in SUB_STATE_ACTIVE */
	data_module_event = new_data_module_event();
	data_module_event->type = DATA_EVT_CONFIG_READY;
	data_module_event->data.cfg.active_mode = true;

	TEST_ASSERT_FALSE(UI_MODULE_EVT_HANDLER(
		(struct app_event_header *)data_module_event));
	app_event_manager_free(data_module_event);

	state_verify(STATE_RUNNING, SUB_STATE_ACTIVE, SUB_SUB_STATE_LOCATION_INACTIVE);

	verify_publication(true);
}

int main(void)
{
	(void)unity_main();
	return 0;
}
