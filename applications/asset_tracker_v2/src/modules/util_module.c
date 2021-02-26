/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <power/reboot.h>
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

static struct k_delayed_work reboot_work;

static struct module_data self = {
	.name = "util",
	.msg_q = NULL,
};

/* Forward declarations. */
static void message_handler(struct util_msg_data *msg);
static void send_reboot_request(void);

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
	send_reboot_request();
}

void k_sys_fatal_error_handler(unsigned int reason, const z_arch_esf_t *esf)
{
	ARG_UNUSED(esf);

	LOG_PANIC();
	send_reboot_request();
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

static void send_reboot_request(void)
{
	/* Flag ensuring that multiple reboot requests are not emitted
	 * upon an error from multiple modules.
	 */
	static bool error_signaled;

	if (!error_signaled) {
		struct util_module_event *util_module_event =
				new_util_module_event();

		util_module_event->type = UTIL_EVT_SHUTDOWN_REQUEST;

		k_delayed_work_submit(&reboot_work,
				      K_SECONDS(CONFIG_REBOOT_TIMEOUT));

		EVENT_SUBMIT(util_module_event);

		error_signaled = true;
	}
}

/* Message handler for all states. */
static void on_all_states(struct util_msg_data *msg)
{
	static int reboot_ack_cnt;

	if (is_cloud_module_event(&msg->module.cloud.header)) {
		switch (msg->module.cloud.type) {
		case CLOUD_EVT_ERROR:
			/* Fall through. */
		case CLOUD_EVT_FOTA_DONE:
			send_reboot_request();
			break;
		case CLOUD_EVT_SHUTDOWN_READY:
			reboot_ack_cnt++;
			break;
		default:
			break;
		}
	}

	if (is_modem_module_event(&msg->module.modem.header)) {
		switch (msg->module.modem.type) {
		case MODEM_EVT_ERROR:
			send_reboot_request();
			break;
		case MODEM_EVT_SHUTDOWN_READY:
			reboot_ack_cnt++;
			break;
		default:
			break;
		}
	}

	if (is_sensor_module_event(&msg->module.sensor.header)) {
		switch (msg->module.sensor.type) {
		case SENSOR_EVT_ERROR:
			send_reboot_request();
			break;
		case SENSOR_EVT_SHUTDOWN_READY:
			reboot_ack_cnt++;
			break;
		default:
			break;
		}
	}

	if (is_gps_module_event(&msg->module.gps.header)) {
		switch (msg->module.gps.type) {
		case GPS_EVT_ERROR_CODE:
			send_reboot_request();
			break;
		case GPS_EVT_SHUTDOWN_READY:
			reboot_ack_cnt++;
			break;
		default:
			break;
		}
	}

	if (is_data_module_event(&msg->module.data.header)) {
		switch (msg->module.data.type) {
		case DATA_EVT_ERROR:
			send_reboot_request();
			break;
		case DATA_EVT_SHUTDOWN_READY:
			reboot_ack_cnt++;
			break;
		default:
			break;
		}
	}

	if (is_app_module_event(&msg->module.app.header)) {
		switch (msg->module.app.type) {
		case APP_EVT_ERROR:
			send_reboot_request();
			break;
		case APP_EVT_SHUTDOWN_READY:
			reboot_ack_cnt++;
			break;
		default:
			break;
		}
	}

	if (is_ui_module_event(&msg->module.ui.header)) {
		switch (msg->module.ui.type) {
		case UI_EVT_ERROR:
			send_reboot_request();
			break;
		case UI_EVT_SHUTDOWN_READY:
			reboot_ack_cnt++;
			break;
		default:
			break;
		}
	}

	/* Reboot if after a shorter timeout if all modules has acknowledged
	 * that the application is ready to shutdown. This ensures a graceful
	 * shutdown.
	 */
	if (reboot_ack_cnt >= module_active_count_get() - 1) {
		LOG_WRN("All modules have ACKed the reboot request.");
		LOG_WRN("Reboot in 5 seconds.");
		k_delayed_work_submit(&reboot_work, K_SECONDS(5));
	}
}

static void message_handler(struct util_msg_data *msg)
{
	if (IS_EVENT(msg, app, APP_EVT_START)) {
		module_start(&self);
		state_set(STATE_INIT);
		k_delayed_work_init(&reboot_work, reboot_work_fn);
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
