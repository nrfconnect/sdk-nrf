/**
 * \brief BA414ep public key command codes and flags
 * \file
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef REGS_COMMANDS_HEADERFILE
#define REGS_COMMANDS_HEADERFILE

/** Force the recomputation of r square
 *
 * In absence of this flag, the previously computed value of r square
 * will be used by the accelerator. Leave this flag out ONLY IF the previous
 * operation in the hardware accelerator instance used the same r.
 *
 * If unsure, always include this flag.
 */
#define SX_PK_OP_FLAGS_RESQUARE_R (1 << 31)

/** Big endian operands.
 *
 * All operands of the command are converted from big endian format.
 * Without this flag, operands are interpreted as little endian.
 */
#define SX_PK_OP_FLAGS_BIGENDIAN (1 << 28)

/** Extra flags to enable modulus and exponent randomization
 *
 * In BA414ep datasheet, those flags are called RandKE and RandMod.
 */
#define SX_PK_OP_FLAGS_MOD_CM ((1 << 24) | (1 << 19))

/** Extra flags to enable scalar and projective coordinates randomization
 *
 * In BA414ep datasheet, those flags are called RandKE and RandProj.
 */
#define SX_PK_OP_FLAGS_ECC_CM ((1 << 24) | (1 << 25))

/** Modular addition operation */
#define PK_OP_MOD_ADD (0x01 | SX_PK_OP_FLAGS_RESQUARE_R | SX_PK_OP_FLAGS_BIGENDIAN)

/** Modular substraction operation */
#define PK_OP_MOD_SUB (0x02 | SX_PK_OP_FLAGS_RESQUARE_R | SX_PK_OP_FLAGS_BIGENDIAN)

/** Modular multiplication operation */
#define PK_OP_MOD_MULT (0x03 | SX_PK_OP_FLAGS_RESQUARE_R | SX_PK_OP_FLAGS_BIGENDIAN)

/** Modular multiplication operation */
#define PK_OP_MOD_REDUCE (0x04 | SX_PK_OP_FLAGS_RESQUARE_R | SX_PK_OP_FLAGS_BIGENDIAN)

/** Modular division operation */
#define PK_OP_MOD_DIV (0x05 | SX_PK_OP_FLAGS_RESQUARE_R | SX_PK_OP_FLAGS_BIGENDIAN)

/** Modular inverse operation */
#define PK_OP_MOD_INV (0x06 | SX_PK_OP_FLAGS_RESQUARE_R | SX_PK_OP_FLAGS_BIGENDIAN)

/** Modular square root operation */
#define PK_OP_MOD_SQRT (0x07 | SX_PK_OP_FLAGS_RESQUARE_R | SX_PK_OP_FLAGS_BIGENDIAN)

/** Multiplication operation */
#define PK_OP_MULT (0x08 | SX_PK_OP_FLAGS_RESQUARE_R | SX_PK_OP_FLAGS_BIGENDIAN)

/** Even modular inversion operation */
#define PK_OP_EVEN_MOD_INV (0x09 | SX_PK_OP_FLAGS_RESQUARE_R | SX_PK_OP_FLAGS_BIGENDIAN)

/** Even modular reduction operation */
#define PK_OP_EVEN_MOD_RED (0x0a | SX_PK_OP_FLAGS_RESQUARE_R | SX_PK_OP_FLAGS_BIGENDIAN)

/** Modular exponentiation operation */
#define PK_OP_MDEXP (0x10 | SX_PK_OP_FLAGS_RESQUARE_R | SX_PK_OP_FLAGS_BIGENDIAN)

/** RSA private key computation */
#define PK_OP_RSA_KEYGEN (0x11 | SX_PK_OP_FLAGS_BIGENDIAN)

/** RSA CRT private key parameters computation */
#define PK_OP_CRT_KEYPARAMS (0x12 | SX_PK_OP_FLAGS_BIGENDIAN)

/** RSA modexp operation with Chinese Remainder Theorem */
#define PK_OP_RSA_CRT (0x13 | SX_PK_OP_FLAGS_BIGENDIAN)

/** RSA modular exponentiation operation with countermeasures */
#define PK_OP_RSA_MDEXP_CM                                                                         \
	(0x15 | SX_PK_OP_FLAGS_RESQUARE_R | SX_PK_OP_FLAGS_BIGENDIAN | SX_PK_OP_FLAGS_MOD_CM)

/** DSA signature generation */
#define PK_OP_DSA_SIGN (0x19 | SX_PK_OP_FLAGS_BIGENDIAN)

/** DSA signature verification */
#define PK_OP_DSA_VER (0x1a | SX_PK_OP_FLAGS_BIGENDIAN)

/** Elliptic curve point doubling */
#define PK_OP_ECC_PT_DOUBLE (0x20 | SX_PK_OP_FLAGS_BIGENDIAN)

/** Elliptic curve point decompression */
#define PK_OP_ECC_PT_DECOMP (0x27 | SX_PK_OP_FLAGS_BIGENDIAN)

/** Montgomery curve point multiplication */
#define PK_OP_MG_PTMUL (0x28)

/** Elliptic curve ECDSA signature verification operation */
#define PK_OP_ECDSA_VER (0x31 | SX_PK_OP_FLAGS_BIGENDIAN)

/** Elliptic curve ECDSA signature generation operation */
#define PK_OP_ECDSA_GEN (0x30 | SX_PK_OP_FLAGS_BIGENDIAN)

/** EC-KCDSA public key generation operation */
#define PK_OP_ECKCDSA_PUBKEY_GEN (0x33 | SX_PK_OP_FLAGS_BIGENDIAN)

/** EC-KCDSA 2nd part of signature operation */
#define PK_OP_ECKCDSA_SIGN (0x34 | SX_PK_OP_FLAGS_BIGENDIAN)

/** EC-KCDSA signature verification operation */
#define PK_OP_ECKCDSA_VER (0x35 | SX_PK_OP_FLAGS_BIGENDIAN)

/** Elliptic curve point addition operation */
#define PK_OP_ECC_PT_ADD (0x21 | SX_PK_OP_FLAGS_BIGENDIAN)

/** Elliptic curve point multiplication operation */
#define PK_OP_ECC_PTMUL (0x22 | SX_PK_OP_FLAGS_BIGENDIAN)

/** Elliptic curve check parameter a & b */
#define PK_OP_CHECK_PARAM_AB (0x23 | SX_PK_OP_FLAGS_BIGENDIAN)

/** Elliptic curve check parameter n */
#define PK_OP_CHECK_PARAM_N (0x24 | SX_PK_OP_FLAGS_BIGENDIAN)

/** Elliptic curve check x,y point */
#define PK_OP_CHECK_XY (0x25 | SX_PK_OP_FLAGS_BIGENDIAN)

/** SM2 signature generation */
#define PK_OP_SM2_GEN (0x2D | SX_PK_OP_FLAGS_BIGENDIAN)

/** SM2 signature verification */
#define PK_OP_SM2_VER (0x2E | SX_PK_OP_FLAGS_BIGENDIAN)

/** SM2 key exchange operation */
#define PK_OP_SM2_EXCH (0x2F | SX_PK_OP_FLAGS_BIGENDIAN)

/** Elliptic curve point on curve check */
#define PK_OP_ECC_PTONCURVE (0x26 | SX_PK_OP_FLAGS_BIGENDIAN)

/** EDDSA point multiplication operation */
#define PK_OP_EDDSA_PTMUL (0x3B)

/** EDDSA 2nd part of signature operation */
#define PK_OP_EDDSA_SIGN (0x3C)

/** EDDSA signature verification operation */
#define PK_OP_EDDSA_VERIFY (0x3D)

/** Point multiplication operation on edwards curve */
#define PK_OP_EDWARDS_PTMUL (0x3E)

/** Miller Rabin primality test */
#define PK_OP_MILLER_RABIN (0x42 | SX_PK_OP_FLAGS_BIGENDIAN)

/** SM9 exponentiation */
#define PK_OP_SM9_EXP (0x50 | SX_PK_OP_FLAGS_BIGENDIAN)

/** SM9 point multiplication in G1 */
#define PK_OP_SM9_PMULG1 (0x51 | SX_PK_OP_FLAGS_BIGENDIAN)

/** SM9 point multiplication in G2 */
#define PK_OP_SM9_PMULG2 (0x52 | SX_PK_OP_FLAGS_BIGENDIAN)

/** SM9 pair */
#define PK_OP_SM9_PAIR (0x53 | SX_PK_OP_FLAGS_BIGENDIAN)

/** SM9 private signature key generation */
#define PK_OP_SM9_PRIVSIGKEYGEN (0x54 | SX_PK_OP_FLAGS_BIGENDIAN)

/** SM9 signature generation */
#define PK_OP_SM9_SIGNATUREGEN (0x55 | SX_PK_OP_FLAGS_BIGENDIAN)

/** SM9 signature verification */
#define PK_OP_SM9_SIGNATUREVERIFY (0x56 | SX_PK_OP_FLAGS_BIGENDIAN)

/** SM9 private encryption key generation */
#define PK_OP_SM9_PRIVENCRKEYGEN (0x57 | SX_PK_OP_FLAGS_BIGENDIAN)

/** SM9 send key */
#define PK_OP_SM9_SENDKEY (0x58 | SX_PK_OP_FLAGS_BIGENDIAN)

/** SM9 reduce h */
#define PK_OP_SM9_REDUCEH (0x59 | SX_PK_OP_FLAGS_BIGENDIAN)

/** EC J-PAKE Generate zero knowledge proof */
#define PK_OP_ECJPAKE_GENERATE_ZKP (0x36 | SX_PK_OP_FLAGS_BIGENDIAN)

/** EC J-PAKE Verify zero knowledge proof */
#define PK_OP_ECJPAKE_VERIFY_ZKP (0x37 | SX_PK_OP_FLAGS_BIGENDIAN)

/** EC J-PAKE 3 Point addition */
#define PK_OP_ECJPAKE_3PT_ADD (0x38 | SX_PK_OP_FLAGS_BIGENDIAN)

/** EC J-PAKE Generate session key */
#define PK_OP_ECJPAKE_GEN_SESS_KEY (0x39 | SX_PK_OP_FLAGS_BIGENDIAN)

/** EC J-PAKE Perform step 2 calculation */
#define PK_OP_ECJPAKE_GEN_STEP_2 (0x3a | SX_PK_OP_FLAGS_BIGENDIAN)

/** SRP user session key generation operation */
#define SX_PK_OP_SRP_USER_KEY_GEN (0x1c | SX_PK_OP_FLAGS_BIGENDIAN)

/** SRP server public key generation */
#define PK_OP_SRP_SERVER_PUBLIC_KEY_GEN (0x1e | SX_PK_OP_FLAGS_BIGENDIAN)

/** SRP server session key generation */
#define PK_OP_SRP_SERVER_SESSION_KEY_GEN (0x1b | SX_PK_OP_FLAGS_BIGENDIAN)

/** Selected predefined curve field mask */
#define PK_OP_FLAGS_SELCUR_MASK 0x00F00000

#endif
