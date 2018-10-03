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
 * @{
 */


#include <zephyr/types.h>


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
	/** Pointer to the end of payload */
	u8_t *payload;
	/** Array where payload is located before it is send */
	u8_t payload_start[CONFIG_PROFILER_CUSTOM_EVENT_BUF_LEN];
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
