#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

target_include_directories(app PRIVATE .)

target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/th_ctrl.c)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/th_shell.c)
