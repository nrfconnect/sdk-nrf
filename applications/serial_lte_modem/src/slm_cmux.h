/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#ifndef SLM_CMUX_
#define SLM_CMUX_

#include <modem/at_cmd_parser.h>

void slm_cmux_init(void);

#if defined(CONFIG_SLM_PPP)
void *slm_cmux_get_ppp_channel_pipe(void);
#endif

int handle_at_cmux(enum at_cmd_type cmd_type);

#endif
