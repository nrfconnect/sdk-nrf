#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

set(locator_tag_APPLICATION_CONFIG_DIR
    "${CMAKE_CURRENT_LIST_DIR}/../configuration"
    CACHE INTERNAL "Application configuration dir controlled by sysbuild"
)

find_package(Sysbuild REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(sysbuild LANGUAGES)
