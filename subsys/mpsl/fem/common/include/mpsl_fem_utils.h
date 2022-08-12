/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MPSL_FEM_UTILS_H__
#define MPSL_FEM_UTILS_H__

#include <stdint.h>
#if !defined(CONFIG_MPSL_FEM_PIN_FORWARDER)
#include <mpsl_fem_config_common.h>

/** @brief Converts pin number extended with port number to an mpsl_fem_pin_t structure.
 *
 * @param[in]     pin_num    Pin number extended with port number.
 * @param[inout]  p_fem_pin  Pointer to be filled with pin represented as an mpsl_fem_pin_t struct.
 */
void mpsl_fem_extended_pin_to_mpsl_fem_pin(uint32_t pin_num, mpsl_fem_pin_t *p_fem_pin);

#endif /* !defined(CONFIG_MPSL_FEM_PIN_FORWARDER) */

#endif /* MPSL_FEM_UTILS_H__ */
