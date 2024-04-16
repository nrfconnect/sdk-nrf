/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SX_REGS_HEADER_FILE
#define SX_REGS_HEADER_FILE

#include <stdint.h>

struct sx_regs;

/** Write a 32 bit value into the register at addr
 *
 * 'regs' set of registers
 * 'addr' address offset of the register in 'regs'
 * 'v' value to write.
 */
void sx_pk_wrreg(struct sx_regs *regs, uint32_t addr, uint32_t v);

/** Read a 32 bit value from the register at addr
 *
 * 'regs' set of registers
 * 'addr' address offset of the register in 'regs'
 *
 * Returns the value read from the register.
 */
uint32_t sx_pk_rdreg(struct sx_regs *regs, uint32_t addr);

/** Write a 64 bit value into the register at addr
 *
 * 'regs' set of registers
 * 'addr' address offset of the register in 'regs'
 * 'v' value to write.
 */
void sx_pk_wrreg64(struct sx_regs *regs, uint64_t addr, uint64_t v);

#endif
