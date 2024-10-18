#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library_named(app_ui)

target_include_directories(app_ui PUBLIC include)
target_include_directories(app_ui PRIVATE include_priv)

target_sources(app_ui PRIVATE common.c)

if(CONFIG_APP_PLATFORM_DK)
  target_sources(app_ui PRIVATE platform_dk.c)
elseif(CONFIG_APP_PLATFORM_THINGY)
  target_sources(app_ui PRIVATE
    platform_thingy.c
    platform_thingy_speaker.c
  )
else()
  message(FATAL_ERROR "Unsupported platform")
endif()

zephyr_linker_sources(DATA_SECTIONS request_listener.ld)
