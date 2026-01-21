/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef ENVEVAL_H__
#define ENVEVAL_H__

#include <modem/lte_lc.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Perform environment evaluation. */
int env_eval(struct lte_lc_env_eval_params *params);

/* Cancel an ongoing environment evaluation. */
int env_eval_cancel(void);

#ifdef __cplusplus
}
#endif

#endif /* ENVEVAL_H__ */
