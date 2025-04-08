/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _BACKEND_H__
#define _BACKEND_H__

#include <drivers/gpio/hpf_gpio.h>

#if !defined(CONFIG_HPF_GPIO_BACKEND_ICMSG) && \
	!defined(CONFIG_HPF_GPIO_BACKEND_MBOX) && \
	!defined(CONFIG_HPF_GPIO_BACKEND_ICBMSG)
#error "Define communication backend type"
#endif

/**
 * @brief Callback function called by backend when new packet arrives.
 *
 * @param packet New packet.
 */
typedef void (*backend_callback_t)(hpf_gpio_data_packet_t *packet);

/**
 * @brief Initialize backend.
 *
 * @param callback Function to be called when new packet arrives.
 */
int backend_init(backend_callback_t callback);


#endif /* BACKEND_H__ */
