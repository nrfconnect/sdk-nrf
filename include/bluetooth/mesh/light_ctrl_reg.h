/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *  @defgroup bt_mesh_light_ctrl_reg Illuminance regulator
 *  @ingroup bt_mesh_light_ctrl
 *  @{
 *  @brief Illuminance regulator API
 */

#ifndef BT_MESH_LIGHT_CTRL_REG_H__
#define BT_MESH_LIGHT_CTRL_REG_H__

#include <zephyr/bluetooth/mesh.h>

#ifdef __cplusplus
extern "C" {
#endif

struct bt_mesh_light_ctrl_reg_coeff {
	/** Upwards coefficient. */
	float up;
	/** Downwards coefficient. */
	float down;
};

/** Illuminance regulator configuration. */
struct bt_mesh_light_ctrl_reg_cfg {
	/** Regulator integral coefficient. */
	struct bt_mesh_light_ctrl_reg_coeff ki;
	/** Regulator proportional coefficient. */
	struct bt_mesh_light_ctrl_reg_coeff kp;
	/** Regulator dead zone (in percentage). */
	float accuracy;
};

/** Common regulator context */
struct bt_mesh_light_ctrl_reg {
	/** Initialize the regulator.
	 *
	 *  @param[in] reg           Illuminance regulator instance.
	 */
	void (*init)(struct bt_mesh_light_ctrl_reg *reg);
	/** Start the regulator.
	 *
	 *  @param[in] reg           Illuminance regulator instance.
	 *  @param[in] lightness     Current lightness level
	 *                           value equal to @c lightness.
	 */
	void (*start)(struct bt_mesh_light_ctrl_reg *reg, uint16_t lightness);
	/** Stop the regulator.
	 *
	 *  @param[in] reg           Illuminance regulator instance.
	 */
	void (*stop)(struct bt_mesh_light_ctrl_reg *reg);
	/** Regulator configuration. */
	struct bt_mesh_light_ctrl_reg_cfg cfg;
	/** Measured value. */
	float measured;
	/** Regulator output update callback. */
	void (*updated)(struct bt_mesh_light_ctrl_reg *reg, float output);
	/** User data, available in update callback. */
	void *user_data;
/** @cond INTERNAL_HIDDEN */
	float target;
	float prev_target;
	int32_t transition_time;
	int64_t transition_start;
/** @endcond */
};

/** @brief Set the target lightness for the regulator.
 *
 *  Sets the target lightness, also known as the setpoint, for the regulator.
 *  Transition time is optional.
 *
 *  @param[in] reg      Illuminance regulator instance.
 *
 *  @param[in] target   Target lightness (setpoint).
 *
 *  @param[in] transition_time  Transition time until the target lightness
 *                              should reach the specified value. Pass 0
 *                              to change immediately.
 */
void bt_mesh_light_ctrl_reg_target_set(struct bt_mesh_light_ctrl_reg *reg,
				       float target,
				       int32_t transition_time);

/** @brief Get the target lightness for the regulator.
 *
 *  Returns the target lightness, taking the requested transition time into
 *  account, for use in regulator implementations.
 *
 *  @param[in] reg      Illuminance regulator instance.
 *
 *  @returns    The current target lightness, interpolated during transition
 *              time.
 */
float bt_mesh_light_ctrl_reg_target_get(struct bt_mesh_light_ctrl_reg *reg);

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_LIGHT_CTRL_REG_H__ */

/** @} */
