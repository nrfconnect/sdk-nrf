/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _FP_FMDN_DULT_INTEGRATION_H_
#define _FP_FMDN_DULT_INTEGRATION_H_

#include <stdint.h>
#include <stddef.h>

#include <dult.h>

/**
 * @defgroup fp_fmdn_dult_integration Fast Pair FMDN DULT integration
 * @brief Internal API for Fast Pair FMDN DULT integration
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Get registered DULT user.
 *
 * @return Pointer to registered DULT user structure. If no user is registered, NULL is returned.
 */
const struct dult_user *fp_fmdn_dult_integration_user_get(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _FP_FMDN_DULT_INTEGRATION_H_ */
