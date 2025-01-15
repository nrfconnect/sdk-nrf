/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SLM_AT_NOTIF_H_
#define SLM_AT_NOTIF_H_

#include <stdbool.h>

/* This keeps track of whether the user is registered to the CGEV notifications.
 * We need them to know when to start/stop the PPP link, but that should not
 * influence what the user receives, so we do the filtering based on this.
 */
extern bool slm_fwd_cgev_notif;

#endif /* SLM_AT_NOTIF_H_ */
