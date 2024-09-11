/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CONEVAL_H__
#define CONEVAL_H__

#include <modem/lte_lc.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Get connection evaluation parameters. */
int coneval_params_get(struct lte_lc_conn_eval_params *params);

#ifdef __cplusplus
}
#endif

#endif /* CONEVAL_H__ */
