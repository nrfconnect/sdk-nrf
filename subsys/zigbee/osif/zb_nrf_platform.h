/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef ZB_NRF_PLATFORM_H__
#define ZB_NRF_PLATFORM_H__

#include <zboss_api.h>
#include <kernel.h>

typedef enum {
	ZIGBEE_EVENT_TX_FAILED,
	ZIGBEE_EVENT_TX_DONE,
	ZIGBEE_EVENT_RX_DONE,
	ZIGBEE_EVENT_APP,
} zigbee_event_t;

#ifdef CONFIG_ZIGBEE_DEBUG_FUNCTIONS
/**@brief Function for suspending zboss thread.
 */
void zigbee_debug_suspend_zboss_thread(void);

/**@brief Function for resuming zboss thread.
 */
void zigbee_debug_resume_zboss_thread(void);

/**@brief Function for getting the state of the Zigbee stack thread
 *        processing suspension.
 *
 * @retval true   Scheduler processing is suspended or zboss thread
 *                is not yet created.
 * @retval false  Scheduler processing is not suspended and zboss thread
 *                is created.
 */
bool zigbee_is_zboss_thread_suspended(void);
#endif /* defined(CONFIG_ZIGBEE_DEBUG_FUNCTIONS) */

/**@brief Function for Zigbee stack initialization
 *
 * @return    0 if success
 */
int zigbee_init(void);

/**@brief Function for checking if the Zigbee stack has been started.
 *
 * @retval true   Zigbee stack has been started.
 * @retval false  Zigbee stack has not been started yet.
 */
bool zigbee_is_stack_started(void);

/* Function for starting Zigbee thread. */
void zigbee_enable(void);

/**@brief Notify ZBOSS thread about a new event.
 *
 * @param[in] event  Event to notify.
 */
void zigbee_event_notify(zigbee_event_t event);

/**@brief Function which waits for event in case
 *        of empty Zigbee stack scheduler queue.
 *
 * @param[in] timeout_us  Maximum amount of time, in microseconds
 *                        for which the ZBOSS task processing may be blocked.
 *
 * @returns The amount of microseconds that the ZBOSS task was blocked.
 */
uint32_t zigbee_event_poll(uint32_t timeout_us);

#endif /* ZB_NRF_PLATFORM_H__ */
