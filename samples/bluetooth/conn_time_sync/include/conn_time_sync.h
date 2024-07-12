/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CONN_TIME_SYNC_H__
#define CONN_TIME_SYNC_H__

/**
 * @file
 * @defgroup conn_time_sync Definitions for the connection time sync sample.
 * @{
 * @brief Definitions for the connection time sync sample.
 *
 * This file contains common definitions and API declarations
 * used by the sample.
 */

#include <stdint.h>
#include <stdbool.h>
#include <zephyr/kernel.h>

/** @brief  Timed action service UUID */
#define BT_UUID_TIMED_ACTION_SERVICE_VAL \
	BT_UUID_128_ENCODE(0xA88278D0, 0x7009, 0x4BEE, 0xA6F8, 0xE1DC3FF02B92)
#define BT_UUID_TIMED_ACTION_SERVICE \
	BT_UUID_DECLARE_128(BT_UUID_TIMED_ACTION_SERVICE_VAL)

/** @brief Timed action characteristic UUID */
#define BT_UUID_TIMED_ACTION_CHAR_VAL \
	BT_UUID_128_ENCODE(0xA88278D1, 0x7009, 0x4BEE, 0xA6F8, 0xE1DC3FF02B92)
#define BT_UUID_TIMED_ACTION_CHAR \
	BT_UUID_DECLARE_128(BT_UUID_TIMED_ACTION_CHAR_VAL)

/** @brief Start central demo. */
void central_start(void);

/** @brief Start peripheral demo. */
void peripheral_start(void);

/** @brief The timed action message
 *
 * The timed action message represents the information
 * sent from the central to the peripheral to allow time synchronization.
 * The anchor point information is the common time reference between
 * the central and peripheral.
 *
 * This samples transmits the anchor point time and trigger time
 * in a single packet. Other applications may consider sending those
 * separately.
 */
struct timed_action {
	uint64_t anchor_point_us_central_clock;
	uint16_t anchor_point_event_counter;
	uint64_t trigger_time_us_central_clock;
	bool led_value;
} __packed;

static inline void timed_action_print(struct timed_action *action)
{
	printk("c_anchor point (time=%llu, counter=%u), c_trigger_time %llu, value %d\n",
		action->anchor_point_us_central_clock,
		action->anchor_point_event_counter,
		action->trigger_time_us_central_clock,
		action->led_value);
}

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

/** @brief Initialize the module handling timed toggling of an LED.
 *
 * @return 0 on success, failure otherwise.
 */
int timed_led_toggle_init(void);

/** @brief Toggle the LED using the given value at the given timestamp.
 *
 * @param value The LED value.
 * @param timestamp_us The time when the LED will be set, in microseconds.
 *                     The time is specified in controller clock units.
 */
void timed_led_toggle_trigger_at(uint8_t value, uint32_t timestamp_us);

#endif

/**
 * @}
 */
