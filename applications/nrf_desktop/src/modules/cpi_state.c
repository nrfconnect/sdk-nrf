/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <misc/util.h>
#include <misc/byteorder.h>

#include "motion_sensor.h"

#include "button_event.h"
#include "config_event.h"
#include "power_event.h"

#define MODULE cpi_state
#include "module_state_event.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_CPI_STATE_LOG_LEVEL);

#define KEY_ID_INCREMENT CONFIG_DESKTOP_CPI_STATE_TOGGLE_UP_BUTTON_ID
#define KEY_ID_DECREMENT CONFIG_DESKTOP_CPI_STATE_TOGGLE_DOWN_BUTTON_ID

enum state {
	STATE_DISABLED,
	STATE_ACTIVE,
	STATE_OFF,
};

static enum state state;
static u32_t cpi = CONFIG_DESKTOP_MOTION_SENSOR_CPI;
static int step_cnt;
static struct config_event *sent_config_event;
static bool cpi_update_required;


static void send_cpi_config_event(u32_t cpi_send)
{
	__ASSERT_NO_MSG(sent_config_event == NULL);

	struct config_event *event = new_config_event(sizeof(cpi_send));

	event->id = SETUP_EVENT_ID(SETUP_MODULE_SENSOR, SENSOR_OPT_CPI);
	sys_put_le32(cpi_send, event->dyndata.data);
	event->store_needed = true;

	EVENT_SUBMIT(event);

	sent_config_event = event;
}

static void update_cpi(void)
{
	BUILD_ASSERT(MOTION_SENSOR_CPI_MIN > 0);
	BUILD_ASSERT(MOTION_SENSOR_CPI_MAX > MOTION_SENSOR_CPI_MIN);

	if ((state == STATE_OFF) || (sent_config_event != NULL)) {
		return;
	}

	int update = step_cnt * CONFIG_DESKTOP_CPI_STATE_STEP_CHANGE;
	int new_cpi = (int)cpi + update;

	if (new_cpi < (int)MOTION_SENSOR_CPI_MIN) {
		new_cpi = MOTION_SENSOR_CPI_MIN;
	} else if (new_cpi > (int)MOTION_SENSOR_CPI_MAX) {
		new_cpi = MOTION_SENSOR_CPI_MAX;
	}

	if (((u32_t) new_cpi != cpi) || cpi_update_required) {
		cpi = (u32_t) new_cpi;
		send_cpi_config_event(cpi);
	}

	cpi_update_required = false;
	step_cnt = 0;
}

static void power_down(void)
{
	state = STATE_OFF;
	module_set_state(MODULE_STATE_OFF);
}

static void wake_up(void)
{
	state = STATE_ACTIVE;
	module_set_state(MODULE_STATE_READY);
	update_cpi();
}

static bool is_my_config_id(u8_t config_id)
{
	return (GROUP_FIELD_GET(config_id) == EVENT_GROUP_SETUP) &&
	       (MOD_FIELD_GET(config_id) == SETUP_MODULE_SENSOR) &&
	       (OPT_FIELD_GET(config_id) == SENSOR_OPT_CPI);
}

static bool handle_button_event(const struct button_event *event)
{
	if (event->pressed) {
		if (unlikely(event->key_id == KEY_ID_INCREMENT)) {
			step_cnt++;
			update_cpi();
		} else if (unlikely(event->key_id == KEY_ID_DECREMENT)) {
			step_cnt--;
			update_cpi();
		}
	}

	return false;
}

static bool handle_config_event(const struct config_event *event)
{
	if (is_my_config_id(event->id)) {
		if (event->dyndata.size != sizeof(cpi)) {
			LOG_ERR("Invalid dyndata size");
			return false;
		}

		if (event == sent_config_event) {
			sent_config_event = NULL;
			update_cpi();
		} else {
			cpi = sys_get_le32(event->dyndata.data);
			step_cnt = 0;
			if (sent_config_event != NULL) {
				cpi_update_required = true;
			}
		}
	}

	return false;
}

static bool event_handler(const struct event_header *eh)
{
	if (is_button_event(eh)) {
		return handle_button_event(cast_button_event(eh));
	}

	if (is_config_event(eh)) {
		return handle_config_event(cast_config_event(eh));
	}

	if (is_module_state_event(eh)) {
		const struct module_state_event *event =
			cast_module_state_event(eh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			__ASSERT_NO_MSG(state == STATE_DISABLED);
			state = STATE_ACTIVE;
		}

		return false;
	}

	if (is_power_down_event(eh)) {
		if (state != STATE_OFF) {
			power_down();
		}

		return false;
	}

	if (is_wake_up_event(eh)) {
		if (state != STATE_ACTIVE) {
			wake_up();
		}

		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, module_state_event);
EVENT_SUBSCRIBE(MODULE, button_event);
EVENT_SUBSCRIBE(MODULE, power_down_event);
EVENT_SUBSCRIBE(MODULE, wake_up_event);
EVENT_SUBSCRIBE_FINAL(MODULE, config_event);
