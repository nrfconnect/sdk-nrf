/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 *    @file
 *          Example project configuration file for CHIP.
 *
 *          This is a place to put application or project-specific overrides
 *          to the default configuration values for general CHIP features.
 *
 */

#pragma once

#ifdef CONFIG_THREAD_WIFI_SWITCHING
/*
 * Reduce the code size by disabling ZCL progress level logs.
 * Door Lock cluster logs are extremely verbose and significantly increase the code size.
 */
#define CHIP_CONFIG_LOG_MODULE_Zcl_PROGRESS 0

/* Do not automatically register Thread Network Commissioning instance. */
#define _NO_NETWORK_COMMISSIONING_DRIVER_
#endif
