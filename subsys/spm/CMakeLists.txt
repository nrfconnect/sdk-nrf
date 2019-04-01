#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

zephyr_sources(spm.c)

if (IMAGE)
  # Only perform merge operation when being part of multi image build.
  set_property(
    GLOBAL APPEND PROPERTY
    HEX_FILES_TO_MERGE
    ${PROJECT_BINARY_DIR}/${KERNEL_HEX_NAME}

    )
  set_property(
    GLOBAL APPEND PROPERTY
    HEX_FILES_TO_MERGE_TARGET
    ${IMAGE}kernel_elf
    )

  set_property(
    GLOBAL APPEND PROPERTY
    HEX_FILES_TO_MERGE
    ${CMAKE_BINARY_DIR}/zephyr/${KERNEL_HEX_NAME}

    )
  set_property(
    GLOBAL APPEND PROPERTY
    HEX_FILES_TO_MERGE_TARGET
    kernel_elf
    )
endif()
