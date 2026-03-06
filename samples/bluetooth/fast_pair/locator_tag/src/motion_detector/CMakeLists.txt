#
# Copyright (c) 2024-2025 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(CONFIG_APP_PLATFORM_DK)
  target_sources(app PRIVATE platform_dk.c)
elseif(CONFIG_APP_PLATFORM_THINGY OR CONFIG_APP_PLATFORM_TAG)
  target_sources(app PRIVATE platform_adxl36x.c)
else()
  message(FATAL_ERROR "Unsupported platform")
endif()
