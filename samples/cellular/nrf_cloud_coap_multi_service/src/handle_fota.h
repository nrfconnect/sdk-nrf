/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef HANDLE_FOTA_H
#define HANDLE_FOTA_H

int handle_fota_init(void);
int handle_fota_begin(void);
int handle_fota_process(void);

#endif /* HANDLE_FOTA_H */
