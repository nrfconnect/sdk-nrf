/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "hpf_pin_map.h"

#include <zephyr/sys/util.h>
#include <nrfx.h>

#if !defined(FLPR_VIO_PORT) || !defined(FLPR_VIO_PIN_OFFSET) || !defined(FLPR_VIO_PIN_INDICES)
#error "Unsupported SoC"
#endif

#define HPF_PIN_MAP_VIO_PORT       FLPR_VIO_PORT
#define HPF_PIN_MAP_VIO_PIN_OFFSET FLPR_VIO_PIN_OFFSET

static const uint8_t hpf_pin_map[] = {
	FLPR_VIO_PIN_INDICES
};

#define HPF_PIN_MAP_VIO_PIN_COUNT ARRAY_SIZE(hpf_pin_map)
#define HPF_PIN_MAP_VIO_VALID_PINS_MASK					    \
	GENMASK(HPF_PIN_MAP_VIO_PIN_OFFSET + HPF_PIN_MAP_VIO_PIN_COUNT - 1, \
		HPF_PIN_MAP_VIO_PIN_OFFSET)

uint8_t hpf_pin_map_to_vio_index(uint8_t port, uint8_t pin)
{
	/* Check if the pin and the port can be accessed by VIO. */
	size_t hpf_map_index = pin - HPF_PIN_MAP_VIO_PIN_OFFSET;

	if ((port != HPF_PIN_MAP_VIO_PORT) ||
		(hpf_map_index >= ARRAY_SIZE(hpf_pin_map))) {
		return HPF_PIN_MAP_VIO_PIN_INVALID;
	}
	return hpf_pin_map[hpf_map_index];
}

uint16_t hpf_pin_map_to_vio_mask(uint32_t gpio_pin_mask)
{
	/* Check if all of the pins specified by the mask can be accessed by VIO. */
	if ((gpio_pin_mask & (~HPF_PIN_MAP_VIO_VALID_PINS_MASK)) != 0) {
		return HPF_PIN_MAP_VIO_MASK_INVALID;
	}
	uint16_t vio_mask = 0;

	for (size_t i = 0; i < ARRAY_SIZE(hpf_pin_map); i++) {
		if (gpio_pin_mask & BIT(i + HPF_PIN_MAP_VIO_PIN_OFFSET)) {
			vio_mask |= BIT(hpf_pin_map[i]);
		}
	}
	return vio_mask;
}
