/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Zigbee ZBOSS OSIF layer API header.
 */

#ifndef ZB_NRF_PLATFORM_H__
#define ZB_NRF_PLATFORM_H__

#include <zboss_api.h>
#include <zephyr/kernel.h>

typedef enum {
	ZIGBEE_EVENT_TX_FAILED,
	ZIGBEE_EVENT_TX_DONE,
	ZIGBEE_EVENT_RX_DONE,
	ZIGBEE_EVENT_APP,
} zigbee_event_t;

/**
 * @defgroup zigbee_zboss_osif Zigbee ZBOSS OSIF API
 * @{
 *
 */

/**@brief Function for checking if the Zigbee stack has been started.
 *
 * @retval true   Zigbee stack has been started.
 * @retval false  Zigbee stack has not been started yet.
 */
bool zigbee_is_stack_started(void);

/**@brief Function for starting the Zigbee thread. */
void zigbee_enable(void);

#ifdef CONFIG_ZIGBEE_DEBUG_FUNCTIONS
/**@brief Function for checking if the ZBOSS thread has been created.
 */
bool zigbee_debug_zboss_thread_is_created(void);

/**@brief Function for suspending the ZBOSS thread.
 */
void zigbee_debug_suspend_zboss_thread(void);

/**@brief Function for resuming the ZBOSS thread.
 */
void zigbee_debug_resume_zboss_thread(void);

/**@brief Function for getting the state of the Zigbee stack thread
 *        processing suspension.
 *
 * @retval true   Scheduler processing is suspended or the ZBOSS thread
 *                is not yet created.
 * @retval false  Scheduler processing is not suspended and the ZBOSS thread
 *                is created.
 */
bool zigbee_is_zboss_thread_suspended(void);
#endif /* defined(CONFIG_ZIGBEE_DEBUG_FUNCTIONS) */

/**
 * @}
 */

/**@brief Function for Zigbee stack initialization
 *
 * @return    0 if success
 */
int zigbee_init(void);

/**@brief Notify the ZBOSS thread about a new event.
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

/**@brief Function for checking if the Zigbee NVRAM has been initialised.
 *
 * @retval ZB_TRUE  Zigbee NVRAM is initialised.
 * @retval ZB_FALSE Zigbee NVRAM is not initialised
 */
zb_bool_t zigbee_is_nvram_initialised(void);

/**@brief Clears the PAN_ID value held in the Zigbee PIB cache.
 *
 * @details The value set is consistent with the behavior of
 *          @c zb_nwk_nib_init() from Zigbee stack NWK layer.
 *
 * Function can be used to ensure that the PIB cache does not store any valid
 * PAN_ID value in scenarios where the device is in "absent from the network"
 * phase (not yet joined or has already left).
 *
 * @return    The newly set PAN_ID value.
 */
uint32_t zigbee_pibcache_pan_id_clear(void);

#endif /* ZB_NRF_PLATFORM_H__ */
