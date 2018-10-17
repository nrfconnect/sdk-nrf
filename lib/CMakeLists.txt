#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

add_subdirectory_ifdef(CONFIG_BSD_LIBRARY bsdlib)
add_subdirectory_ifdef(CONFIG_SENSOR_SIM sensor_sim)
