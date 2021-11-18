#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_include_directories(include)
target_sources(app PRIVATE ui.c)
target_sources_if_dt_alias("buzzer-pwm" app PRIVATE buzzer.c)
target_sources_if_dt_alias("rgb-pwm" app PRIVATE led_pwm.c)
