/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#ifdef CONFIG_WIFI_NRF70
#include <platform/nrfconnect/OTAImageProcessorImplWiFi.h>
using OTAImageProcessorBaseImpl = chip::DeviceLayer::OTAImageProcessorImplWiFi;
#else
#include <platform/nrfconnect/OTAImageProcessorImpl.h>
using OTAImageProcessorBaseImpl = chip::DeviceLayer::OTAImageProcessorImpl;
#endif /* CONFIG_WIFI_NRF70 */
