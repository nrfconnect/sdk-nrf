/*
 * Copyright (c) 2026 Nordic Semiconductor
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef RTFW_INTERNAL_H_
#define RTFW_INTERNAL_H_

void rtfw_doorbell_init(void);
void rtfw_doorbell_notify(void);
void rtfw_delivery_signal(void);

#endif /* RTFW_INTERNAL_H_ */
