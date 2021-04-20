/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr.h>
#include <stdio.h>

#include <net/openthread.h>
#include <openthread/thread.h>
#include <openthread/netdata.h>

#include "net_event.h"

#define MODULE net
#include <caf/events/module_state_event.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_PELION_CLIENT_NET_LOG_LEVEL);


static struct k_delayed_work connecting_work;
static enum net_state net_state;


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
		LOG_INF("Neighbor addr:%s age:%" PRIu32, log_strdup(addr_str), info.mAge);

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
		LOG_INF("Route prefix:%s default:%s preferred:%s", log_strdup(addr_str),
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
	k_delayed_work_submit(&connecting_work, K_SECONDS(5));
}

static void send_net_state_event(void)
{
	struct net_state_event *event = new_net_state_event();

	event->id = MODULE_ID(net);
	event->state = net_state;
	EVENT_SUBMIT(event);
}

static void set_net_state_event(enum net_state state)
{
	if (state == net_state) {
		return;
	}

	net_state = state;

	if (net_state == NET_STATE_CONNECTED) {
		k_delayed_work_cancel(&connecting_work);
	} else {
		k_delayed_work_submit(&connecting_work, K_NO_WAIT);
	}

	send_net_state_event();
}

static void on_thread_state_changed(uint32_t flags, void *context)
{
	static bool has_role;

	struct openthread_context *ot_context = context;
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
		set_net_state_event(NET_STATE_CONNECTED);
	} else {
		set_net_state_event(NET_STATE_DISCONNECTED);
	}
}

static void connect_ot(void)
{
	openthread_set_state_changed_cb(on_thread_state_changed);
	openthread_start(openthread_get_default_context());

	LOG_INF("OT connection requested");
}

static bool event_handler(const struct event_header *eh)
{
	if (is_module_state_event(eh)) {
		struct module_state_event *event = cast_module_state_event(eh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			static bool initialized;

			__ASSERT_NO_MSG(!initialized);
			initialized = true;

			k_delayed_work_init(&connecting_work, connecting_work_handler);

			set_net_state_event(NET_STATE_DISCONNECTED);

			connect_ot();
			module_set_state(MODULE_STATE_READY);
			k_delayed_work_submit(&connecting_work, K_NO_WAIT);
		}

		return false;
	}

	/* Event not handled but subscribed. */
	__ASSERT_NO_MSG(false);

	return false;
}

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, module_state_event);
