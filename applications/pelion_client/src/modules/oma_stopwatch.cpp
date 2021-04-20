/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>

#include "mbed-cloud-client/MbedCloudClient.h"

#include "pelion_event.h"
#include "net_event.h"

#define MODULE oma_stopwatch
#include <caf/events/module_state_event.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_PELION_CLIENT_OMA_STOPWATCH_LOG_LEVEL);


#define OMA_OBJECT_STOPWATCH  "3350"
#define OMA_RESOURCE_CUMULATIVE_TIME "5544"


static M2MResource *resource;
static struct k_delayed_work resource_work;

static bool updated;
static bool update_pending;
static int64_t base_time;

static bool registered;
static bool connected;


static void update_stopwatch(void)
{
	if (!registered || !connected) {
		return;
	}

	if (updated && !update_pending) {
		unsigned int time_seconds = (k_uptime_get() - base_time)/1000;

		updated = false;
		update_pending = true;
		resource->set_value((float)time_seconds);

		LOG_INF("Resource %p, set value %u", resource, time_seconds);
	}
}

static void resource_work_handler(struct k_work *work)
{
	LOG_INF("Update timer");

	updated = true;
	update_stopwatch();

	k_delayed_work_submit(&resource_work, K_SECONDS(CONFIG_PELION_CLIENT_OMA_STOPWATCH_TIMEOUT));
}

static void on_value_updated(const char *arg)
{
	LOG_INF("Resource %s, value updated %u", log_strdup(arg), (unsigned)resource->get_value_float());
	if (resource->get_value_float() == 0.0) {
		LOG_INF("Stopwatch reset requested");
		base_time = k_uptime_get();
	}
}

static void notification_status_callback(const M2MBase& object,
					 const M2MBase::MessageDeliveryStatus status,
					 const M2MBase::MessageType,
					 void *arg)
{
	LOG_DBG("Resource %p notification, object:%s status:%d", (void*)&object,
		log_strdup(object.uri_path()), status);
	if (status == M2MBase::MESSAGE_STATUS_DELIVERED) {
		update_pending = false;
		if (updated) {
			update_stopwatch();
		}
	}
}

static int resource_init(M2MObjectList *object_list)
{
	M2MObject *object = M2MInterfaceFactory::create_object(OMA_OBJECT_STOPWATCH);
	if (!object) {
		LOG_ERR("Could not create an object");
		return -ENOMEM;
	}
	LOG_INF("Object %p created", (void*)object);

	M2MObjectInstance *instance = object->create_object_instance((uint16_t)0);
	if (!instance) {
		LOG_ERR("Could not create an object instance");
		return -ENOMEM;
	}
	LOG_INF("Object instance %p created", (void*)instance);

	resource = instance->create_dynamic_resource(OMA_RESOURCE_CUMULATIVE_TIME,
						     "Stopwatch",
						     M2MResourceInstance::FLOAT,
						     true);
	if (!resource) {
		LOG_ERR("Could not create a resource, abort.");
		return -ENOMEM;
	}

	resource->set_operation(M2MBase::GET_PUT_ALLOWED);
	resource->set_message_delivery_status_cb(notification_status_callback, NULL);
	resource->set_value_updated_function(on_value_updated);
	resource->set_auto_observable(true);
	LOG_INF("Object resource %p created", (void*)resource);

	updated = true;
	update_stopwatch();

	object_list->push_back(object);

	return 0;
}

static bool handle_net_state_event(const struct net_state_event *event)
{
	if (event->state == NET_STATE_CONNECTED) {
		connected = true;
		update_stopwatch();
	} else {
		connected = false;
	}

	return false;
}

static bool handle_pelion_state_event(const struct pelion_state_event *event)
{
	if (event->state == PELION_STATE_REGISTERED) {
		registered = true;
		update_stopwatch();
	} else {
		registered = false;
	}

	return false;
}

static bool handle_pelion_create_objects_event(const struct pelion_create_objects_event *event)
{
	static bool objects_created;

	__ASSERT_NO_MSG(!objects_created);

	objects_created = true;

	int err = resource_init((M2MObjectList*)event->object_list);
	if (err) {
		module_set_state(MODULE_STATE_ERROR);
	}

	return false;
}

static bool event_handler(const struct event_header *eh)
{
	if (is_module_state_event(eh)) {
		struct module_state_event *event = cast_module_state_event(eh);
		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			static bool initialized;

			__ASSERT_NO_MSG(!initialized);
			initialized = true;

			k_delayed_work_init(&resource_work, resource_work_handler);
			k_delayed_work_submit(&resource_work, K_NO_WAIT);

			module_set_state(MODULE_STATE_READY);
		}

		return false;
	}

	if (is_net_state_event(eh)) {
		return handle_net_state_event(cast_net_state_event(eh));
	}

	if (is_pelion_state_event(eh)) {
		return handle_pelion_state_event(cast_pelion_state_event(eh));
	}

	if (is_pelion_create_objects_event(eh)) {
		return handle_pelion_create_objects_event(cast_pelion_create_objects_event(eh));
	}

	/* Event not handled but subscribed. */
	__ASSERT_NO_MSG(false);

	return false;
}

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, module_state_event);
EVENT_SUBSCRIBE(MODULE, net_state_event);
EVENT_SUBSCRIBE(MODULE, pelion_state_event);
EVENT_SUBSCRIBE(MODULE, pelion_create_objects_event);
