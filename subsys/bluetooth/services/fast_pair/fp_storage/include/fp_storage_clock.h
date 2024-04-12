/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _FP_STORAGE_CLOCK_H_
#define _FP_STORAGE_CLOCK_H_

#include <sys/types.h>

/**
 * @defgroup fp_storage_clock Fast Pair Beacon Clock storage for the FMDN extension
 * @brief Internal API of Fast Pair Beacon Clock storage for the FMDN extension
 *
 * The module must be initialized before using API functions.
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Get the Beacon Clock checkpoint value from the boot time stage.
 *
 * @param[out] boot_checkpoint Boot time value of the Beacon Clock checkpoint.
 *
 * @return 0 If the operation was successful. Otherwise, a (negative) error code is returned.
 */
int fp_storage_clock_boot_checkpoint_get(uint32_t *boot_checkpoint);

/** Get the current value of the Beacon Clock checkpoint.
 *
 * @param[out] current_checkpoint Current value of the Beacon Clock checkpoint.
 *
 * @return 0 If the operation was successful. Otherwise, a (negative) error code is returned.
 */
int fp_storage_clock_current_checkpoint_get(uint32_t *current_checkpoint);

/** Update the current value of the Beacon Clock checkpoint.
 *
 * @param[in] current_clock Beacon Clock value to be saved.
 *
 * @return 0 If the operation was successful. Otherwise, a (negative) error code is returned.
 */
int fp_storage_clock_checkpoint_update(uint32_t current_clock);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _FP_STORAGE_CLOCK_H_ */
