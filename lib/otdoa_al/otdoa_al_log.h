/*
 * Copyright (c) 2025 PHY Wireless, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-PHYW
 */

#ifndef OTDOA_AL_LOG_H__
#define OTDOA_AL_LOG_H__

#ifndef HOST
#include <zephyr/logging/log.h>

#else /* HOST */
void otdoa_log_err(const char *fmt, ...);
void otdoa_log_wrn(const char *fmt, ...);
void otdoa_log_inf(const char *fmt, ...);
void otdoa_log_dbg(const char *fmt, ...);
void otdoa_log_hexdump_inf(void *data, unsigned int length, char *str);

#define LOG_DBG(fmt, ...)			    otdoa_log_dbg(fmt, ##__VA_ARGS__)
#define LOG_INF(fmt, ...)			    otdoa_log_inf(fmt, ##__VA_ARGS__)
#define LOG_HEXDUMP_DBG(_data, _length, _str) otdoa_log_hexdump_inf(_data, _length, _str)
#define LOG_WRN(fmt, ...)			    otdoa_log_wrn(fmt, ##__VA_ARGS__)
#define LOG_ERR(fmt, ...)			    otdoa_log_err(fmt, ##__VA_ARGS__)

#define LOG_MODULE_DECLARE(...)		/* unused in HOST */

#endif /* ndef HOST */

void otdoa_log_init(void);

#endif /* OTDOA_AL_LOG_H__ */
