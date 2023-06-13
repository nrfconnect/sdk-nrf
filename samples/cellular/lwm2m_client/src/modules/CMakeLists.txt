#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_include_directories(include)

target_sources_ifdef(CONFIG_SENSOR_MODULE
    app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/sensor_module.c
    )
