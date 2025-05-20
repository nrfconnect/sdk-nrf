/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _TIMESLOT_MNG_H_
#define _TIMESLOT_MNG_H_

#include <zephyr/types.h>
#include <dm.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Timeslot request structure */
struct timeslot_request {
	/* Distance measurement request structure */
	struct dm_request dm_req;

	/* The desired start time of timeslot */
	uint32_t start_time;

	/* Timeslot length*/
	uint32_t timeslot_length_us;

	/* Ranging window length */
	uint32_t window_length_us;
};

/** @brief Append an element to the end of a queue.
 *
 *  @param req Address of the structure with request parameters.
 *  @param start_ref_tick Reference start time tick.
 *  @param window_len Ranging window length.
 *  @param timeslot_len Timeslot length.
 *
 *  @retval -ENOMEM when the tiemslot queue is full or a memory allocation error.
 *  @retval -EAGAIN when a single peer has a maximum number of timeslots scheduled.
 *  @retval -EBUSY when the timeslot cannot be scheduled due to time restrictions.
 */
int timeslot_queue_append(struct dm_request *req, uint32_t start_ref_tick,
			  uint32_t window_len, uint32_t timeslot_len);

/** @brief Peek element at the head of queue.
 *
 *  @param None
 *
 *  @retval Head element, or NULL if queue is empty.
 */
struct timeslot_request *timeslot_queue_peek(void);

/** @brief Remove the first item from the timeslot queue.
 *
 *  @param None
 */
void timeslot_queue_remove_first(void);

#ifdef __cplusplus
}
#endif

#endif /* _TIMESLOT_MNG_H_ */
