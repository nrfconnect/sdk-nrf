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

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Background provisioning service - assumed to run in it's own thread.
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


#ifdef __cplusplus
}
#endif

#endif /* NRF_PROVISIONING_INTERNAL_H__ */
