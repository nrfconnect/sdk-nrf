/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef REGS_CURVES_HEADERFILE
#define REGS_CURVES_HEADERFILE

/** Operation field flag for ECC: Weierstrass prime field */
#define PK_OP_FLAGS_PRIME 0x00000000

/** Operation field flag for ECC: Weierstrass binary field */
#define PK_OP_FLAGS_EVEN 0x00000080

/** Operation field flag for ECC: NIST P192 curve */
#define PK_OP_FLAGS_SELCUR_P192 0x00400000

/** Operation field flag for ECC: NIST P256 curve */
#define PK_OP_FLAGS_SELCUR_P256 0x00100000

/** Operation field flag for ECC: NIST P384 curve */
#define PK_OP_FLAGS_SELCUR_P384 0x00200000

/** Operation field flag for ECC: NIST P521 curve */
#define PK_OP_FLAGS_SELCUR_P521 0x00300000

/** Operation field flag for Curve 25519 used in X25519*/
#define PK_OP_FLAGS_SELCUR_X25519 0x00500000

/** Operation field flag for Curve 25519 */
#define PK_OP_FLAGS_SELCUR_ED25519 0x00600000

/** Operation field flag for Edwards 448 */
#define PK_OP_FLAGS_EDWARDS_448 0x04000000

#endif
