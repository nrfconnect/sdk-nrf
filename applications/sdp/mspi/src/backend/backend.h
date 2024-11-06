/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _BACKEND_H__
#define _BACKEND_H__

#include <drivers/gpio/nrfe_gpio.h>

#if !IS_ENABLED(CONFIG_GPIO_NRFE_EGPIO_BACKEND_ICMSG) && \
	!IS_ENABLED(CONFIG_GPIO_NRFE_EGPIO_BACKEND_MBOX) && \
	!IS_ENABLED(CONFIG_GPIO_NRFE_EGPIO_BACKEND_ICBMSG)
#error "Define communication backend type"
#endif

/**
 * @brief Callback function called by backend when new packet arrives.
 *
 * @param packet New packet.
 */
typedef void (*backend_callback_t)(nrfe_gpio_data_packet_t *packet);

/**
 * @brief Initialize backend.
 *
 * @param callback Function to be called when new packet arrives.
 */
int backend_init(backend_callback_t callback);


#endif /* BACKEND_H__ */
