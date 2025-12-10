#
# Copyright (c) 2024-2025 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

target_link_libraries(app PRIVATE app_ui)

if(CONFIG_APP_PLATFORM_DK)
  target_sources(app PRIVATE platform_dk.c)
elseif(CONFIG_APP_PLATFORM_THINGY)
  target_sources(app PRIVATE platform_thingy.c)
else()
  message(FATAL_ERROR "Unsupported platform")
endif()
