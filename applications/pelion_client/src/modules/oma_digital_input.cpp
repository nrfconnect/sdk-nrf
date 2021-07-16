/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>

#include "mbed-cloud-client/MbedCloudClient.h"

#include "pelion_event.h"

#define MODULE oma_digital_input
#include <caf/events/module_state_event.h>

#include <caf/events/button_event.h>
#include <caf/events/net_state_event.h>
#include <caf/key_id.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_PELION_CLIENT_OMA_DIGITAL_INPUT_LOG_LEVEL);


#define OMA_OBJECT_DIGITAL_INPUT  "3200"
#define OMA_RESOURCE_DIGITAL_INPUT_STATE "5500"
#define OMA_RESOURCE_DIGITAL_INPUT_COUNTER "5501"


struct input_state {
	M2MResource *resource;
	bool state;
	bool updated;
	bool update_pending;
};

struct input_counter {
	M2MResource *resource;
	unsigned int counter;
	bool updated;
	bool update_pending;
};

struct button {
	M2MObjectInstance *instance;
	struct input_state state;
	struct input_counter counter;
	uint16_t key_id;
};

static struct button buttons[] = {
	{ .key_id = KEY_ID(0x00, 0x00) },
	{ .key_id = KEY_ID(0x00, 0x01) },
	{ .key_id = KEY_ID(0x00, 0x02) },
	{ .key_id = KEY_ID(0x00, 0x03) },
};
static bool registered;
static bool connected;


static void update_button(struct button *button)
{
	if (!registered || !connected) {
		return;
	}

	if (button->state.updated && !button->state.update_pending) {
		button->state.updated = false;
		button->state.update_pending = true;
		button->state.resource->set_value(button->state.state);
		LOG_INF("Resource %p written", (void*)button->state.resource);
	}

	if (button->counter.updated && !button->counter.update_pending) {
		button->counter.updated = false;
		button->counter.update_pending = true;
		button->counter.resource->set_value(button->counter.counter);
		LOG_INF("Resource %p written", (void*)button->counter.resource);
	}
}

static void update_buttons(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(buttons); i++) {
		update_button(&buttons[i]);
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
		for (size_t i = 0; i < ARRAY_SIZE(buttons); i++) {
			if (buttons[i].state.resource == &object) {
				LOG_INF("Resource %p updated", (void*)buttons[i].state.resource);
				buttons[i].state.update_pending = false;
				update_button(&buttons[i]);
				break;
			}
			if (buttons[i].counter.resource == &object) {
				LOG_INF("Resource %p updated", (void*)buttons[i].counter.resource);
				buttons[i].counter.update_pending = false;
				update_button(&buttons[i]);
				break;
			}
		}
	}
}

static int instance_init(M2MObject *object, struct button *button, size_t instance_id)
{
	M2MObjectInstance *instance;
	M2MResource *resource;

	instance = object->create_object_instance((uint16_t)instance_id);
	if (!instance) {
		LOG_ERR("Could not create an object instance");
		return -ENOMEM;
	}
	LOG_INF("Object instance %p created", (void*)instance);
	button->instance = instance;

	/* Digital input state */
	resource = instance->create_dynamic_resource(OMA_RESOURCE_DIGITAL_INPUT_STATE,
						     "Button State",
						     M2MResourceInstance::BOOLEAN,
						     true);
	if (!resource) {
		LOG_ERR("Could not create a resource, abort.");
		return -ENOMEM;
	}

	resource->set_operation(M2MBase::GET_ALLOWED);
	resource->set_message_delivery_status_cb(notification_status_callback, NULL);
	resource->set_auto_observable(true);

	LOG_INF("Resource %p created", (void*)resource);
	button->state.resource = resource;

	/* Digital input counter */
	resource = instance->create_dynamic_resource(OMA_RESOURCE_DIGITAL_INPUT_COUNTER,
						     "Press Counter",
						     M2MResourceInstance::INTEGER,
						     true);
	if (!resource) {
		LOG_ERR("Could not create a resource, abort.");
		return -ENOMEM;
	}

	resource->set_operation(M2MBase::GET_ALLOWED);
	resource->set_message_delivery_status_cb(notification_status_callback, NULL);
	resource->set_auto_observable(true);

	LOG_INF("Resource %p created", (void*)resource);
	button->counter.resource = resource;

	return 0;
}

static int resource_init(M2MObjectList *object_list)
{
	M2MObject *object = M2MInterfaceFactory::create_object(OMA_OBJECT_DIGITAL_INPUT);
	if (!object) {
		LOG_ERR("Could not create an object");
		return -ENOMEM;
	}
	LOG_INF("Object %p created", (void*)object);

	for (size_t i = 0; i < ARRAY_SIZE(buttons); i++) {
		int err = instance_init(object, &buttons[i], i);

		if (err) {
			return err;
		}

		buttons[i].state.updated = true;
		buttons[i].counter.updated = true;

		update_button(&buttons[i]);
	}

	LOG_INF("Object %p resources created", (void*)object);

	object_list->push_back(object);

	return 0;
}

static bool handle_button_event(const struct button_event *event)
{
	for (size_t i = 0; i < ARRAY_SIZE(buttons); i++) {
		if (buttons[i].key_id == event->key_id) {
			buttons[i].state.state = event->pressed;
			buttons[i].state.updated = true;

			if (event->pressed) {
				buttons[i].counter.counter++;
				buttons[i].counter.updated = true;
			}

			update_button(&buttons[i]);
		}
	}

	return false;
}

static bool handle_net_state_event(const struct net_state_event *event)
{
	if (event->state == NET_STATE_CONNECTED) {
		connected = true;
		update_buttons();
	} else {
		connected = false;
	}

	return false;
}

static bool handle_pelion_state_event(const struct pelion_state_event *event)
{
	if (event->state == PELION_STATE_REGISTERED) {
		registered = true;
		update_buttons();
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

			module_set_state(MODULE_STATE_READY);
		}

		return false;
	}

	if (is_button_event(eh)) {
		return handle_button_event(cast_button_event(eh));
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
EVENT_SUBSCRIBE(MODULE, button_event);
EVENT_SUBSCRIBE(MODULE, module_state_event);
EVENT_SUBSCRIBE(MODULE, net_state_event);
EVENT_SUBSCRIBE(MODULE, pelion_state_event);
EVENT_SUBSCRIBE(MODULE, pelion_create_objects_event);
