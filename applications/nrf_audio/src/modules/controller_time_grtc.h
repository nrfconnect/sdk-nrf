/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _CONTROLLER_TIME_GRTC_H_
#define _CONTROLLER_TIME_GRTC_H_

 #include <stdint.h>

/** @brief Obtain the current Bluetooth controller time.
 *
 * The timestamps are based upon this clock.
 *
 * @return The current controller time.
 */
uint64_t controller_time_us_get(void);

/** @brief Set the controller to trigger a PPI event at the given timestamp.
 *
 * @param timestamp_us The timestamp where it will trigger.
 */
void controller_time_trigger_set(uint64_t timestamp_us);

/** @brief Get the address of the event that will trigger.
 *
 * @return The address of the event that will trigger.
 */
uint32_t controller_time_trigger_event_addr_get(void);

#endif /* _CONTROLLER_TIME_GRTC_H_ */
