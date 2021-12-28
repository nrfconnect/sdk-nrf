/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Event manager header.
 */

#ifndef _EVENT_MANAGER_H_
#define _EVENT_MANAGER_H_


/**
 * @defgroup event_manager Event Manager
 * @brief Event Manager
 *
 * @{
 */

#include <zephyr.h>
#include <zephyr/types.h>
#include <sys/__assert.h>
#include <logging/log.h>

#include <event_manager_priv.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Create an event listener object.
 *
 * @param lname   Module name.
 * @param cb_fn  Pointer to the event handler function.
 */
#define EVENT_LISTENER(lname, cb_fn) _EVENT_LISTENER(lname, cb_fn)


/** @brief Subscribe a listener to an event type as first module that is
 *  being notified.
 *
 * @param lname  Name of the listener.
 * @param ename  Name of the event.
 */
#define EVENT_SUBSCRIBE_FIRST(lname, ename)							\
	_EVENT_SUBSCRIBE(lname, ename, _EM_MARKER_FIRST_ELEMENT);				\
	const struct {} _CONCAT(_CONCAT(__event_subscriber_, ename), first_sub_redefined) = {}


/** @brief Subscribe a listener to the early notification list for an
 *  event type.
 *
 * @param lname  Name of the listener.
 * @param ename  Name of the event.
 */
#define EVENT_SUBSCRIBE_EARLY(lname, ename) \
	_EVENT_SUBSCRIBE(lname, ename, _EM_SUBS_PRIO_ID(_EM_SUBS_PRIO_EARLY))


/** @brief Subscribe a listener to the normal notification list for an event
 *  type.
 *
 * @param lname  Name of the listener.
 * @param ename  Name of the event.
 */
#define EVENT_SUBSCRIBE(lname, ename) \
	_EVENT_SUBSCRIBE(lname, ename, _EM_SUBS_PRIO_ID(_EM_SUBS_PRIO_NORMAL))


/** @brief Subscribe a listener to an event type as final module that is
 *  being notified.
 *
 * @param lname  Name of the listener.
 * @param ename  Name of the event.
 */
#define EVENT_SUBSCRIBE_FINAL(lname, ename)							\
	_EVENT_SUBSCRIBE(lname, ename, _EM_MARKER_FINAL_ELEMENT);				\
	const struct {} _CONCAT(_CONCAT(__event_subscriber_, ename), final_sub_redefined) = {}


/** @brief Declare an event type.
 *
 * This macro provides declarations required for an event to be used
 * by other modules.
 *
 * @param ename  Name of the event.
 */
#define EVENT_TYPE_DECLARE(ename) _EVENT_TYPE_DECLARE(ename)


/** @brief Declare an event type with dynamic data size.
 *
 * This macro provides declarations required for an event to be used
 * by other modules.
 * Declared event will use dynamic data.
 *
 * @param ename  Name of the event.
 */
#define EVENT_TYPE_DYNDATA_DECLARE(ename) _EVENT_TYPE_DYNDATA_DECLARE(ename)


/** @brief Define an event type.
 *
 * This macro defines an event type. In addition, it defines functions
 * specific to the event type and the event type structure.
 *
 * For every defined event, the following functions are created, where
 * <i>%event_type</i> is replaced with the given event type name @p ename
 * (for example, button_event):
 * - new_<i>%event_type</i>  - Allocates an event of a given type.
 * - is_<i>%event_type</i>   - Checks if the event header that is provided
 *                            as argument represents the given event type.
 * - cast_<i>%event_type</i> - Casts the event header that is provided
 *                            as argument to an event of the given type.
 *
 * @param ename     	   Name of the event.
 * @param init_log_en	   Bool indicating if the event is logged
 *                         by default.
 * @param log_fn  	   Function to stringify an event of this type.
 * @param ev_info_struct   Data structure describing the event type.
 */
#define EVENT_TYPE_DEFINE(ename, init_log_en, log_fn, ev_info_struct) \
	_EVENT_TYPE_DEFINE(ename, init_log_en, log_fn, ev_info_struct)


/** @brief Verify if an event ID is valid.
 *
 * The pointer to an event type structure is used as its ID. This macro
 * validates that the provided pointer is within the range where event
 * type structures are defined.
 *
 * @param id  ID.
 */
#define ASSERT_EVENT_ID(id) \
	__ASSERT_NO_MSG((id >= _event_type_list_start) && (id < _event_type_list_end))




/** @brief Submit an event.
 *
 * This helper macro simplifies the event submission.
 *
 * @param event  Pointer to the event object.
 */
#define EVENT_SUBMIT(event) _event_submit(&event->header)


/** @brief Initialize the Event Manager.
 *
 * @retval 0 If the operation was successful. Error values can be added by overwriting
 *         event_manager_trace_event_init function.
 */
int event_manager_init(void);


/** @brief Trace event execution.
 * The behavior of this function depends on the actual implementation.
 * The default implementation of this function is no-operation.
 * It is annotated as weak and is meant to be overridden by layer
 * adding support for profiling mechanism.
 *
 * @param eh        Pointer to the event header of the event that is processed by Event Manager.
 * @param is_start  Bool value indicating if this occurrence is related
 *                  to start or end of the event processing.
 **/
void event_manager_trace_event_execution(const struct event_header *eh,
				  bool is_start);


/** @brief Trace event submission.
 * The behavior of this function depends on the actual implementation.
 * The default implementation of this function is no-operation.
 * It is annotated as weak and is meant to be overridden by layer
 * adding support for profiling mechanism.
 *
 * @param eh          Pointer to the event header of the event that is processed by Event Manager.
 * @param trace_info  Custom event description for profiling.
 **/
void event_manager_trace_event_submission(const struct event_header *eh,
				const void *trace_info);


/** @brief Initialize tracing in the Event Manager.
 * The behavior of this function depends on the actual implementation.
 * The default implementation of this function is no-operation.
 * It is annotated as weak and is meant to be overridden by layer
 * adding support for profiling mechanism.
 *
 * @retval 0 If the operation was successful. Error values can be added by
 *         overwriting this function.
 **/
int event_manager_trace_event_init(void);

/** @brief Allocate event.
 * The behavior of this function depends on the actual implementation.
 * The default implementation of this function is same as k_malloc.
 * It is annotated as weak and can be overridden by user.
 *
 * @param size  Amount of memory requested (in bytes).
 * @retval Address of the allocated memory if successful, otherwise NULL.
 **/
void *event_manager_alloc(size_t size);


/** @brief Free memory occupied by the event.
 * The behavior of this function depends on the actual implementation.
 * The default implementation of this function is same as k_free.
 * It is annotated as weak and can be overridden by user.
 *
 * @param addr  Pointer to previously allocated memory.
 **/
void event_manager_free(void *addr);


/** @brief Log event
 *
 * This helper macro simplifies event logging.
 *
 * @param eh  Pointer to the event header of the event that is processed by Event Manager.
 * @param ... `printf`- like format string and variadic list of arguments corresponding to
 *            the format string.
 */
#define EVENT_MANAGER_LOG(eh, ...) do {								\
	LOG_MODULE_DECLARE(event_manager, CONFIG_EVENT_MANAGER_LOG_LEVEL);			\
	if (IS_ENABLED(CONFIG_EVENT_MANAGER_LOG_EVENT_TYPE)) {					\
		LOG_INF("e:%s " GET_ARG_N(1, __VA_ARGS__), eh->type_id->name			\
			COND_CODE_0(NUM_VA_ARGS_LESS_1(__VA_ARGS__),				\
			    (),									\
			    (, GET_ARGS_LESS_N(1, __VA_ARGS__))					\
			));									\
	} else {										\
		LOG_INF(__VA_ARGS__);								\
	}											\
} while (0)


#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _EVENT_MANAGER_H_ */
