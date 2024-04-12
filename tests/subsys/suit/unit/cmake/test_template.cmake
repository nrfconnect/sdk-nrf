#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# Override board, since all tests must use unit test board
set(BOARD unit_testing)

# Find the path to the Zephyr directory
find_package(Zephyr COMPONENTS unittest REQUIRED HINTS $ENV{ZEPHYR_BASE})

# Define common paths
set(SUIT_SUBSYS_DIR ${ZEPHYR_NRF_MODULE_DIR}/subsys/suit)

# Link manifest library, required by suit_types.h, suit_processor.h and suit_seq_exec.h
target_link_libraries(testbinary PRIVATE manifest)

add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../mocks" "${PROJECT_BINARY_DIR}/test_mocks")
