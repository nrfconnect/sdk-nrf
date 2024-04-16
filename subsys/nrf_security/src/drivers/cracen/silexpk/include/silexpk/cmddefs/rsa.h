/** RSA command definitions
 *
 * @file
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CMDDEF_RSA_HEADER_FILE
#define CMDDEF_RSA_HEADER_FILE

#ifdef __cplusplus
extern "C" {
#endif

struct sx_pk_cmd_def;

/**
 * @addtogroup SX_PK_CMDS
 *
 * @{
 */

/** RSA modular exponentiation operation with countermeasures*/
extern const struct sx_pk_cmd_def *const SX_PK_CMD_RSA_MOD_EXP_CM;

/** Modular exponentiation operation (for RSA) with Chinese Remainder Theorem */
extern const struct sx_pk_cmd_def *const SX_PK_CMD_MOD_EXP_CRT;

/** RSA private key including lambda(n) computation from primes
 *
 * Lambda(n) is also called the Carmichael's totient function or
 * Carmichael function.
 */
extern const struct sx_pk_cmd_def *const SX_PK_CMD_RSA_KEYGEN;

/** RSA CRT private key parameters computation */
extern const struct sx_pk_cmd_def *const SX_PK_CMD_RSA_CRT_KEYPARAMS;

/** Round of Miller-Rabin primality test */
extern const struct sx_pk_cmd_def *const SX_PK_CMD_MILLER_RABIN;

/** @} */

/** Input slots for ::SX_PK_CMD_RSA_MOD_EXP_CM */
struct sx_pk_inops_rsa_mod_exp_cm {
	struct sx_pk_slot m;	    /**< Modulus **/
	struct sx_pk_slot lambda_n; /**< Lambda_n **/
	struct sx_pk_slot input;    /**< Base **/
	struct sx_pk_slot exp;	    /**< Exponent **/
	struct sx_pk_slot blind;    /**< Blinding factor **/
};

/** Input slots for ::SX_PK_CMD_MOD_EXP_CRT */
struct sx_pk_inops_crt_mod_exp {
	struct sx_pk_slot p;	/**< Prime number p **/
	struct sx_pk_slot q;	/**< Prime number q **/
	struct sx_pk_slot in;	/**< Input **/
	struct sx_pk_slot dp;	/**< d mod (p-1), with d the private key **/
	struct sx_pk_slot dq;	/**< d mod (q-1), with d the private key **/
	struct sx_pk_slot qinv; /**< q^(-1) mod p **/
};

/** Input slots for ::SX_PK_CMD_RSA_KEYGEN */
struct sx_pk_inops_rsa_keygen {
	struct sx_pk_slot p; /**< Prime number p **/
	struct sx_pk_slot q; /**< Prime number q **/
	struct sx_pk_slot e; /**< Public exponent **/
};

/** Input slots for ::SX_PK_CMD_RSA_CRT_KEYPARAMS */
struct sx_pk_inops_rsa_crt_keyparams {
	struct sx_pk_slot p;	   /**< Prime number p **/
	struct sx_pk_slot q;	   /**< Prime number q **/
	struct sx_pk_slot privkey; /**< Private key **/
};

/** Input slots for ::SX_PK_CMD_MILLER_RABIN */
struct sx_pk_inops_miller_rabin {
	struct sx_pk_slot n; /**< Candidate prime value **/
	struct sx_pk_slot a; /**< Random value **/
};

#ifdef __cplusplus
}
#endif

#endif
