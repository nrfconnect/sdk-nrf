/** DSA command definitions
 *
 * @file
 */
/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CMDDEF_DSA_HEADER_FILE
#define CMDDEF_DSA_HEADER_FILE

#ifdef __cplusplus
extern "C" {
#endif

struct sx_pk_cmd_def;

/**
 * @addtogroup SX_PK_CMDS
 *
 * @{
 */

/** DSA signature generation */
extern const struct sx_pk_cmd_def *const SX_PK_CMD_DSA_SIGN;

/** DSA signature verification */
extern const struct sx_pk_cmd_def *const SX_PK_CMD_DSA_VER;

/** @} */

/** Input slots for ::SX_PK_CMD_DSA_SIGN */
struct sx_pk_inops_dsa_sign {
	struct sx_pk_slot p;	   /**< Prime modulus **/
	struct sx_pk_slot q;	   /**< Prime divisor of p-1 **/
	struct sx_pk_slot g;	   /**< Generator **/
	struct sx_pk_slot k;	   /**< Random value **/
	struct sx_pk_slot privkey; /**< Private key **/
	struct sx_pk_slot h;	   /**< Hash digest **/
};

/** Input slots for ::SX_PK_CMD_DSA_VER */
struct sx_pk_inops_dsa_ver {
	struct sx_pk_slot p;	  /**< Prime modulus **/
	struct sx_pk_slot q;	  /**< Prime divisor of p-1 **/
	struct sx_pk_slot g;	  /**< Generator **/
	struct sx_pk_slot pubkey; /**< Public key **/
	struct sx_pk_slot r;	  /**< First part of signature **/
	struct sx_pk_slot s;	  /**< Second part of signature **/
	struct sx_pk_slot h;	  /**< Hash digest **/
};

#ifdef __cplusplus
}
#endif

#endif
