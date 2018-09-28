/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef _SYSVIEW_EVENT_LOGGER_INIT_H_
#define _SYSVIEW_EVENT_LOGGER_INIT_H

#include <zephyr.h>
#include <kernel_structs.h>
#include <systemview/SEGGER_SYSVIEW.h>
#include <rtt/SEGGER_RTT.h>

/* Functions called when initializing SysView */

u32_t sysview_get_timestamp(void);
void SEGGER_SYSVIEW_Conf(void);

#endif /* _SYSVIEW_EVENT_LOGGER_INIT_H */
