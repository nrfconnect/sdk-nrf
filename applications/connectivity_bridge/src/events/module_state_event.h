/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _MODULE_STATE_EVENT_H_
#define _MODULE_STATE_EVENT_H_

/**
 * @brief Module Event
 * @defgroup module_state_event Module State Event
 * @{
 */

#include <string.h>
#include <zephyr/toolchain.h>

#include <app_event_manager.h>
#include <app_event_manager_profiler_tracer.h>

#ifdef __cplusplus
extern "C" {
#endif


/** Module state list. */
#define MODULE_STATE_LIST	\
	X(READY)		\
	X(OFF)			\
	X(STANDBY)		\
	X(ERROR)

/** Module states. */
enum module_state {
#define X(name) _CONCAT(MODULE_STATE_, name),
	MODULE_STATE_LIST
#undef X

	MODULE_STATE_COUNT
};

/** Module event. */
struct module_state_event {
	struct app_event_header header;

	const void *module_id;
	enum module_state state;
};

APP_EVENT_TYPE_DECLARE(module_state_event);


#if defined(MODULE)

#define MODULE_NAME STRINGIFY(MODULE)

const void * const _CONCAT(__module_, MODULE) = MODULE_NAME;

static inline void module_set_state(enum module_state state)
{
	__ASSERT_NO_MSG(state < MODULE_STATE_COUNT);

	struct module_state_event *event = new_module_state_event();

	event->module_id = _CONCAT(__module_, MODULE);
	event->state = state;
	APP_EVENT_SUBMIT(event);
}

#endif


static inline bool check_state(const struct module_state_event *event,
		const void *module_id, enum module_state state)
{
	if ((event->module_id == module_id) && (event->state == state)) {
		return true;
	}
	return false;
}


#define MODULE_ID(mname) ({							\
			extern const void * const _CONCAT(__module_, mname);	\
			_CONCAT(__module_, mname);				\
		})


#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _MODULE_STATE_EVENT_H_ */
