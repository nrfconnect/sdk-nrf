/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *  @defgroup bt_mesh_light_ctrl_reg_spec Specification-defined illuminance regulator
 *  @ingroup bt_mesh_light_ctrl
 *  @{
 *  @brief Specification-defined illuminance regulator
 */

#ifndef BT_MESH_LIGHT_CTRL_REG_SPEC_H__
#define BT_MESH_LIGHT_CTRL_REG_SPEC_H__

#include <bluetooth/mesh/light_ctrl_reg.h>

#ifdef __cplusplus
extern "C" {
#endif

/**  @def BT_MESH_LIGHT_CTRL_REG_SPEC_INIT
 *
 *   @brief Initialization macro for @ref bt_mesh_light_ctrl_reg_spec.
 */
#define BT_MESH_LIGHT_CTRL_REG_SPEC_INIT                                       \
	{                                                                      \
		.reg = {                                                       \
			.init = bt_mesh_light_ctrl_reg_spec_init,              \
			.start = bt_mesh_light_ctrl_reg_spec_start,            \
			.stop = bt_mesh_light_ctrl_reg_spec_stop               \
		}                                                              \
	}

/** Specification-defined illuminance regulator context. */
struct bt_mesh_light_ctrl_reg_spec {
	/** Common regulator context. */
	struct bt_mesh_light_ctrl_reg reg;
	/** Regulator step timer. */
	struct k_work_delayable timer;
	/** Internal integral sum. */
	float i;
	/** Regulator enabled flag. */
	bool enabled;
	/* If true, internal integral sum can be negative until it becomes positive. */
	bool neg;
};

/** @cond INTERNAL_HIDDEN */
void bt_mesh_light_ctrl_reg_spec_init(struct bt_mesh_light_ctrl_reg *reg);
void bt_mesh_light_ctrl_reg_spec_start(struct bt_mesh_light_ctrl_reg *reg, uint16_t lightness);
void bt_mesh_light_ctrl_reg_spec_stop(struct bt_mesh_light_ctrl_reg *reg);
/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_LIGHT_CTRL_REG_SPEC_H__ */

/** @} */
