/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef FOTA_SUPPORT_COAP_H
#define FOTA_SUPPORT_COAP_H

int coap_fota_init(void);
int coap_fota_thread_fn(void);

#endif /* FOTA_SUPPORT_COAP_H */
