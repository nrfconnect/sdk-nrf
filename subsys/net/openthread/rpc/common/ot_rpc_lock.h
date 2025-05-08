/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef OT_RPC_LOCK_H_
#define OT_RPC_LOCK_H_

/** @brief Lock OpenThread stack mutex.
 *
 * This function and @ref ot_rpc_mutex_unlock are used by:
 * - OpenThread RPC client library: when receiving a callback from the server device.
 *   The application SHOULD override the default, weak implementations of these functions
 *   to synchronize the callback execution with application threads that call OpenThread
 *   API functions.
 * - OpenThread RPC server library: before and after accessing the OpenThread stack while
 *   processing a received RPC command.
 *
 * Rationale:
 *
 * OpenThread API is not thread-safe, therefore a user is responsible for assuring
 * exclusive access to the OpenThread stack.
 *
 * This remains valid when using OpenThread RPC client library, but there is one catch:
 * when using RPC architecture, OpenThread main loop runs on the server device and user
 * callbacks are delivered to the client device using the nRF RPC thread pool. For this
 * reason, in order to guarantee thread-safety, OpenThread RPC client needs to take the
 * mutex that the application uses for synchronizing OpenThread API calls.
 */
void ot_rpc_mutex_lock(void);

/** @brief Unlock OpenThread stack mutex.
 *
 * See @ref ot_rpc_mutex_lock for detailed information.
 */
void ot_rpc_mutex_unlock(void);

#endif
