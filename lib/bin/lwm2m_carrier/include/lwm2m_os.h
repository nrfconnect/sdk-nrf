/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef LWM2M_OS_H__

#include <stddef.h>
#include <stdint.h>

/**@file lwm2m_os.h
 *
 * @defgroup lwm2m_carrier_os LWM2M OS layer
 * @{
 */

/**
 * @brief Maximum number of timers that the system must support.
 */
#define LWM2M_OS_MAX_TIMER_COUNT 6

#define LWM2M_LOG_LEVEL_NONE 0
#define LWM2M_LOG_LEVEL_ERR  1
#define LWM2M_LOG_LEVEL_WRN  2
#define LWM2M_LOG_LEVEL_INF  3
#define LWM2M_LOG_LEVEL_TRC  4

/**
 * @brief Timer callback function.
 */
typedef void (*lwm2m_os_timer_handler_t)(void *timer);

/**
 * @brief Initialize the LWM2M OS layer.
 */
int lwm2m_os_init(void);

/**
 * @brief Allocate memory.
 */
void *lwm2m_os_malloc(size_t size);

/**
 * @brief Free memory.
 */
void lwm2m_os_free(void *ptr);

/**
 * @brief Get uptime, in milliseconds.
 */
int64_t lwm2m_os_uptime_get(void);

/**
 * @brief Put a thread to a sleep.
 */
int lwm2m_os_sleep(int ms);

/**
 * @brief Reboot system.
 */
void lwm2m_os_sys_reset(void);

/**
 * @brief Get a random value.
 */
uint32_t lwm2m_os_rand_get(void);

/**
 * @brief Delete a non-volatile storage entry.
 */
int lwm2m_os_storage_delete(uint16_t id);

/**
 * @brief Read an entry from non-volatile storage.
 */
int lwm2m_os_storage_read(uint16_t id, void *data, size_t len);

/**
 * @brief Write an entry to non-volatile storage.
 */
int lwm2m_os_storage_write(uint16_t id, const void *data, size_t len);

/**
 * @brief Request a timer from the OS.
 */
void *lwm2m_os_timer_get(lwm2m_os_timer_handler_t handler);

/**
 * @brief Release a timer.
 */
void lwm2m_os_timer_release(void *timer);

/**
 * @brief Start a timer.
 */
int lwm2m_os_timer_start(void *timer, int32_t timeout);

/**
 * @brief Cancel a timer run.
 */
void lwm2m_os_timer_cancel(void *timer);

/**
 * @brief Obtain the time remaining on a timer.
 */
int32_t lwm2m_os_timer_remaining(void *timer);

/**
 * @brief Create a string copy for a logger subsystem.
 */
const char *lwm2m_os_log_strdup(const char *str);

/**
 * @brief Log a message.
 */
void lwm2m_os_log(int level, const char *fmt, ...);
/**@} */
#endif /* LWM2M_OS_H__ */
