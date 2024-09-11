/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NCELLMEAS_H__
#define NCELLMEAS_H__

#include <modem/lte_lc.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Initiate a neighbor cell measurement. */
int ncellmeas_start(struct lte_lc_ncellmeas_params *params);

/* Cancel an ongoing neighbor cell measurement. */
int ncellmeas_cancel(void);

#ifdef __cplusplus
}
#endif

#endif /* NCELLMEAS_H__ */
