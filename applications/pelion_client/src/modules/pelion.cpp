/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <pal.h>

#include "mbed-cloud-client/MbedCloudClient.h"

#include "pelion_event.h"

#define MODULE pelion
#include <caf/events/module_state_event.h>
#include <caf/events/net_state_event.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_PELION_CLIENT_PELION_LOG_LEVEL);


static MbedCloudClient cloud_client;
static M2MObjectList object_list;

static bool net_connected;
static bool objects_created;

static enum pelion_state pelion_state;

static connectionStatusCallback net_status_cb;
static void *net_cb_arg;

#if defined(CONFIG_PELION_CLIENT_USE_APPLICATION_NETWORK_CALLBACK)
extern "C" palStatus_t pal_plat_setConnectionStatusCallback(uint32_t interfaceIndex,
							    connectionStatusCallback callback,
							    void *arg)
{
	net_status_cb = callback;
	net_cb_arg = arg;

	return PAL_SUCCESS;
}
#endif /* CONFIG_PELION_CLIENT_USE_APPLICATION_NETWORK_CALLBACK */

static void set_pelion_state(enum pelion_state state)
{
	if (pelion_state == state) {
		return;
	}

	pelion_state = state;

	struct pelion_state_event *event = new_pelion_state_event();
	event->state = state;
	EVENT_SUBMIT(event);
}

#if CONFIG_PELION_UPDATE
static void on_update_progress(uint32_t progress, uint32_t total)
{
	unsigned perc = (progress * 100) / total;
	LOG_INF("Update in progress: %u%% (total:%u)", perc, (unsigned)total);
}

static void on_update_authorize_priority(int32_t request, uint64_t priority)
{
	switch(request) {
	case MbedCloudClient::UpdateRequestDownload:
		LOG_WRN("Firmware download requested");
		cloud_client.update_authorize(request);
		break;
	case MbedCloudClient::UpdateRequestInstall:
		LOG_WRN("Firmware installation requested");
		cloud_client.update_authorize(request);
		break;
	default:
		LOG_ERR("Unknown update request (%d)", (int)request);
		break;
	}
}
#endif

static void on_client_registered(void)
{
	LOG_INF("Client registered to the cloud");

	const ConnectorClientEndpointInfo *endpoint = cloud_client.endpoint_info();

	LOG_INF("Internal Endpoint Name: %s", log_strdup(endpoint->internal_endpoint_name.c_str()));
	LOG_INF("Endpoint Name: %s", log_strdup(endpoint->endpoint_name.c_str()));
	LOG_INF("Device ID: %s", log_strdup(endpoint->internal_endpoint_name.c_str()));

	set_pelion_state(PELION_STATE_REGISTERED);
}

static void on_client_registration_updated(void)
{
	LOG_INF("Client registration updated");
	set_pelion_state(PELION_STATE_REGISTERED);
}

static void on_client_unregistered(void)
{
	LOG_INF("Client unregistered from the cloud");
	set_pelion_state(PELION_STATE_UNREGISTERED);
}

static void on_status_changed(int status)
{
	switch (status) {
	case MbedCloudClient::Unregistered:
		on_client_unregistered();
		break;

	case MbedCloudClient::Registered:
		on_client_registered();
		break;

	case MbedCloudClient::RegistrationUpdated:
		on_client_registration_updated();
		break;

	case MbedCloudClient::AlertMode:
		LOG_INF("Client alert mode");
		break;

	case MbedCloudClient::Paused:
		LOG_INF("Client paused");
		break;

	default:
		LOG_ERR("Unknown client status");
		__ASSERT_NO_MSG(false);
		break;
	}
}

static void on_client_error(int code)
{
	const char *description = cloud_client.error_description();
	LOG_ERR("Error: %s (code:%d)", log_strdup(description), code);

	switch (code) {
	case M2MInterface::NetworkError:
		set_pelion_state(PELION_STATE_UNREGISTERED);
		break;
	case M2MInterface::InvalidCertificates:
		set_pelion_state(PELION_STATE_SUSPENDED);
		break;
	default:
		/* Ignore */
		break;
	}
}

static void pelion_setup(void)
{
	if (net_connected && objects_created) {
		cloud_client.setup(NULL);
	}
}

static void update_connection_state(void)
{
	if (net_connected) {
		cloud_client.resume(NULL);
	} else {
		cloud_client.pause();
	}
}

static void trigger_objects_creation(void)
{
	struct pelion_create_objects_event *event = new_pelion_create_objects_event();

	event->object_list = &object_list;

	EVENT_SUBMIT(event);
}

static void complete_objects_creation(void)
{
	cloud_client.add_objects(object_list);

	objects_created = true;
	pelion_setup();
}

static int pelion_init(void)
{
	cloud_client.init();

	cloud_client.on_error(on_client_error);
	cloud_client.on_status_changed(on_status_changed);

#if CONFIG_PELION_UPDATE
	cloud_client.set_update_authorize_priority_handler(on_update_authorize_priority);
	cloud_client.set_update_progress_handler(on_update_progress);
#endif

	set_pelion_state(PELION_STATE_INITIALIZED);

	trigger_objects_creation();

	return 0;
}

static bool handle_net_state_event(const struct net_state_event *event)
{
	net_connected = (event->state == NET_STATE_CONNECTED);

	if (IS_ENABLED(CONFIG_PELION_CLIENT_USE_APPLICATION_NETWORK_CALLBACK)
	    && net_status_cb) {
		net_status_cb(net_connected ? PAL_NETWORK_STATUS_CONNECTED :
			      PAL_NETWORK_STATUS_DISCONNECTED, net_cb_arg);
	}

	switch (pelion_state) {
	case PELION_STATE_INITIALIZED:
		pelion_setup();
		break;

	case PELION_STATE_REGISTERED:
	case PELION_STATE_UNREGISTERED:
		update_connection_state();
		break;

	case PELION_STATE_SUSPENDED:
	case PELION_STATE_DISABLED:
	default:
		/* Ignore */
		break;
	}

	return false;
}

static bool event_handler(const struct event_header *eh)
{
	if (is_module_state_event(eh)) {
		struct module_state_event *event = cast_module_state_event(eh);
		if (check_state(event, MODULE_ID(pelion_fcc), MODULE_STATE_READY)) {
			static bool initialized;

			__ASSERT_NO_MSG(!initialized);
			initialized = true;

			if (pelion_init()) {
				LOG_ERR("Cannot initialize");
				module_set_state(MODULE_STATE_ERROR);
			} else {
				module_set_state(MODULE_STATE_READY);
			}
		}

		return false;
	}

	if (is_net_state_event(eh)) {
		return handle_net_state_event(cast_net_state_event(eh));
	}

	if (is_pelion_create_objects_event(eh)) {
		complete_objects_creation();
		return false;
	}

	/* Event not handled but subscribed. */
	__ASSERT_NO_MSG(false);

	return false;
}

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, module_state_event);
EVENT_SUBSCRIBE(MODULE, net_state_event);
EVENT_SUBSCRIBE_FINAL(MODULE, pelion_create_objects_event);
