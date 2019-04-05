#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

cmake_minimum_required(VERSION 3.8.2)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(bootloader)

# This allows the bootloader sample to be built as a separate image in
# a multi-image build context.

if (IMAGE)
  set(KERNEL_ELF ${logical_target_for_zephyr_elf})
  set_property(
    GLOBAL APPEND PROPERTY
    HEX_FILES_TO_MERGE
    ${PROJECT_BINARY_DIR}/zephyr/${KERNEL_HEX_NAME}
    )
  set_property(
    GLOBAL APPEND PROPERTY
    HEX_FILES_TO_MERGE_TARGET
    ${KERNEL_ELF}
    )
endif()

zephyr_library_sources(src/main.c)

