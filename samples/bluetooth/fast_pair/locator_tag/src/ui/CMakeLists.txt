#
# Copyright (c) 2024-2026 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

target_sources(app PRIVATE common.c)

if(CONFIG_APP_PLATFORM_DK)
  target_sources(app PRIVATE platform_dk.c)
elseif(CONFIG_APP_PLATFORM_THINGY)
  target_sources(app PRIVATE
    platform_ref.c
    platform_thingy_speaker.c
  )
elseif(CONFIG_APP_PLATFORM_TAG)
  target_sources(app PRIVATE platform_ref.c)
else()
  message(FATAL_ERROR "Unsupported platform")
endif()

zephyr_linker_sources(DATA_SECTIONS request_listener.ld)
