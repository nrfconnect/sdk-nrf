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
 * @brief Wait for the modem to enter the desired functional mode.
 *	  Can be used to verify the modem's functional mode prior to writing credentials / AT
 *	  commands.
 *
 * This function waits for the modem to enter the desired functional mode.
 * It checks the current functional mode at regular intervals and returns
 * when the desired mode is reached or the timeout is reached.
 *
 * @param timeout The time to wait between checks in seconds.
 * @param iterations The number of times to check the functional mode.
 * @param offline_needed If true, waits for the modem to enter offline mode. If not, either
 *			 normal or lte activation mode is accepted.
 *
 * @return 0 on success, negative error code on failure.
 */
int nrf_provisioning_wait_for_desired_mode(int timeout,
					   int iterations,
					   bool offline_needed);

#ifdef __cplusplus
}
#endif

#endif /* NRF_PROVISIONING_INTERNAL_H__ */
