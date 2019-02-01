#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/board.c)

target_sources_ifdef(CONFIG_DESKTOP_ACCEL_LIS3DH app
                     PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/accel_lis3dh.c)

target_sources_ifdef(CONFIG_DESKTOP_ACCEL_BMA400 app
                     PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/accel_bma400.c)

target_sources_ifdef(CONFIG_DESKTOP_MOTION_OPTICAL_ENABLE app PRIVATE
                     ${CMAKE_CURRENT_SOURCE_DIR}/motion_optical.c
                     ${CMAKE_CURRENT_SOURCE_DIR}/motion_optical_priv.c)

target_sources_ifdef(CONFIG_DESKTOP_MOTION_TOUCHPAD_ENABLE app PRIVATE
                     ${CMAKE_CURRENT_SOURCE_DIR}/motion_touchpad.c)

target_sources_ifdef(CONFIG_DESKTOP_MOTION_BUTTONS_ENABLE app
                     PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/motion_buttons.c)

target_sources_ifdef(CONFIG_DESKTOP_MOTION_SIMULATED_ENABLE app PRIVATE
                     ${CMAKE_CURRENT_SOURCE_DIR}/motion_simulated.c)

target_sources_ifdef(CONFIG_DESKTOP_BUTTONS_EXPANDER_ENABLE app
                     PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/buttons_expander.c)

target_sources_ifdef(CONFIG_DESKTOP_BUTTONS_MATRIX_ENABLE app
                     PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/buttons_matrix.c)

target_sources_ifdef(CONFIG_DESKTOP_WHEEL_ENABLE app
                     PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/wheel.c)

target_sources_ifdef(CONFIG_DESKTOP_LED_ENABLE app
		     PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/leds.c)

target_sources_ifdef(CONFIG_DESKTOP_BATTERY_DISCRETE app
                     PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/battery_discrete.c)
