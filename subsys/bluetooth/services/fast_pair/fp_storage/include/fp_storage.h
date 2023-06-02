/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _FP_STORAGE_H_
#define _FP_STORAGE_H_

#include <sys/types.h>

/**
 * @defgroup fp_storage Fast Pair storage API for common operations
 * @brief Internal API for common operations of Fast Pair storage
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Perform a reset to the default factory settings for all storage modules.
 *
 * Clears the Fast Pair storage. If the reset operation is interrupted by system reboot or power
 * outage, the reset is automatically resumed at the stage of loading the Fast Pair storage.
 * It prevents the Fast Pair storage from ending in unwanted state after the reset interruption.
 *
 * @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int fp_storage_factory_reset(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _FP_STORAGE_H_ */
