/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#ifndef SLM_PPP_
#define SLM_PPP_

#include <modem/at_cmd_parser.h>

/** @retval 0 on success. */
int slm_ppp_init(void);

int handle_at_ppp(enum at_cmd_type cmd_type);

#endif
