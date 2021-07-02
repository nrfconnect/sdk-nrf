/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/**
 * @file sb_fota_os.h
 *
 * @defgroup sb_fota_os Softbank modem FOTA OS layer
 * @{
 */

#ifndef MODEM_FOTA_OS_H_
#define MODEM_FOTA_OS_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

/**
 * @brief Allocate memory.
 */
void *modem_fota_os_malloc(size_t size);

/**
 * @brief Allocate memory and zero it.
 */
void *modem_fota_os_calloc(size_t nmemb, size_t size);

/**
 * @brief Free memory.
 */
void modem_fota_os_free(void *ptr);

/**
 * @brief Get uptime, in milliseconds.
 */
int64_t modem_fota_os_uptime_get(void);

/**
 * @brief Get uptime, in milliseconds.
 */
uint32_t modem_fota_os_uptime_get_32(void);

/**
 * @brief Put a thread to a sleep.
 */
int modem_fota_os_sleep(int ms);

/**
 * @brief Reboot system.
 */
void modem_fota_os_sys_reset(void);

/**
 * @brief Get a random value.
 */
uint32_t modem_fota_os_rand_get(void);

#define MODEM_FOTA_N_SEMAPHORES 4

/** Opaque type for OS semaphore */
struct modem_fota_sem;

/**
 * @brief Allocate a semaphore from OS
 *
 * @return pointer to allocated semaphore or NULL on failure.
 */
struct modem_fota_sem *modem_fota_sem_alloc();

/**
 * @brief Give a semaphore.
 *
 * @param sem pointer to a taken semaphore.
 */
void modem_fota_sem_give(struct modem_fota_sem *sem);

/**
 * @brief Take a semaphore.
 *
 * @param sem pointer to semaphore
 * @param timeout_ms timeout in milliseconds, or negative if call can block forever.
 * @return zero on success or negative error code on failure.
 */
int modem_fota_sem_take(struct modem_fota_sem *sem, int timeout_ms);

/**
 * @brief Reset semaphore count to zero.
 *
 * @param sem pointer to a semaphore.
 */
void modem_fota_sem_reset(struct modem_fota_sem *sem);


#define MODEM_FOTA_N_EVENTS 6
#define MODEM_FOTA_N_DELAYED_EVENTS 1

/** Opaque event type */
struct modem_fota_event;

/** Opaque delayed event type */
struct modem_fota_delayed_event;

struct modem_fota_event *modem_fota_event_init(void (*callback)(void *));
void modem_fota_event_send(struct modem_fota_event *evt);
struct modem_fota_delayed_event *modem_fota_delayed_event_init(void (*callback)(void *));
void modem_fota_delayed_event_send(struct modem_fota_delayed_event *evt, int delay_ms);

#define MODEM_FOTA_N_TIMERS 2

/** Opaque timer type */
struct modem_fota_timer;

struct modem_fota_timer *modem_fota_timer_init(void (*callback)(void *));
void modem_fota_timer_start(struct modem_fota_timer *timer, int delay_ms);
void modem_fota_timer_stop(struct modem_fota_timer *timer);
bool modem_fota_timer_is_running(struct modem_fota_timer *timer);

int64_t modem_fota_timegm64(const struct tm *time);


#define FOTA_LOG_LEVEL_NONE 0U
#define FOTA_LOG_LEVEL_ERR  1U
#define FOTA_LOG_LEVEL_WRN  2U
#define FOTA_LOG_LEVEL_INF  3U
#define FOTA_LOG_LEVEL_DBG  4U

void modem_fota_log(int level, const char *fmt, ...);
const char *modem_fota_log_strdup(const char *str);
void modem_fota_logdump(const char *str, const void *data, size_t len);

#define FOTA_LOG_ERR(...) modem_fota_log(FOTA_LOG_LEVEL_ERR, __VA_ARGS__);
#define FOTA_LOG_WRN(...) modem_fota_log(FOTA_LOG_LEVEL_WRN, __VA_ARGS__);
#define FOTA_LOG_INF(...) modem_fota_log(FOTA_LOG_LEVEL_INF, __VA_ARGS__);
#define FOTA_LOG_DBG(...) modem_fota_log(FOTA_LOG_LEVEL_DBG, __VA_ARGS__);


#define MODEM_FOTA_SETTINGS_PREFIX "modem_fota"

/** Structure used to load settings from persistent storage */
struct modem_fota_settings {
	const char *name;	/** Name of the setting */
	size_t len;			/** Size of data, or zero for variable lenght data like strings */
	void *ptr;			/** Pointer to runtime storage.
					    Strings are allocated and ptr stored here. */
};

/** Load array of settings from persistent storage.
 *  Settings should be given as an array that ends with element where name is NULL.
 */
void modem_fota_load_settings(const struct modem_fota_settings *settings);

/** Store one settings value.
 * MODEM_FOTA_SETTINGS_PREFIX is automatically added to the name.
 */
void modem_fota_store_setting(const char *name, size_t len, const void *ptr);

/** Apply modem FW update */
void sb_fota_apply_update(void);

/** @} */

#ifdef UNITTESTING
/* For unittesting purposes, I need dummy types of every opaque struct */
struct modem_fota_sem
{
	int val;
};
struct modem_fota_event
{
	int val;
};
struct modem_fota_delayed_event
{
	int val;
};
struct modem_fota_timer
{
	int val;
};
#endif

#endif /* MODEM_FOTA_OS_H_ */
