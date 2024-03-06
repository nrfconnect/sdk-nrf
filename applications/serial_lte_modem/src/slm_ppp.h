/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#ifndef SLM_PPP_
#define SLM_PPP_

#include <modem/at_cmd_parser.h>
#include <stdbool.h>

/* Whether to forward CGEV notifications to the SLM UART. */
extern bool slm_fwd_cgev_notifs;

/** @retval 0 on success. */
int slm_ppp_init(void);

bool slm_ppp_is_running(void);

#endif
