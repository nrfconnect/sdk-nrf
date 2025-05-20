/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MPSL_FEM_POWER_MODEL_INTERFACE_H__
#define MPSL_FEM_POWER_MODEL_INTERFACE_H__

#include <mpsl_fem_power_model.h>

/**
 * @brief Get the power model used to calculate the power split for a Front-End Module.
 *        The implementation of this function has to be provided if
 *        the MPSL_FEM_POWER_MODEL Kconfig option is selected, either by a built-in
 *        model implementation or by a user to provide the model callbacks.
 *
 * @return Struct containing the power model's callbacks.
 */
const mpsl_fem_power_model_t *mpsl_fem_power_model_to_use_get(void);

#endif /* MPSL_FEM_POWER_MODEL_INTERFACE_H__ */
