/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef INTERNAL_HEADER_FILE
#define INTERNAL_HEADER_FILE

#include <stdint.h>
#include <silexpk/core.h>

void sx_pk_wrreg(struct sx_regs *regs, uint32_t addr, uint32_t v);

uint32_t sx_pk_rdreg(struct sx_regs *regs, uint32_t addr);

struct sx_regs *sx_pk_get_regs(void);

struct sx_pk_capabilities *sx_pk_get_caps(void);

#endif
