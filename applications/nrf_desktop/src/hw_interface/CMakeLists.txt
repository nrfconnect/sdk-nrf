#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/board.c)

target_sources_ifdef(CONFIG_DESKTOP_MOTION_SENSOR_ENABLE app PRIVATE
                     ${CMAKE_CURRENT_SOURCE_DIR}/motion_sensor.c)

target_sources_ifdef(CONFIG_DESKTOP_MOTION_BUTTONS_ENABLE app
                     PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/motion_buttons.c)

target_sources_ifdef(CONFIG_DESKTOP_MOTION_SIMULATED_ENABLE app PRIVATE
                     ${CMAKE_CURRENT_SOURCE_DIR}/motion_simulated.c)

target_sources_ifdef(CONFIG_DESKTOP_BUTTONS_SIM_ENABLE app
		     PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/buttons_sim.c)

target_sources_ifdef(CONFIG_DESKTOP_PASSKEY_BUTTONS app
                     PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/passkey_buttons.c)

target_sources_ifdef(CONFIG_DESKTOP_SELECTOR_HW_ENABLE app
                     PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/selector_hw.c)

target_sources_ifdef(CONFIG_DESKTOP_WHEEL_ENABLE app
                     PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/wheel.c)

target_sources_ifdef(CONFIG_DESKTOP_BATTERY_CHARGER_DISCRETE app
                     PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/battery_charger.c)

target_sources_ifdef(CONFIG_DESKTOP_BATTERY_MEAS app
                     PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/battery_meas.c)
