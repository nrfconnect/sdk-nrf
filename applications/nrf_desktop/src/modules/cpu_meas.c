/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <debug/cpu_load.h>

#define MODULE cpu_meas
#include "module_state_event.h"

#include "cpu_load_event.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_CPU_MEAS_LOG_LEVEL);

static struct k_delayed_work cpu_load_read;


static void send_cpu_load_event(uint32_t load)
{
	struct cpu_load_event *event = new_cpu_load_event();

	event->load = load;
	EVENT_SUBMIT(event);
}

static void cpu_load_read_fn(struct k_work *work)
{
	send_cpu_load_event(cpu_load_get());
	cpu_load_reset();

	k_delayed_work_submit(&cpu_load_read,
		K_MSEC(CONFIG_DESKTOP_CPU_MEAS_PERIOD));
}

static void init(void)
{
	/* When this option is enabled, CPU load measurement is periodically
	 * resetted. Only cpu_meas module should reset the measurement.
	 */
	BUILD_ASSERT(!IS_ENABLED(CONFIG_CPU_LOAD_LOG_PERIODIC));

	static bool initialized;

	__ASSERT_NO_MSG(!initialized);
	initialized = true;

	k_delayed_work_init(&cpu_load_read, cpu_load_read_fn);
	int err = cpu_load_init();

	if (err) {
		LOG_ERR("CPU load init failed, err: %d", err);
		module_set_state(MODULE_STATE_ERROR);
	} else {
		k_delayed_work_submit(&cpu_load_read,
			K_MSEC(CONFIG_DESKTOP_CPU_MEAS_PERIOD));
		module_set_state(MODULE_STATE_READY);
	}
}

static bool event_handler(const struct event_header *eh)
{
	if (is_module_state_event(eh)) {
		struct module_state_event *event = cast_module_state_event(eh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			init();
		}

		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, module_state_event);
