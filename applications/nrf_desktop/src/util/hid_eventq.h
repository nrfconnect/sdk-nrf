/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief HID event queue header.
 */

#ifndef _HID_EVENTQ_H_
#define _HID_EVENTQ_H_

/**
 * @defgroup hid_eventq HID event queue
 * @brief Utility that simplifies queuing HID events (keypresses).
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/sys/slist.h>

/**@brief Event queue structure. */
struct hid_eventq {
	sys_slist_t root;
	uint16_t cnt;
	uint16_t cnt_max;
};

/**
 * @brief Initialize a HID event queue object instance.
 *
 * A HID event queue object instance must be initialized before used.
 *
 * @param[in] q			HID event queue object.
 * @param[in] max_queued	Limit of enqueued HID events for the queue.
 */
void hid_eventq_init(struct hid_eventq *q, uint16_t max_queued);

/**
 * @brief Check if a HID event queue is full
 *
 * The function checks if number of enqueued HID events is equal to limit of enqueued HID events.
 *
 * @param[in] q		HID event queue object.
 *
 * @return true if queue is full, false otherwise.
 */
bool hid_eventq_is_full(const struct hid_eventq *q);

/**
 * @brief Check if a HID event queue is empty
 *
 * The function checks if number of enqueued HID events is equal zero.
 *
 * @param[in] q		HID event queue object.
 *
 * @return true if queue is empty, false otherwise.
 */
bool hid_eventq_is_empty(const struct hid_eventq *q);

/**
 * @brief Enqueue a keypress event in HID event queue
 *
 * The function enqueues an event related to key press or release.
 *
 * If drop oldest is disabled, the function returns an error if it reached a limit of enqueued
 * events. Otherwise, the function tries to remove the oldest keypress events ensuring that an
 * enqueued event related to a key press is not removed if a matching key release event cannot be
 * removed.
 *
 * @param[in] q			HID event queue object.
 * @param[in] id		ID of the enqueued key.
 * @param[in] pressed		Information if the key was pressed or released.
 * @param[in] drop_oldest	Drop the oldest HID events to make space for the new one.
 *
 * @retval 0 when successful.
 * @retval -ENOBUFS if reached limit of enqueued HID events.
 * @retval -ENOMEM if internal memory allocation failed.
 */
int hid_eventq_keypress_enqueue(struct hid_eventq *q, uint16_t id, bool pressed, bool drop_oldest);

/**
 * @brief Get an enqueued keypress event from HID event queue
 *
 * The function gets the first enqueued event from the HID event queue.
 *
 * @param[in] q			HID event queue object.
 * @param[out] id		ID of the enqueued key.
 * @param[out] pressed		Information if the key was pressed or released.
 *
 * @retval 0 when successful.
 * @retval -ENOENT if there is no keypress event enqueued.
 */
int hid_eventq_keypress_dequeue(struct hid_eventq *q, uint16_t *id, bool *pressed);

/**
 * @brief Reset a HID event queue object instance.
 *
 * Resetting HID event queue results in dropping all of the enqueued keypresses.
 *
 * @param[in] q		HID event queue object.
 */
void hid_eventq_reset(struct hid_eventq *q);

/**
 * @brief Cleanup a HID event queue object instance.
 *
 * The function removes enqueued events with a timestamp lower than the provided minimal valid
 * timestamp.
 *
 * Enqueued event related to a key press is not removed if a matching key release event cannot be
 * removed. In that case, the function only removes items up to the first unpaired key press.
 *
 * @param[in] q			HID event queue object.
 * @param[in] min_timestamp	Minimal valid timestamp.
 */
void hid_eventq_cleanup(struct hid_eventq *q, int64_t min_timestamp);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /*_HID_EVENTQ_H_ */
