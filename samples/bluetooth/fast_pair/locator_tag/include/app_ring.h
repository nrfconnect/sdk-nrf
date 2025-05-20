/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef APP_RING_H_
#define APP_RING_H_

#include <stdint.h>

/**
 * @defgroup fmdn_sample_ring Locator Tag sample ringing module
 * @brief Locator Tag sample ringing module
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Initialize the ringing module.
 *
 * @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int app_ring_init(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* APP_RING_H_ */
