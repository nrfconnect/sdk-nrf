/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef METHOD_GNSS_H
#define METHOD_GNSS_H

void gnss_event_handler(int event);
void gnss_fix_work_fn(struct k_work *item);
void gnss_timeout_work_fn(struct k_work *item);

#endif /* METHOD_GNSS_H */
