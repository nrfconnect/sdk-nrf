/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CFUN_H__
#define CFUN_H__

#include <modem/lte_lc.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Get the modem's functional mode. */
int cfun_mode_get(enum lte_lc_func_mode *mode);

/* Set the modem's functional mode. */
int cfun_mode_set(enum lte_lc_func_mode mode);

#ifdef __cplusplus
}
#endif

#endif /* CFUN_H__ */
