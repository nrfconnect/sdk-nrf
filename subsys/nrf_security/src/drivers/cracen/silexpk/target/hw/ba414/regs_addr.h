/**
 * \brief BA414ep hardware register addresses
 *
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef REGS_ADDR_HEADERFILE
#define REGS_ADDR_HEADERFILE

#define PK_REG_CONFIG	       0x00
#define PK_REG_COMMAND	       0x04
#define PK_REG_CONTROL	       0x08
#define PK_REG_STATUS	       0x0C
#define PK_REG_VERSION	       0x10
#define PK_REG_TIMER	       0x14
#define PK_REG_HWCONFIG	       0x18
#define PK_REG_OPSIZE	       0x1C
#define PK_REG_MEMOFFSET       0x20
#define PK_REG_MICROCODEOFFSET 0x24

#define PK_RB_CONTROL_START_OP	   0x00000001
#define PK_RB_CONTROL_CLEAR_IRQ	   0x00000002
#define PK_RB_CONFIG_PTRS(a, b, c) (a | (b << 8) | (c << 16))

#endif
