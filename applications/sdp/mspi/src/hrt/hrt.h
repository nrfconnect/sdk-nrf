/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _HRT_H__
#define _HRT_H__

#include <stdint.h>

void write_single_by_word(uint32_t* data, uint8_t data_len, uint32_t counter_top);

#endif /* _HRT_H__ */
