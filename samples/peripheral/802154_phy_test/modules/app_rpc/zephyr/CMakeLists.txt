#
# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

if (CONFIG_APP_RPC)

zephyr_library()
zephyr_include_directories(include)
zephyr_library_sources_ifdef(CONFIG_NRF_RPC src/app_rpc.c)

endif()
