#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# The sample uses the sysbuild/configuration/<board> scheme for the sysbuild configuration files.
set(SB_APPLICATION_CONFIG_DIR
    "${CMAKE_CURRENT_LIST_DIR}/configuration/\${NORMALIZED_BOARD_TARGET}")

find_package(Sysbuild REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(sysbuild LANGUAGES)
