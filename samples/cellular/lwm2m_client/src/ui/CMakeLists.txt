#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_include_directories(include)

target_sources_ifdef(CONFIG_UI_INPUT
	app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ui_input.c)

target_sources_ifdef(CONFIG_UI_SENSE_LED
	app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ui_sense_led.c)

target_sources_ifdef(CONFIG_UI_BUZZER
	app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ui_buzzer.c)

target_sources_ifdef(CONFIG_UI_LED
	app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ui_led.c)
