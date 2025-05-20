/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NRF_PROVISIONING_INTERNAL_H__
#define NRF_PROVISIONING_INTERNAL_H__

#include <stddef.h>
#include <stdint.h>

#include <zephyr/settings/settings.h>
#include <modem/lte_lc.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Background provisioning service - assumed to run in its own thread.
 *
 * Revealed only to be able to test the functionality.
 */
int nrf_provisioning_req(void);

/**
 * @brief Seconds to next provisioning
 *
 * @return int Seconds to next provisioning
 */
int nrf_provisioning_schedule(void);

/**
 * @brief Reads
 *
 * Revealed only to be able to test the functionality.
 *
 * @param key
 * @param len_rd
 * @param read_cb
 * @param cb_arg
 * @return int
 */
int nrf_provisioning_set(const char *key, size_t len_rd,
				settings_read_cb read_cb, void *cb_arg);


/**
 * @brief Notify a provisioning event and wait for the modem to enter the desired functional mode.
 *
 * This function notifies the provided provisioning event via the callback and waits for the modem
 * to enter the corresponding functional mode. It periodically checks the modem's functional mode
 * and returns when the desired mode is reached or the timeout expires.
 *
 * @param timeout_seconds Number of seconds to wait for the modem to reach the desired mode.
 * @param event The provisioning event to notify. Must be either NRF_PROVISIONING_EVENT_NEED_OFFLINE
 *              or NRF_PROVISIONING_EVENT_NEED_ONLINE.
 * @param callback_data_internal Pointer to callback data structure containing the callback function
 *                              and user data. Must not be NULL.
 *
 * @retval 0 If the modem entered the desired functional mode within the timeout.
 * @retval -EINVAL If the callback data is NULL or the event is invalid.
 * @retval -ETIMEDOUT If the modem did not enter the desired mode within the timeout.
 * @retval < 0 Other negative error codes from lte_lc_func_mode_get() on failure.
 */
int nrf_provisioning_notify_event_and_wait_for_functional_mode(
				int timeout_seconds,
				enum nrf_provisioning_event event,
				nrf_provisioning_event_cb_t callback);

#ifdef __cplusplus
}
#endif

#endif /* NRF_PROVISIONING_INTERNAL_H__ */
