/** Elliptic curve cryptographic command definitions
 *
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CMDDEF_ECC_HEADER_FILE
#define CMDDEF_ECC_HEADER_FILE

#ifdef __cplusplus
extern "C" {
#endif

struct sx_pk_cmd_def;

/**
 * @addtogroup SX_PK_CMDS
 *
 * @{
 */

/** Montgomery point multiplication for X25519 and X448
 *
 * All operands for this command use a little endian representation.
 * Operands should be decoded and clamped as defined in specifications
 * for X25519 and X448.
 */
extern const struct sx_pk_cmd_def *const SX_PK_CMD_MONTGOMERY_PTMUL;

/** Elliptic curve ECDSA signature verification operation */
extern const struct sx_pk_cmd_def *const SX_PK_CMD_ECDSA_VER;

/** Elliptic curve ECDSA signature generation operation
 *
 * This operation can be protected with blinding counter measures.
 * When used with counter measures, the operation can randomly
 * fail with SX_ERR_NOT_INVERTIBLE. If it happens, the operation
 * should be retried with a new blinding factor. See the user guide
 * 'countermeasures' section for more information.
 */
extern const struct sx_pk_cmd_def *const SX_PK_CMD_ECDSA_GEN;

/** EC-KCDSA public key generation operation
 *
 * When used with counter measures, the operation can randomly
 * fail with SX_ERR_NOT_INVERTIBLE. If it happens, the operation
 * should be retried with a new blinding factor. See the user guide
 * 'countermeasures' section for more information.
 */
extern const struct sx_pk_cmd_def *const SX_PK_CMD_ECKCDSA_PUBKEY_GEN;

/** EC-KCDSA 2nd part of signature operation */
extern const struct sx_pk_cmd_def *const SX_PK_CMD_ECKCDSA_SIGN;

/** EC-KCDSA signature verification operation */
extern const struct sx_pk_cmd_def *const SX_PK_CMD_ECKCDSA_VER;

/** Elliptic curve point addition operation */
extern const struct sx_pk_cmd_def *const SX_PK_CMD_ECC_PT_ADD;

/** Elliptic curve point multiplication operation
 *
 * This operation can be protected with blinding counter measures.
 * When used with counter measures, the operation can randomly
 * fail with SX_ERR_NOT_INVERTIBLE. If it happens, the operation
 * should be retried with a new blinding factor. See the user guide
 * 'countermeasures' section for more information.
 */
extern const struct sx_pk_cmd_def *const SX_PK_CMD_ECC_PTMUL;

/** Elliptic curve point decompression operation */
extern const struct sx_pk_cmd_def *const SX_PK_CMD_ECC_PT_DECOMP;

/** Elliptic curve check parameters a & b */
extern const struct sx_pk_cmd_def *const SX_PK_CMD_CHECK_PARAM_AB;

/** Elliptic curve check parameter n != p */
extern const struct sx_pk_cmd_def *const SX_PK_CMD_CHECK_PARAM_N;

/** Elliptic curve check x,y point < p */
extern const struct sx_pk_cmd_def *const SX_PK_CMD_CHECK_XY;

/** Elliptic curve point doubling */
extern const struct sx_pk_cmd_def *const SX_PK_CMD_ECC_PT_DOUBLE;

/** Elliptic curve point on curve check */
extern const struct sx_pk_cmd_def *const SX_PK_CMD_ECC_PTONCURVE;

/** SM2 signature generation operation
 *
 * This operation can be protected with blinding counter measures.
 * When used with counter measures, the operation can randomly
 * fail with SX_ERR_NOT_INVERTIBLE. If it happens, the operation
 * should be retried with a new blinding factor. See the user guide
 * 'countermeasures' section for more information.
 */
extern const struct sx_pk_cmd_def *const SX_PK_CMD_SM2_GEN;

/** SM2 signature verification operation */
extern const struct sx_pk_cmd_def *const SX_PK_CMD_SM2_VER;

/** SM2 key exchange operation */
extern const struct sx_pk_cmd_def *const SX_PK_CMD_SM2_EXCH;

/** @} */

/** Input slots for ::SX_PK_CMD_ECKCDSA_SIGN */
struct sx_pk_inops_eckcdsa_sign {
	struct sx_pk_slot d; /**< Private key **/
	struct sx_pk_slot k; /**< Random value **/
	struct sx_pk_slot r; /**< First part of signature **/
	struct sx_pk_slot h; /**< Hash digest **/
};

/** Input slots for ::SX_PK_CMD_MONTGOMERY_PTMUL */
struct sx_pk_inops_montgomery_mult {
	struct sx_pk_slot p; /**< Point P **/
	struct sx_pk_slot k; /**< Scalar **/
};

/** Input slots for ::SX_PK_CMD_ECC_PT_ADD */
struct sx_pk_inops_ecp_add {
	struct sx_pk_slot p1x; /**< x-coordinate of point P1 **/
	struct sx_pk_slot p1y; /**< y-coordinate of point P1 **/
	struct sx_pk_slot p2x; /**< x-coordinate of point P2 **/
	struct sx_pk_slot p2y; /**< y-coordinate of point P2 **/
};

/** Input slots for ::SX_PK_CMD_ECC_PTMUL */
struct sx_pk_inops_ecp_mult {
	struct sx_pk_slot k;  /**< Scalar **/
	struct sx_pk_slot px; /**< x-coordinate of point P **/
	struct sx_pk_slot py; /**< y-coordinate of point P **/
};

/** Input slots for ::SX_PK_CMD_ECC_PT_DOUBLE */
struct sx_pk_inops_ecp_double {
	struct sx_pk_slot px; /**< x-coordinate of point P **/
	struct sx_pk_slot py; /**< y-coordinate of point P **/
};

/** Input slots for ::SX_PK_CMD_ECC_PTONCURVE */
struct sx_pk_inops_ec_ptoncurve {
	struct sx_pk_slot px; /**< x-coordinate of point P **/
	struct sx_pk_slot py; /**< y-coordinate of point P **/
};

/** Input slots for ::SX_PK_CMD_ECC_PT_DECOMP */
struct sx_pk_inops_ec_pt_decompression {
	struct sx_pk_slot x; /**< x-coordinate of compressed point **/
};

/** Input slots for ::SX_PK_CMD_ECDSA_VER &
 * ::SX_PK_CMD_ECKCDSA_VER & ::SX_PK_CMD_SM2_VER
 */
struct sx_pk_inops_ecdsa_verify {
	struct sx_pk_slot qx; /**< x-coordinate of public key **/
	struct sx_pk_slot qy; /**< y-coordinate of public key **/
	struct sx_pk_slot r;  /**< First part of signature **/
	struct sx_pk_slot s;  /**< Second part of signature **/
	struct sx_pk_slot h;  /**< Hash digest **/
};

/** Input slots for ::SX_PK_CMD_ECDSA_GEN & ::SX_PK_CMD_SM2_GEN */
struct sx_pk_inops_ecdsa_generate {
	struct sx_pk_slot d; /**< Private key **/
	struct sx_pk_slot k; /**< Random value **/
	struct sx_pk_slot h; /**< Hash digest **/
};

/** Input slots for ::SX_PK_CMD_ECKCDSA_PUBKEY_GEN */
struct sx_pk_inops_eckcdsa_generate {
	struct sx_pk_slot d; /**< Private key **/
};

/** Inputs slots for ::SX_PK_CMD_CHECK_PARAM_AB */
struct sx_pk_inops_check_param_ab {
	struct sx_pk_slot p; /**< p parameter of curve */
	struct sx_pk_slot a; /**< a parameter of curve */
	struct sx_pk_slot b; /**< b parameter of curve */
};

/** Inputs slots for ::SX_PK_CMD_CHECK_PARAM_N */
struct sx_pk_inops_check_param_n {
	struct sx_pk_slot p; /**< p parameter of curve */
	struct sx_pk_slot n; /**< n parameter of curve */
};

/** Inputs slots for ::SX_PK_CMD_CHECK_XY */
struct sx_pk_inops_check_xy {
	struct sx_pk_slot p; /**< p parameter of curve */
	struct sx_pk_slot x; /**< x-coordinate */
	struct sx_pk_slot y; /**< y-coordinate */
};

/** Input slots for ::SX_PK_CMD_SM2_EXCH */
struct sx_pk_inops_sm2_exchange {
	struct sx_pk_slot d;   /**< Private key **/
	struct sx_pk_slot k;   /**< Random value **/
	struct sx_pk_slot qx;  /**< x-coordinate of public key **/
	struct sx_pk_slot qy;  /**< y-coordinate of public key **/
	struct sx_pk_slot rbx; /**< x-coordinate of random value from B **/
	struct sx_pk_slot rby; /**< y-coordinate of random value from B **/
	struct sx_pk_slot cof; /**< Cofactor **/
	struct sx_pk_slot rax; /**< x-coordinate of random value from A **/
	struct sx_pk_slot w;   /**< (log2(n)/2)-1, with n the curve order **/
};

#ifdef __cplusplus
}
#endif

#endif
