/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *  @defgroup regulator_pi_reg_bt_mesh Light Lightness Control Spec Regulator
 *  @{
 *  @brief Light Lightness Control spec regulator
 */

#ifndef REGULATOR_PI_REG_BT_MESH_H__
#define REGULATOR_PI_REG_BT_MESH_H__

#include <regulator/pi_reg.h>

#ifdef __cplusplus
extern "C" {
#endif

/**  @def REGULATOR_PI_REG_BT_MESH_INIT
 *
 *   @brief Initialization macro for @ref regulator_pi_reg_bt_mesh.
 */
#define REGULATOR_PI_REG_BT_MESH_INIT                                          \
	{                                                                          \
		.reg = {                                                               \
			.init = regulator_pi_reg_bt_mesh_init,                             \
			.start = regulator_pi_reg_bt_mesh_start,                           \
			.stop = regulator_pi_reg_bt_mesh_stop                              \
		}                                                                      \
	}

/** Light Lightness Control spec regulator context. */
struct regulator_pi_reg_bt_mesh {
	/** Common regulator context. */
	struct regulator_pi_reg reg;
	/** Regulator step timer. */
	struct k_work_delayable timer;
	/** Internal integral sum. */
	float i;
	/** Regulator enabled flag. */
	bool enabled;
};

/** @cond INTERNAL_HIDDEN */
void regulator_pi_reg_bt_mesh_init(struct regulator_pi_reg *reg);
void regulator_pi_reg_bt_mesh_start(struct regulator_pi_reg *reg);
void regulator_pi_reg_bt_mesh_stop(struct regulator_pi_reg *reg);
/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* REGULATOR_PI_REG_BT_MESH_H__ */

/** @} */
