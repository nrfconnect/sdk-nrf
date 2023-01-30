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

#define DEFAULT_UART_BAUDRATE DT_PROP_OR(DT_NODELABEL(uart1), current_speed, 115200)

#endif /* DEVICE_CFG_H__ */
