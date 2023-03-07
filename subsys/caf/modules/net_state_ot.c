/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/kernel.h>
#include <stdio.h>

#include <zephyr/net/openthread.h>
#include <openthread/thread.h>
#include <openthread/netdata.h>

#define MODULE net_state
#include <caf/events/module_state_event.h>
#include <caf/events/power_manager_event.h>
#include <caf/events/net_state_event.h>
#include <caf/events/factory_reset_event.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_CAF_NET_STATE_OT_LOG_LEVEL);


#define NET_MODULE_ID_STATE_READY_WAIT_FOR                          \
	(IS_ENABLED(CONFIG_CAF_FACTORY_RESET_REQUEST) ?             \
		MODULE_ID(factory_reset_request) : MODULE_ID(main))


static struct k_work_delayable connecting_work;
static enum net_state net_state;
static bool initialized;

#if IS_ENABLED(CONFIG_CAF_LOG_NET_STATE_WAITING)
#define STATE_WAITING_PERIOD CONFIG_CAF_LOG_NET_STATE_WAITING_PERIOD
#else
#define STATE_WAITING_PERIOD 0
#endif

static void format_address(char *buffer, size_t buffer_size, const uint8_t *addr_m8,
			   size_t addr_size)
{
	if (addr_size == 0) {
		buffer[0] = 0;
		return;
	}

	size_t pos = 0;

	for (size_t i = 0; i < addr_size; i++) {
		int ret = snprintf(&buffer[pos], buffer_size - pos, "%.2x", addr_m8[i]);

		if ((ret > 0) && ((size_t)ret < buffer_size - pos)) {
			pos += ret;
		} else {
			break;
		}
	}
}


static bool check_neighbors(otInstance *instance)
{
	otNeighborInfoIterator iterator = OT_NEIGHBOR_INFO_ITERATOR_INIT;
	otNeighborInfo info;

	bool has_neighbors = false;

	while (otThreadGetNextNeighborInfo(instance, &iterator, &info) == OT_ERROR_NONE) {
		char addr_str[ARRAY_SIZE(info.mExtAddress.m8) * 2 + 1];

		format_address(addr_str, sizeof(addr_str), info.mExtAddress.m8,
			       ARRAY_SIZE(info.mExtAddress.m8));
		LOG_INF("Neighbor addr:%s age:%" PRIu32, addr_str, info.mAge);

		has_neighbors = true;

		if (!IS_ENABLED(CONFIG_LOG)) {
			/* If logging is disabled, stop when a neighbor is found. */
			break;
		}
	}

	return has_neighbors;
}

static bool check_routes(otInstance *instance)
{
	otNetworkDataIterator iterator = OT_NETWORK_DATA_ITERATOR_INIT;
	otBorderRouterConfig config;

	bool route_available = false;

	while (otNetDataGetNextOnMeshPrefix(instance, &iterator, &config) == OT_ERROR_NONE) {
		char addr_str[ARRAY_SIZE(config.mPrefix.mPrefix.mFields.m8) * 2 + 1] = {0};

		format_address(addr_str, sizeof(addr_str), config.mPrefix.mPrefix.mFields.m8,
			       ARRAY_SIZE(config.mPrefix.mPrefix.mFields.m8));
		LOG_INF("Route prefix:%s default:%s preferred:%s", addr_str,
			(config.mDefaultRoute)?("yes"):("no"),
			(config.mPreferred)?("yes"):("no"));

		route_available = route_available || config.mDefaultRoute;

		if (route_available && !IS_ENABLED(CONFIG_LOG)) {
			/* If logging is disabled, stop when a route is found. */
			break;
		}
	}

	return route_available;
}

static void connecting_work_handler(struct k_work *work)
{
	LOG_INF("Waiting for OT connection...");
	k_work_reschedule(&connecting_work, K_SECONDS(STATE_WAITING_PERIOD));
}

static void send_net_state_event(enum net_state state)
{
	struct net_state_event *event = new_net_state_event();

	event->id = MODULE_ID(MODULE);
	event->state = state;
	APP_EVENT_SUBMIT(event);
}

static void set_net_state(enum net_state state)
{
	if (state == net_state) {
		return;
	}

	net_state = state;

	if (net_state == NET_STATE_CONNECTED) {
		if (IS_ENABLED(CONFIG_CAF_POWER_MANAGER_EVENTS)) {
			power_manager_restrict(MODULE_IDX(MODULE), POWER_MANAGER_LEVEL_ALIVE);
		}
	} else {
		if (IS_ENABLED(CONFIG_CAF_POWER_MANAGER_EVENTS)) {
			power_manager_restrict(MODULE_IDX(MODULE), POWER_MANAGER_LEVEL_MAX);
		}
	}
	send_net_state_event(state);
}

static void on_thread_state_changed(otChangedFlags flags, struct openthread_context *ot_context,
				    void *user_data)
{
	static bool has_role;

	bool has_neighbors = check_neighbors(ot_context->instance);
	bool route_available = check_routes(ot_context->instance);

	LOG_INF("state: 0x%.8x has_neighbours:%s route_available:%s", flags,
		(has_neighbors)?("yes"):("no"), (route_available)?("yes"):("no"));

	if (flags & OT_CHANGED_THREAD_ROLE) {
		switch (otThreadGetDeviceRole(ot_context->instance)) {
		case OT_DEVICE_ROLE_LEADER:
			LOG_INF("Leader role set");
			has_role = true;
			break;

		case OT_DEVICE_ROLE_CHILD:
			LOG_INF("Child role set");
			has_role = true;
			break;

		case OT_DEVICE_ROLE_ROUTER:
			LOG_INF("Router role set");
			has_role = true;
			break;

		case OT_DEVICE_ROLE_DISABLED:
		case OT_DEVICE_ROLE_DETACHED:
		default:
			LOG_INF("No role set");
			has_role = false;
			break;
		}
	}

	if (has_role && has_neighbors && route_available) {
		set_net_state(NET_STATE_CONNECTED);
	} else {
		set_net_state(NET_STATE_DISCONNECTED);
	}
}
static struct openthread_state_changed_cb ot_state_chaged_cb = {
	.state_changed_cb = on_thread_state_changed
};

static void connect_ot(void)
{
	openthread_state_changed_cb_register(openthread_get_default_context(), &ot_state_chaged_cb);
	openthread_start(openthread_get_default_context());

	LOG_INF("OT connection requested");
}

static bool handle_state_event(const struct module_state_event *event)
{
	if (!check_state(event, NET_MODULE_ID_STATE_READY_WAIT_FOR, MODULE_STATE_READY)) {
		return false;
	}
	__ASSERT_NO_MSG(!initialized);
	initialized = true;

	set_net_state(NET_STATE_DISCONNECTED);

	connect_ot();
	module_set_state(MODULE_STATE_READY);

	if (IS_ENABLED(CONFIG_CAF_LOG_NET_STATE_WAITING)) {
		k_work_init_delayable(&connecting_work, connecting_work_handler);
		k_work_reschedule(&connecting_work, K_NO_WAIT);
	}

	return false;
}

static bool handle_reset_event(void)
{
	struct openthread_context *ot_context = openthread_get_default_context();
	otError err;

	/* This event has to apear before initialization */
	__ASSERT_NO_MSG(!initialized);

	LOG_WRN("Storage reset requested");
	openthread_api_mutex_lock(ot_context);
	err = otInstanceErasePersistentInfo(ot_context->instance);
	openthread_api_mutex_unlock(ot_context);
	/* It can fail only if called with OpenThread stack enabled.
	 * This event should not appear after the OpenThread is started.
	 * If it does - there is some huge coding error.
	 */
	__ASSERT_NO_MSG(err == OT_ERROR_NONE);

	return false;
}

static bool app_event_handler(const struct app_event_header *aeh)
{
	if (is_module_state_event(aeh)) {
		return handle_state_event(cast_module_state_event(aeh));
	}

	if (IS_ENABLED(CONFIG_CAF_LOG_NET_STATE_WAITING) &&  is_net_state_event(aeh)) {
		if (net_state == NET_STATE_DISCONNECTED) {
			k_work_reschedule(&connecting_work, K_NO_WAIT);
		} else {
			/* Cancel cannot fail if executed from another work's context. */
			(void)k_work_cancel_delayable(&connecting_work);
		}

		return false;
	}

	if (IS_ENABLED(CONFIG_CAF_FACTORY_RESET_EVENTS) &&
	    is_factory_reset_event(aeh)) {
		return handle_reset_event();
	}

	/* Event not handled but subscribed. */
	__ASSERT_NO_MSG(false);

	return false;
}

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, module_state_event);
#if IS_ENABLED(CONFIG_CAF_FACTORY_RESET_EVENTS)
	APP_EVENT_SUBSCRIBE(MODULE, factory_reset_event);
#endif
#if CONFIG_CAF_LOG_NET_STATE_WAITING
	APP_EVENT_SUBSCRIBE(MODULE, net_state_event);
#endif
