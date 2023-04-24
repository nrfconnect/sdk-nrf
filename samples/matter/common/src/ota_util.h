/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include <platform/nrfconnect/ExternalFlashManager.h>

#if CONFIG_CHIP_OTA_REQUESTOR
#include "ota_image_processor_base_impl.h"
#include <platform/nrfconnect/OTAImageProcessorImpl.h>

/**
 * Select recommended OTA image processor implementation.
 *
 * If the application uses QSPI external flash and enables API for controlling
 * power states of peripherals, select the implementation that automatically
 * powers off the external flash when no longer needed. Otherwise, select the
 * most basic implementation.
 */
chip::DeviceLayer::OTAImageProcessorImpl &GetOTAImageProcessor();

/** Initialize basic OTA requestor.
 *
 * Initialize all necessary components and start the OTA requestor state machine.
 * Assume that the device is not able to ask a user for consent before applying
 * an update so the confirmation must be done on the OTA provider side.
 */
void InitBasicOTARequestor();

#endif /* CONFIG_CHIP_OTA_REQUESTOR */

/**
 * Get FlashHandler static instance.
 *
 * Returned object can be used to control the QSPI external flash,
 * which can be introduced into sleep mode and woken up on demand.
 */
chip::DeviceLayer::ExternalFlashManager &GetFlashHandler();
