/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-5-Clause-Nordic
 */

#ifndef _MODULE_STATE_EVENT_H_
#define _MODULE_STATE_EVENT_H_

/**
 * @brief Module Event
 * @defgroup module_state_event Module State Event
 * @{
 */

#include <string.h>

#include "event_manager.h"

#ifdef __cplusplus
extern "C" {
#endif


/* Module states */
extern const void *MODULE_STATE_READY;
extern const void *MODULE_STATE_OFF;
extern const void *MODULE_STATE_STANDBY;
extern const void *MODULE_STATE_ERROR;


/* Module event */
struct module_state_event {
	struct event_header header;

	const void *module_id;
	const void *state;
};

EVENT_TYPE_DECLARE(module_state_event);


#if defined(MODULE)

#define MODULE_NAME STRINGIFY(MODULE)

const void *_CONCAT(__module_, MODULE) = MODULE_NAME;

static inline void module_set_state(const void *state)
{
	struct module_state_event *event = new_module_state_event();

	if (event) {
		event->module_id = _CONCAT(__module_, MODULE);
		event->state = state;
		EVENT_SUBMIT(event);
	}
}

#endif


static inline bool check_state(const struct module_state_event *event,
		const void *module_id, const void *state)
{
	if ((event->module_id == module_id) && (event->state == state)) {
		return true;
	}
	return false;
}


#define MODULE_ID(mname) ({					\
			extern void *_CONCAT(__module_, mname);	\
			_CONCAT(__module_, mname);		\
		})


#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _MODULE_STATE_EVENT_H_ */
