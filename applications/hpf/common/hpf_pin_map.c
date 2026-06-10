/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "hpf_pin_map.h"

#include <zephyr/sys/util.h>

#if defined(CONFIG_SOC_NRF54L15) || defined(CONFIG_SOC_NRF54LM20A) || defined(CONFIG_SOC_NRF54LM20B)
static const uint8_t hpf_pin_map[] = {
	4,  /* Physical pin 0  */
	0,  /* Physical pin 1  */
	1,  /* Physical pin 2  */
	3,  /* Physical pin 3  */
	2,  /* Physical pin 4  */
	5,  /* Physical pin 5  */
	6,  /* Physical pin 6  */
	7,  /* Physical pin 7  */
	8,  /* Physical pin 8  */
	9,  /* Physical pin 9  */
	10, /* Physical pin 10 */
};

#define HPF_PIN_MAP_VIO_PORT 2
#define HPF_PIN_MAP_VIO_PIN_OFFSET 0

#elif defined(CONFIG_SOC_NRF54LV10A) || defined(CONFIG_SOC_NRF54LC10A)
static const uint8_t hpf_pin_map[] = {
	4,  /* Physical pin 15 */
	0,  /* Physical pin 16 */
	1,  /* Physical pin 17 */
	3,  /* Physical pin 18 */
	2,  /* Physical pin 19 */
	5,  /* Physical pin 20 */
	6,  /* Physical pin 21 */
	7,  /* Physical pin 22 */
	8,  /* Physical pin 23 */
	9,  /* Physical pin 24 */
};

#define HPF_PIN_MAP_VIO_PORT 1
#define HPF_PIN_MAP_VIO_PIN_OFFSET 15

#elif defined(CONFIG_SOC_NRF7120)
static const uint8_t hpf_pin_map[] = {
	6,  /* Physical pin 0  */
	7,  /* Physical pin 1  */
	8,  /* Physical pin 2  */
	9,  /* Physical pin 3  */
	10, /* Physical pin 4  */
	11, /* Physical pin 5  */
	0,  /* Physical pin 6  */
	1,  /* Physical pin 7  */
	2,  /* Physical pin 8  */
	3,  /* Physical pin 9  */
	4,  /* Physical pin 10 */
	5,  /* Physical pin 11 */
};

#define HPF_PIN_MAP_VIO_PORT 2
#define HPF_PIN_MAP_VIO_PIN_OFFSET 0
#endif

#if !defined(HPF_PIN_MAP_VIO_PORT) || !defined(HPF_PIN_MAP_VIO_PIN_OFFSET)
#error "Unsupported SoC"
#endif

#define HPF_PIN_MAP_VIO_PIN_COUNT ARRAY_SIZE(hpf_pin_map)
#define HPF_PIN_MAP_VIO_VALID_PINS_MASK					    \
	GENMASK(HPF_PIN_MAP_VIO_PIN_OFFSET + HPF_PIN_MAP_VIO_PIN_COUNT - 1, \
		HPF_PIN_MAP_VIO_PIN_OFFSET)

uint8_t hpf_pin_map_to_vio_index(uint8_t port, uint8_t pin)
{
	/* Check if the pin and the port can be accessed by VIO. */
	if ((port != HPF_PIN_MAP_VIO_PORT) || (pin < HPF_PIN_MAP_VIO_PIN_OFFSET) ||
		(pin >= (HPF_PIN_MAP_VIO_PIN_OFFSET + HPF_PIN_MAP_VIO_PIN_COUNT))) {
		return HPF_PIN_MAP_VIO_PIN_INVALID;
	}
	return hpf_pin_map[pin - HPF_PIN_MAP_VIO_PIN_OFFSET];
}

uint16_t hpf_pin_map_to_vio_mask(uint32_t gpio_pin_mask)
{
	/* Check if all of the pins specified by the mask can be accessed by VIO. */
	if ((gpio_pin_mask & (~HPF_PIN_MAP_VIO_VALID_PINS_MASK)) != 0) {
		return HPF_PIN_MAP_VIO_MASK_INVALID;
	}
	uint16_t vio_mask = 0;

	for (int i = 0; i < HPF_PIN_MAP_VIO_PIN_COUNT; i++) {
		if (gpio_pin_mask & BIT(i + HPF_PIN_MAP_VIO_PIN_OFFSET)) {
			vio_mask |= BIT(hpf_pin_map[i]);
		}
	}
	return vio_mask;
}
