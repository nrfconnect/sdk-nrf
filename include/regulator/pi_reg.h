/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *  @defgroup regulator_pi_reg PI regulator
 *  @{
 *  @brief PI regulator API
 */

#ifndef REGULATOR_PI_REG_H__
#define REGULATOR_PI_REG_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct regulator_pi_reg_coeff {
	/** Upwards coefficient. */
	float up;
	/** Downwards coefficient. */
	float down;
};

/** PI regulator configuration. */
struct regulator_pi_reg_cfg {
	/** Regulator integral coefficient. */
	struct regulator_pi_reg_coeff ki;
	/** Regulator proportional coefficient. */
	struct regulator_pi_reg_coeff kp;
	/** Regulator dead zone (in percentage). */
	float accuracy;
};

/** Common regulator context */
struct regulator_pi_reg {
	/** Initialize the regulator. */
	void (*init)(struct regulator_pi_reg *reg);
	/** Start the regulator. */
	void (*start)(struct regulator_pi_reg *reg);
	/** Stop the regulator. */
	void (*stop)(struct regulator_pi_reg *reg);
	/** Regulator configuration. */
	struct regulator_pi_reg_cfg cfg;
	/** Measured value. */
	float measured;
	/** Regulator output update callback. */
	void (*updated)(struct regulator_pi_reg *reg, float output);
	/** User data, available in update callback. */
	void *user_data;
/** @cond INTERNAL_HIDDEN */
	float target;
	float prev_target;
	int32_t transition_time;
	int64_t transition_start;
/** @endcond */
};

/** @brief Set the target for the regulator.
 *
 *  Sets the target, also known as the setpoint, for the regulator.
 *  Transition time is optional.
 *
 *  @param[in] reg      Regulator instance.
 *
 *  @param[in] target   Target (setpoint).
 *
 *  @param[in] transition_time  Transition time until the target
 *                              should reach the specified value. Pass 0
 *                              to change immediately.
 */
void regulator_pi_reg_target_set(struct regulator_pi_reg *reg,
				       float target,
				       int32_t transition_time);

/** @brief Get the target for the regulator.
 *
 *  Returns the target, taking the requested transition time into
 *  account, for use in regulator implementations.
 *
 *  @param[in] reg      Regulator instance.
 *
 *  @returns    The current target, interpolated during transition
 *              time.
 */
float regulator_pi_reg_target_get(struct regulator_pi_reg *reg);

#ifdef __cplusplus
}
#endif

#endif /* REGULATOR_PI_REG_H__ */

/** @} */
