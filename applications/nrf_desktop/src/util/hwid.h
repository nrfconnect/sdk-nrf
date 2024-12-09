/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _HWID_H_
#define _HWID_H_

#ifdef __cplusplus
extern "C" {
#endif

#define HWID_LEN 8

void hwid_get(uint8_t *buf, size_t buf_size);

#ifdef __cplusplus
}
#endif

#endif /* _HWID_H_ */
