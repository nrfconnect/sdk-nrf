#
# Copyright (c) 2024-2025 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

target_include_directories(app PRIVATE include)

# Headers from the include_priv directory must only be used in the scope of this directory.
target_include_directories(app PRIVATE include_priv)

# Platform indepedent source files.
target_sources(app PRIVATE common.c)

if(CONFIG_APP_PLATFORM_DK)
  target_sources(app PRIVATE platform_dk.c)
  target_link_libraries(app PRIVATE app_ui)
elseif(CONFIG_APP_PLATFORM_THINGY)
  target_sources(app PRIVATE platform_thingy.c)
else()
  message(FATAL_ERROR "Unsupported platform")
endif()
