/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
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

#define CHIP_CONFIG_LOG_MODULE_Zcl_PROGRESS 0
#define CHIP_CONFIG_LOG_MODULE_InteractionModel_PROGRESS 0
#define CHIP_CONFIG_LOG_MODULE_InteractionModel_DETAIL 0
#define CHIP_CONFIG_LOG_MODULE_DataManagement_PROGRESS 0
#define CHIP_CONFIG_LOG_MODULE_FabricProvisioning_PROGRESS 0
#define CHIP_CONFIG_LOG_MODULE_SecureChannel_PROGRESS 0

#define CHIP_DEVICE_CONFIG_DYNAMIC_ENDPOINT_COUNT CONFIG_BRIDGE_MAX_DYNAMIC_ENDPOINTS_NUMBER
