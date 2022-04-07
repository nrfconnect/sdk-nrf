/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** \addtogroup streamctrl Stream control - control transfer and processing of the audio streams
 *  @{
 */

/** @file ctrl_events.h
 *  @brief Control events - events used for transfer and processing of the audio streams
 *
 */

#ifndef _CTRL_EVENTS_H_
#define _CTRL_EVENTS_H_

#include <zephyr.h>

#include "ble_trans.h"
#include "button_handler.h"

/** @brief Event sources
 *
 * These are the sources that can post activity events to the queue.
 */
enum event_src {
	EVT_SRC_BUTTON,
	EVT_SRC_PEER,
};

/** @brief Events for activity from event sources
 *
 * This type wraps activity/events from the various event
 * sources, with an identification of the source, for
 * posting to the event queue.
 */
struct event_t {
	enum event_src event_source;

	union {
		struct button_evt button_activity;
		enum ble_evt_type link_activity;
	};
};

/**@brief  Check if event queue is empty
 *
 * @retval True if queue is empty, false if not
 */
bool ctrl_events_queue_empty(void);

/**@brief  Put event in k_msgq
 *
 * @param  event   Pointer to event
 *
 * @retval 0 Event sent.
 * @retval -EFAULT Try to send event with address NULL
 * @retval -ENOMSG Returned without waiting or queue purged.
 * @retval -EAGAIN Waiting period timed out.
 */
int ctrl_events_put(struct event_t *event);

/**@brief  Get event from k_msgq
 *
 * @param  my_event  Event to get from the queue
 * @param  timeout   Time to wait for event. Can be K_FOREVER
 *		     K_NO_WAIT or a specific time using K_MSEC.
 *
 * @retval 0 Event received.
 * @retval -ENOMSG Returned without waiting.
 * @retval -EAGAIN Waiting period timed out.
 */
int ctrl_events_get(struct event_t *my_event, k_timeout_t timeout);

#endif /* _CTRL_EVENTS_H_ */

/** @} */ /* End addtogroup */
