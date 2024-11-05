/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SUIT_EVENTS_H__
#define SUIT_EVENTS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/* Pendable events that threads can wait for. */

/** Event indicating a successful execution of the suit-invoke command. */
#define SUIT_EVENT_INVOKE_SUCCESS BIT(0)

/** Event indicating an unsuccessful execution of the suit-invoke command. */
#define SUIT_EVENT_INVOKE_FAIL BIT(1)

/** Mask for events, related to the execution of the suit-invoke command. */
#define SUIT_EVENT_INVOKE_MASK (SUIT_EVENT_INVOKE_SUCCESS | SUIT_EVENT_INVOKE_FAIL)

/** CPU ID indicating all supported CPU IDs */
#define SUIT_EVENT_CPU_ID_ANY 0xFF

/** Special value of the timeout that represents infinite time.  */
#define SUIT_WAIT_FOREVER 0xFFFFFFFF

/**
 * @brief Post one or more events for a given CPU.
 *
 * This routine posts one or more events to an internal event object for a given CPU.
 * All tasks waiting using @ref suit_event_wait whose waiting conditions become met by this
 * posting immediately unpend.
 *
 * @note This API is a proxy for @ref k_event_post API that splits the event mask
 *       into 8 distinct 4-bit pools for 8 different CPU IDs.
 *
 * @param cpu_id  The CPU ID for which the events will be posted.
 *                Use @ref SUIT_EVENT_CPU_ID_ANY to post an event for all of the CPUs.
 * @param events  Set of events to post for a given CPU.
 *
 * @retval Previous value of the events for a given CPU.
 */
uint32_t suit_event_post(uint8_t cpu_id, uint8_t events);

/**
 * @brief Clear the events for given CPU.
 *
 * This routine clears (resets) the specified events for given CPU stored in
 * an event object. Use @ref SUIT_EVENT_CPU_ID_ANY to clear events for all CPUs.
 *
 * @note This API is a proxy for @ref k_event_clear API that splits the event mask
 *       into 8 distinct 4-bit pools for 8 different CPU IDs.
 *
 * @param cpu_id  The CPU ID for which the events will be cleared.
 *                Use @ref SUIT_EVENT_CPU_ID_ANY to clear events for all of the CPUs.
 * @param events  Set of events to clear for a given CPU.
 *
 * @retval Previous value of the events for a given CPU.
 */
uint32_t suit_event_clear(uint8_t cpu_id, uint8_t events);

/**
 * @brief Wait for any of the specified events assigned to a given CPU.
 *
 * This routine waits on internal @a event object untli any of the specifies
 * events for a given CPU ID have been posted, or the maximum wait time
 * @a timeout has expired.
 * A thread may wait on up to 4 distinctly numbered events that are expressed
 * as bits in a single 8-bit word.
 *
 * @note This API is a proxy for @ref k_event_wait API that splits the event mask
 *       into 8 distinct 4-bit pools for 8 different CPU IDs.
 *
 * @param cpu_id   The CPU ID whose events to wait for.
 *                 Use @ref SUIT_EVENT_CPU_ID_ANY to wait for an event for any of the CPUs.
 * @param events   Set of desired events on which to wait
 * @param reset    If true, clear the set of events for given CPU ID tracked by the
 *                 event object before waiting. If false, do not clear the events.
 * @param timeout  Waiting period for the desired set of events.
 *                 Use @ref SUIT_WAIT_FOREVER to wait forever.
 *
 * @retval set of matching events for a given CPU upon success.
 * @retval 0 if matching events were not received for a given CPU within the specified time.
 */
uint32_t suit_event_wait(uint8_t cpu_id, uint8_t events, bool reset, uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif /* SUIT_EVENTS_H__ */
