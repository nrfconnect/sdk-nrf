#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

target_include_directories(app PRIVATE .)
target_sources_ifdef(CONFIG_EXTERNAL_SENSORS app PRIVATE
                        ${CMAKE_CURRENT_SOURCE_DIR}/ext_sensors.c)
