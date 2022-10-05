/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Multiple-listener callback header.
 */

#ifndef _MULTI_CALLBACK_H
#define _MULTI_CALLBACK_H

/**
 * @defgroup multi_callback Multiple-listener callbacks
 * @{
 * @brief Library for creating multiple-listener callbacks (multi-callbacks)
 *
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/kernel.h>

/**
 * @brief Define (statically) a multi-listener callback (multi-callback).
 *
 * @param name The name to be given to the multi-callback.
 * @param cb_type The callback typename.
 * @param slots The maximum number of listeners which can be simultaneously registered.
 */
#define MCB_DEFINE(name, cb_type, slots)				\
	static cb_type name[slots];					\
	static K_MUTEX_DEFINE(MCB_ ## name ## _mutex);			\
	static inline int MCB_ ## name ## _listen(cb_type cb)		\
	{								\
		k_mutex_lock(&MCB_ ## name ## _mutex, K_FOREVER);	\
		for (int mcb_i = 0; mcb_i < ARRAY_SIZE(name); mcb_i++) {\
			if (name[mcb_i] == NULL) {			\
				name[mcb_i] = cb;			\
				k_mutex_unlock(&MCB_ ## name ## _mutex);\
				return mcb_i;				\
			}						\
		}							\
		k_mutex_unlock(&MCB_ ## name ## _mutex);		\
		return -ENOSPC;						\
	}								\
									\
	static inline int MCB_ ## name ## _unlisten(cb_type cb)		\
	{								\
		k_mutex_lock(&MCB_ ## name ## _mutex, K_FOREVER);	\
		for (int mcb_i = 0; mcb_i < ARRAY_SIZE(name); mcb_i++) {\
			if (name[mcb_i] == cb) {			\
				name[mcb_i] = NULL;			\
				k_mutex_unlock(&MCB_ ## name ## _mutex);\
				return mcb_i;				\
			}						\
		}							\
		k_mutex_unlock(&MCB_ ## name ## _mutex);		\
		return -ENOENT;						\
	}								\
									\
	static inline int MCB_ ## name ## _unlisten_slot(int slot)	\
	{								\
		if (name[slot] == NULL) {				\
			return -ENOENT;					\
		}							\
		k_mutex_lock(&MCB_ ## name ## _mutex, K_FOREVER);	\
		name[slot] = NULL;					\
		k_mutex_unlock(&MCB_ ## name ## _mutex);		\
		return 0;						\
	}

/**
 * @brief Call a multi-callback.
 *
 * All listeners which have been added to the multi-callback will be called.
 * Call order is not guaranteed.
 *
 * @param name The name of the multi-callback to call
 * @param ... The params to pass to the callback
 */
#define MCB_CALL(name, ...) do {					\
	k_mutex_lock(&MCB_ ## name ## _mutex, K_FOREVER);		\
	for (int mcb_i = 0; mcb_i < ARRAY_SIZE(name); mcb_i++) {	\
		if (name[mcb_i] != NULL) {				\
			name[mcb_i](__VA_ARGS__);			\
		}							\
	}								\
	k_mutex_unlock(&MCB_ ## name ## _mutex);			\
} while (0)

/**
 * @brief Add a listener to a multi-callback.
 *
 * @param name The name of the multi-callback to add a listener to.
 * @param cb The listener callback to add.
 * @return Returns the slot the listener was assigned to if successful;
 *	   Otherwise, returns -ENOSPC (no slots available).
 */
#define MCB_LISTEN(name, cb) MCB_ ## name ## _listen(cb)

/**
 * @brief Remove a listener from a multi-callback by value.
 *
 * @param name The name of the multi-callback to remove the listener from.
 * @param cb The listener to be removed.
 * @return Returns the slot from which the listener was removed if successful;
 *	   Otherwise, returns -ENOENT (listener not found).
 */
#define MCB_UNLISTEN(name, cb) MCB_ ## name ## _unlisten(cb)

/**
 * @brief Remove a listener from a multi-callback by slot.
 *
 * @param name The name of the multi-callback to remove the listener from.
 * @param slot The slot of the listener to be removed.
 * @return Returns 0 if the slot was successfully cleared;
 *	   Otherwise, returns -ENOENT (slot is already empty).
 */
#define MCB_UNLISTEN_SLOT(name, slot) MCB_ ## name ## _unlisten_slot(slot)

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _MULTI_CALLBACK_H */
