/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/** @file mqtt_os.h
 *
 * @brief MQTT Client depends on certain OS specific functionality. The needed
 *        methods are mapped here and should be implemented based on OS in use.
 *
 * @details Memory management, mutex, logging and wall clock are the needed
 *          functionality for MQTT module. The needed interfaces are defined
 *          in the OS. OS specific port of the interface shall be provided.
 *
 */

#ifndef MQTT_OS_H_
#define MQTT_OS_H_

#include <stddef.h>
#include <kernel.h>

#include <net/net_core.h>

#include "mqtt_internal.h"

#ifdef __cplusplus
extern "C" {
#endif

extern struct k_mutex mqtt_mutex;
extern struct k_mem_slab mqtt_slab;

/**@brief Method to get trace logs from the module. */
#define MQTT_TRC(...) NET_DBG(__VA_ARGS__)

/**@brief Method to error logs from the module. */
#define MQTT_ERR(...) NET_ERR(__VA_ARGS__)

/**@brief Initialize the mutex for the module, if any.
 *
 * @details This method is called during module initialization @ref mqtt_init.
 */
static inline void mqtt_mutex_init(void)
{
	k_mutex_init(&mqtt_mutex);
}

/**@brief Acquire lock on the module specific mutex, if any.
 *
 * @details This is assumed to be a blocking method until the acquisition
 *          of the mutex succeeds.
 */
static inline void mqtt_mutex_lock(void)
{
	(void)k_mutex_lock(&mqtt_mutex, K_FOREVER);
}

/**@brief Release the lock on the module specific mutex, if any.
 */
static inline void mqtt_mutex_unlock(void)
{
	k_mutex_unlock(&mqtt_mutex);
}

/**@brief Method to allocate memory for internal use in the module.
 *
 * @param[in] size Size of memory requested.
 *
 * @retval A valid pointer on success, NULL otherwise.
 */
static inline void *mqtt_malloc(size_t size)
{
	void *mem;

	if (size != MQTT_MAX_PACKET_LENGTH) {
		return NULL;
	}

	if (k_mem_slab_alloc(&mqtt_slab, (void **)&mem, K_NO_WAIT) < 0) {
		return NULL;
	}

	return mem;
}

/**@brief Method to free any allocated memory for internal use in the module.
 *
 * @param[in] ptr Memory to be freed.
 */
static inline void mqtt_free(void *ptr)
{
	void *mem = ptr;

	k_mem_slab_free(&mqtt_slab, (void **)&mem);
}

/**@brief Method to get the sys tick or a wall clock in millisecond resolution.
 *
 * @retval Current wall clock or sys tick value in milliseconds.
 */
static inline u32_t mqtt_sys_tick_in_ms_get(void)
{
	return k_uptime_get_32();
}

/**@brief Method to get elapsed time in milliseconds since the last activity.
 *
 * @param[in] last_activity The value since elapsed time is requested.
 *
 * @retval Time elapsed since last_activity time.
 */
static inline u32_t mqtt_elapsed_time_in_ms_get(uint32_t last_activity)
{
	s32_t diff = k_uptime_get_32() - last_activity;

	if (diff < 0) {
		return 0;
	}

	return diff;
}

#ifdef __cplusplus
}
#endif

#endif /* MQTT_OS_H_ */
