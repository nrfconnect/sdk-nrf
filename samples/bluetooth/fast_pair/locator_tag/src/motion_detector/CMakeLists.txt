#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library_named(app_motion_detector)

target_include_directories(app_motion_detector PUBLIC include)

if(CONFIG_APP_PLATFORM_DK)
  target_sources(app_motion_detector PRIVATE platform_dk.c)
  target_link_libraries(app_motion_detector PRIVATE app_ui)
else()
  target_sources(app_motion_detector PRIVATE platform_default.c)
endif()
