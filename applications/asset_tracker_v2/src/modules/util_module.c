/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <sys/reboot.h>
#include <logging/log.h>

#define MODULE util_module

#include "modules_common.h"
#include "events/app_module_event.h"
#include "events/cloud_module_event.h"
#include "events/data_module_event.h"
#include "events/sensor_module_event.h"
#include "events/util_module_event.h"
#include "events/gps_module_event.h"
#include "events/modem_module_event.h"
#include "events/ui_module_event.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_UTIL_MODULE_LOG_LEVEL);

struct util_msg_data {
	union {
		struct cloud_module_event cloud;
		struct ui_module_event ui;
		struct sensor_module_event sensor;
		struct data_module_event data;
		struct app_module_event app;
		struct gps_module_event gps;
		struct modem_module_event modem;
	} module;
};

/* Util module super states. */
static enum state_type {
	STATE_INIT,
	STATE_REBOOT_PENDING
} state;

/* Forward declarations. */
static void reboot_work_fn(struct k_work *work);
static void message_handler(struct util_msg_data *msg);
static void send_reboot_request(enum shutdown_reason reason);

/* Delayed work that is used to trigger a reboot. */
static K_WORK_DELAYABLE_DEFINE(reboot_work, reboot_work_fn);

static struct module_data self = {
	.name = "util",
	.msg_q = NULL,
};

/* Convenience functions used in internal state handling. */
static char *state2str(enum state_type new_state)
{
	switch (new_state) {
	case STATE_INIT:
		return "STATE_INIT";
	case STATE_REBOOT_PENDING:
		return "STATE_REBOOT_PENDING";
	default:
		return "Unknown";
	}
}

static void state_set(enum state_type new_state)
{
	if (new_state == state) {
		LOG_DBG("State: %s", state2str(state));
		return;
	}

	LOG_DBG("State transition %s --> %s",
		state2str(state),
		state2str(new_state));

	state = new_state;
}

/* Handlers */
static bool event_handler(const struct event_header *eh)
{
	if (is_modem_module_event(eh)) {
		struct modem_module_event *event = cast_modem_module_event(eh);
		struct util_msg_data util_msg = {
			.module.modem = *event
		};

		message_handler(&util_msg);
	}

	if (is_cloud_module_event(eh)) {
		struct cloud_module_event *event = cast_cloud_module_event(eh);
		struct util_msg_data util_msg = {
			.module.cloud = *event
		};

		message_handler(&util_msg);
	}

	if (is_gps_module_event(eh)) {
		struct gps_module_event *event = cast_gps_module_event(eh);
		struct util_msg_data util_msg = {
			.module.gps = *event
		};

		message_handler(&util_msg);
	}

	if (is_sensor_module_event(eh)) {
		struct sensor_module_event *event =
				cast_sensor_module_event(eh);
		struct util_msg_data util_msg = {
			.module.sensor = *event
		};

		message_handler(&util_msg);
	}

	if (is_ui_module_event(eh)) {
		struct ui_module_event *event = cast_ui_module_event(eh);
		struct util_msg_data util_msg = {
			.module.ui = *event
		};

		message_handler(&util_msg);
	}

	if (is_app_module_event(eh)) {
		struct app_module_event *event = cast_app_module_event(eh);
		struct util_msg_data util_msg = {
			.module.app = *event
		};

		message_handler(&util_msg);
	}

	if (is_data_module_event(eh)) {
		struct data_module_event *event = cast_data_module_event(eh);
		struct util_msg_data util_msg = {
			.module.data = *event
		};

		message_handler(&util_msg);
	}

	return false;
}

void bsd_recoverable_error_handler(uint32_t err)
{
	send_reboot_request(REASON_GENERIC);
}

void k_sys_fatal_error_handler(unsigned int reason, const z_arch_esf_t *esf)
{
	ARG_UNUSED(esf);

	LOG_PANIC();
	send_reboot_request(REASON_GENERIC);
}

/* Static module functions. */
static void reboot(void)
{
	LOG_ERR("Rebooting!");
#if !defined(CONFIG_DEBUG) && defined(CONFIG_REBOOT)
	LOG_PANIC();
	sys_reboot(0);
#else
	while (true) {
		k_cpu_idle();
	}
#endif
}

static void reboot_work_fn(struct k_work *work)
{
	reboot();
}

static void send_reboot_request(enum shutdown_reason reason)
{
	/* Flag ensuring that multiple reboot requests are not emitted
	 * upon an error from multiple modules.
	 */
	static bool error_signaled;

	if (!error_signaled) {
		struct util_module_event *util_module_event =
				new_util_module_event();

		util_module_event->type = UTIL_EVT_SHUTDOWN_REQUEST;
		util_module_event->reason = reason;

		k_work_reschedule(&reboot_work,
				      K_SECONDS(CONFIG_REBOOT_TIMEOUT));

		EVENT_SUBMIT(util_module_event);

		state_set(STATE_REBOOT_PENDING);

		error_signaled = true;
	}
}

/* This API should be called exactly once for each _SHUTDOWN_READY event received from active
 * modules in the application. When this API has been called a set number of times equal to the
 * number of active modules, a reboot will be scheduled.
 */
static void reboot_ack_check(uint32_t module_id)
{
	/* Reboot after a shorter timeout if all modules have acknowledged that they are ready
	 * to reboot, ensuring a graceful shutdown. If not all modules respond to the shutdown
	 * request, the application will be shut down after a longer duration scheduled upon the
	 * initial error event.
	 */
	if (modules_shutdown_register(module_id)) {
		LOG_WRN("All modules have ACKed the reboot request.");
		LOG_WRN("Reboot in 5 seconds.");
		k_work_reschedule(&reboot_work, K_SECONDS(5));
	}
}

/* Message handler for STATE_INIT. */
static void on_state_init(struct util_msg_data *msg)
{
	if (IS_EVENT(msg, cloud,  CLOUD_EVT_FOTA_DONE)) {
		send_reboot_request(REASON_FOTA_UPDATE);
	}

	if ((IS_EVENT(msg, cloud,  CLOUD_EVT_ERROR))	 ||
	    (IS_EVENT(msg, modem,  MODEM_EVT_ERROR))	 ||
	    (IS_EVENT(msg, sensor, SENSOR_EVT_ERROR))	 ||
	    (IS_EVENT(msg, gps,	   GPS_EVT_ERROR_CODE))	 ||
	    (IS_EVENT(msg, data,   DATA_EVT_ERROR))	 ||
	    (IS_EVENT(msg, app,	   APP_EVT_ERROR))	 ||
	    (IS_EVENT(msg, ui,	   UI_EVT_ERROR))) {
		send_reboot_request(REASON_GENERIC);
		return;
	}
}

/* Message handler for STATE_REBOOT_PENDING. */
static void on_state_reboot_pending(struct util_msg_data *msg)
{
	if (IS_EVENT(msg, cloud, CLOUD_EVT_SHUTDOWN_READY)) {
		reboot_ack_check(msg->module.cloud.data.id);
		return;
	}

	if (IS_EVENT(msg, modem, MODEM_EVT_SHUTDOWN_READY)) {
		reboot_ack_check(msg->module.modem.data.id);
		return;
	}

	if (IS_EVENT(msg, sensor, SENSOR_EVT_SHUTDOWN_READY)) {
		reboot_ack_check(msg->module.sensor.data.id);
		return;
	}

	if (IS_EVENT(msg, gps, GPS_EVT_SHUTDOWN_READY)) {
		reboot_ack_check(msg->module.gps.data.id);
		return;
	}

	if (IS_EVENT(msg, data, DATA_EVT_SHUTDOWN_READY)) {
		reboot_ack_check(msg->module.data.data.id);
		return;
	}

	if (IS_EVENT(msg, app, APP_EVT_SHUTDOWN_READY)) {
		reboot_ack_check(msg->module.app.data.id);
		return;
	}

	if (IS_EVENT(msg, ui, UI_EVT_SHUTDOWN_READY)) {
		reboot_ack_check(msg->module.ui.data.id);
		return;
	}
}

/* Message handler for all states. */
static void on_all_states(struct util_msg_data *msg)
{
	if (IS_EVENT(msg, app, APP_EVT_START)) {
		int err = module_start(&self);

		if (err) {
			LOG_ERR("Failed starting module, error: %d", err);
			send_reboot_request(REASON_GENERIC);
		}

		state_set(STATE_INIT);
	}
}

static void message_handler(struct util_msg_data *msg)
{
	switch (state) {
	case STATE_INIT:
		on_state_init(msg);
		break;
	case STATE_REBOOT_PENDING:
		on_state_reboot_pending(msg);
		break;
	default:
		LOG_WRN("Unknown utility module state.");
		break;
	}

	on_all_states(msg);
}

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE_EARLY(MODULE, app_module_event);
EVENT_SUBSCRIBE_EARLY(MODULE, modem_module_event);
EVENT_SUBSCRIBE_EARLY(MODULE, cloud_module_event);
EVENT_SUBSCRIBE_EARLY(MODULE, gps_module_event);
EVENT_SUBSCRIBE_EARLY(MODULE, ui_module_event);
EVENT_SUBSCRIBE_EARLY(MODULE, sensor_module_event);
EVENT_SUBSCRIBE_EARLY(MODULE, data_module_event);
