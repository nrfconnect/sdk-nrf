#
# Copyright (c) 2024-2026 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

target_sources(app PRIVATE common.c)

if(CONFIG_APP_PLATFORM_DK)
  target_sources(app PRIVATE platform_dk.c)
elseif(CONFIG_APP_PLATFORM_REF)
  target_sources(app PRIVATE platform_ref.c)
  target_sources_ifdef(CONFIG_APP_UI_USE_HW_SPEAKER app PRIVATE platform_ref_speaker.c)
else()
  message(FATAL_ERROR "Unsupported platform")
endif()

zephyr_linker_sources(DATA_SECTIONS request_listener.ld)
