/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef REDMOB_H__
#define REDMOB_H__

#include <modem/lte_lc.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Read the current reduced mobility mode.
 *
 * @deprecated since v2.8.0.
 */
int redmob_get(enum lte_lc_reduced_mobility_mode *mode);

/**
 * Set reduced mobility mode.
 *
 * @deprecated since v2.8.0.
 */
int redmob_set(enum lte_lc_reduced_mobility_mode mode);

#ifdef __cplusplus
}
#endif

#endif /* REDMOB_H__ */
