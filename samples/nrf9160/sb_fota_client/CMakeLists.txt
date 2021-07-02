#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

cmake_minimum_required(VERSION 3.8.2)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(app)

# NORDIC SDK APP START
target_sources(app PRIVATE src/main.c)
zephyr_library_sources_ifdef(CONFIG_SHELL src/ping.c)
zephyr_library_sources_ifdef(CONFIG_SHELL src/shell.c)
# NORDIC SDK APP END
