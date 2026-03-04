#
# Copyright (c) 2024-2026 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# Platform indepedent source files.
target_sources(app PRIVATE common.c)

if(CONFIG_APP_PLATFORM_DK)
  target_sources(app PRIVATE platform_emulated.c)
elseif(CONFIG_APP_PLATFORM_REF)
  target_sources(app PRIVATE platform_ref.c)
else()
  message(FATAL_ERROR "Unsupported platform")
endif()
