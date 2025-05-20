#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library_named(app_motion_detector)

target_include_directories(app_motion_detector PUBLIC include)

target_link_libraries(app_motion_detector PRIVATE app_ui)

if(CONFIG_APP_PLATFORM_DK)
  target_sources(app_motion_detector PRIVATE platform_dk.c)
elseif(CONFIG_APP_PLATFORM_THINGY)
  target_sources(app_motion_detector PRIVATE platform_thingy.c)
  # Workaround build order issue related to offsets.h not being generated
  # before the platform_thingy.c file which includes it is built.
  add_dependencies(app_motion_detector offsets_h)
else()
  message(FATAL_ERROR "Unsupported platform")
endif()
