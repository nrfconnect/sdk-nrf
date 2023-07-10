#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

find_package(Python3 REQUIRED)

#
# Create CMake target for building a DFU Multi Image package.
#
# Required arguments:
#   IMAGE_IDS     list of numeric identifiers (signed integers) of all images to be
#                 included in the package
#   IMAGE_PATHS   list of paths to image files to be included in the package; must be
#                 of the same length as IMAGE_IDS. The package builder is indifferent
#                 to image formats used.
#   OUTPUT        location of the created package
#
function(dfu_multi_image_package TARGET_NAME)
    cmake_parse_arguments(ARG "" "OUTPUT" "IMAGE_IDS;IMAGE_PATHS" ${ARGN})

    if (NOT DEFINED ARG_IMAGE_IDS OR NOT ARG_IMAGE_PATHS OR NOT ARG_OUTPUT)
        message(FATAL_ERROR "All IMAGE_IDS, IMAGE_PATHS and OUTPUT arguments must be specified")
    endif()

    # Prepare dfu_multi_image_tool.py argument list
    set(SCRIPT_ARGS "create")

    foreach(image IN ZIP_LISTS ARG_IMAGE_IDS ARG_IMAGE_PATHS)
        list(APPEND SCRIPT_ARGS "--image" "${image_0}" "${image_1}")
    endforeach()

    list(APPEND SCRIPT_ARGS ${ARG_OUTPUT})

    # Pass the argument list via file to avoid hitting Windows command-line length limit
    string(REPLACE ";" "\n" SCRIPT_ARGS "${SCRIPT_ARGS}")
    file(GENERATE OUTPUT ${ARG_OUTPUT}.args CONTENT ${SCRIPT_ARGS})

    add_custom_target(${TARGET_NAME} ALL
        COMMAND
        ${Python3_EXECUTABLE}
        ${ZEPHYR_NRF_MODULE_DIR}/scripts/bootloader/dfu_multi_image_tool.py
        @${ARG_OUTPUT}.args
        BYPRODUCTS
        ${ARG_OUTPUT}
    )
endfunction()
