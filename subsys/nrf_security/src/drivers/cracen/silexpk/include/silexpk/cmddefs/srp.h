/** SRP command definitions
 *
 * @file
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CMDDEF_SRP_HEADER_FILE
#define CMDDEF_SRP_HEADER_FILE

#ifdef __cplusplus
extern "C" {
#endif

struct sx_pk_cmd_def;

/**
 * @addtogroup SX_PK_CMDS
 *
 * @{
 */

/** SRP user key generation */
extern const struct sx_pk_cmd_def *const SX_PK_CMD_SRP_USER_KEY_GEN;

/** SRP user key generation with countermeasures */
extern const struct sx_pk_cmd_def *const SX_PK_CMD_SRP_USER_KEY_GEN_CM;

/** SRP server public key generation */
extern const struct sx_pk_cmd_def *const SX_PK_CMD_SRP_SERVER_PUBLIC_KEY_GEN;

/** SRP server public key generation with countermeasures */
extern const struct sx_pk_cmd_def *const SX_PK_CMD_SRP_SERVER_PUBLIC_KEY_GEN_CM;

/** SRP server session key generation */
extern const struct sx_pk_cmd_def *const SX_PK_CMD_SRP_SERVER_SESSION_KEY_GEN;

/** SRP server session key generation with countermeasures */
extern const struct sx_pk_cmd_def *const SX_PK_CMD_SRP_SERVER_SESSION_KEY_GEN_CM;

/** @} */

/** Input slots for ::SX_PK_CMD_SRP_USER_KEY_GEN */
struct sx_pk_inops_srp_user_keyparams {
	struct sx_pk_slot n; /**< Safe prime number **/
	struct sx_pk_slot g; /**< Generator of the multiplicative group **/
	struct sx_pk_slot a; /**< Random value **/
	/* k * g^x + g^t with t random salt, k value derived by
	 * both sides (for example k = H(n, g))
	 */
	struct sx_pk_slot b;
	/* Hash of (s, p) with s a random salt and p the user password */
	struct sx_pk_slot x;
	struct sx_pk_slot k; /**< Hash of (n, g) **/
	struct sx_pk_slot u; /**< Hash of (g^a, b) **/
};

/** Inputs slots for ::SX_PK_CMD_SRP_SERVER_PUBLIC_KEY_GEN */
struct sx_pk_inops_srp_server_public_key_gen {
	struct sx_pk_slot n; /**< Safe prime */
	struct sx_pk_slot g; /**< Generator */
	struct sx_pk_slot k; /**< Hash digest */
	struct sx_pk_slot v; /**< Exponentiated hash digest */
	struct sx_pk_slot b; /**< Random */
};

/** Inputs slots for ::SX_PK_CMD_SRP_SERVER_SESSION_KEY_GEN */
struct sx_pk_inops_srp_server_session_key_gen {
	struct sx_pk_slot n; /**< Safe prime */
	struct sx_pk_slot a; /**< Random */
	struct sx_pk_slot u; /**< Hash digest */
	struct sx_pk_slot v; /**< Exponentiated hash digest */
	struct sx_pk_slot b; /**< Random */
};

#ifdef __cplusplus
}
#endif

#endif
