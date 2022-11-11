/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BL_MONOTONIC_COUNTERS_H_
#define BL_MONOTONIC_COUNTERS_H_

#include <string.h>
#include <zephyr/types.h>
#include <drivers/nrfx_common.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TYPE_COUNTERS 1 /* Type referring to counter collection. */
#define BL_MONOTONIC_COUNTERS_DESC_NSIB 0x1      /* Counter used by NSIB to check the firmware version */

/**
 * @brief Get the number of monotonic counter slots.
 *
 * @param[in]   counter_desc  Counter description.
 * @param[out]  counter_slots Number of slots occupied by the counter.
 *
 * @retval 0        Success
 * @retval -EINVAL  Cannot find counters with description @p counter_desc or the pointer to
 *                  @p counter_slots is NULL.
 */
int num_monotonic_counter_slots(uint16_t counter_desc, uint16_t *counter_slots);

/**
 * @brief Get the current HW monotonic counter.
 *
 * @param[in]  counter_desc  Counter description.
 * @param[out] counter_value The value of the current counter.
 *
 * @retval 0        Success
 * @retval -EINVAL  Cannot find counters with description @p counter_desc or the pointer to
 *                  @p counter_value is NULL.
 */
int get_monotonic_counter(uint16_t counter_desc, uint16_t *counter_value);

/**
 * @brief Set the current HW monotonic counter.
 *
 * @note FYI for users looking at the values directly in flash:
 *       Values are stored with their bits flipped. This is to squeeze one more
 *       value out of the counter.
 *
 * @param[in]  counter_desc Counter description.
 * @param[in]  new_counter  The new counter value. Must be larger than the
 *                          current value.
 *
 * @retval 0        The counter was updated successfully.
 * @retval -EINVAL  @p new_counter is invalid (must be larger than current
 *                  counter, and cannot be 0xFFFF).
 * @retval -ENOMEM  There are no more free counter slots (see
 *                  @kconfig{CONFIG_SB_NUM_VER_COUNTER_SLOTS}).
 */
int set_monotonic_counter(uint16_t counter_desc, uint16_t new_counter);

#ifdef __cplusplus
}
#endif

#endif /* BL_MONOTONIC_COUNTERS_H_ */
