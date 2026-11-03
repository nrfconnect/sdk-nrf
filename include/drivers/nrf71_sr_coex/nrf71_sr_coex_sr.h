/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @brief nRF71 coexistence driver interface for the Short-Range radio driver.
 *
 * The Short-Range (SR) radio driver registers a set of callbacks once at boot
 * and reports its power state. The coexistence driver uses the callbacks to
 * enable SR participation in coexistence and to hand over the SR priority
 * ranges that match what the Coexistence Manager was configured with.
 */

#ifndef NRF71_SR_COEX_SR_H__
#define NRF71_SR_COEX_SR_H__

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrf71_sr_coex_sr nRF71 coexistence SR driver interface
 * @{
 */

/** Let the SR driver pick the priority by packet type and activity criticality. */
#define NRF71_SR_COEX_PRIO_LEVEL_AUTO 0xFFU

/**
 * @brief COEXC priority (PTI) range.
 *
 * Lower values are higher priority. @c start is the numerically largest
 * (lowest priority) value and @c end the smallest (highest priority).
 */
struct nrf71_sr_coex_pti_range {
	/** Lowest-priority value of the range. */
	uint8_t start;
	/** Highest-priority value of the range. */
	uint8_t end;
	/** Distance between two consecutive values in the range. */
	uint8_t step;
};

/** @brief SR client priority ranges. */
struct nrf71_sr_coex_sr_prio {
	/** SR Rx client range. */
	struct nrf71_sr_coex_pti_range rx;
	/** SR Tx client range. */
	struct nrf71_sr_coex_pti_range tx;
	/** SR Rx client range for critical activities. */
	struct nrf71_sr_coex_pti_range rx_critical;
	/** SR Tx client range for critical activities. */
	struct nrf71_sr_coex_pti_range tx_critical;
	/**
	 * Index of the value to use within the ranges, or
	 * @ref NRF71_SR_COEX_PRIO_LEVEL_AUTO.
	 */
	uint8_t level;
};

/** @brief SR radio power events. */
enum nrf71_sr_coex_sr_power_event {
	/** The SR radio is about to power down. */
	NRF71_SR_COEX_SR_PREPARE_POWER_DOWN,
	/** The SR radio has powered up and is ready. */
	NRF71_SR_COEX_SR_POWERED_UP_READY,
};

/**
 * @brief Callbacks provided by the SR driver.
 *
 * Both callbacks are called from thread context and may block briefly.
 */
struct nrf71_sr_coex_sr_driver_cb {
	/**
	 * Enable or disable posting of SR hardware client requests to COEXC.
	 *
	 * @retval 0 on success, negative errno otherwise.
	 */
	int (*enable)(bool enable);
	/**
	 * Apply the SR client priority ranges.
	 *
	 * @retval 0 on success, negative errno otherwise.
	 */
	int (*set_priority)(const struct nrf71_sr_coex_sr_prio *prio);
};

/**
 * @brief Register the SR driver callbacks.
 *
 * Called once at boot. Until then the coexistence driver runs without an SR
 * driver and treats SR requests as applied. The structure must stay valid for
 * the lifetime of the system.
 *
 * @param cb Callbacks. Individual members may be NULL.
 *
 * @retval 0         On success.
 * @retval -EINVAL   @p cb is NULL.
 * @retval -EALREADY Callbacks are already registered.
 */
int nrf71_sr_coex_sr_register(const struct nrf71_sr_coex_sr_driver_cb *cb);

/**
 * @brief Report an SR radio power event.
 *
 * On power-up the coexistence driver applies the SR priority ranges and
 * enables coexistence in the SR driver; on power-down it disables it.
 *
 * @param event Power event.
 *
 * @retval 0       On success.
 * @retval -EBUSY  Another SR power transition is in progress.
 * @retval -EIO    The SR driver rejected the coexistence enable.
 * @retval -EINVAL Unknown event.
 */
int nrf71_sr_coex_sr_power_notify(enum nrf71_sr_coex_sr_power_event event);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* NRF71_SR_COEX_SR_H__ */
