/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DECT_APP_TIME_H
#define DECT_APP_TIME_H

uint64_t dect_app_modem_time_now(void);
void dect_app_modem_time_save(uint64_t const *time);

#endif /* DECT_APP_TIME_H */
