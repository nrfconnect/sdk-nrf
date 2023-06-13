#
# Copyright (c) 2020 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

target_include_directories(app PRIVATE .)

target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/link_api.c)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/link_shell.c)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/link_shell_print.c)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/link_shell_pdn.c)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/link_settings.c)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/link.c)
