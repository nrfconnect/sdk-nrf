/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MPSL_FEM_UTILS_H__
#define MPSL_FEM_UTILS_H__

#if defined(CONFIG_MPSL_FEM_PIN_FORWARDER)

#include <stdint.h>

/** @brief Embeds GPIO port number into GPIO pin number.
 *
 * @param[inout]  pin  Pointer to be filled with updated GPIO pin number.
 * @param[in]     lbl  Label property from a gpio phandle-array property the update
 *                     should be done for.
 */
void mpsl_fem_pin_extend_with_port(uint8_t *pin, const char *lbl);

#endif /* defined(CONFIG_MPSL_FEM_PIN_FORWARDER) */

#endif /* MPSL_FEM_UTILS_H__ */
