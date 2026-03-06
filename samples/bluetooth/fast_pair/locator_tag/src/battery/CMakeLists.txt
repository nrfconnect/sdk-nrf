#
# Copyright (c) 2024-2026 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# Platform indepedent source files.
target_sources(app PRIVATE common.c)

if(CONFIG_APP_PLATFORM_DK OR CONFIG_APP_PLATFORM_TAG)
  target_sources(app PRIVATE platform_emulated.c)
elseif(CONFIG_APP_PLATFORM_THINGY)
  target_sources(app PRIVATE platform_thingy.c)
else()
  message(FATAL_ERROR "Unsupported platform")
endif()
