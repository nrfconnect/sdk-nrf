/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef _HWID_H_
#define _HWID_H_

#define HWID_LEN 8

void hwid_get(uint8_t *buf, size_t buf_size);

#endif /* _HWID_H_ */
