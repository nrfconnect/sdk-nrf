/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MOSH_NTN_H
#define MOSH_NTN_H

/* Enable modem location updates from external GNSS. */
int ntn_location_update_enable(uint32_t interval);

/* Disable modem location updates from external GNSS. */
int ntn_location_update_disable(void);

#endif /* MOSH_NTN_H */
