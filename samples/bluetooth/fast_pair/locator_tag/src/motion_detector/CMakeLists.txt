#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library_named(app_motion_detector)

target_include_directories(app_motion_detector PUBLIC include)

target_sources(app_motion_detector PRIVATE platform_default.c)
