#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_include_directories(include)
target_sources(app PRIVATE ui.c)
target_sources_ifdef(CONFIG_UI_BUZZER app PRIVATE buzzer.c)
target_sources_ifdef(CONFIG_UI_NMOS app PRIVATE nmos.c)
target_sources_ifdef(CONFIG_UI_LED_USE_PWM app PRIVATE led_pwm.c)
