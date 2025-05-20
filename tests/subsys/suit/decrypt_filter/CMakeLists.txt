#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(integration_test_suit_decrypt_filter)
include(../cmake/test_template.cmake)

# Create a fake suit_mci interface library so that suit_decrypt_filter can link to it
zephyr_interface_library_named(suit_mci)
target_include_directories(suit_mci INTERFACE ${ZEPHYR_NRF_MODULE_DIR}/subsys/suit/mci/include)

zephyr_library_link_libraries(suit_stream_filters_interface)
zephyr_library_link_libraries(suit_stream_sources_interface)
zephyr_library_link_libraries(suit_mci)
zephyr_library_link_libraries(suit_decrypt_test_utils)
