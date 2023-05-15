/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef ZEPHYR_DRIVERS_NPMX_NPMX_CONFIG_H__
#define ZEPHYR_DRIVERS_NPMX_NPMX_CONFIG_H__

#if defined(NPM1300_ENG_B)
    #include <npmx_config_npm1300_eng_b.h>
#else
    #error "Unknown device."
#endif

#endif /* ZEPHYR_DRIVERS_NPMX_NPMX_CONFIG_H__ */
