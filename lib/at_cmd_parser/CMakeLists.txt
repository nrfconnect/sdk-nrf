#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

zephyr_library()
zephyr_library_sources(
    src/at_cmd_parser.c
    src/at_utils.c
    src/at_params.c
)
zephyr_include_directories(include)
