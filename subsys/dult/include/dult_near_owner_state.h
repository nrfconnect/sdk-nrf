/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _DULT_NEAR_OWNER_STATE_H_
#define _DULT_NEAR_OWNER_STATE_H_

#include <stdint.h>
#include <stddef.h>

#include "dult.h"

/**
 * @defgroup dult_near_owner_state Detecting Unwanted Location Trackers near owner state
 * @brief Private API for DULT specification implementation near owner state module
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Get current DULT near owner state.
 *
 *  @return Current DULT near owner state
 */
enum dult_near_owner_state_mode dult_near_owner_state_get(void);

/** @brief Reset DULT near owner state.
 *
 *  Resets DULT near owner state to boot value which is equal to
 *  the @ref DULT_NEAR_OWNER_STATE_MODE_NEAR_OWNER.
 */
void dult_near_owner_state_reset(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _DULT_NEAR_OWNER_STATE_H_ */
