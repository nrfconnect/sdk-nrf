/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#ifndef SLM_CMUX_
#define SLM_CMUX_

#include <modem/at_cmd_parser.h>

void slm_cmux_init(void);

int handle_at_cmux(enum at_cmd_type cmd_type);

#if defined(CONFIG_SLM_PPP)
struct modem_pipe;
struct modem_pipe *slm_cmux_reserve_ppp_channel(void);

void slm_cmux_release_ppp_channel(void);
#endif /* CONFIG_SLM_PPP */

#endif
