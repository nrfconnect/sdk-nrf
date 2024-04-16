/** Modular exponentiation command definitions
 *
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CMDDEF_MODEXP_HEADER_FILE
#define CMDDEF_MODEXP_HEADER_FILE

#ifdef __cplusplus
extern "C" {
#endif

struct sx_pk_cmd_def;

/**
 * @addtogroup SX_PK_CMDS
 *
 * @{
 */

/** Modular exponentiation operation
 *
 * Use this operation with non-secret/public operands
 */
extern const struct sx_pk_cmd_def *const SX_PK_CMD_MOD_EXP;

/** Modular exponentiation operation in the finite field
 *
 * The modulo needs to be prime. This does not work for RSA which
 * uses integer factorization crypto (IFC) and is not in a finite field.
 * Use this operation with secret/private operands
 */
extern const struct sx_pk_cmd_def *const SX_PK_CMD_FF_MODEXP;

/** @} */

/** Input slots for ::SX_PK_CMD_MOD_EXP &
 * ::SX_PK_CMD_FF_MODEXP
 */
struct sx_pk_inops_mod_exp {
	struct sx_pk_slot m;	 /**< Modulus **/
	struct sx_pk_slot input; /**< Base **/
	struct sx_pk_slot exp;	 /**< Exponent **/
};

#ifdef __cplusplus
}
#endif

#endif
