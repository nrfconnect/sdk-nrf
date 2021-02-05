#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_include_directories(include)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ui.c)
target_sources_ifdef(CONFIG_UI_BUTTON
	app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/button.c
	)
target_sources_ifdef(CONFIG_UI_BUZZER
	app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/buzzer.c
	)
target_sources_ifdef(
	CONFIG_UI_LED_USE_PWM
	app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/led_pwm.c
	)
