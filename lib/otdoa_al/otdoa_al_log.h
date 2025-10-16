/*
 * Copyright (c) 2025 PHY Wireless, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-PHYW
 */

#ifndef OTDOA_AL_LOG_H__
#define OTDOA_AL_LOG_H__

#include <otdoa_al/otdoa_log.h>

#define OTDOA_LOG_DBG(fmt, ...)			    otdoa_log_dbg(fmt, ##__VA_ARGS__)
#define OTDOA_LOG_INF(fmt, ...)			    otdoa_log_inf(fmt, ##__VA_ARGS__)
#define OTDOA_LOG_HEXDUMP_INF(_data, _length, _str) otdoa_log_hexdump_inf(_data, _length, _str)
#define OTDOA_LOG_WRN(fmt, ...)			    otdoa_log_wrn(fmt, ##__VA_ARGS__)
#define OTDOA_LOG_ERR(fmt, ...)			    otdoa_log_err(fmt, ##__VA_ARGS__)

#endif /* OTDOA_AL_LOG_H__ */
