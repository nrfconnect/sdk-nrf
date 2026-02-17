/** Edwards curve command definitions
 *
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CMDDEF_EDWARDS_HEADER_FILE
#define CMDDEF_EDWARDS_HEADER_FILE

#ifdef __cplusplus
extern "C" {
#endif

struct sx_pk_cmd_def;

/**
 * @addtogroup SX_PK_CMDS
 *
 * @{
 */

/** EdDSA point multiplication operation
 *
 * All operands for this command use a little endian representation.
 * Operands should be decoded and clamped as defined in specifications
 * for Ed25519 or Ed448.
 */
extern const struct sx_pk_cmd_def *const SX_PK_CMD_EDDSA_PTMUL;

/** EdDSA 2nd part of signature operation
 *
 * All operands for this command use a little endian representation.
 * Operands should be decoded and clamped as defined in specifications
 * for Ed25519 or Ed448.
 */
extern const struct sx_pk_cmd_def *const SX_PK_CMD_EDDSA_SIGN;

/** EdDSA signature verification operation
 *
 * All operands for this command use a little endian representation.
 * Operands should be decoded and clamped as defined in specifications
 * for Ed25519 or Ed448.
 */
extern const struct sx_pk_cmd_def *const SX_PK_CMD_EDDSA_VER;

/** @} */

/** Input slots for ::SX_PK_CMD_EDDSA_SIGN */
struct sx_pk_inops_eddsa_sign {
	struct sx_pk_dblslot k; /**< Scalar with a size double of other operands **/
	struct sx_pk_dblslot r; /**< Signature part 1 **/
	struct sx_pk_slot s;	/**< Signature part 2 **/
};

/** Input slots for ::SX_PK_CMD_EDDSA_VER */
struct sx_pk_inops_eddsa_ver {
	struct sx_pk_dblslot k;	 /**< Scalar with a size double of other operands **/
	struct sx_pk_slot ay;	 /**< Encoded public key  **/
	struct sx_pk_slot sig_s; /**< Signature part 2 **/
	struct sx_pk_slot ry;	 /**< y-coordinate of r **/
};

/** Input slots for ::SX_PK_CMD_EDDSA_PTMUL */
struct sx_pk_inops_eddsa_ptmult {
	struct sx_pk_dblslot r; /**< Scalar **/
};

#ifdef __cplusplus
}
#endif

#endif
