/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _FP_FHN_CLOCK_H_
#define _FP_FHN_CLOCK_H_

#include <stdint.h>
#include <stddef.h>

/**
 * @defgroup fp_fhn_clock Fast Pair FHN Clock
 * @brief Internal API for Fast Pair FHN Clock
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Read the FHN Clock value. Time is measured in seconds.
 *
 * @return Clock value in seconds.
 */
uint32_t fp_fhn_clock_read(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _FP_FHN_CLOCK_H_ */
