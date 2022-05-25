/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
/* Purpose: trace interface intended for internal usage on library level */

#ifndef PTT_TRACE_H__
#define PTT_TRACE_H__

#ifdef TESTS

#include "stdio.h"
#define PTT_TRACE printf

#else
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(phy_tt);

#define PTT_TRACE(...) LOG_DBG(__VA_ARGS__)

#endif

#define PTT_TRACE_FUNC_ENTER() PTT_TRACE("%s ->\n", __func__)
#define PTT_TRACE_FUNC_ENTER_WITH_EVT(evt_id) PTT_TRACE("%s -> evt_id: %d\n", __func__, evt_id)
#define PTT_TRACE_FUNC_EXIT() PTT_TRACE("%s -<\n", __func__)
#define PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret) PTT_TRACE("%s -< ret: %d\n", __func__, ret)

#endif /* PTT_TRACE_H__ */
