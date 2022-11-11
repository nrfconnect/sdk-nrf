/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BL_MONOTONIC_COUNTERS_H_
#define BL_MONOTONIC_COUNTERS_H_

#include <string.h>
#include <zephyr/types.h>
#include <drivers/nrfx_common.h>
#include <nrfx_nvmc.h>
#include <errno.h>
#include <pm_config.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get the number of monotonic counter slots.
 *
 * @return The number of slots. If the provision page does not contain the
 *         information, 0 is returned.
 */
uint16_t num_monotonic_counter_slots(void);

/**
 * @brief Get the current HW monotonic counter.
 *
 * @return The current value of the counter.
 */
uint16_t get_monotonic_counter(void);

/**
 * @brief Set the current HW monotonic counter.
 *
 * @note FYI for users looking at the values directly in flash:
 *       Values are stored with their bits flipped. This is to squeeze one more
 *       value out of the counter.
 *
 * @param[in]  new_counter  The new counter value. Must be larger than the
 *                          current value.
 *
 * @retval 0        The counter was updated successfully.
 * @retval -EINVAL  @p new_counter is invalid (must be larger than current
 *                  counter, and cannot be 0xFFFF).
 * @retval -ENOMEM  There are no more free counter slots (see
 *                  @kconfig{CONFIG_SB_NUM_VER_COUNTER_SLOTS}).
 */
int set_monotonic_counter(uint16_t new_counter);

  /** @} */

#ifdef __cplusplus
}
#endif

#endif /* BL_MONOTONIC_COUNTERS_H_ */
