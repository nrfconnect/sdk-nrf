/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _HPF_PIN_MAP_H__
#define _HPF_PIN_MAP_H__

#include <stdint.h>

#define HPF_PIN_MAP_VIO_PIN_INVALID  UINT8_MAX
#define HPF_PIN_MAP_VIO_MASK_INVALID UINT16_MAX

/**
 * @brief Convert GPIO pin number to VIO index.
 *
 * @param[in] port GPIO port number.
 * @param[in] pin  GPIO pin number.
 *
 * @return VIO index corresponding to the given GPIO pin, or HPF_PIN_MAP_VIO_PIN_INVALID
 *         if the pin cannot be accessed by VIO.
 */
uint8_t hpf_pin_map_to_vio_index(uint8_t port, uint8_t pin);

/**
 * @brief Convert GPIO pin mask to VIO mask.
 *
 * @param[in] gpio_pin_mask Mask of GPIO pins.
 *
 * @return VIO mask corresponding to the given GPIO pin mask, or HPF_PIN_MAP_VIO_MASK_INVALID
 *         if any of the pins cannot be accessed by VIO.
 */
uint16_t hpf_pin_map_to_vio_mask(uint32_t gpio_pin_mask);

#endif /* _HPF_PIN_MAP_H__ */
