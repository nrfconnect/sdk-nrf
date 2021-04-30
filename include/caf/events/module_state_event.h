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

#include <sys/atomic.h>
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

/**
 * @brief Structure that provides a flag for every module
 */
struct module_flags {
	ATOMIC_DEFINE(f, CONFIG_CAF_MODULES_FLAGS_COUNT);
};

/**
 * @brief Check if given module index is valid
 *
 * @param module_idx The index to check
 * @retval true index is valid
 * @retval false index is out of valid range
 */
static inline bool module_check_id_valid(size_t module_idx)
{
	return (module_idx < module_count());
}

/**
 * @brief Check if there is 0 in all the flags
 *
 * @param mf     Pointer to modules flags variable
 * @retval true  All the flags have value of 0
 * @retval false Any of the flags is not 0
 */
static inline bool module_flags_check_zero(const struct module_flags *mf)
{
	const atomic_t *target = mf->f;

	for (size_t n = ATOMIC_BITMAP_SIZE(CONFIG_CAF_MODULES_FLAGS_COUNT); n > 0; --n) {
		if (atomic_get(target++) != 0) {
			return false;
		}
	}
	return true;
}

/**
 * @brief Test single module bit
 *
 * @param mf         Pointer to modules flags variable
 * @param module_idx The index of the module whose bit should be tested
 * @retval true  The module bit in flags is set
 * @retval false The module bit in flags is cleared
 */
static inline bool module_flags_test_bit(const struct module_flags *mf, size_t module_idx)
{
	return atomic_test_bit(mf->f, module_idx);
}

/**
 * @brief Clear single module bit
 *
 * @param mf         Pointer to modules flags variable
 * @param module_idx The index of the module whose bit should be cleared
 */
static inline void module_flags_clear_bit(struct module_flags *mf, size_t module_idx)
{
	atomic_clear_bit(mf->f, module_idx);
}

/**
 * @brief Set single module bit
 *
 * @param mf         Pointer to modules flags variable
 * @param module_idx The index of the module whose bit should be set
 */
static inline void module_flags_set_bit(struct module_flags *mf, size_t module_idx)
{
	atomic_set_bit(mf->f, module_idx);
}

/**
 * @brief Set single module bit to specified value
 *
 * @param mf         Pointer to module flags variable
 * @param module_idx The index of the module whose bit should be set
 * @param val        The value whose should be set in a specified modules bit
 */
static inline void module_flags_set_bit_to(struct module_flags *mf, size_t module_idx, bool val)
{
	atomic_set_bit_to(mf->f, module_idx, val);
}

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

/* Declare elements required in the module C file,
 * even if the file was already included before.
 */
#if defined(MODULE) && !defined(MODULE_NAME)

#ifdef __cplusplus
extern "C" {
#endif

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

#ifdef __cplusplus
}
#endif

#endif /* defined(MODULE) && !defined(MODULE_NAME) */
