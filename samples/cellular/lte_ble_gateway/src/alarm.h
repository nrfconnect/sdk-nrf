/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _ALARM_H_
#define _ALARM_H_

void alarm(void);
void send_aggregated_data(struct k_work *work);

#endif /* _ALARM_H_ */
