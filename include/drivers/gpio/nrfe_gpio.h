/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NRFE_GPIO_H
#define NRFE_GPIO_H

#include <drivers/nrfx_common.h>
#include <sdp/nrfe_common.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief eGPIO opcodes. */
typedef enum {
	NRFE_GPIO_PIN_CONFIGURE = 0, /* Configure eGPIO pin. */
	NRFE_GPIO_PIN_CLEAR     = 1, /* Clear eGPIO pin. */
	NRFE_GPIO_PIN_SET       = 2, /* Set eGPIO pin. */
	NRFE_GPIO_PIN_TOGGLE    = 3, /* Toggle eGPIO pin. */
} nrfe_gpio_opcode_t;

/** @brief eGPIO data packet. */
typedef struct __packed {
	uint8_t opcode; /* eGPIO opcode. */
	uint32_t pin; /* Pin number when opcode is NRFE_GPIO_PIN_CONFIGURE, pin mask for others. */
	uint8_t port; /* Port number. */
	uint32_t flags; /* Configuration flags when opcode
			 * is NRFE_GPIO_PIN_CONFIGURE (gpio_flags_t).
			 * Not used in other cases.
			 */
} nrfe_gpio_data_packet_t;

typedef struct {
	struct nrfe_shared_data_lock lock;
	nrfe_gpio_data_packet_t data;
} nrfe_gpio_mbox_data_t;

#ifdef __cplusplus
}
#endif

#endif /* NRFE_GPIO_H */
