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
#include <net/nrf_provisioning.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Seconds to next provisioning
 *
 * @return int Seconds to next provisioning
 * @retval -EFAULT if the library is not initialized.
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
 * @brief Notify a provisioning event and wait for the modem to enter the desired
 *	  connectivity state.
 *
 * This function notifies the provided provisioning event through the provided callback and waits
 * for the modem to enter the corresponding connectivity state. It periodically checks the modem's
 * functional mode and network registration status until the desired state is reached or the
 * specified timeout expires.
 *
 * The function is intended to be used to notify the application about the need to switch
 * between offline and online connectivity states to perform provisioning operations.
 *
 * Offline state is required to generate and write credentials to the modem, and online state is
 * required for the library to communicate with the provisioning service.
 *
 * @param timeout_seconds Number of seconds to wait for the modem to reach the desired mode.
 * @param event The provisioning event to notify. Must be either
 *		NRF_PROVISIONING_EVENT_NEED_LTE_DEACTIVATED or
 *		NRF_PROVISIONING_EVENT_NEED_LTE_ACTIVATED.
 * @param callback Pointer to callback data structure containing the callback function
 *		   and user data. Must not be NULL.
 *
 * @retval 0 If the modem entered the desired state within the timeout.
 * @retval -EINVAL If the callback data is NULL or the provided event is invalid.
 * @retval -ETIMEDOUT If the modem did not enter the desired state within the timeout.
 * @retval < 0 Other negative error codes from lte_lc_func_mode_get() on failure.
 */
int nrf_provisioning_notify_event_and_wait_for_modem_state(int timeout_seconds,
							   enum nrf_provisioning_event event,
							   nrf_provisioning_event_cb_t callback);

#ifdef __cplusplus
}
#endif

#endif /* NRF_PROVISIONING_INTERNAL_H__ */
