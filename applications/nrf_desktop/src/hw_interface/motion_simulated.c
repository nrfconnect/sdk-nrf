/* Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <sys/atomic.h>

#include "event_manager.h"
#include "motion_event.h"
#include "power_event.h"
#include "hid_event.h"

#include <shell/shell.h>
#include <shell/shell_rtt.h>

#define MODULE motion
#include "module_state_event.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_MOTION_LOG_LEVEL);

#define SCALE CONFIG_DESKTOP_MOTION_SIMULATED_SCALE_FACTOR

enum {
	STATE_IDLE,
	STATE_FETCHING,
	STATE_SUSPENDED,
};

struct vertex {
	int x;
	int y;
};

/* Coords sequence for generated trajectory */
const static struct vertex coords[] = {
	{-50 * SCALE,	0},
	{-35 * SCALE,	-35 * SCALE},
	{0,		-50 * SCALE},
	{35 * SCALE,	-35 * SCALE},
	{50 * SCALE,	0},
	{35 * SCALE,	35 * SCALE},
	{0,		50 * SCALE},
	{-35 * SCALE,	35 * SCALE},
};

/* Time for transition between two points from trajectory */
const static int edge_time = CONFIG_DESKTOP_MOTION_SIMULATED_EDGE_TIME;

static int x_cur;
static int y_cur;
static atomic_t state;
static atomic_t connected;


static void motion_event_send(s16_t dx, s16_t dy)
{
	struct motion_event *event = new_motion_event();

	event->dx = dx;
	event->dy = dy;
	EVENT_SUBMIT(event);
}

static void generate_motion_event(void)
{
	BUILD_ASSERT_MSG((edge_time & (edge_time - 1)) == 0,
			 "Edge time must be power of 2");

	u32_t t = k_uptime_get_32();
	size_t v1_id = (t / edge_time) % ARRAY_SIZE(coords);
	size_t v2_id = (v1_id + 1) % ARRAY_SIZE(coords);

	const struct vertex *v1 = &coords[v1_id];
	const struct vertex *v2 = &coords[v2_id];

	int edge_pos = t % edge_time;
	int x_new = v1->x + (v2->x - v1->x) * edge_pos / edge_time;
	int y_new = v1->y + (v2->y - v1->y) * edge_pos / edge_time;

	motion_event_send(x_new - x_cur, y_new - y_cur);

	x_cur = x_new;
	y_cur = y_new;
}

static bool event_handler(const struct event_header *eh)
{
	if (is_hid_report_subscription_event(eh)) {
		const struct hid_report_subscription_event *event =
			cast_hid_report_subscription_event(eh);

		if (event->report_id == REPORT_ID_MOUSE) {
			static u8_t peer_count;

			if (event->enabled) {
				__ASSERT_NO_MSG(peer_count < UCHAR_MAX);
				peer_count++;
			} else {
				__ASSERT_NO_MSG(peer_count > 0);
				peer_count--;
			}

			bool new_state = (peer_count != 0);
			bool old_state = atomic_set(&connected, new_state);

			if (old_state != new_state && new_state) {
				if (atomic_get(&state) == STATE_FETCHING) {
					generate_motion_event();
				}
			}
		}
		return false;
	}

	if (is_hid_report_sent_event(eh)) {
		const struct hid_report_sent_event *event =
			cast_hid_report_sent_event(eh);

		if ((event->report_id == REPORT_ID_MOUSE) &&
				(atomic_get(&state) == STATE_FETCHING)) {
			generate_motion_event();
		}

		return false;
	}

	if (is_module_state_event(eh)) {
		struct module_state_event *event = cast_module_state_event(eh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			atomic_set(&state, STATE_IDLE);
			LOG_INF("Simulated motion: ready");
		}

		return false;
	}

	if (is_power_down_event(eh)) {
		atomic_set(&state, STATE_SUSPENDED);

		return false;
	}

	if (is_wake_up_event(eh)) {
		atomic_set(&state, STATE_IDLE);

		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, power_down_event);
EVENT_SUBSCRIBE(MODULE, module_state_event);
EVENT_SUBSCRIBE(MODULE, wake_up_event);
EVENT_SUBSCRIBE(MODULE, hid_report_sent_event);
EVENT_SUBSCRIBE(MODULE, hid_report_subscription_event);


static int start_motion(const struct shell *shell, size_t argc, char **argv)
{
	if (!atomic_cas(&state, STATE_IDLE, STATE_FETCHING)) {
		shell_print(shell, "Simulated motion inactive"
				    " or already fetching");
		return 0;
	}
	if (atomic_get(&connected)) {
		generate_motion_event();
	}
	shell_print(shell, "Started generating simulated motion");

	return 0;
}

static int stop_motion(const struct shell *shell, size_t argc, char **argv)
{
	if (!atomic_cas(&state, STATE_FETCHING, STATE_IDLE)) {
		shell_print(shell, "Simulated motion is not fetching");
		return 0;
	}
	shell_print(shell, "Stopped generating simulated motion");

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_motion_sim,
	SHELL_CMD(start, NULL, "Start motion", start_motion),
	SHELL_CMD(stop, NULL, "Stop motion", stop_motion),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(motion_sim, &sub_motion_sim,
		   "Simulated motion commands", NULL);
