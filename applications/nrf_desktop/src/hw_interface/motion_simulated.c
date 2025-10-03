/* Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>

#include <app_event_manager.h>
#include "motion_event.h"
#include <caf/events/power_event.h>
#include "hid_event.h"

#include <zephyr/shell/shell.h>

#define MODULE motion
#include <caf/events/module_state_event.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_MOTION_LOG_LEVEL);

#define SCALE CONFIG_DESKTOP_MOTION_SIMULATED_SCALE_FACTOR

enum state {
	STATE_DISCONNECTED,
	STATE_IDLE,
	STATE_FETCHING,
	STATE_SUSPENDED,
	STATE_SUSPENDED_DISCONNECTED,
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
static enum state state;
/* If shell is enabled, wait for shell command to start generating motion.
 * Otherwise, start generating motion right after boot.
 */
static bool generating_motion = !IS_ENABLED(CONFIG_SHELL);


static void motion_event_send(int16_t dx, int16_t dy)
{
	struct motion_event *event = new_motion_event();

	event->dx = dx;
	event->dy = dy;
	APP_EVENT_SUBMIT(event);
}

static uint64_t get_timestamp(void)
{
	/* Use higher resolution if available. */
	if (IS_ENABLED(CONFIG_TIMER_HAS_64BIT_CYCLE_COUNTER)) {
		return k_cyc_to_us_near64(k_cycle_get_64());
	} else {
		return k_cyc_to_us_near64(k_cycle_get_32());
	}
}

static void generate_new_position(int *x_new, int *y_new)
{
	BUILD_ASSERT((edge_time & (edge_time - 1)) == 0,
			 "Edge time must be power of 2");

	if (generating_motion) {
		uint64_t t = get_timestamp();
		size_t v1_id = (t / edge_time) % ARRAY_SIZE(coords);
		size_t v2_id = (v1_id + 1) % ARRAY_SIZE(coords);

		const struct vertex *v1 = &coords[v1_id];
		const struct vertex *v2 = &coords[v2_id];

		int edge_pos = t % edge_time;

		*x_new = v1->x + (v2->x - v1->x) * edge_pos / edge_time;
		*y_new = v1->y + (v2->y - v1->y) * edge_pos / edge_time;
	} else {
		*x_new = x_cur;
		*y_new = y_cur;
	}
}

static void generate_motion_event(void)
{
	__ASSERT_NO_MSG(state == STATE_FETCHING);

	int x_new;
	int y_new;

	generate_new_position(&x_new, &y_new);

	motion_event_send(x_new - x_cur, y_new - y_cur);

	x_cur = x_new;
	y_cur = y_new;
}

static bool app_event_handler(const struct app_event_header *aeh)
{
	if (is_hid_report_subscription_event(aeh)) {
		const struct hid_report_subscription_event *event =
			cast_hid_report_subscription_event(aeh);

		if ((event->report_id == REPORT_ID_MOUSE) ||
		    (event->report_id == REPORT_ID_BOOT_MOUSE)) {
			static uint8_t peer_count;

			if (event->enabled) {
				__ASSERT_NO_MSG(peer_count < UCHAR_MAX);
				peer_count++;
			} else {
				__ASSERT_NO_MSG(peer_count > 0);
				peer_count--;
			}

			if ((peer_count == 1) && event->enabled) {
				/* The first HID subscriber connected. */
				if (state == STATE_DISCONNECTED) {
					state = generating_motion ?
						STATE_FETCHING : STATE_IDLE;

					if (state == STATE_FETCHING) {
						generate_motion_event();
					}
				} else {
					__ASSERT_NO_MSG(state == STATE_SUSPENDED_DISCONNECTED);
					state = STATE_SUSPENDED;
				}
			} else if ((peer_count == 0) && !event->enabled) {
				/* All of the HID subscribers disconnected. */
				if ((state == STATE_FETCHING) || (state == STATE_IDLE)) {
					state = STATE_DISCONNECTED;
				} else {
					__ASSERT_NO_MSG(state == STATE_SUSPENDED);
					state = STATE_SUSPENDED_DISCONNECTED;
				}
			}
		}

		return false;
	}

	if (is_hid_report_sent_event(aeh)) {
		const struct hid_report_sent_event *event = cast_hid_report_sent_event(aeh);

		if ((event->report_id == REPORT_ID_MOUSE) ||
		    (event->report_id == REPORT_ID_BOOT_MOUSE)) {
			if (state == STATE_FETCHING) {
				generate_motion_event();
				if (!generating_motion) {
					state = STATE_IDLE;
				}
			}
		}

		return false;
	}

	if (is_module_state_event(aeh)) {
		struct module_state_event *event = cast_module_state_event(aeh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			/* Ensure proper synchronization of the shell commands. */
			BUILD_ASSERT(!IS_ENABLED(CONFIG_SHELL) || !IS_ENABLED(CONFIG_SMP));
			__ASSERT_NO_MSG(!IS_ENABLED(CONFIG_SHELL) || !k_is_preempt_thread());

			LOG_INF("Simulated motion: ready");
		}

		return false;
	}

	if (IS_ENABLED(CONFIG_DESKTOP_MOTION_PM_EVENTS) &&
	    is_power_down_event(aeh)) {
		if (state == STATE_DISCONNECTED) {
			state = STATE_SUSPENDED_DISCONNECTED;
		} else if ((state == STATE_IDLE) || (state == STATE_FETCHING)) {
			state = STATE_SUSPENDED;
		}

		return false;
	}

	if (IS_ENABLED(CONFIG_DESKTOP_MOTION_PM_EVENTS) &&
	    is_wake_up_event(aeh)) {
		if (state == STATE_SUSPENDED_DISCONNECTED) {
			state = STATE_DISCONNECTED;
		} else if (state == STATE_SUSPENDED) {
			state = generating_motion ? STATE_FETCHING : STATE_IDLE;

			if (state == STATE_FETCHING) {
				generate_motion_event();
			}
		}

		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, module_state_event);
#if CONFIG_DESKTOP_MOTION_PM_EVENTS
APP_EVENT_SUBSCRIBE(MODULE, power_down_event);
APP_EVENT_SUBSCRIBE(MODULE, wake_up_event);
#endif
APP_EVENT_SUBSCRIBE(MODULE, hid_report_sent_event);
APP_EVENT_SUBSCRIBE(MODULE, hid_report_subscription_event);

#if CONFIG_SHELL

static int start_motion(const struct shell *shell, size_t argc, char **argv)
{
	k_sched_lock();

	if (!generating_motion) {
		generating_motion = true;
		shell_print(shell, "Started generating simulated motion");

		if (state == STATE_IDLE) {
			state = STATE_FETCHING;
			generate_motion_event();
		}
	}

	k_sched_unlock();

	return 0;
}

static int stop_motion(const struct shell *shell, size_t argc, char **argv)
{
	k_sched_lock();

	if (generating_motion) {
		generating_motion = false;
		shell_print(shell, "Stopped generating simulated motion");
	}

	k_sched_unlock();

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_motion_sim,
	SHELL_CMD(start, NULL, "Start motion", start_motion),
	SHELL_CMD(stop, NULL, "Stop motion", stop_motion),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(motion_sim, &sub_motion_sim, "Simulated motion commands", NULL);

#endif /* CONFIG_SHELL */
