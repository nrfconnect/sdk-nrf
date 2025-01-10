/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NRF_802154_PLATFORM_SL_LPTIMER_GRTC_PM_UTILS_H_
#define NRF_802154_PLATFORM_SL_LPTIMER_GRTC_PM_UTILS_H_

#include <stdint.h>

/**
 * @brief Updates the trigger time of an upcoming event, allowing forwarding it to
 * the Power Management module.
 *
 * After some delay, the event's trigger time is sent to the Power Management module
 * via @ref pm_policy_event_register or @ref pm_policy_event_update.
 *
 * This function opens a sequence that should be closed
 * by @ref nrf_802154_platform_sl_lptimer_pm_utils_event_unregister call.
 * In the meantime, the @ref nrf_802154_platform_sl_lptimer_pm_utils_event_update can be called
 * multiple times.
 *
 * @note There is no guarantee, that the aforementioned pm_policy methods will be called before
 * the event's trigger time.
 *
 * @param  grtc_ticks  When the event will occur, in absolute GRTC ticks.
 */
void nrf_802154_platform_sl_lptimer_pm_utils_event_update(uint64_t grtc_ticks);

/**
 * @brief Informs that there are no events planned and the Power Management event
 * registered by @ref nrf_802154_platform_sl_lptimer_pm_utils_event_update requires unregistration.
 *
 * Unless already called and if the @ref nrf_802154_platform_sl_lptimer_pm_utils_event_update
 * precedes this function, the function will call @ref pm_policy_event_unregister with some delay.
 */
static inline void nrf_802154_platform_sl_lptimer_pm_utils_event_unregister(void)
{
	return nrf_802154_platform_sl_lptimer_pm_utils_event_update(0);
}

#endif /* NRF_802154_PLATFORM_SL_LPTIMER_GRTC_PM_UTILS_H_ */
