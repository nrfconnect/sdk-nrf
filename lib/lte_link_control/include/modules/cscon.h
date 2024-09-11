/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CSCON_H__
#define CSCON_H__

#ifdef __cplusplus
extern "C" {
#endif

/* CSCON command parameters */
#define AT_CSCON_RRC_MODE_INDEX	     1
#define AT_CSCON_READ_RRC_MODE_INDEX 2

/* Enable notifications. */
int cscon_notifications_enable(void);

#ifdef __cplusplus
}
#endif

#endif /* CSCON_H__ */
