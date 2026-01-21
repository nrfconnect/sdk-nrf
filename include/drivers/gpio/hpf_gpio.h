/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef HPF_GPIO_H
#define HPF_GPIO_H

#include <nrfx.h>
#include <hpf/hpf_common.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief eGPIO opcodes. */
typedef enum {
	HPF_GPIO_PIN_CONFIGURE = 0, /* Configure eGPIO pin. */
	HPF_GPIO_PIN_CLEAR     = 1, /* Clear eGPIO pin. */
	HPF_GPIO_PIN_SET       = 2, /* Set eGPIO pin. */
	HPF_GPIO_PIN_TOGGLE    = 3, /* Toggle eGPIO pin. */
} hpf_gpio_opcode_t;

/** @brief eGPIO data packet. */
typedef struct __packed {
	uint8_t opcode; /* eGPIO opcode. */
	uint32_t pin; /* Pin number when opcode is HPF_GPIO_PIN_CONFIGURE, pin mask for others. */
	uint8_t port; /* Port number. */
	uint32_t flags; /* Configuration flags when opcode
			 * is HPF_GPIO_PIN_CONFIGURE (gpio_flags_t).
			 * Not used in other cases.
			 */
} hpf_gpio_data_packet_t;

typedef struct {
	struct hpf_shared_data_lock lock;
	hpf_gpio_data_packet_t data;
} hpf_gpio_mbox_data_t;

#ifdef __cplusplus
}
#endif

#endif /* HPF_GPIO_H */
