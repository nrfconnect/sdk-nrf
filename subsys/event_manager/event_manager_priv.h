/*
 * Copyright (c) 2018-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/* Event manager private header.
 *
 * Although these defines are globally visible they must not be used directly.
 */

#ifndef _EVENT_MANAGER_PRIV_H_
#define _EVENT_MANAGER_PRIV_H_

#ifdef __cplusplus
extern "C" {
#endif


/* Macros related to sorting of elements in the event subscribers section. */

/* Markers used for ordering elements in subscribers array for each event type.
 * To ensure ordering markers should go in alphabetical order.
 */

#define _EM_MARKER_ARRAY_START   _a
#define _EM_MARKER_FIRST_ELEMENT _b
#define _EM_MARKER_PRIO_ELEMENTS _p
#define _EM_MARKER_FINAL_ELEMENT _y
#define _EM_MARKER_ARRAY_END     _z


/* Macro expanding ordering level into string.
 * The level must be between 00 and 99. Leading zero is required to ensure
 * proper sorting.
 */
#define _EM_SUBS_PRIO_ID(level) _CONCAT(_CONCAT(_EM_MARKER_PRIO_ELEMENTS, level), _)

/* There are 2 default ordering levels of event subscribers. */
#define _EM_SUBS_PRIO_EARLY  05
#define _EM_SUBS_PRIO_NORMAL 10


/* Convenience macros generating section names. */

#define _EVENT_SUBSCRIBERS_SECTION_PREFIX(ename, marker) \
	_CONCAT(_CONCAT(event_subscribers_, ename), marker)

#define _EVENT_SUBSCRIBERS_SECTION_NAME(ename, marker) \
	STRINGIFY(_EVENT_SUBSCRIBERS_SECTION_PREFIX(ename, marker))


/* Macros related to subscriber array tags
 * Each tag is a zero-length element that which is placed by linker at the start
 * and the end of the subscriber array respectively.
 */

/* Convenience macro generating tag name. */
#define _EM_TAG_NAME(prefix) _CONCAT(prefix, _tag)

/* Zero-length subscriber to be used as a tag. */
#define _EVENT_SUBSCRIBERS_TAG(ename, marker)							\
	const struct {} _EM_TAG_NAME(_EVENT_SUBSCRIBERS_SECTION_PREFIX(ename, marker))		\
	__used __aligned(__alignof(struct event_subscriber))					\
	__attribute__((__section__(_EVENT_SUBSCRIBERS_SECTION_NAME(ename, marker)))) = {};

/* Macro defining subscriber array boundary tags. */
#define _EVENT_SUBSCRIBERS_ARRAY_TAGS(ename)			\
	_EVENT_SUBSCRIBERS_TAG(ename, _EM_MARKER_ARRAY_START)	\
	_EVENT_SUBSCRIBERS_TAG(ename, _EM_MARKER_ARRAY_END)

/* Pointer to the first element of subscriber array for a given event type. */
#define _EVENT_SUBSCRIBERS_START_TAG(ename)							\
	((const struct event_subscriber *)							\
	 &_EM_TAG_NAME(_EVENT_SUBSCRIBERS_SECTION_PREFIX(ename, _EM_MARKER_ARRAY_START))	\
	 )

/* Pointer to the element past the last element of subscriber array for a given event type. */
#define _EVENT_SUBSCRIBERS_END_TAG(ename)							\
	((const struct event_subscriber *)							\
	 &_EM_TAG_NAME(_EVENT_SUBSCRIBERS_SECTION_PREFIX(ename, _EM_MARKER_ARRAY_END))		\
	 )


/* Subscribe a listener to an event. */
#define _EVENT_SUBSCRIBE(lname, ename, prio)							\
	const struct event_subscriber _CONCAT(_CONCAT(__event_subscriber_, ename), lname)	\
	__used __aligned(__alignof(struct event_subscriber))					\
	__attribute__((__section__(_EVENT_SUBSCRIBERS_SECTION_NAME(ename, prio)))) = {		\
		.listener = &_CONCAT(__event_listener_, lname),					\
	}


/* Pointer to event type definition is used as event type identifier. */
#define _EVENT_ID(ename) (&_CONCAT(__event_type_, ename))


/* Macro generates a function of name new_ename where ename is provided as
 * an argument. Allocator function is used to create an event of the given
 * ename type.
 */
#define _EVENT_ALLOCATOR_FN(ename)						\
	static inline struct ename *_CONCAT(new_, ename)(void)			\
	{									\
		struct ename *event =						\
			(struct ename *)event_manager_alloc(sizeof(*event));	\
		BUILD_ASSERT(offsetof(struct ename, header) == 0,		\
				 "");						\
		event->header.type_id = _EVENT_ID(ename);			\
		return event;							\
	}


/* Macro generates a function of name new_ename where ename is provided as
 * an argument. Allocator function is used to create an event of the given
 * ename type.
 */
#define _EVENT_ALLOCATOR_DYNDATA_FN(ename)						\
	static inline struct ename *_CONCAT(new_, ename)(size_t size)			\
	{										\
		struct ename *event =							\
			(struct ename *)event_manager_alloc(sizeof(*event) + size);	\
		BUILD_ASSERT((offsetof(struct ename, dyndata) +				\
				  sizeof(event->dyndata.size)) ==			\
				 sizeof(*event), "");					\
		BUILD_ASSERT(offsetof(struct ename, header) == 0,			\
				 "");							\
		event->header.type_id = _EVENT_ID(ename);				\
		event->dyndata.size = size;						\
		return event;								\
	}


/* Macro generates a function of name cast_ename where ename is provided as
 * an argument. Casting function is used to convert event_header pointer
 * into pointer to event matching the given ename type.
 */
#define _EVENT_CASTER_FN(ename)									\
	static inline struct ename *_CONCAT(cast_, ename)(const struct event_header *eh)	\
	{											\
		struct ename *event = NULL;							\
		if (eh->type_id == _EVENT_ID(ename)) {						\
			event = CONTAINER_OF(eh, struct ename, header);				\
		}										\
		return event;									\
	}


/* Macro generates a function of name is_ename where ename is provided as
 * an argument. Typecheck function is used to check if pointer to event_header
 * belongs to the event matching the given ename type.
 */
#define _EVENT_TYPECHECK_FN(ename) \
	static inline bool _CONCAT(is_, ename)(const struct event_header *eh)	\
	{									\
		return (eh->type_id == _EVENT_ID(ename));			\
	}



/* Declarations and definitions - for more details refer to public API. */
#define _EVENT_LISTENER(lname, notification_fn)						\
	STRUCT_SECTION_ITERABLE(event_listener, _CONCAT(__event_listener_, lname)) = {	\
		.name = STRINGIFY(lname),						\
		.notification = (notification_fn),					\
	}


#define _EVENT_TYPE_DECLARE_COMMON(ename)				\
	extern Z_DECL_ALIGN(struct event_type) _CONCAT(__event_type_, ename);		\
	_EVENT_CASTER_FN(ename);					\
	_EVENT_TYPECHECK_FN(ename)


#define _EVENT_TYPE_DECLARE(ename)					\
	_EVENT_TYPE_DECLARE_COMMON(ename);				\
	_EVENT_ALLOCATOR_FN(ename)


#define _EVENT_TYPE_DYNDATA_DECLARE(ename)				\
	_EVENT_TYPE_DECLARE_COMMON(ename);				\
	_EVENT_ALLOCATOR_DYNDATA_FN(ename)


#define _EVENT_TYPE_DEFINE(ename, init_log_en, log_fn, trace_data_pointer)		\
	_EVENT_SUBSCRIBERS_ARRAY_TAGS(ename);						\
	STRUCT_SECTION_ITERABLE(event_type, _CONCAT(__event_type_, ename)) = {		\
		.name            = STRINGIFY(ename),					\
		.subs_start      = _EVENT_SUBSCRIBERS_START_TAG(ename),			\
		.subs_stop       = _EVENT_SUBSCRIBERS_END_TAG(ename),			\
		.init_log_enable = init_log_en,						\
		.log_event       = (IS_ENABLED(CONFIG_LOG) ? (log_fn) : (NULL)),	\
		.trace_data      = (IS_ENABLED(CONFIG_EVENT_MANAGER_PROFILER_TRACER) ?	\
					(trace_data_pointer) : (NULL)),			\
	}


/**
 * @brief Bitmask indicating event is displayed.
 */
struct event_manager_event_display_bm {
	ATOMIC_DEFINE(flags, CONFIG_EVENT_MANAGER_MAX_EVENT_CNT);
};

extern struct event_manager_event_display_bm _event_manager_event_display_bm;

#ifdef __cplusplus
}
#endif

#endif /* _EVENT_MANAGER_PRIV_H_ */
