/**
 * \brief Isolated Key hardware register addresses
 * \file
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef IK_REGS_ADDR_HEADERFILE
#define IK_REGS_ADDR_HEADERFILE

#define PK_IK_OFFSET 0x1000

#define IK_REG_START		      (PK_IK_OFFSET + 0x00)
#define IK_REG_STATUS		      (PK_IK_OFFSET + 0x04)
#define IK_REG_INITDATA		      (PK_IK_OFFSET + 0x08)
#define IK_REG_NONCE		      (PK_IK_OFFSET + 0x0C)
#define IK_REG_PERSONALIZATION_STRING (PK_IK_OFFSET + 0x10)
#define IK_REG_RESEED_INTERVAL_LSB    (PK_IK_OFFSET + 0x14)
#define IK_REG_RESEED_INTERVAL_MSB    (PK_IK_OFFSET + 0x18)
#define IK_REG_PK_CONTROL	      (PK_IK_OFFSET + 0x1C)
#define IK_REG_PK_COMMAND	      (PK_IK_OFFSET + 0x20)
#define IK_REG_PK_STATUS	      (PK_IK_OFFSET + 0x24)
#define IK_REG_SOFT_RST		      (PK_IK_OFFSET + 0x28)
#define IK_REG_HW_CONFIG	      (PK_IK_OFFSET + 0x2C)

#define IK_PK_CONTROL_START_OP	0x00000001
#define IK_PK_CONTROL_CLEAR_IRQ 0x00000002
#define IK_NB_PRIV_KEYS_MASK	0x000000F0
#define IK_CURVE_MASK		0x00000C00
#define IK_PK_BUSY_MASK		0x00010000

#define IK_KEYGEN_OK	    (1 << 2)
#define IK_CTRDRBG_BUSY	    (1 << 4)
#define IK_CATASTROPHIC_ERR (1 << 5)

#endif
