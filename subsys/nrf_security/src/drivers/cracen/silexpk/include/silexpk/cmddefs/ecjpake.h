/** EC J-Pake command definitions
 *
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CMDDEF_ECJPAKE_HEADER_FILE
#define CMDDEF_ECJPAKE_HEADER_FILE

#ifdef __cplusplus
extern "C" {
#endif

struct sx_pk_cmd_def;

/**
 * @addtogroup SX_PK_CMDS
 *
 * @{
 */

/** EC J-PAKE Generate zero knowledge proof */
extern const struct sx_pk_cmd_def *const SX_PK_CMD_ECJPAKE_GENERATE_ZKP;

/** EC J-PAKE Verify zero knowledge proof */
extern const struct sx_pk_cmd_def *const SX_PK_CMD_ECJPAKE_VERIFY_ZKP;

/** EC J-PAKE 3 point add */
extern const struct sx_pk_cmd_def *const SX_PK_CMD_ECJPAKE_3PT_ADD;

/** EC J-PAKE Generate step 2 */
extern const struct sx_pk_cmd_def *const SX_PK_CMD_ECJPAKE_GEN_STEP_2;

/** EC J-PAKE Generate session key */
extern const struct sx_pk_cmd_def *const SX_PK_CMD_ECJPAKE_GEN_SESS_KEY;

/** @} */

/** Input slots for ::SX_PK_CMD_ECJPAKE_GENERATE_ZKP */
struct sx_pk_inops_ecjpake_generate_zkp {
	struct sx_pk_slot v; /**< Random input */
	struct sx_pk_slot x; /**< Exponent */
	struct sx_pk_slot h; /**< Hash digest */
};

/** Input slots for ::SX_PK_CMD_ECJPAKE_VERIFY_ZKP */
struct sx_pk_inops_ecjpake_verify_zkp {
	struct sx_pk_slot xv;  /**< Point V on the curve, x-coordinate */
	struct sx_pk_slot yv;  /**< Point V on the curve, y-coordinate */
	struct sx_pk_slot xx;  /**< Point X on the curve, x-coordinate */
	struct sx_pk_slot yx;  /**< Point X on the curve, y-coordinate */
	struct sx_pk_slot r;   /**< Proof */
	struct sx_pk_slot h;   /**< Hash digest */
	struct sx_pk_slot xg2; /**< Point G on the curve, x-coordinate */
	struct sx_pk_slot yg2; /**< Point G on the curve, y-coordinate */
};

/** Input slots for ::SX_PK_CMD_ECJPAKE_3PT_ADD */
struct sx_pk_inops_ecjpake_3pt_add {
	struct sx_pk_slot x2_1; /**< Point X2 on the curve, x-coordinate */
	struct sx_pk_slot x2_2; /**< Point X2 on the curve, y-coordinate */
	struct sx_pk_slot x3_1; /**< Point X3 on the curve, x-coordinate */
	struct sx_pk_slot x3_2; /**< Point X3 on the curve, y-coordinate */
	struct sx_pk_slot x1_1; /**< Point X1 on the curve, x-coordinate */
	struct sx_pk_slot x1_2; /**< Point X1 on the curve, y-coordinate */
};

/** Input slots for ::SX_PK_CMD_ECJPAKE_GEN_STEP_2 */
struct sx_pk_inops_ecjpake_gen_step_2 {
	struct sx_pk_slot x4_1; /**< Point X4 on the curve, x-coordinate */
	struct sx_pk_slot x4_2; /**< Point X4 on the curve, y-coordinate */
	struct sx_pk_slot x3_1; /**< Point X3 on the curve, x-coordinate */
	struct sx_pk_slot x3_2; /**< Point X3 on the curve, y-coordinate */
	struct sx_pk_slot x2s;	/**< Random value * Password */
	struct sx_pk_slot x1_1; /**< Point X1 on the curve, x-coordinate */
	struct sx_pk_slot x1_2; /**< Point X1 on the curve, y-coordinate */
	struct sx_pk_slot s;	/**< Password */
};

/** Input slots for ::SX_PK_CMD_ECJPAKE_GEN_SESS_KEY */
struct sx_pk_inops_ecjpake_gen_sess_key {
	struct sx_pk_slot x4_1; /**< Point X4 on the curve, x-coordinate */
	struct sx_pk_slot x4_2; /**< Point X4 on the curve, y-coordinate */
	struct sx_pk_slot b_1;	/**< Point B on the curve, x-coordinate */
	struct sx_pk_slot b_2;	/**< Point B on the curve, y-coordinate */
	struct sx_pk_slot x2;	/**< Random value */
	struct sx_pk_slot x2s;	/**< Random value * Password */
};

#ifdef __cplusplus
}
#endif

#endif
