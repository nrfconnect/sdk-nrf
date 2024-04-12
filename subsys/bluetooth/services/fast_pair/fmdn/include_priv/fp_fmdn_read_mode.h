/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _FP_FMDN_READ_MODE_H_
#define _FP_FMDN_READ_MODE_H_

#include <stdint.h>

/**
 * @defgroup fp_fmdn_read_mode Fast Pair FMDN read mode
 * @brief Internal API for Fast Pair FMDN read mode
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Request EIK recovery.
 *
 * @param[out] accepted True: the request is accepted
 *                      False: the request is rejected
 *
 * @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int fp_fmdn_read_mode_recovery_mode_request(bool *accepted);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _FP_FMDN_READ_MODE_H_ */
