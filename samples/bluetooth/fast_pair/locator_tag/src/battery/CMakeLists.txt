#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library_named(app_battery)

target_include_directories(app_battery PUBLIC include)
target_include_directories(app_battery PRIVATE include_priv)

# Platform indepedent source files.
target_sources(app_battery PRIVATE common.c)

if(CONFIG_APP_PLATFORM_DK)
  target_sources(app_battery PRIVATE platform_dk.c)
  target_link_libraries(app_battery PRIVATE app_ui)
elseif(CONFIG_APP_PLATFORM_THINGY)
  target_sources(app_battery PRIVATE platform_thingy.c)
else()
  message(FATAL_ERROR "Unsupported platform")
endif()
