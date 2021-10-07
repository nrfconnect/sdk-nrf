/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef LOG_MOCK_H_
#define LOG_MOCK_H_

#include <zephyr/types.h>

void log_mock_hexdump(uint8_t *buf, int32_t len, char *s);
int32_t log_mock_get_buf_len(void);
int32_t log_mock_get_buf_data(int32_t index);
void log_mock_clear(void);

#ifdef LOG_ERR
#undef LOG_ERR
#endif
#define LOG_ERR(...)

#ifdef LOG_DBG
#undef LOG_DBG
#endif
#define LOG_DBG(...)

#ifdef LOG_HEXDUMP_DBG
#undef LOG_HEXDUMP_DBG
#endif
#define LOG_HEXDUMP_DBG(...)     log_mock_hexdump(__VA_ARGS__)

#ifdef LOG_MODULE_REGISTER
#undef LOG_MODULE_REGISTER
#endif
#define LOG_MODULE_REGISTER(...)

#ifdef LOG_MODULE_DECLARE
#undef LOG_MODULE_DECLARE
#endif
#define LOG_MODULE_DECLARE(...)

#endif /* LOG_MOCK_H_ */
