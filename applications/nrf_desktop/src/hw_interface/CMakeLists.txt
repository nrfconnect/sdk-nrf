#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/board.c)

target_sources_ifdef(CONFIG_DESKTOP_MOTION_OPTICAL_ENABLE app PRIVATE
                     ${CMAKE_CURRENT_SOURCE_DIR}/motion_optical.c)

target_sources_ifdef(CONFIG_DESKTOP_MOTION_BUTTONS_ENABLE app
                     PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/motion_buttons.c)

target_sources_ifdef(CONFIG_DESKTOP_MOTION_SIMULATED_ENABLE app PRIVATE
                     ${CMAKE_CURRENT_SOURCE_DIR}/motion_simulated.c)

target_sources_ifdef(CONFIG_DESKTOP_BUTTONS_ENABLE app
                     PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/buttons.c)

target_sources_ifdef(CONFIG_DESKTOP_WHEEL_ENABLE app
                     PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/wheel.c)

target_sources_ifdef(CONFIG_DESKTOP_LED_ENABLE app
		     PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/leds.c)

target_sources_ifdef(CONFIG_DESKTOP_BATTERY_DISCRETE app
                     PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/battery_discrete.c)
