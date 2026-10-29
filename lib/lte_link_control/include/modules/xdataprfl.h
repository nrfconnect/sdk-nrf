/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef XDATAPRFL_H__
#define XDATAPRFL_H__

#include <modem/lte_lc.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Set the data profile. */
int xdataprfl_set(enum lte_lc_data_profile profile);

/* Get the data profile. */
int xdataprfl_get(enum lte_lc_data_profile *profile);

/* Set the data profile at modem initialization, but only if it differs from the current one. */
int xdataprfl_init_apply(enum lte_lc_data_profile profile);

#ifdef __cplusplus
}
#endif

#endif /* XDATAPRFL_H__ */
