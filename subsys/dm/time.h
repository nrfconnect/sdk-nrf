/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _TIME_H_
#define _TIME_H_

#include <zephyr/types.h>
#include "hal/nrf_rtc.h"


#ifdef __cplusplus
extern "C" {
#endif

#define TICKS_TO_US(ticks) ((uint32_t)((((uint64_t) ticks) * (1000000)) / (NRF_RTC_INPUT_FREQ)))
#define US_TO_RTC_TICKS(time) ((uint32_t)((((uint64_t)time) * (NRF_RTC_INPUT_FREQ)) / (1000000)))
#define RTC_COUNTER_MAX (NRF_RTC_COUNTER_MAX)

/** @brief Get current time tick
 *
 *  @param None
 *
 *  @retval Current value.
 */
uint32_t time_now(void);

/** @brief Calculate the distance between t1 and t2.
 *
 *  @param t1 Start time.
 *  @param t2 End time.
 *
 *  @retval Difference between start and end.
 */
uint32_t time_distance_get(uint32_t t1, uint32_t t2);

#ifdef __cplusplus
}
#endif

#endif /* _TIME_H_ */
