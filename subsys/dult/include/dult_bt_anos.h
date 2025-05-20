/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _DULT_BT_ANOS_H_
#define _DULT_BT_ANOS_H_

#include <stdint.h>
#include <stddef.h>

/**
 * @defgroup dult_bt_anos Detecting Unwanted Location Trackers Bluetooth ANOS
 * @brief Private API for DULT specification implementation of the Bluetooth ANOS module
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** @brief DULT Bluetooth ANOS sound callback structure. */
struct dult_bt_anos_sound_cb {
	/** @brief Start sound action.
	 *
	 *  This callback is called to notify the upper layer that the connected
	 *  peer requested start sound action over the Bluetooth GATT transport.
	 */
	void (*sound_start)(void);

	/** @brief Stop sound action.
	 *
	 *  This callback is called to notify the upper layer that the connected
	 *  peer requested stop sound action over the Bluetooth GATT transport.
	 */
	void (*sound_stop)(void);
};

/** @brief Register DULT Bluetooth ANOS sound callback structure.
 *
 *  @param cb Sound callback structure.
 */
void dult_bt_anos_sound_cb_register(const struct dult_bt_anos_sound_cb *cb);

/** @brief Notify the DULT Bluetooth ANOS module about the sound state change.
 *
 *  @param active True when the play sound action is in progress.
 *                False: otherwise.
 *  @param native True when the play sound action was triggered by this module.
 *                False: otherwise.
 */
void dult_bt_anos_sound_state_change_notify(bool active, bool native);

/** @brief Enable DULT Bluetooth ANOS.
 *
 *  @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int dult_bt_anos_enable(void);

/** @brief Reset DULT Bluetooth ANOS.
 *
 *  @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int dult_bt_anos_reset(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _DULT_BT_ANOS_H_ */
