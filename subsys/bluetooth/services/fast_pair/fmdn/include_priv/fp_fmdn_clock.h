/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _FP_FMDN_CLOCK_H_
#define _FP_FMDN_CLOCK_H_

#include <stdint.h>
#include <stddef.h>

/**
 * @defgroup fp_fmdn_clock Fast Pair FMDN Clock
 * @brief Internal API for Fast Pair FMDN Clock
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Read the FMDN Clock value. Time is measured in seconds.
 *
 * @return Clock value in seconds.
 */
uint32_t fp_fmdn_clock_read(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _FP_FMDN_CLOCK_H_ */
