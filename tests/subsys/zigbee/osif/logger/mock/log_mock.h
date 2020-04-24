/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef LOG_MOCK_H_
#define LOG_MOCK_H_

#include <zephyr/types.h>

void log_mock_hexdump(u8_t *buf, s32_t len, char *s);
s32_t log_mock_get_buf_len(void);
s32_t log_mock_get_buf_data(s32_t index);
void log_mock_clear(void);

#ifdef LOG_HEXDUMP_DBG
#undef LOG_HEXDUMP_DBG
#endif
#define LOG_HEXDUMP_DBG(...)     log_mock_hexdump(__VA_ARGS__)

#ifdef LOG_MODULE_REGISTER
#undef LOG_MODULE_REGISTER
#endif
#define LOG_MODULE_REGISTER(...)

#endif /* LOG_MOCK_H_ */
