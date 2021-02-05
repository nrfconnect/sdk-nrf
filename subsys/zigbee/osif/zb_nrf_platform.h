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

/**@brief Schedule single-param callback execution.
 *
 * This API is thread- and ISR- safe.
 * It performs all necessary actions:
 *  - Forwards request from ISR to thread context
 *  - Schedules the callback in ZBOSS scheduler queue
 *  - Wakes up the Zigbee task.
 *
 * @param func    function to execute
 * @param param - callback parameter - usually ref to packet buffer
 *
 * @return RET_OK or RET_OVERFLOW.
 */
zb_ret_t zigbee_schedule_callback(zb_callback_t func, zb_uint8_t param);

/**@brief Schedule two-param callback execution.
 *
 * This API is thread- and ISR- safe.
 * It performs all necessary actions:
 *  - Forwards request from ISR to thread context
 *  - Schedules the callback in ZBOSS scheduler queue
 *  - Wakes up the Zigbee task.
 *
 * @param func        function to execute
 * @param param       zb_uint8_t callback parameter - usually,
 *                    ref to packet buffer
 * @param user_param  zb_uint16_t additional user parameter
 *
 * @return RET_OK or RET_OVERFLOW.
 */
zb_ret_t zigbee_schedule_callback2(zb_callback2_t func, zb_uint8_t param,
				   zb_uint16_t user_param);


/**@brief Schedule alarm - callback to be executed after timeout.
 *
 * Function will be called via scheduler after timeout expired, with possible
 * delays due to locking/unlocking and scheduling mechanism.
 * Timer resolution depends on implementation and is limited to a single
 * Beacon Interval (15.36 msec).
 * Same callback can be scheduled for execution more then once.
 *
 * This API is thread- and ISR- safe.
 * It performs all necessary actions:
 *  - Forwards request from ISR to thread context
 *  - Schedules the callback in ZBOSS scheduler queue
 *  - Wakes up the Zigbee task.
 *
 * @param func       function to call via scheduler
 * @param param      parameter to pass to the function
 * @param timeout_bi timeout, in beacon intervals
 *
 * @return RET_OK or RET_OVERFLOW
 */
zb_ret_t zigbee_schedule_alarm(zb_callback_t func, zb_uint8_t param,
			       zb_time_t run_after);

/**@brief Cancel previously scheduler alarm.
 *
 * This API cancels alarms scheduled via zigbee_schedule_alarm() API
 *
 * This API is thread- and ISR- safe.
 * It performs all necessary actions:
 *  - Forwards request from ISR to thread context
 *  - Schedules the callback in ZBOSS scheduler queue
 *  - Wakes up the Zigbee task.
 *
 * @param func - function to call via scheduler
 * @param param - parameter to pass to the function
 *
 * @return RET_OK or RET_OVERFLOW
 */
zb_ret_t zigbee_schedule_alarm_cancel(zb_callback_t func, zb_uint8_t param);

/**@brief Allocate OUT buffer, call a callback when the buffer is available.
 *
 * Use default buffer size _func(alloc single standard buffer).
 * If buffer is available, schedules callback for execution immediately.
 * If no buffers are available now, schedule callback later,
 * when buffer will be available.
 *
 * This API is thread- and ISR- safe.
 * It performs all necessary actions:
 *  - Forwards request from ISR to thread context
 *  - Schedules the callback in ZBOSS scheduler queue
 *  - Wakes up the Zigbee task.
 *
 * @param func - function to execute.
 * @return RET_OK or RET_OVERFLOW
 */
zb_ret_t zigbee_get_out_buf_delayed(zb_callback_t func);

/**@brief Allocate IN buffer, call a callback when the buffer is available.
 *
 * Use default buffer size _func(alloc single standard buffer).
 * If buffer is available, schedules callback for execution immediately.
 * If no buffers are available now, schedule callback later,
 * when buffer will be available.
 *
 * This API is thread- and ISR- safe.
 * It performs all necessary actions:
 *  - Forwards request from ISR to thread context
 *  - Schedules the callback in ZBOSS scheduler queue
 *  - Wakes up the Zigbee task.
 *
 * @param func - function to execute.
 * @return RET_OK or RET_OVERFLOW
 */
zb_ret_t zigbee_get_in_buf_delayed(zb_callback_t func);

/**@brief Allocate OUT buffer, call a callback when the buffer is available.
 *
 * If buffer is available, schedules callback for execution immediately.
 * If no buffers are available now, schedule callback later,
 * when buffer will be available.
 *
 * This API is thread- and ISR- safe.
 * It performs all necessary actions:
 *  - Forwards request from ISR to thread context
 *  - Schedules the callback in ZBOSS scheduler queue
 *  - Wakes up the Zigbee task.
 *
 * @param func     function to execute.
 * @param param    second parameter to pass to the function
 * @param max_size required maximum buffer payload size (in bytes).
 *                 It can be bigger or smaller than the default buffer size.
 *                 Depending on the specific value, the buffer pool may decide
 *                 to use a fraction of buffer or long buffers.
 *                 Special value 0 means "single default buffer".
 * @return RET_OK or RET_OVERFLOW
 */
zb_ret_t zigbee_get_out_buf_delayed_ext(zb_callback2_t func, zb_uint16_t param,
					zb_uint16_t max_size);

/**@brief Allocate IN buffer, call a callback when the buffer is available.
 *
 * If buffer is available, schedules callback for execution immediately.
 * If no buffers are available now, schedule callback later,
 * when buffer will be available.
 *
 * @param func     function to execute.
 * @param param    second parameter to pass to the function
 * @param max_size required maximum buffer payload size (in bytes).
 *                 It can be bigger or smaller than the default buffer size.
 *                 Depending on the specific value, the buffer pool may decide
 *                 to use a fraction of buffer or long buffers.
 *                 Special value 0 means "single default buffer".
 * @return RET_OK or error code.
 */
zb_ret_t zigbee_get_in_buf_delayed_ext(zb_callback2_t func, zb_uint16_t param,
				   zb_uint16_t max_size);

#endif /* ZB_NRF_PLATFORM_H__ */
