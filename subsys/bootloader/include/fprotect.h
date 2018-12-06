/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef FPROTECT_H_
#define FPROTECT_H_

#include <stdint.h>
#include <string.h>

/* Flash protect specified area */
void fprotect_area(u32_t start, size_t length);

#endif /* FPROTECT_H_ */
