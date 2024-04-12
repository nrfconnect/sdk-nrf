/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _FP_FMDN_BATTERY_H_
#define _FP_FMDN_BATTERY_H_

#include <stdint.h>
#include <stddef.h>

/**
 * @defgroup fp_fmdn_battery Fast Pair FMDN Battery
 * @brief Internal API for Fast Pair FMDN Battery
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Battery levels */
enum fp_fmdn_battery_level {
	/* Battery level indication unsupported. */
	FP_FMDN_BATTERY_LEVEL_NONE = 0x00,

	/* Normal battery level. */
	FP_FMDN_BATTERY_LEVEL_NORMAL = 0x01,

	/* Low battery level. */
	FP_FMDN_BATTERY_LEVEL_LOW = 0x02,

	/* Critically low battery level (battery replacement needed soon). */
	FP_FMDN_BATTERY_LEVEL_CRITICAL = 0x03,
};

/** @brief Get the current battery level.
 *
 *  This function gets the current battery level as quantified by the FMDN
 *  Accessory specification.
 *
 *  @return Battery level. See the @ref fp_fmdn_battery_level for the detailed
 *  description.
 */
enum fp_fmdn_battery_level fp_fmdn_battery_level_get(void);

/** Battery callback structure.  */
struct fp_fmdn_battery_cb {
	/** @brief Indicate a change in battery level.
	 *
	 *  Indicate that battery level as defined by the @ref fp_fmdn_battery_level
	 *  enumerator has changed.
	 */
	void (*battery_level_changed)(void);
};

/** @brief Register the battery callbacks.
 *
 *  This function registers the battery callbacks. The registered callback set is reset
 *  during the @ref bt_fast_pair_disable operation.
 *
 *  @param cb Battery callback structure.
 *
 *  @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int fp_fmdn_battery_cb_register(const struct fp_fmdn_battery_cb *cb);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _FP_FMDN_BATTERY_H_ */
