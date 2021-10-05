/*
 * Copyright (c) 2018-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _MODULE_STATE_EVENT_H_
#define _MODULE_STATE_EVENT_H_

/**
 * @file
 * @defgroup caf_module_state_event CAF Module State Event
 * @{
 * @brief CAF Module State Event.
 */

#include <sys/atomic.h>
#include <event_manager.h>
#include <event_manager_profiler.h>

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


/**
 * @brief Get number of modules.
 *
 * @return Number of modules.
 */
static inline size_t module_count(void)
{
	return (&MODULE_ID_LIST_STOP - &MODULE_ID_LIST_START);
}

/**
 * @brief Get ID of module with given index.
 *
 * @param[in] idx Index of the module.
 *
 * @return Module ID.
 */
static inline const void *module_id_get(size_t idx)
{
	if (idx >= module_count()) {
		return NULL;
	}
	return *((&MODULE_ID_LIST_START) + idx);
}

/** @brief Get index of module.
 *
 * For example, the MODULE_IDX(buttons) can be used to get module index of module named buttons.
 *
 * @param[in] mname Name of the module.
 *
 * @return Index of the module.
 */
#define MODULE_IDX(mname) ({                                        \
		MODULE_ID_PTR_VAR_EXTERN_DEC(mname);                \
		&MODULE_ID_PTR_VAR(mname) - &MODULE_ID_LIST_START;  \
	})

/**
 * @brief Structure that provides a flag for every module available in application.
 */
struct module_flags {
	ATOMIC_DEFINE(f, CONFIG_CAF_MODULES_FLAGS_COUNT);
};

/**
 * @brief Check if given module index is valid.
 *
 * @param[in] module_idx The index to check.
 *
 * @retval true  Index is valid.
 * @retval false Index is out of valid range.
 */
static inline bool module_check_id_valid(size_t module_idx)
{
	return (module_idx < module_count());
}

/**
 * @brief Check if there is 0 in all the flags
 *
 * @param[in] mf Pointer to module flags variable.
 *
 * @retval true  All the flags have value of 0.
 * @retval false Any of the flags is not 0.
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
 * @brief Test single module bit.
 *
 * @param[in] mf         Pointer to module flags variable.
 * @param[in] module_idx The index of the selected module.
 *
 * @retval true  The module bit in flags is set.
 * @retval false The module bit in flags is cleared.
 */
static inline bool module_flags_test_bit(const struct module_flags *mf, size_t module_idx)
{
	return atomic_test_bit(mf->f, module_idx);
}

/**
 * @brief Clear single module bit
 *
 * @param[in,out] mf		Pointer to module flags variable.
 * @param[in] module_idx	The index of the selected module.
 */
static inline void module_flags_clear_bit(struct module_flags *mf, size_t module_idx)
{
	atomic_clear_bit(mf->f, module_idx);
}

/**
 * @brief Set single module bit.
 *
 * @param[in,out] mf		Pointer to module flags variable.
 * @param[in] module_idx	The index of the selected module.
 */
static inline void module_flags_set_bit(struct module_flags *mf, size_t module_idx)
{
	atomic_set_bit(mf->f, module_idx);
}

/**
 * @brief Set single module bit to specified value
 *
 * @param[in,out] mf		Pointer to module flags variable.
 * @param[in] module_idx	The index of the selected module.
 * @param[in] val		The value to be set in a specified module's bit.
 */
static inline void module_flags_set_bit_to(struct module_flags *mf, size_t module_idx, bool val)
{
	atomic_set_bit_to(mf->f, module_idx, val);
}

/** Module states. */
enum module_state {
	/** Module is active. This state is reported when the module is initialized or woken up
	 *  after suspending.
	 */
	MODULE_STATE_READY,

	/** Module is suspended in reaction to @ref power_down_event. The module cannot submit
	 *  @ref wake_up_event.
	 */
	MODULE_STATE_OFF,

	/** Module is suspended in reaction to @ref power_down_event. The module can submit
	 *  @ref wake_up_event.
	 */
	MODULE_STATE_STANDBY,

	/** Module reported fatal error. */
	MODULE_STATE_ERROR,

	/** Number of module states. */
	MODULE_STATE_COUNT
};

/** @brief Module state event.
 *
 * The module state event is submitted by a module to inform that state of the module changed.
 * The @ref module_set_state can be used to submit the module state event for a module.
 * The @ref check_state can be used in event_handler to check if the event carries information that
 * selected module reported selected state. See @ref module_state for details about available
 * module states.
 *
 * Name of the module must be defined as MODULE before including the module_state_event.h header.
 * For example, "#define MODULE buttons" defines name of the module as buttons. The module name is
 * used to identify a module.
 *
 * Every application module must register for module state event and submit this event when state
 * of the module changes. An application module can initialize itself when all the required
 * application modules are initialized (report MODULE_STATE_READY). This ensures proper
 * initialization order of the application modules.
 */
struct module_state_event {
	/** Event header. */
	struct event_header header;

	/** ID of the module. */
	const void *module_id;

	/** New state of the module. */
	enum module_state state;
};

EVENT_TYPE_DECLARE(module_state_event);


/** @brief Check if the selected module reported the selected state.
 *
 * The function can be used in event handler to verify if received module state event informs that
 * selected module reported selected state. The @ref MODULE_ID can be used to get module ID of
 * module with selected name.
 *
 * @param[in] event		Poitner to handled module state event.
 * @param[in] module_id		ID of the selected module.
 * @param[in] state		Selected module state.
 *
 * @retval true  The module state event informs that selected module reported selected state.
 *		 Otherwise, false is returned.
 */
static inline bool check_state(const struct module_state_event *event,
		const void *module_id, enum module_state state)
{
	if ((event->module_id == module_id) && (event->state == state)) {
		return true;
	}
	return false;
}


/** @brief Get module ID.
 *
 * For example, the MODULE_ID(buttons) can be used to get module ID of module named buttons.
 *
 * @param[in] mname Name of the module.
 *
 * @return ID of the module.
 */
#define MODULE_ID(mname) ({                                  \
			MODULE_ID_PTR_VAR_EXTERN_DEC(mname); \
			MODULE_ID_PTR_VAR(mname);            \
		})


#ifdef __cplusplus
}
#endif

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


/** @brief Submit module state event to inform that state of the module changed.
 *
 * ID of the module is automatically assigned based on name that is defined as MODULE.
 *
 * @param[in] state New state of the module.
 */
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

/**
 * @}
 */
