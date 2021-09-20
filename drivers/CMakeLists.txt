#
# Copyright (c) 2018-2020 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

add_subdirectory_ifdef(CONFIG_SENSOR sensor)
add_subdirectory_ifdef(CONFIG_NETWORKING net)
add_subdirectory_ifdef(CONFIG_MPSL mpsl)
add_subdirectory(hw_cc310)
add_subdirectory(entropy)
add_subdirectory(gps)
add_subdirectory(serial)
