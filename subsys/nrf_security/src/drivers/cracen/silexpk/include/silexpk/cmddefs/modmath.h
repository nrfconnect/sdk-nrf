/** Modular math command definitions
 *
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CMDDEF_MODMATH_HEADER_FILE
#define CMDDEF_MODMATH_HEADER_FILE

#ifdef __cplusplus
extern "C" {
#endif

struct sx_pk_cmd_def;

/**
 * @addtogroup SX_PK_CMDS
 *
 * @{
 */

/** Modular addition of operands A and B */
extern const struct sx_pk_cmd_def *const SX_PK_CMD_MOD_ADD;

/** Modular substraction of operands A and B */
extern const struct sx_pk_cmd_def *const SX_PK_CMD_MOD_SUB;

/** Modular multiplication of operands A and B with odd modulo */
extern const struct sx_pk_cmd_def *const SX_PK_CMD_ODD_MOD_MULT;

/** Modular inversion of an operand with even modulo */
extern const struct sx_pk_cmd_def *const SX_PK_CMD_EVEN_MOD_INV;

/** Modular reduction of an operand with even modulo */
extern const struct sx_pk_cmd_def *const SX_PK_CMD_EVEN_MOD_REDUCE;

/** Modular reduction of an operand with odd modulo */
extern const struct sx_pk_cmd_def *const SX_PK_CMD_ODD_MOD_REDUCE;

/** Modular division of operands A and B with odd modulo */
extern const struct sx_pk_cmd_def *const SX_PK_CMD_ODD_MOD_DIV;

/** Modular inversion of an operand with odd modulo */
extern const struct sx_pk_cmd_def *const SX_PK_CMD_ODD_MOD_INV;

/** Modular square root **/
extern const struct sx_pk_cmd_def *const SX_PK_CMD_MOD_SQRT;

/** Multiplication **/
extern const struct sx_pk_cmd_def *const SX_PK_CMD_MULT;

/** @} */

/** Input slots for ::SX_PK_CMD_ODD_MOD_INV & ::SX_PK_CMD_ODD_MOD_REDUCE
 * & ::SX_PK_CMD_MOD_SQRT & ::SX_PK_CMD_EVEN_MOD_INV &
 * ::SX_PK_CMD_ODD_MOD_REDUCE
 */
struct sx_pk_inops_mod_single_op_cmd {
	struct sx_pk_slot n; /**< Modulus **/
	struct sx_pk_slot b; /**< Value **/
};

/** Input slots for ::SX_PK_CMD_MOD_ADD & ::SX_PK_CMD_MOD_SUB
 * & ::SX_PK_CMD_ODD_MOD_MULT & ::SX_PK_CMD_ODD_MOD_DIV
 */
struct sx_pk_inops_mod_cmd {
	struct sx_pk_slot n; /**< Modulus **/
	struct sx_pk_slot a; /**< Operand A **/
	struct sx_pk_slot b; /**< Operand B **/
};

/** Input slots for ::SX_PK_CMD_MULT */
struct sx_pk_inops_mult {
	struct sx_pk_slot a; /**< First scalar value **/
	struct sx_pk_slot b; /**< Second scalar value **/
};

#ifdef __cplusplus
}
#endif

#endif
