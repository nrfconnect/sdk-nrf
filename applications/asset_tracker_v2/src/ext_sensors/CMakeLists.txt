#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_include_directories(.)
target_sources_ifdef(CONFIG_EXTERNAL_SENSORS app PRIVATE
                        ${CMAKE_CURRENT_SOURCE_DIR}/ext_sensors.c)
