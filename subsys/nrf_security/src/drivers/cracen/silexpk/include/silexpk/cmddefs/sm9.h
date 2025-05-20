/** SM9 command definitions
 *
 * @file
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CMDDEF_SM9_HEADER_FILE
#define CMDDEF_SM9_HEADER_FILE

#ifdef __cplusplus
extern "C" {
#endif

struct sx_pk_cmd_def;

/**
 * @addtogroup SX_PK_CMDS
 *
 * @{
 */

/** SM9 Exponentiation in GT */
extern const struct sx_pk_cmd_def *const SX_PK_CMD_SM9_EXP;

/** SM9 Point multiplication in G1 */
extern const struct sx_pk_cmd_def *const SX_PK_CMD_SM9_PMULG1;

/** SM9 Point multiplication in G2 */
extern const struct sx_pk_cmd_def *const SX_PK_CMD_SM9_PMULG2;

/** SM9 Pairing */
extern const struct sx_pk_cmd_def *const SX_PK_CMD_SM9_PAIR;

/** SM9 private signature key generation */
extern const struct sx_pk_cmd_def *const SX_PK_CMD_SM9_PRIVSIGKEYGEN;

/** SM9 Signature generation */
extern const struct sx_pk_cmd_def *const SX_PK_CMD_SM9_SIGNATUREGEN;

/** SM9 Signature verification */
extern const struct sx_pk_cmd_def *const SX_PK_CMD_SM9_SIGNATUREVERIFY;

/** SM9 Encrypt private key */
extern const struct sx_pk_cmd_def *const SX_PK_CMD_SM9_PRIVENCRKEYGEN;

/** SM9 Send key */
extern const struct sx_pk_cmd_def *const SX_PK_CMD_SM9_SENDKEY;

/** SM9 Reduce H */
extern const struct sx_pk_cmd_def *const SX_PK_CMD_SM9_REDUCEH;

/** @} */

/** Input slots for ::SX_PK_CMD_SM9_EXP */
struct sx_pk_inops_sm9_exp {
	struct sx_pk_slot h;	 /**< exponent */
	struct sx_pk_slot t;	 /**< polynomial base */
	struct sx_pk_slot g[12]; /**< g in GT */
};

/** Input slots for ::SX_PK_CMD_SM9_PMULG1 */
struct sx_pk_inops_sm9_pmulg1 {
	struct sx_pk_slot p1x0; /**< x-coordinate */
	struct sx_pk_slot p1y0; /**< y-coordinate */
	struct sx_pk_slot ke;	/**< scalar */
	struct sx_pk_slot t;	/**< polynomial base */
};

/** Input slots for ::SX_PK_CMD_SM9_PMULG2 */
struct sx_pk_inops_sm9_pmulg2 {
	struct sx_pk_slot p2x0; /**< x-coordinate 0 */
	struct sx_pk_slot p2x1; /**< x-coordinate 1 */
	struct sx_pk_slot p2y0; /**< y-coordinate 0 */
	struct sx_pk_slot p2y1; /**< y-coordinate 1 */
	struct sx_pk_slot ke;	/**< scalar */
	struct sx_pk_slot t;	/**< polynomial base */
};

/** Input slots for ::SX_PK_CMD_SM9_PAIR */
struct sx_pk_inops_sm9_pair {
	struct sx_pk_slot qx0; /**< Q x-coordinate 0 */
	struct sx_pk_slot qx1; /**< Q x-coordinate 1 */
	struct sx_pk_slot qy0; /**< Q y-coordinate 0 */
	struct sx_pk_slot qy1; /**< Q y-coordinate 1 */
	struct sx_pk_slot px0; /**< P x-coordinate */
	struct sx_pk_slot py0; /**< P y-coordinate */
	struct sx_pk_slot f;   /**< frobenius constant */
	struct sx_pk_slot t;   /**< polynomial base */
};

/** Input slots for ::SX_PK_CMD_SM9_PRIVSIGKEYGEN */
struct sx_pk_inops_sm9_sigpkgen {
	struct sx_pk_slot p1x0; /**< x-coordinate */
	struct sx_pk_slot p1y0; /**< y-coordinate */
	struct sx_pk_slot h;	/**< scalar */
	struct sx_pk_slot ks;	/**< scalar */
	struct sx_pk_slot t;	/**< polynomial base */
};

/** Input slots for ::SX_PK_CMD_SM9_SIGNATUREGEN */
struct sx_pk_inops_sm9_signaturegen {
	struct sx_pk_slot dsx0; /**< x-coordinate */
	struct sx_pk_slot dsy0; /**< y-coordinate */
	struct sx_pk_slot h;	/**< scalar */
	struct sx_pk_slot r;	/**< scalar */
	struct sx_pk_slot t;	/**< polynomial base */
};

/** Input slots for ::SX_PK_CMD_SM9_SIGNATUREVERIFY */
struct sx_pk_inops_sm9_signatureverify {
	struct sx_pk_slot h1;	   /**< scalar */
	struct sx_pk_slot p2x0;	   /**< x-coordinate */
	struct sx_pk_slot p2x1;	   /**< x-coordinate */
	struct sx_pk_slot p2y0;	   /**< y-coordinate */
	struct sx_pk_slot p2y1;	   /**< y-coordinate */
	struct sx_pk_slot ppubsx0; /**< x-coordinate 0 */
	struct sx_pk_slot ppubsx1; /**< x-coordinate 1 */
	struct sx_pk_slot ppubsy0; /**< y-coordinate 0 */
	struct sx_pk_slot ppubsy1; /**< y-coordinate 1 */
	struct sx_pk_slot sx0;	   /**< x-coordinate */
	struct sx_pk_slot sy0;	   /**< y-coordinate */
	struct sx_pk_slot h;	   /**< scalar */
	struct sx_pk_slot f;	   /**< frobenius constant */
	struct sx_pk_slot t;	   /**< polynomial base */
	struct sx_pk_slot g[12];   /**< input */
};

/** Input slots for ::SX_PK_CMD_SM9_PRIVENCRKEYGEN */
struct sx_pk_inops_sm9_privencrkeygen {
	struct sx_pk_slot p2x0; /**< x-coordinate 0 */
	struct sx_pk_slot p2x1; /**< x-coordinate 1 */
	struct sx_pk_slot p2y0; /**< y-coordinate 0 */
	struct sx_pk_slot p2y1; /**< y-coordinate 1 */
	struct sx_pk_slot h;	/**< scalar */
	struct sx_pk_slot ks;	/**< scalar */
	struct sx_pk_slot t;	/**< polynomial base */
};

/** Input slots for ::SX_PK_CMD_SM9_SENDKEY */
struct sx_pk_inops_sm9_sendkey {
	struct sx_pk_slot p1x0;	   /**< x-coordinate */
	struct sx_pk_slot p1y0;	   /**< y-coordinate */
	struct sx_pk_slot ppubex0; /**< x-coordinate */
	struct sx_pk_slot ppubey0; /**< y-coordinate */
	struct sx_pk_slot h;	   /**< scalar */
	struct sx_pk_slot r;	   /**< scalar */
	struct sx_pk_slot t;	   /**< polynomial base */
};

/** Input slots for ::SX_PK_CMD_SM9_REDUCEH */
struct sx_pk_inops_sm9_reduceh {
	struct sx_pk_slot h; /**< scalar */
	struct sx_pk_slot t; /**< polynomial base */
};

#ifdef __cplusplus
}
#endif

#endif
