/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SSF_RESET_EVT_SERVICE_H__
#define SSF_RESET_EVT_SERVICE_H__

#include <stdint.h>

#include <nrfx.h>
#include <sdfw/sdfw_services/ssf_errno.h>

/** Error code in response from server if it failed to register or deregister
 *  the requested subscription.
 */
#define SSF_RESET_EVT_ERROR_SUB 1

/**
 * @brief       User callback invoked by client of reset event service for incoming notifications.
 *
 * @param[in]   domains         Bitfield of domains that will be reset.
 * @param[in]   delay_ms        Delay in milliseconds until the reset occurs.
 * @param[in]   user_data       Pointer to user data context
 *
 * @return      0 on success, otherwise a negative error code. Note that a non-0 value only results
 *              in an error being logged.
 */
typedef int (*ssf_reset_evt_callback)(uint32_t domains, uint32_t delay_ms, void *user_data);

/**
 * @brief       Subscribe to reset event notifications.
 *
 * @param[in]   callback Callback to be invoked for incoming notifications.
 * @param[in]   context  User context given to the callback on incoming events.
 *
 * @return      0 on success
 *              -SSF_EINVAL if already subscribed to the service
 *              -SSF_EFAULT if response from server is not 0
 */
int ssf_reset_evt_subscribe(ssf_reset_evt_callback callback, void *context);

/**
 * @brief       Unsubscribe from all reset event service notifications.
 *
 * @return      0 on success
 *              -SSF_EINVAL if callback is NULL
 *              -SSF_EFAULT if response from server is not 0
 */
int ssf_reset_evt_unsubscribe(void);

/**
 * @brief       Notify all subscribed clients about an incoming reset.
 *              Note that this is a server side function.
 *
 * @param[in]   domains        Bitfield of domains that will be reset.
 * @param[in]   delay_ms       Delay in milliseconds before the incoming reset.
 *
 * @return      Value returned by `ssf_server_notif`
 */
int ssf_reset_evt_notify(uint32_t domains, uint32_t delay_ms);

#endif /* SSF_RESET_EVT_SERVICE_H__ */
