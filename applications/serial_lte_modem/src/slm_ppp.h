/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#ifndef SLM_PPP_
#define SLM_PPP_

#include <stdbool.h>

/** @retval 0 on success. */
int slm_ppp_init(void);

bool slm_ppp_is_stopped(void);

#endif
