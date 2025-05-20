/**
 * \brief BA414ep public key command codes and flags for IK
 * \file
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef IK_REGS_COMMANDS_HEADERFILE
#define IK_REGS_COMMANDS_HEADERFILE

#include "../ba414/regs_commands.h"

/** Flag for IK operations */
#define SX_PK_OP_IK (1 << 17)

/** IK ECC public key generation */
#define PK_OP_IK_PUB_GEN ((0 << 8) | 1 | SX_PK_OP_IK | SX_PK_OP_FLAGS_BIGENDIAN)

/** IK ECC ECDSA signature generation */
#define PK_OP_IK_ECDSA_SIGN ((1 << 8) | 1 | SX_PK_OP_IK | SX_PK_OP_FLAGS_BIGENDIAN)

/** IK ECC point multiplication */
#define PK_OP_IK_ECC_PTMUL ((2 << 8) | 1 | SX_PK_OP_IK | SX_PK_OP_FLAGS_BIGENDIAN)

#define PK_OP_IK_EXIT (0x00 | SX_PK_OP_IK)

#endif
