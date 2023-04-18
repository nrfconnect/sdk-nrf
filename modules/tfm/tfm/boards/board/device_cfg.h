/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DEVICE_CFG_H__
#define DEVICE_CFG_H__

/**
 * \file device_cfg.h
 * \brief
 * This is the default device configuration file with all peripherals
 * defined and configured to be use via the secure and/or non-secure base
 * address.
 */
/* ARRAY_SIZE causes a conflict as it is defined both by TF-M and indirectly by devicetree.h */
#undef ARRAY_SIZE
#include <autoconf.h>
#include <zephyr/devicetree.h>

#define TFM_UART uart##NRF_SECURE_UART_INSTANCE

#define DEFAULT_UART_BAUDRATE DT_PROP_OR(DT_NODELABEL(TFM_UART), current_speed, 115200)

#if (defined(CONFIG_TFM_SECURE_UART0) || DOMAIN_NS == 1U) && \
	defined(CONFIG_TFM_UART0_HWFC_ENABLED)
#define DEFAULT_UART_CONTROL ARM_USART_FLOW_CONTROL_RTS_CTS
#elif defined(CONFIG_TFM_SECURE_UART1) && \
	defined(CONFIG_TFM_UART1_HWFC_ENABLED)
#define DEFAULT_UART_CONTROL ARM_USART_FLOW_CONTROL_RTS_CTS
#else
#define DEFAULT_UART_CONTROL 0
#endif

#endif /* DEVICE_CFG_H__ */
