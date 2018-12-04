/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef _SYSTEM_PROFILER_H_
#define _SYSTEM_PROFILER_H_

/**
 * @brief Profiler
 * @defgroup profiler Profiler
 *
 * Profiler module provides interface to log custom data while system
 * is running.
 *
 * Before sending information about event occurrence, corresponding event type
 * has to be registered. Event receives unique ID after it's registered - ID is
 * used when information about event occurrence is sent. Data attached to event
 * has to match data declared during event registration.
 *
 * Profiler may use various implementations. Currently SEGGER SystemView
 * protocol (desktop application from SEGGER may be used to visualize custom
 * events) and custom (Nordic) protocol are implemented.
 *
 * @warning Currently up to 32 events may be profiled.
 * @{
 */


#include <zephyr/types.h>

#ifndef CONFIG_MAX_NUMBER_OF_CUSTOM_EVENTS
#define CONFIG_MAX_NUMBER_OF_CUSTOM_EVENTS 0
#endif

/** @brief Set of flags for enabling/disabling profiling for given events.
 */
extern u32_t profiler_enabled_events;


/** @brief Number of events registered in profiler.
 */
extern u8_t profiler_num_events;


/** @brief Data types for logging in system profiler.
 */
enum profiler_arg {
	PROFILER_ARG_U8,
	PROFILER_ARG_S8,
	PROFILER_ARG_U16,
	PROFILER_ARG_S16,
	PROFILER_ARG_U32,
	PROFILER_ARG_S32,
	PROFILER_ARG_STRING,
	PROFILER_ARG_TIMESTAMP
};


/** @brief Buffer for event's data.
 *
 * Buffer required for data, which is send with event.
 */
struct log_event_buf {
#ifdef CONFIG_PROFILER
	/** Pointer to the end of payload */
	u8_t *payload;
	/** Array where payload is located before it is send */
	u8_t payload_start[CONFIG_PROFILER_CUSTOM_EVENT_BUF_LEN];
#endif
};


/** @brief Function to initialize system profiler module.
 *
 * @return Zero if successful
 */
#ifdef CONFIG_PROFILER
int profiler_init(void);
#else
static inline int profiler_init(void) {return 0; }
#endif


/** @brief Funciton to terminate profiler.
 */
#ifdef CONFIG_PROFILER
void profiler_term(void);
#else
static inline void profiler_term(void) {}
#endif

/** @brief Function to retrieve description of an event.
 *
 * @param profiler_event_id Event ID in profiler.
 *
 * @return Event description.
 */
#ifdef CONFIG_PROFILER
const char *profiler_get_event_descr(size_t profiler_event_id);
#else
static inline const char *profiler_get_event_descr(size_t profiler_event_id)
{
	return NULL;
}
#endif

/** @brief Function to check if profiling is enabled for given event.
 *
 * @param profiler_event_id Event ID in profiler.
 *
 * @return Logical value indicating if event is currently profiled.
 */
static inline bool is_profiling_enabled(size_t profiler_event_id)
{
	if (IS_ENABLED(CONFIG_PROFILER)) {
		__ASSERT_NO_MSG(profiler_event_id < CONFIG_MAX_NUMBER_OF_CUSTOM_EVENTS);
		return (profiler_enabled_events & BIT(profiler_event_id)) != 0;
	}
	return false;
}

/** @brief Function to register type of event in system profiler.
 *
 * @warning Function is thread safe, but not safe to use in interrupts.
 * @param name Name of event type.
 * @param args Names of data values send with event.
 * @param arg_types Types of data values send with event.
 * @param arg_cnt Number of data values send with event.
 *
 * @return ID given to event type in system profiler.
 */
#ifdef CONFIG_PROFILER
u16_t profiler_register_event_type(const char *name, const char **args,
				   const enum profiler_arg *arg_types,
				   u8_t arg_cnt);
#else
static inline u16_t profiler_register_event_type(const char *name,
			const char **args, const enum profiler_arg *arg_types,
			u8_t arg_cnt) {return 0; }
#endif


/** @brief Function to initialize buffer for events' data.
 *
 * @param buf Pointer to data buffer.
 */
#ifdef CONFIG_PROFILER
void profiler_log_start(struct log_event_buf *buf);
#else
static inline void profiler_log_start(struct log_event_buf *buf) {}
#endif


/** @brief Function to encode and add data to buffer.
 *
 * @warning Buffer has to be initialized with event_log_start function first.
 * @param data Data to add to buffer.
 * @param buf Pointer to data buffer.
 */
#ifdef CONFIG_PROFILER
void profiler_log_encode_u32(struct log_event_buf *buf, u32_t data);
#else
static inline void profiler_log_encode_u32(struct log_event_buf *buf,
					   u32_t data) {}
#endif


/** @brief Function to encode and add event's address in memory to buffer.
 *
 * Used for event identification
 * @warning Buffer has to be initialized with event_log_start function first.
 *
 * @param buf Pointer to data buffer.
 * @param mem_address Memory address to encode.
 */
#ifdef CONFIG_PROFILER
void profiler_log_add_mem_address(struct log_event_buf *buf,
				  const void *mem_address);
#else
static inline void profiler_log_add_mem_address(struct log_event_buf *buf,
						const void *mem_address) {}
#endif


/** @brief Function to send data added to buffer to host.
 *
 * @note This funciton only sends data which is already stored in buffer.
 * Data is added to buffer using event_log_add_32 and
 * event_log_add_mem_address functions.
 * @param event_type_id ID of event in system profiler.
 * It is given to event type while it is registered.
 * @param buf Pointer to data buffer.
 */
#ifdef CONFIG_PROFILER
void profiler_log_send(struct log_event_buf *buf, u16_t event_type_id);
#else
static inline void profiler_log_send(struct log_event_buf *buf,
				     u16_t event_type_id) {}
#endif


/**
 * @}
 */

#endif /* _SYSTEM_PROFILER_H_ */
