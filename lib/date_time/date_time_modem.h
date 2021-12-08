/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DATE_TIME_MODEM_H_
#define DATE_TIME_MODEM_H_

int date_time_modem_get(int64_t *date_time_ms);
void date_time_modem_store(struct tm *ltm);
void date_time_modem_xtime_subscribe(void);

#endif /* DATE_TIME_MODEM_H_ */
