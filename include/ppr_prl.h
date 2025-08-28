/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef PPR_PRL_H_
#define PPR_PRL_H_

#include <stdint.h>

void ppr_prl_configure(uint32_t max_samples, uint32_t vtim_cnttop);

void ppr_prl_start(void);

#endif /* PPR_PRL_H_ */