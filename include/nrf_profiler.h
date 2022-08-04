/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _NRF_PROFILER_H_
#define _NRF_PROFILER_H_

/**
 * @defgroup nrf_profiler Profiler
 * @brief Profiler
 *
 * @{
 */


#include <zephyr/types.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/__assert.h>

#ifndef CONFIG_NRF_PROFILER_MAX_NUMBER_OF_APP_EVENTS
/** Maximum number of events. */
#define CONFIG_NRF_PROFILER_MAX_NUMBER_OF_APP_EVENTS 0
#endif

#ifndef CONFIG_NRF_PROFILER_NUMBER_OF_INTERNAL_EVENTS
/** Number of internal events. */
#define CONFIG_NRF_PROFILER_NUMBER_OF_INTERNAL_EVENTS 0
#endif

/** Maximum number of events including user application events and internal events. */
#define NRF_PROFILER_MAX_NUMBER_OF_APPLICATION_AND_INTERNAL_EVENTS \
	((CONFIG_NRF_PROFILER_MAX_NUMBER_OF_APP_EVENTS) + \
	(CONFIG_NRF_PROFILER_NUMBER_OF_INTERNAL_EVENTS))

/** @brief Bitmask indicating event is enabled.
 * This structure is private to nrf_profiler and should not be referred from outside.
 */
struct nrf_profiler_event_enabled_bm {
	ATOMIC_DEFINE(flags, NRF_PROFILER_MAX_NUMBER_OF_APPLICATION_AND_INTERNAL_EVENTS);
};

/** @brief Set of flags for enabling/disabling profiling for given event types.
 */
extern struct nrf_profiler_event_enabled_bm _nrf_profiler_event_enabled_bm;


/** @brief Number of event types registered in the Profiler.
 */
extern uint8_t nrf_profiler_num_events;


/** @brief Data types for profiling.
 */
enum nrf_profiler_arg {
	NRF_PROFILER_ARG_U8,
	NRF_PROFILER_ARG_S8,
	NRF_PROFILER_ARG_U16,
	NRF_PROFILER_ARG_S16,
	NRF_PROFILER_ARG_U32,
	NRF_PROFILER_ARG_S32,
	NRF_PROFILER_ARG_STRING,
	NRF_PROFILER_ARG_TIMESTAMP
};


/** @brief Buffer required for data that is sent with the event.
 */
struct log_event_buf {
#ifdef CONFIG_NRF_PROFILER
	/** Pointer to the end of the payload. */
	uint8_t *payload;
	/** Array where the payload is located before it is sent. */
	uint8_t payload_start[CONFIG_NRF_PROFILER_CUSTOM_EVENT_BUF_LEN];
#endif
};


/** @brief Initialize the Profiler.
 *
 * @note This function is thread-safe, but not safe to use in
 *       interrupts.
 *
 * @retval 0 If the operation was successful.
 */
#ifdef CONFIG_NRF_PROFILER
int nrf_profiler_init(void);
#else
static inline int nrf_profiler_init(void) {return 0; }
#endif


/** @brief Terminate the Profiler.
 */
#ifdef CONFIG_NRF_PROFILER
void nrf_profiler_term(void);
#else
static inline void nrf_profiler_term(void) {}
#endif

/** @brief Retrieve the description of an event type.
 *
 * @param nrf_profiler_event_id Event ID.
 *
 * @return Event description.
 */
#ifdef CONFIG_NRF_PROFILER
const char *nrf_profiler_get_event_descr(size_t nrf_profiler_event_id);
#else
static inline const char *nrf_profiler_get_event_descr(size_t nrf_profiler_event_id)
{
	return NULL;
}
#endif

/** @brief Check if profiling is enabled for a given event type.
 *
 * @param nrf_profiler_event_id Event ID.
 *
 * @return Logical value indicating if the event type is currently profiled.
 */
static inline bool is_profiling_enabled(size_t nrf_profiler_event_id)
{
	if (IS_ENABLED(CONFIG_NRF_PROFILER)) {
		__ASSERT_NO_MSG(nrf_profiler_event_id <
					NRF_PROFILER_MAX_NUMBER_OF_APPLICATION_AND_INTERNAL_EVENTS);
		return atomic_test_bit(_nrf_profiler_event_enabled_bm.flags, nrf_profiler_event_id);
	}
	return false;
}

/** @brief Register an event type.
 *
 * @note This function is thread-safe, but not safe to use in
 *       interrupts.
 *
 * @param name Name of the event type.
 * @param args Names of data values sent with the event.
 * @param arg_types Types of data values sent with the event.
 * @param arg_cnt Number of data values sent with the event.
 *
 * @return ID assigned to the event type.
 */
#ifdef CONFIG_NRF_PROFILER
uint16_t nrf_profiler_register_event_type(const char *name, const char * const *args,
				   const enum nrf_profiler_arg *arg_types,
				   uint8_t arg_cnt);
#else
static inline uint16_t nrf_profiler_register_event_type(const char *name,
			const char * const *args, const enum nrf_profiler_arg *arg_types,
			uint8_t arg_cnt) {return 0; }
#endif


/** @brief Initialize a buffer for the data of an event.
 *
 * @param buf Pointer to the data buffer.
 */
#ifdef CONFIG_NRF_PROFILER
void nrf_profiler_log_start(struct log_event_buf *buf);
#else
static inline void nrf_profiler_log_start(struct log_event_buf *buf) {}
#endif

/** @brief Encode and add uint32_t data type to a buffer.
 *
 * @note The buffer must be initialized with @ref nrf_profiler_log_start
 *       before calling this function.
 *
 * @param buf Pointer to the data buffer.
 * @param data Data to add to the buffer.
 */
#ifdef CONFIG_NRF_PROFILER
void nrf_profiler_log_encode_uint32(struct log_event_buf *buf, uint32_t data);
#else
static inline void nrf_profiler_log_encode_uint32(struct log_event_buf *buf,
					      uint32_t data) {}
#endif

/** @brief Encode and add int32_t data type to a buffer.
 *
 * @note The buffer must be initialized with @ref nrf_profiler_log_start
 *       before calling this function.
 *
 * @param buf Pointer to the data buffer.
 * @param data Data to add to the buffer.
 */
#ifdef CONFIG_NRF_PROFILER
void nrf_profiler_log_encode_int32(struct log_event_buf *buf, int32_t data);
#else
static inline void nrf_profiler_log_encode_int32(struct log_event_buf *buf,
					     int32_t data) {}
#endif

/** @brief Encode and add uint16_t data type to a buffer.
 *
 * @note The buffer must be initialized with @ref nrf_profiler_log_start
 *       before calling this function.
 *
 * @param buf Pointer to the data buffer.
 * @param data Data to add to the buffer.
 */
#ifdef CONFIG_NRF_PROFILER
void nrf_profiler_log_encode_uint16(struct log_event_buf *buf, uint16_t data);
#else
static inline void nrf_profiler_log_encode_uint16(struct log_event_buf *buf,
					      uint16_t data) {}
#endif

/** @brief Encode and add int16_t data type to a buffer.
 *
 * @note The buffer must be initialized with @ref nrf_profiler_log_start
 *       before calling this function.
 *
 * @param buf Pointer to the data buffer.
 * @param data Data to add to the buffer.
 */
#ifdef CONFIG_NRF_PROFILER
void nrf_profiler_log_encode_int16(struct log_event_buf *buf, int16_t data);
#else
static inline void nrf_profiler_log_encode_int16(struct log_event_buf *buf,
					     int16_t data) {}
#endif

/** @brief Encode and add uint8_t data type to a buffer.
 *
 * @note The buffer must be initialized with @ref nrf_profiler_log_start
 *       before calling this function.
 *
 * @param buf Pointer to the data buffer.
 * @param data Data to add to the buffer.
 */
#ifdef CONFIG_NRF_PROFILER
void nrf_profiler_log_encode_uint8(struct log_event_buf *buf, uint8_t data);
#else
static inline void nrf_profiler_log_encode_uint8(struct log_event_buf *buf,
					     uint8_t data) {}
#endif

/** @brief Encode and add int8_t data type to a buffer.
 *
 * @note The buffer must be initialized with @ref nrf_profiler_log_start
 *       before calling this function.
 *
 * @param buf Pointer to the data buffer.
 * @param data Data to add to the buffer.
 */
#ifdef CONFIG_NRF_PROFILER
void nrf_profiler_log_encode_int8(struct log_event_buf *buf, int8_t data);
#else
static inline void nrf_profiler_log_encode_int8(struct log_event_buf *buf,
					    int8_t data) {}
#endif

/** @brief Encode and add string to a buffer.
 *
 * Maximum 255 characters can be sent (the rest is omitted).
 *
 * @note The buffer must be initialized with @ref nrf_profiler_log_start
 *       before calling this function.
 *
 * @param buf Pointer to the data buffer.
 * @param string String to add to the buffer.
 */
#ifdef CONFIG_NRF_PROFILER
void nrf_profiler_log_encode_string(struct log_event_buf *buf, const char *string);
#else
static inline void nrf_profiler_log_encode_string(struct log_event_buf *buf,
					      const char *string) {}
#endif


/** @brief Encode and add the event's address in memory to the buffer.
 *
 * This information is used for event identification.
 *
 * @note The buffer must be initialized with @ref nrf_profiler_log_start
 *       before calling this function.
 *
 * @param buf Pointer to the data buffer.
 * @param mem_address Memory address to encode.
 */
#ifdef CONFIG_NRF_PROFILER
void nrf_profiler_log_add_mem_address(struct log_event_buf *buf,
				  const void *mem_address);
#else
static inline void nrf_profiler_log_add_mem_address(struct log_event_buf *buf,
						const void *mem_address) {}
#endif


/** @brief Send data from the buffer to the host.
 *
 * This function only sends data that is already stored in the buffer.
 * Use @ref nrf_profiler_log_encode_uint32, @ref nrf_profiler_log_encode_int32,
 * @ref nrf_profiler_log_encode_uint16, @ref nrf_profiler_log_encode_int16,
 * @ref nrf_profiler_log_encode_uint8, @ref nrf_profiler_log_encode_int8,
 * @ref nrf_profiler_log_encode_string or @ref nrf_profiler_log_add_mem_address
 * to add data to the buffer.
 *
 * @param event_type_id Event type ID as assigned to the event type
 *                      when it is registered.
 * @param buf Pointer to the data buffer.
 */
#ifdef CONFIG_NRF_PROFILER
void nrf_profiler_log_send(struct log_event_buf *buf, uint16_t event_type_id);
#else
static inline void nrf_profiler_log_send(struct log_event_buf *buf,
				     uint16_t event_type_id) {}
#endif


/**
 * @}
 */

#endif /* _NRF_PROFILER_H_ */
