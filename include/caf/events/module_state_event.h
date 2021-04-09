/*
 * Copyright (c) 2018-2021 Nordic Semiconductor ASA
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

#include "event_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MODULE_ID_LIST_SECTION_PREFIX module_id_list

#define MODULE_ID_PTR_VAR(mname) _CONCAT(__module_, mname)
#define MODULE_ID_LIST_SECTION_NAME   STRINGIFY(MODULE_ID_LIST_SECTION_PREFIX)
#define MODULE_ID_LIST_START _CONCAT(__start_, MODULE_ID_LIST_SECTION_PREFIX)
#define MODULE_ID_LIST_STOP  _CONCAT(__stop_,  MODULE_ID_LIST_SECTION_PREFIX)
#define MODULE_ID_PTR_VAR_EXTERN_DEC(mname) \
	extern const void * const MODULE_ID_PTR_VAR(mname)

extern const void * const MODULE_ID_LIST_START;
extern const void * const MODULE_ID_LIST_STOP;

static inline size_t module_count(void)
{
	return (&MODULE_ID_LIST_STOP - &MODULE_ID_LIST_START);
}

static inline const void * const module_id_get(size_t idx)
{
	if (idx >= module_count()) {
		return NULL;
	}
	return *((&MODULE_ID_LIST_START) + idx);
}

#define MODULE_IDX(mname) ({                                        \
		MODULE_ID_PTR_VAR_EXTERN_DEC(mname);                \
		&MODULE_ID_PTR_VAR(mname) - &MODULE_ID_LIST_START;  \
	})


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
	struct event_header header;

	const void *module_id;
	enum module_state state;
};

EVENT_TYPE_DECLARE(module_state_event);


#if defined(MODULE)

#define MODULE_NAME STRINGIFY(MODULE)

MODULE_ID_PTR_VAR_EXTERN_DEC(MODULE);
const void * const MODULE_ID_PTR_VAR(MODULE)
	__used __attribute__((__section__(MODULE_ID_LIST_SECTION_NAME)))
	= MODULE_NAME;


static inline void module_set_state(enum module_state state)
{
	__ASSERT_NO_MSG(state < MODULE_STATE_COUNT);

	struct module_state_event *event = new_module_state_event();

	event->module_id = MODULE_ID_PTR_VAR(MODULE);
	event->state = state;
	EVENT_SUBMIT(event);
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


#define MODULE_ID(mname) ({                                  \
			MODULE_ID_PTR_VAR_EXTERN_DEC(mname); \
			MODULE_ID_PTR_VAR(mname);            \
		})


#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _MODULE_STATE_EVENT_H_ */
