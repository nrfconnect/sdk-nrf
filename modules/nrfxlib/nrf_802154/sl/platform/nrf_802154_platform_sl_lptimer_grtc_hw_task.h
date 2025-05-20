/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NRF_802154_PLATFORM_SL_LPTIMER_GRTC_HW_TASK_H_
#define NRF_802154_PLATFORM_SL_LPTIMER_GRTC_HW_TASK_H_

#include <stdint.h>

/**
 * @brief Sets up cross-domain hardware connections necessary to trigger a RADIO task.
 *
 * This function configures cross-domain hardware connections necessary to trigger a RADIO task with
 * an event from GRTC. These connections are identical for all RADIO tasks.
 *
 * @note Every call to this function must be paired with a call to @ref
 * nrf_802154_platform_timestamper_cross_domain_connections_clear.
 */
void nrf_802154_platform_sl_lptimer_hw_task_cross_domain_connections_setup(uint32_t cc_channel);

/**
 * @brief Clears cross-domain hardware connections necessary to capture a timestamp.
 */
void nrf_802154_platform_sl_lptimer_hw_task_cross_domain_connections_clear(void);

/**
 * @brief Sets up local domain hardware connections necessary to trigger a RADIO task.
 *
 * This function configures local-domain hardware connections necessary to trigger a RADIO task with
 * an event from GRTC. These connections must be setup separately for every RADIO task.
 *
 * @param  dppi_ch  Local domain DPPI channel that the RADIO task to be triggered subscribes to.
 */
void nrf_802154_platform_sl_lptimer_hw_task_local_domain_connections_setup(uint32_t dppi_ch,
									   uint32_t cc_channel);

/**
 * @brief Clears local domain hardware connections necessary to trigger a RADIO task.
 */
void nrf_802154_platform_sl_lptimer_hw_task_local_domain_connections_clear(void);

#endif /* NRF_802154_PLATFORM_SL_LPTIMER_GRTC_HW_TASK_H_ */
