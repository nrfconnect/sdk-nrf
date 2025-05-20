/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef XFACTORYRESET_H__
#define XFACTORYRESET_H__

#include <modem/lte_lc.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Reset modem to factory settings.
 *
 * @deprecated since v2.8.0.
 */
int xfactoryreset_reset(enum lte_lc_factory_reset_type type);

#ifdef __cplusplus
}
#endif

#endif /* XFACTORYRESET_H__ */
