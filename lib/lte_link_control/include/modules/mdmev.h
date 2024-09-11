/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MDMEV_H__
#define MDMEV_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Enable modem domain events. */
int mdmev_enable(void);

/* Disable modem domain events. */
int mdmev_disable(void);

#ifdef __cplusplus
}
#endif

#endif /* MDMEV_H__ */
