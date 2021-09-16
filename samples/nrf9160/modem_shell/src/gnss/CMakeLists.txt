#
# Copyright (c) 2020 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

target_include_directories(app PRIVATE .)

target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/gnss_shell.c)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/gnss_common.c)
target_sources_ifdef(CONFIG_MOSH_GNSS_INTERFACE app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/gnss_interface.c)
target_sources_ifdef(CONFIG_MOSH_GNSS_GPS_DRIVER app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/gnss_gps_driver.c)
target_sources_ifdef(CONFIG_SUPL_CLIENT_LIB app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/gnss_supl_support.c)
